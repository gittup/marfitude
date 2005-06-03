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

#include "SDL.h"

#include "timer.h"

/** @file
 * Updates some timer variables each frame
 */

Uint32 curTime;  /**< current time since initialization */
Uint32 ticDiff;	 /**< number of ticks between frames */
double timeDiff; /**< time (seconds) between frames */

/** Starts the timer */
void init_timer(void)
{
	curTime = SDL_GetTicks();
	timeDiff = 0;
}

/** Update the timer. This should be called each frame */
void update_timer(void)
{
	Uint32 tmp;
	tmp = SDL_GetTicks();
	ticDiff = tmp - curTime;
	timeDiff = (double)ticDiff / 1000.0;
	curTime = tmp;
}
