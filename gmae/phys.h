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
 * Adds some mechanics (velocity/acceleration, etc) to objects
 */
#ifndef phys_h
#define phys_h

#include "util/math/vector.h"

/** An object structre */
struct obj {
	union vector pos;   /**< position of object */
	union vector vel;   /**< velocity of object */
	union vector acc;   /**< acceleration of object */
	/* need jerk for camera movement? */
	union vector axis;  /**< axis of rotation */
	double theta;       /**< amount of rotation */
	double rotvel;      /**< rotation velocity */
	double rotacc;      /**< rotation acceleration */
	float mass;         /**< object's mass */
};

void phys_init(void);
void phys_quit(void);
void new_obj(struct obj *o);
void delete_obj(struct obj *o);

#endif
