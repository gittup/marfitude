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
#include <stdlib.h>
#include <string.h>

#include "sdl_mixer/SDL_mixer.h"

#include "sdlfatalerror.h"
#include "sounds.h"
#include "log.h"

#include "util/memtest.h"
#include "util/textprogress.h"
#include "util/fatalerror.h"
#include "util/flist.h"
#include "util/strfunc.h"

/** The directory all the .wav files are located */
#define SOUNDDIR "sounds/"

/** @file
 * Provides access to all the wavs in the sounds/ directory
 */

/** Maps a filename to a Mix_Chunk */
struct snd_entry {
	Mix_Chunk *chunk; /**< The Mix_Chunk for this file */
	char *name;       /**< The filename used to load this sound */
};

static struct snd_entry *sounds = NULL;
static int num_sounds = 0;
static int sndInited = 0;

static int ValidWavFile(const char *s);

int ValidWavFile(const char *s)
{
	int x = 0;
	while(s[x]) x++;
	if(x < 4) return 0;
	if(s[x-1] != 'v') return 0;
	if(s[x-2] != 'a') return 0;
	if(s[x-3] != 'w') return 0;
	if(s[x-4] != '.') return 0;
	return 1;
}

/** Calls Mix_PlayChannel on the appropriate sound chunk for @a snd.
 * @param snd The sound number retrieved from sound_num()
 */
void MPlaySound(int snd)
{
	if(snd < 0 || snd >= num_sounds) return;
	if(Mix_PlayChannel(-1, sounds[snd].chunk, 0) == -1) {
		Log(("Warning: out of sound channels!\n"));
	}
}

/** Retrieves the sound index from the file @a name.
 * @param name The filename that the sound was loaded from
 * @return The sound index
 */
int sound_num(const char *name)
{
	int x;
	for(x=0;x<num_sounds;x++) {
		if(strcmp(name, sounds[x].name) == 0)
			return x;
	}
	fprintf(stderr, "Warning: Sound '%s' not found.\n", name);
	return -1;
}

/** Loads all sounds in the "sounds" directory, and assigns them numbers. The
 * sounds can then be retrieved by calling sound_num() with the file name.
 */
int init_sounds(void)
{
	int x;
	char *t;
	struct flist f;

	flist_foreach(&f, SOUNDDIR) {
		if(ValidWavFile(f.filename)) {
			sounds = (struct snd_entry*)realloc(sounds, sizeof(struct snd_entry) * (num_sounds+1));
			sounds[num_sounds].name = malloc(strlen(f.filename)+1);
			strcpy(sounds[num_sounds].name, f.filename);
			sounds[num_sounds].chunk = NULL;
			num_sounds++;
		}
	}

	/* This just means the memory has been allocated */
	sndInited = 1;

	progress_meter("Loading sounds");
	for(x=0;x<num_sounds;x++) {
		t = cat_str(SOUNDDIR, sounds[x].name);
		sounds[x].chunk = Mix_LoadWAV(t);
		if(!sounds[x].chunk) {
			Log(("Error loading sound '%s'\n", t));
			SDLError("Loading sounds");
			free(t);
			return 2;
		}
		free(t);
		update_progress(x+1, num_sounds);
	}
	end_progress_meter();
	return 0;
}

/** Frees all the information used to manage the sounds */
void quit_sounds(void)
{
	int x;
	if(!sndInited) return;
	for(x=0;x<num_sounds;x++) {
		free(sounds[x].name);
	}
	free(sounds);
	printf("Sounds shutdown\n");
}
