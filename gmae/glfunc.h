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

/** @file
 * Manages the OpenGL initialization, and provides some utility functions
 */

/** The height of a font character */
#define FONT_HEIGHT 14
/** The width of a font character */
#define FONT_WIDTH 10

int init_gl(void);
void quit_gl(void);
void look_at(double ex, double ey, double ez, double cx, double cy, double cz, double ux, double uy, double uz);
void perspective_projection(double fov, double aspect, double z1, double z2);
void set_ortho_projection(void);
void reset_projection(void);
void print_gl(int x, int y, const char *msg, ...);
void set_font_size(float size);
void update_screen(void);

int display_width(void);
int display_height(void);
