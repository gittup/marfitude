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
#include <string.h>

#include "SDL.h"

#include "joy.h"
#include "cfg.h"
#include "log.h"

#include "util/memtest.h"

SDL_Joystick **joys = NULL;
char joyButtonCfg[23] = "joystick.ignorejs00b00";
int joyInited = 0;

int JoyIgnoreButton(int joy, int button)
{
	joyButtonCfg[17] = joy / 10 + '0';
	joyButtonCfg[18] = joy % 10 + '0';
	joyButtonCfg[20] = button / 10 + '0';
	joyButtonCfg[21] = button % 10 + '0';
	return CfgI(joyButtonCfg);
}

void InitJoystick(void)
{
	int i;
	const char *name;
	if(CfgI("joystick.joystickenable") == 0) return;
	Log(("Starting joystick...\n"));
	i = SDL_NumJoysticks();
	if(i)
	{
		Log(("JoystickInit(): Found %i joystick", i));
		if(i != 1)
		{
			Log(("s"));
		}
		Log(("\n"));
		joys = (SDL_Joystick**)malloc(sizeof(SDL_Joystick*) * i);
	}
	else
	{
		Log(("JoystickInit(): No joysticks found!\n"));
	}

	for(i=0;i<SDL_NumJoysticks();i++)
	{
		name = SDL_JoystickName(i);
		Log((" Joystick %d: %s\n",i,name ? name : "Unknown Joystick"));
		joys[i] = SDL_JoystickOpen(i);
		Log(("%i button\n", SDL_JoystickNumButtons(joys[i])));
	}
	joyInited = 1;
}

void QuitJoystick(void)
{
	int i;
	if(!joyInited) return;
	for(i=0;i<SDL_NumJoysticks();i++)
	{
		SDL_JoystickClose(joys[i]);
	}
	if(joys) free(joys);
	joys = NULL;
	Log(("Joystick shutdown\n"));
}

/*
void WatchJoystick(SDL_Joystick *joystick)
{
	const char *name;
	int i, done;
	SDL_Event event;
	int x, y, draw;

	name = SDL_JoystickName(SDL_JoystickIndex(joystick));
	printf("Watching joystick %d: (%s)\n", SDL_JoystickIndex(joystick),
	       name ? name : "Unknown Joystick");
	printf("Joystick has %d axes, %d hats, %d balls, and %d buttons\n",
	       SDL_JoystickNumAxes(joystick), SDL_JoystickNumHats(joystick),
	       SDL_JoystickNumBalls(joystick),SDL_JoystickNumButtons(joystick));

		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
			    case SDL_JOYAXISMOTION:
				printf("Joystick %d axis %d value: %d\n",
				       event.jaxis.which,
				       event.jaxis.axis,
				       event.jaxis.value);
				break;
			    case SDL_JOYHATMOTION:
				printf("Joystick %d hat %d value:",
				       event.jhat.which,
				       event.jhat.hat);
				if ( event.jhat.value == SDL_HAT_CENTERED )
					printf(" centered");
				if ( event.jhat.value & SDL_HAT_UP )
					printf(" up");
				if ( event.jhat.value & SDL_HAT_RIGHT )
					printf(" right");
				if ( event.jhat.value & SDL_HAT_DOWN )
					printf(" down");
				if ( event.jhat.value & SDL_HAT_LEFT )
					printf(" left");
				printf("\n");
				break;
			    case SDL_JOYBALLMOTION:
				printf("Joystick %d ball %d delta: (%d,%d)\n",
				       event.jball.which,
				       event.jball.ball,
				       event.jball.xrel,
				       event.jball.yrel);
				break;
			    case SDL_JOYBUTTONDOWN:
				printf("Joystick %d button %d down\n",
				       event.jbutton.which,
				       event.jbutton.button);
				break;
			    case SDL_JOYBUTTONUP:
				printf("Joystick %d button %d up\n",
				       event.jbutton.which,
				       event.jbutton.button);
				break;
			    case SDL_KEYDOWN:
				if ( event.key.keysym.sym != SDLK_ESCAPE ) {
					break;
				}
			    case SDL_QUIT:
				done = 1;
				break;
			    default:
				break;
			}
		}
		for ( i=0; i<SDL_JoystickNumButtons(joystick); ++i ) {
			SDL_Rect area;

			area.x = i*34;
			area.y = SCREEN_HEIGHT-34;
			area.w = 32;
			area.h = 32;
			if (SDL_JoystickGetButton(joystick, i) == SDL_PRESSED) {
				SDL_FillRect(screen, &area, 0xFFFF);
			} else {
				SDL_FillRect(screen, &area, 0x0000);
			}
			SDL_UpdateRects(screen, 1, &area);
		}

		draw = !draw;
		x = (((int)SDL_JoystickGetAxis(joystick, 0))+32768);
		x *= SCREEN_WIDTH;
		x /= 65535;
		if ( x < 0 ) {
			x = 0;
		} else
		if ( x > (SCREEN_WIDTH-16) ) {
			x = SCREEN_WIDTH-16;
		}
		y = (((int)SDL_JoystickGetAxis(joystick, 1))+32768);
		y *= SCREEN_HEIGHT;
		y /= 65535;
		if ( y < 0 ) {
			y = 0;
		} else
		if ( y > (SCREEN_HEIGHT-16) ) {
			y = SCREEN_HEIGHT-16;
		}
	printf("There are %d joysticks attached\n", SDL_NumJoysticks());
	for ( i=0; i<SDL_NumJoysticks(); ++i ) {
		name = SDL_JoystickName(i);
		printf("Joystick %d: %s\n",i,name ? name : "Unknown Joystick");
	}

	if ( argv[1] ) {
		joystick = SDL_JoystickOpen(atoi(argv[1]));
		if ( joystick == NULL ) {
			printf("Couldn't open joystick %d: %s\n", atoi(argv[1]),
			       SDL_GetError());
		} else {
			WatchJoystick(joystick);
			SDL_JoystickClose(joystick);
		}
	}
}*/
