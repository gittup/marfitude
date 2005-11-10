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
#include <time.h>
#include <unistd.h>

#include "SDL.h"

#include "main.h"
#include "audio.h"
#include "cfg.h"
#include "event.h"
#include "glfunc.h"
#include "fft.h"
#include "input.h"
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
#include "util/slist.h"

/** @file
 * Initialize all the sub-systems and run the main execution loop.
 */

static void shutdown(void);

static int quit = 0;

/** Causes the main game loop to exit on the next iteration */
void gmae_quit(void)
{
	quit = 1;
}

void shutdown(void)
{
	switch_scene(NULLSCENE);
	switch_menu(NULLMENU);
	quit_sounds();
	quit_joystick();
	quit_gl();
	quit_audio();
	quit_input();
	quit_events();
	quit_slist();
	quit_fft();
	quit_config();
	quit_log();
}

/** Hurrah for main(). It lets me run things. */
int main(int argc, char **argv)
{
	char c;
	char *convertSong = NULL;

	srand(time(NULL));
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
				enable_logging();
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
	if(init_log())
	{
		printf("Error creating log file!");
		shutdown();
		return 1;
	}

	if(init_config())
	{
		ELog(("ERROR: Couldn't set configuration options!\n"));
		shutdown();
		return 1;
	}

	if(init_audio())
	{
		ELog(("ERROR: Couldn't start audio!\n"));
		shutdown();
		return 1;
	}

	/* if the user just wanted to generate the WAM file, do that
	 * and quit (this can be done in a script to generate all the WAMs
	 * before playing the game, so the initial loads are quick)
	 */
	if(convertSong != NULL)
	{
		printf("Generating WAM file for: %s\n", convertSong);
		write_wam(convertSong);
		shutdown();
		return 0;
	}

	/* finish initializing the rest of the subsystems */
	if(init_gl())
	{
		ELog(("Error initializing SDL/OpenGL!\n"));
		shutdown();
		return 1;
	}

	if(init_sounds())
	{
		ELog(("ERROR: Couldn't load sounds!\n"));
		shutdown();
		return 1;
	}

	init_joystick();

	if(configure_joykeys())
	{
		switch_scene(NULLSCENE);
		if(switch_menu(CONFIGMENU))
		{
			ELog(("Error switching to the configuration menu.\n"));
			shutdown();
			return 1;
		}
	}
	else
	{
		switch_scene(INTROSCENE);
		if(switch_menu(NOMENU))
		{
			ELog(("Error enabling the menu.\n"));
			shutdown();
			return 1;
		}
		if(switch_scene(MAINSCENE))
		{
			ELog(("Error switching to main scene.\n"));
			switch_menu(MAINMENU);
		}
	}

	init_input();

	while(!quit)
	{
		Log(("input loop\n"));
		input_loop();
		Log(("update_timer\n"));
		update_timer();
		Log(("Scene Render\n"));
		active_scene()->render();
		Log(("Menu Render\n"));
		active_menu()->render();
		Log(("Update Screen\n"));
		update_screen();
		Log(("Next loop\n"));
		SDL_Delay(1);
	}
	
	shutdown();
	return 0;
}
