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

#include "SDL.h"

#include "joy.h"
#include "cfg.h"
#include "log.h"

#include "util/memtest.h"

/** @file
 * Loads joysticks from SDL, and can ignore button events.
 */

static SDL_Joystick **joys = NULL;
static char joyButtonCfg[14] = "ignorejs00b00";
static int joyInited = 0;

/** Checks if a joystick button has been set to be ignored by the config file.
 * This is really just here cuz my joystick makes an SDL button event
 * whenever I hit the axis. Since all button events activate the menu, it was
 * pretty annoying.
 *
 * @param joy the joystick (event.jbutton.which)
 * @param button the button (event.jbutton.button)
 * @return 1 if the button is ignored, 0 if not.
 */
int joy_ignore_button(int joy, int button)
{
	joyButtonCfg[8] = joy / 10 + '0';
	joyButtonCfg[9] = joy % 10 + '0';
	joyButtonCfg[11] = button / 10 + '0';
	joyButtonCfg[12] = button % 10 + '0';
	return cfg_get_int("joystick", joyButtonCfg, 0);
}

/** Opens all of the joysticks from SDL */
void init_joystick(void)
{
	int i;
	const char *name;
	if(cfg_get_int("joystick", "joystickenable", 1) == 0) return;
	Log(("Starting joystick...\n"));
	i = SDL_NumJoysticks();
	if(i) {
		Log(("JoystickInit(): Found %i joystick(s)\n", i));
		joys = malloc(sizeof(SDL_Joystick*) * i);
	} else {
		Log(("JoystickInit(): No joysticks found!\n"));
	}

	for(i=0;i<SDL_NumJoysticks();i++) {
		name = SDL_JoystickName(i);
		Log((" Joystick %d: %s\n",i,name ? name : "Unknown Joystick"));
		joys[i] = SDL_JoystickOpen(i);
		Log(("%i button\n", SDL_JoystickNumButtons(joys[i])));
	}
	joyInited = 1;
}

/** Closes all of the SDL joysticks */
void quit_joystick(void)
{
	int i;
	if(!joyInited) return;
	for(i=0;i<SDL_NumJoysticks();i++) {
		SDL_JoystickClose(joys[i]);
	}
	if(joys) free(joys);
	joys = NULL;
	joyInited = 0;
	Log(("Joystick shutdown\n"));
}
