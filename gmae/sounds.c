#include <stdio.h>
#include <stdlib.h>

#include "SDL_mixer.h"

#include "sounds.h"
#include "sndlist.h"
#include "log.h"

#include "memtest.h"
#include "textprogress.h"
#include "fatalerror.h"
#include "sdlfatalerror.h"

Mix_Chunk **Sounds;
int sndInited = 0;

void SDLPlaySound(Mix_Chunk *snd)
{
	if(Mix_PlayChannel(-1, snd, 0) == -1)
	{
		Log(("Warning: out of sound channels!\n"));
	}
}

int InitSounds(void)
{
	int x;

	Sounds = (Mix_Chunk**)malloc(sizeof(Mix_Chunk*)*NUM_SOUNDS);
	sndInited = 1;

	ProgressMeter("Loading sounds");
	for(x=0;x<NUM_SOUNDS;x++)
	{
		Sounds[x] = Mix_LoadWAV(SND_LIST[x]);
		if(!Sounds[x])
		{
			Log(("Error loading sound '%s'\n", SND_LIST[x]));
			SDLError("Loading sounds");
			return 0;
		}
		UpdateProgress(x+1, NUM_SOUNDS);
	}
	EndProgressMeter();
	return 1;
}

void QuitSounds(void)
{
	if(!sndInited) return;
	free(Sounds);
	printf("Sounds shutdown\n");
}
