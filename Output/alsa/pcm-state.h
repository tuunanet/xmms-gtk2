#ifndef ALSA_PCM_STATE_H
#define ALSA_PCM_STATE_H

typedef enum
{
	ALSA_PCM_WAIT,
	ALSA_PCM_WRITE,
	ALSA_PCM_POLL
}
AlsaPcmAction;

AlsaPcmAction alsa_pcm_action(int paused, int prebuffering,
			      int period_buffered, int prepared);

#endif
