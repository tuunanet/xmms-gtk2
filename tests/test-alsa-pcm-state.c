#include <glib.h>

#include "../Output/alsa/pcm-state.h"

static void test_waits_until_audio_is_ready(void)
{
	g_assert_cmpint(alsa_pcm_action(TRUE, FALSE, TRUE, TRUE), ==,
			ALSA_PCM_WAIT);
	g_assert_cmpint(alsa_pcm_action(FALSE, TRUE, TRUE, TRUE), ==,
			ALSA_PCM_WAIT);
	g_assert_cmpint(alsa_pcm_action(FALSE, FALSE, FALSE, TRUE), ==,
			ALSA_PCM_WAIT);
}

static void test_primes_prepared_pcm_without_polling(void)
{
	g_assert_cmpint(alsa_pcm_action(FALSE, FALSE, TRUE, TRUE), ==,
			ALSA_PCM_WRITE);
}

static void test_polls_running_pcm(void)
{
	g_assert_cmpint(alsa_pcm_action(FALSE, FALSE, TRUE, FALSE), ==,
			ALSA_PCM_POLL);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/alsa-pcm/waits-until-audio-is-ready",
			test_waits_until_audio_is_ready);
	g_test_add_func("/alsa-pcm/primes-prepared-pcm-without-polling",
			test_primes_prepared_pcm_without_polling);
	g_test_add_func("/alsa-pcm/polls-running-pcm", test_polls_running_pcm);

	return g_test_run();
}
