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
 * A particle manager
 */
#include "phys.h"

/** Describes a single particle */
struct particle {
	struct obj o; /**< The particle's object info */
	float c[4];   /**< The particle's color */
	float life;   /**< TTL in seconds */
};

int init_particles(void);
void quit_particles(void);
void draw_particles(void);
struct particle *create_particle(void (*draw)(const struct particle *));
void clear_particles(void);
