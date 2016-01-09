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

#include "timer.h"
#include "event.h"
#include "SDL.h"

/** @file
 * Updates some timer variables each frame
 */

static Uint32 cur_time;  /**< current time since initialization */
static Uint32 tic_delta; /**< number of ticks between frames */

/** Starts the timer */
void init_timer(void)
{
	cur_time = SDL_GetTicks();
}

/** Update the timer. This should be called each frame */
void update_timer(void)
{
	Uint32 tmp;
	double timer_delta;

	tmp = SDL_GetTicks();
	tic_delta = tmp - cur_time;
	timer_delta = (double)tic_delta / 1000.0;
	cur_time = tmp;
	fire_event("timer tic delta", &tic_delta);
	fire_event("timer delta", &timer_delta);
}

/** Gets the current time, in timer tics. */
Uint32 timer_cur(void)
{
	return cur_time;
}

/** Gets the delta in timer tics between the current and previous frame. */
Uint32 timer_tic_delta(void)
{
	return tic_delta;
}
