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

#include "sdl_mixer/SDL_mixer.h"

#include "audio.h"
#include "cfg.h"
#include "util/sdlfatalerror.h"

int audioInited = 0;

int InitAudio(void)
{
	int stereo;
	Uint16 flags;

	printf("Starting audio...\n");
	if(CfgI("sound.bits") == 16)
		flags = AUDIO_S16SYS;
	else
		flags = AUDIO_U8;

	if(CfgEq("sound.stereo", "yes"))
		stereo = 2;
	else
		stereo = 1;

	if(Mix_OpenAudio(CfgI("sound.hz"), flags, stereo, CfgI("sound.buffersize")))
	{
		SDLError("initializing audio");
		return 1;
	}
	Mix_AllocateChannels(CfgI("sound.channels"));
	audioInited = 1;
	printf("Audio initialized at %iHz, %i channels\n", CfgI("sound.hz"), CfgI("sound.channels"));
	return 0;
}

void QuitAudio(void)
{
	if(!audioInited) return;
	Mix_CloseAudio();
	printf("Audio shutdown\n");
}
