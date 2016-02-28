#include "ArCepstral.h"
#include "ArSoundsQueue.h"
#include "ArSoundPlayer.h"

extern ArCepstral g_Cepstral;
extern ArSoundsQueue g_SoundsQueue;

#ifndef TIMEOUT_ATTENTE_HUMAIN
	#define TIMEOUT_ATTENTE_HUMAIN 30
#endif
#ifndef MAX_ATTEMPTS_FAILED
	#define MAX_ATTEMPTS_FAILED 3
#endif