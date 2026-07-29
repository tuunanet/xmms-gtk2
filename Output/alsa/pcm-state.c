#include "pcm-state.h"

AlsaPcmAction alsa_pcm_action(int paused, int prebuffering,
			      int period_buffered, int prepared)
{
	if (paused || prebuffering || !period_buffered)
		return ALSA_PCM_WAIT;
	if (prepared)
		return ALSA_PCM_WRITE;
	return ALSA_PCM_POLL;
}
