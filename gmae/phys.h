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

/** @file
 * Adds some mechanics (velocity/acceleration, etc) to objects
 */

/** The red byte in a color vector */
#define RED 0
/** The green byte in a color vector */
#define GREEN 1
/** The blue byte in a color vector */
#define BLUE 2
/** The alpha byte in a color vector */
#define ALPHA 3

/** A vector structure */
struct vector {
	double x; /**< The x parameter */
	double y; /**< The y parameter */
	double z; /**< The z parameter */
};

/** An object structre */
struct obj {
	struct vector pos;  /**< position of object */
	struct vector vel;  /**< velocity of object */
	struct vector acc;  /**< acceleration of object */
	/* need jerk for camera movement? */
	struct vector axis; /**< axis of rotation */
	double theta;       /**< amount of rotation */
	double rotvel;      /**< rotation velocity */
	double rotacc;      /**< rotation acceleration */
	float mass;         /**< object's mass */
};

struct obj *NewObj(void);
void DeleteObj(struct obj *o);
void UpdateObjs(double dt);
void CheckObjs(void);
