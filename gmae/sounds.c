/*
   Marfitude
   Copyright (C) 2004 Mike Shal

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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
