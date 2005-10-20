/*
   Marfitude
   Copyright (C) 2005 Mike Shal

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
#include "sdlfatalerror.h"

/** @file
 * Starts SDL_Mixer based on configuration parameters
 */

static int audioInited = 0;
static double delay;

/** Initializes SDL_Mixer
 * @return 0 on success, 1 on failure
 */
int init_audio(void)
{
	int stereo;
	int hz;
	int channels;
	int buffersize;
	Uint16 flags;

	if(audioInited)
		return 0;

	printf("Starting audio...\n");
	if(cfg_get_int("sound", "bits", 16) == 16)
		flags = AUDIO_S16SYS;
	else
		flags = AUDIO_U8;

	if(cfg_eq("sound", "stereo", "yes"))
		stereo = 2;
	else
		stereo = 1;

	hz = cfg_get_int("sound", "hz", 44100);
	channels = cfg_get_int("sound", "channels", 32);
	buffersize = cfg_get_int("sound", "buffersize", 512);
	delay = (double)buffersize / (double)hz;

	if(Mix_OpenAudio(hz, flags, stereo, buffersize))
	{
		SDLError("initializing audio");
		return 1;
	}
	Mix_AllocateChannels(channels);
	audioInited = 1;
	printf("Audio initialized at %iHz, %i channels\n", hz, channels);
	return 0;
}

/** Quits SDL_Mixer if the audio has been initialized. */
void quit_audio(void)
{
	if(!audioInited) return;
	Mix_CloseAudio();
	printf("Audio shutdown\n");
	audioInited = 0;
}

/** Find the delay caused by the audio buffer in seconds. */
double audio_delay(void)
{
	if(!audioInited)
		return 0.0;
	return delay;
}
