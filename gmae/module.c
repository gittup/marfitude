/*
   Marfitude
   Copyright (C) 2006 Mike Shal

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
#include <string.h>

#include "sdl_mixer/SDL_mixer.h"
#include "sdl_mixer/mikmod/mikmod_internals.h"

#include "module.h"
#include "cfg.h"
#include "log.h"
#include "sdlfatalerror.h"

#include "util/memtest.h"
#include "util/fatalerror.h"

/** @file
 * Gets access to the modules. Also sets up some MikMod parameters based
 * on configuration values.
 */

/** The module currently being played by MikMod */
MODULE *mod = NULL;

static Mix_Music *modMusic = NULL;
static void seek_handler(void);

/** Start the mod @a modFile
 * @param modFile Only the coolest files can be modFiles
 * @return 0 on success, else on error
 */
int start_module(const char *modFile)
{
	Log(("Loading module...\n"));
	modMusic = Mix_LoadMUS(modFile);

	if(!modMusic)
	{
		SDLError("Loading mod file");
		return 1;
	}
	Log(("Mod loaded\n"));
	if(Mix_PlayMusic(modMusic, 1) == -1)
	{
		SDLError("Playing mod file");
		return 2;
	}
	Log(("Mod ready\n"));

	mod = Player_GetModule();
	if(mod == NULL) {
		Error("retrieving module with MikMod's Player_GetModule()");
		return 3;
	}

	mod->loop = 0; /* don't want to keep looping forever! */

	/* Always set interpolation. */
	md_mode |= DMODE_INTERP;

	return 0;
}

/** Stop the currently playing mod, if any */
void stop_module(void)
{
	if(modMusic == NULL) return;
	Log(("Freeing Mod music\n"));
	Mix_FreeMusic(modMusic);
	modMusic = NULL;
	Log(("Module stopped\n"));
}

/** Seek the currently played module to the absolute tic value @a tic.
 *
 * @param tic The tic to seek to.
 */
void seek_module(int tic)
{
	int paused;
	int count = 0;
	MikMod_player_t old;

	if(modMusic == NULL) return;

	Player_Mute(MUTE_INCLUSIVE, 0, mod->numchn-1);
	old = MikMod_RegisterPlayer(seek_handler);

	/* Make sure the song is not paused */
	paused = Player_Paused();
	if(paused)
		Player_TogglePause();

	while(count < tic) {
		Player_HandleTick();
		count++;
	}

	/* Return to the previous pause state */
	if(paused)
		Player_TogglePause();
	Player_Unmute(MUTE_INCLUSIVE, 0, mod->numchn-1);
	MikMod_RegisterPlayer(old);
}

void seek_handler(void) { /* empty handler */ }
