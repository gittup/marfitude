#include <stdio.h>

#include "SDL_mixer.h"

#include "cfg.h"
#include "../util/sdlfatalerror.h"

int audioInited = 0;

int InitAudio()
{
	printf("Starting audio...\n");
	if(Mix_OpenAudio(CfgI("sound.hz"), MIX_DEFAULT_FORMAT, 2, CfgI("sound.buffersize")))
	{
		SDLError("initializing audio");
		return 0;
	}
	Mix_AllocateChannels(CfgI("sound.channels"));
	audioInited = 1;
	printf("Audio initialized at %iHz, %i channels\n", CfgI("sound.hz"), CfgI("sound.channels"));
	return 1;
}

void QuitAudio()
{
	if(!audioInited) return;
	Mix_CloseAudio();
	printf("Audio shutdown\n");
}
