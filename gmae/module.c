#include <stdio.h>
#include <string.h>

#include "SDL_mixer.h"
#include "mikmod.h"

#include "module.h"
#include "cfg.h"
#include "log.h"

#include "memtest.h"
#include "fatalerror.h"
#include "sdlfatalerror.h"

Mix_Music *modMusic = NULL;
MODULE *mod = NULL;

int StartModule(char *modFile)
{
	Log(("Loading module...\n"));
	modMusic = Mix_LoadMUS(modFile);

	if(!modMusic)
	{
		SDLError("Loading mod file");
		return 0;
	}
	Log(("Mod loaded\n"));
	if(Mix_PlayMusic(modMusic, 1) == -1)
	{
		SDLError("Playing mod file");
		return 0;
	}
	Player_TogglePause();
	Log(("Mod ready\n"));

	mod = Player_GetModule();
	if(mod == NULL) {
		Error("retrieving module with MikMod's Player_GetModule()");
		return 0;
	}

	mod->loop = 0; /* don't want to keep looping forever! */

	if(CfgEq("sound.interpolation", "yes"))
		md_mode |= DMODE_INTERP;
	else
		md_mode &= !DMODE_INTERP;
	Log(("interp mode set\n"));
	return 1;
}

void StopModule(void)
{
	if(!modMusic) return;
	Log(("Freeing Mod music\n"));
	Mix_FreeMusic(modMusic);
	Log(("Module stopped\n"));
}
