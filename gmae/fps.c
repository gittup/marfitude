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

#include "SDL_opengl.h"

#include "fps.h"
#include "glfunc.h"
#include "timer.h"

/** @file
 * Calculates and displays FPS
 */

static Uint32 fpsticks, frames;
static float fps;

/** Initializes some FPS variables needed to calculate the frames per second */
void init_fps(void)
{
	frames = 0;
	fpsticks = 0;
	fps = 0.0;
}

/** Displays the FPS in the top corner of the screen */
void print_fps(void)
{
	set_font_size(.75);
	glColor4f(0.0, 1.0, 1.0, 1.0);
	if(fps < 100.0) print_gl(640 - 75, 5, "FPS: %2.1f", fps);
	else print_gl(640 - 75, 5, "FPS:%3.1f", fps);
	set_font_size(1.0);
}

/** Increments the frame count by one, and recalculates the FPS every second */
void update_fps(void)
{
	frames++;
	fpsticks += timer_tic_delta();
	if(fpsticks >= 1000) { /* one second */
		fps = (float)frames*1000.0/(float)(fpsticks);
		frames = 0;
		fpsticks = 0;
	}
}
