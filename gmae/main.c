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
#include <math.h>
#include <ctype.h> /* for isprint */
#include <getopt.h>
#include <unistd.h>

#include "main.h"
#include "audio.h"
#include "cfg.h"
#include "event.h"
#include "glfunc.h"
#include "fft.h"
#include "joy.h"
#include "log.h"
#include "menu.h"
#include "scene.h"
#include "sdlfatalerror.h"
#include "sounds.h"
#include "timer.h"
#include "wam.h"

#include "util/memtest.h"
#include "util/fatalerror.h"

static void Shutdown(void);

int quit = 0;

void Shutdown(void)
{
	SwitchScene(NULLSCENE);
	SwitchMenu(NULLMENU);
	QuitSounds();
	QuitJoystick();
	QuitGL();
	QuitAudio();
	QuitEvents();
	QuitFFT();
	QuitConfig();
	QuitLog();
}

int main(int argc, char **argv)
{
	char c;
	char *convertSong = NULL;

	/* parse all the command line options
	 * this is pretty much verbatim from the GNU help page
	 */
	while((c = getopt(argc, argv, "hlm:")) != -1)
	{
		switch(c)
		{
			case 'h':
				printf("%s options\n\t-h: This help message\n\t-m <file>: Generate the WAM note file for the mod 'file'\n\t-l: Enable logging\n\n", argv[0]);
				return 0;
				break;
			case 'm':
				convertSong = optarg;
				break;
			case 'l':
				logging = 1;
				break;
			case '?':
				if(isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
				break;
		}
	}

	if(MARFDATADIR[0] && chdir(MARFDATADIR)) {
		ELog(("ERROR: Couldn't change to the %s directory\n", MARFDATADIR));
	}

	/* initialize all the different subsystems, or quit
	 * if they fail for some reason
	 */
	if(InitLog())
	{
		printf("Error creating log file!");
		Shutdown();
		return 1;
	}

	if(InitConfig())
	{
		ELog(("ERROR: Couldn't set configuration options!\n"));
		Shutdown();
		return 1;
	}

	if(InitAudio())
	{
		ELog(("ERROR: Couldn't start audio!\n"));
		Shutdown();
		return 1;
	}

	/* if the user just wanted to generate the WAM file, do that
	 * and quit (this can be done in a script to generate all the WAMs
	 * before playing the game, so the initial loads are quick)
	 */
	if(convertSong != NULL)
	{
		printf("Generating WAM file for: %s\n", convertSong);
		WriteWam(convertSong);
		Shutdown();
		return 0;
	}

	/* finish initializing the rest of the subsystems */
	if(InitGL())
	{
		ELog(("Error initializing SDL/OpenGL!\n"));
		Shutdown();
		return 1;
	}

	if(InitSounds())
	{
		ELog(("ERROR: Couldn't load sounds!\n"));
		Shutdown();
		return 1;
	}

	InitJoystick();

	SDL_EnableKeyRepeat(0, 0); /* disable key repeating */
	SDL_ShowCursor(SDL_DISABLE);
	if(ConfigureJoyKey())
	{
		SwitchScene(NULLSCENE);
		if(SwitchMenu(CONFIGMENU))
		{
			ELog(("Error switching to the configuration menu.\n"));
			Shutdown();
			return 1;
		}
	}
	else
	{
		SwitchScene(INTROSCENE);
		if(SwitchMenu(NOMENU))
		{
			ELog(("Error enabling the menu.\n"));
			Shutdown();
			return 1;
		}
		if(SwitchScene(MAINSCENE))
		{
			ELog(("Error switching to main scene.\n"));
			SwitchMenu(MAINMENU);
		}
	}

	ClearEvents(); /* clears event cue - nothing to do with registering */

	while(!quit)
	{
		Log(("EventLoop\n"));
		EventLoop();
		Log(("UpdateTimer\n"));
		UpdateTimer();
		Log(("Scene Render\n"));
		ActiveScene()->Render();
		Log(("Menu Render\n"));
		ActiveMenu()->Render();
		Log(("Update Screen\n"));
		UpdateScreen();
		Log(("Next loop\n"));
		SDL_Delay(1);
	}
	
	Shutdown();
	return 0;
}
