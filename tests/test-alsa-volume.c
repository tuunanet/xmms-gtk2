#include <glib.h>
#include <string.h>

#include "../Output/alsa/alsa.h"

struct alsa_config alsa_cfg;

static void test_falls_back_to_software_volume_without_a_mixer(void)
{
	int left = -1;
	int right = -1;

	memset(&alsa_cfg, 0, sizeof (alsa_cfg));
	alsa_cfg.mixer_card = G_MAXINT;
	alsa_cfg.mixer_device = g_strdup("PCM");
	alsa_cfg.vol.left = 73;
	alsa_cfg.vol.right = 41;

	g_test_expect_message(NULL, G_LOG_LEVEL_WARNING,
			      "*Attaching to mixer*failed*");
	g_test_expect_message(NULL, G_LOG_LEVEL_WARNING,
			      "*using software volume control*");
	alsa_get_volume(&left, &right);
	g_test_assert_expected_messages();
	g_assert_true(alsa_cfg.soft_volume);
	g_assert_cmpint(left, ==, 73);
	g_assert_cmpint(right, ==, 41);

	alsa_set_volume(35, 65);
	alsa_get_volume(&left, &right);
	g_assert_cmpint(left, ==, 35);
	g_assert_cmpint(right, ==, 65);

	g_free(alsa_cfg.mixer_device);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/alsa-volume/falls-back-without-mixer",
			test_falls_back_to_software_volume_without_a_mixer);

	return g_test_run();
}
