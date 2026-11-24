/*
 * Non-GUI feedback loop for output-plugin pacing.
 *
 * Loads a real built OutputPlugin .so and drives it the same way an input
 * plugin's decode thread does (open_audio, spin on buffer_free, write_audio,
 * sample output_time), then asserts the contract the seek bar depends on:
 *
 *   1. open_audio succeeds,
 *   2. the reported output time never runs ahead of wall clock + slack,
 *   3. once the plugin's configurable buffers are exceeded, playback is
 *      paced: the wall clock to deliver the audio exceeds the buffering
 *      headroom by a wide margin,
 *   4. the device clock advances (the stream actually plays).
 *
 * Violating (2) or racing the clock outright is the "seek bar slides super
 * quick, no sound" symptom: the disk_writer plugin violates (2) because
 * output_time() is its written time and buffer_free() is unbounded. A
 * device plugin whose audio thread never starts the stream violates (3)
 * and (4).
 *
 * The audio duration deliberately exceeds the plugin's maximum buffering
 * (thread_buffer_time up to 10 s + device buffer) so that back-pressure
 * reaches the input loop; a shorter probe would be swallowed by the
 * plugin's internal buffering and pace nothing.
 *
 * Usage: test-output-pacing /path/to/plugin.so
 */
#include <glib.h>
#include <glib/gstdio.h>
#include <dlfcn.h>

#include "../xmms/plugin.h"

/*
 * Core symbols an output plugin resolves against the xmms process itself.
 * The remote-control calls fail fast when no server is listening, which is
 * what we want here: the harness runs outside xmms.
 */
gint ctrlsocket_get_session_id(void)
{
	return -1;
}

gboolean effects_enabled(void)
{
	return FALSE;
}

EffectPlugin *get_current_effect_plugin(void)
{
	return NULL;
}

#define RATE 44100
#define CHANNELS 2
#define DURATION_MS 8000
#define CHUNK_BYTES 8192
#define BYTES_PER_MS (RATE * CHANNELS * 2 / 1000)
#define TOTAL_BYTES (BYTES_PER_MS * DURATION_MS)
/*
 * With the plugin defaults (thread_buffer_time 3000 ms + device buffer
 * time 500 ms) a healthy plugin still buffers ~3.5 s before back-pressure
 * reaches this loop; 8 s of audio must then take >= 3 s of wall clock.
 * A plugin that never paces finishes in well under 1 s.
 */
#define MIN_PACING_MS 3000
#define CLOCK_SLACK_MS 500
#define HARD_CAP_MS 60000
/* Device clock must be audibly advancing near the end of the probe. */
#define MIN_FINAL_OUTPUT_TIME_MS (DURATION_MS / 4)

static gchar *plugin_path;
static gchar *sandbox_dir;

static void test_output_plugin_paces_playback(void)
{
	gpointer handle;
	OutputPlugin *(*get_oplugin_info) (void);
	OutputPlugin *op;
	GTimer *timer;
	guint8 chunk[CHUNK_BYTES];
	glong written = 0;
	gint open_rc, output_time;
	gdouble elapsed_ms;

	handle = dlopen(plugin_path, RTLD_NOW | RTLD_GLOBAL);
	if (handle == NULL)
		g_error("dlopen(%s) failed: %s", plugin_path, dlerror());

	get_oplugin_info = (OutputPlugin * (*)(void)) dlsym(handle, "get_oplugin_info");
	if (get_oplugin_info == NULL)
		g_error("dlsym(get_oplugin_info) failed: %s", dlerror());
	op = get_oplugin_info();
	if (op == NULL)
		g_error("get_oplugin_info() returned NULL");
	g_test_message("plugin under test: %s", op->description);

	if (op->init)
		op->init();

	open_rc = op->open_audio(FMT_S16_NE, RATE, CHANNELS);
	if (open_rc == 0)
		g_test_skip("output device could not be opened on this machine");
	else
	{
		timer = g_timer_new();
		memset(chunk, 0, sizeof (chunk));
		output_time = 0;
		while (written < TOTAL_BYTES)
		{
			elapsed_ms = g_timer_elapsed(timer, NULL) * 1000;
			if (elapsed_ms > HARD_CAP_MS)
				g_error("%s: unwritten audio after %.0f ms "
					"of wall time; back-pressure is "
					"stuck", op->description, elapsed_ms);

			/* The seek bar reads output_time(); it must never
			 * run ahead of the wall clock. */
			output_time = op->output_time();
			g_assert_cmpint(output_time, <=,
					(gint) elapsed_ms + CLOCK_SLACK_MS);

			if (op->buffer_free() >= (gint) sizeof (chunk))
			{
				op->write_audio(chunk, sizeof (chunk));
				written += sizeof (chunk);
				if (written % (BYTES_PER_MS * 2000)
				    < sizeof (chunk))
					g_test_message("%.0f ms wall / "
						       "%ld ms audio written"
						       " / output_time %d ms",
						       elapsed_ms,
						       written / BYTES_PER_MS,
						       output_time);
			}
			else
				g_usleep(1000);
		}
		elapsed_ms = g_timer_elapsed(timer, NULL) * 1000;
		output_time = op->output_time();

		op->close_audio();
		g_timer_destroy(timer);
		g_test_message("final: %.0f ms wall clock for %d ms of audio,"
			       " output_time %d ms", elapsed_ms, DURATION_MS,
			       output_time);

		g_assert_cmpfloat(elapsed_ms, >=, MIN_PACING_MS);
		g_assert_cmpint(output_time, >=, MIN_FINAL_OUTPUT_TIME_MS);
	}
}

int main(int argc, char **argv)
{
	gchar *sandbox;

	/* Sandbox HOME before any glib call: g_get_home_dir() caches, and
	 * plugin config reads plus any disk_writer output must land in a
	 * throwaway directory instead of the user's real profile. */
	sandbox = g_dir_make_tmp("xmms-test-output-pacing-XXXXXX", NULL);
	g_assert_nonnull(sandbox);
	g_setenv("HOME", sandbox, TRUE);
	sandbox_dir = sandbox;

	g_test_init(&argc, &argv, NULL);
	/* Output plugins legitimately log warnings (e.g. mixer element not
	 * found on soft-mixed devices); keep them visible but non-fatal. */
	g_log_set_always_fatal((GLogLevelFlags) G_LOG_FATAL_MASK);

	if (argc < 2)
		g_error("usage: %s /path/to/output-plugin.so", argv[0]);
	plugin_path = argv[1];

	g_test_add_func("/output-pacing/plugin-paces-playback",
			test_output_plugin_paces_playback);

	g_test_run();
	g_free(sandbox_dir);

	return 0;
}
