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

#include "phys.h"
#include "log.h"

#include "util/slist.h" 

struct slist *objs = NULL; 

/** Creates a new object structure and returns it. All parameters are zero
 * except the rotation axis, which is (0.0, 0.0, 1.0) and the mass, which is
 * 1.0
 * @return A pointer to an object structure
 */
struct obj *NewObj(void)
{
	struct obj *o;
	o = (struct obj*)calloc(1, sizeof(struct obj));
	o->axis.z = 1.0;
	o->mass = 1.0;
	objs = slist_append(objs, (void *)o);
	return o;
}

/** Deletes the object structure @a o
 * @param o The object to delete
 */
void DeleteObj(struct obj *o)
{
	free(o);
	objs = slist_remove(objs, (void *)o);
}

/** Update the positions of objects based on velocity/acceleration/rotation
 * parameters.
 * @param dt The time that has elapsed, in seconds
 */
void UpdateObjs(double dt)
{
	struct slist *t;

	slist_foreach(t, objs) {
		struct obj *o = t->data;
		double tmpx, tmpy, tmpz, tmpr;

		tmpx = o->acc.x * dt;
		tmpy = o->acc.y * dt;
		tmpz = o->acc.z * dt;
		tmpr = o->rotacc * dt;

		o->pos.x += o->vel.x * dt + tmpx * dt / 2.0;
		o->pos.y += o->vel.y * dt + tmpy * dt / 2.0;
		o->pos.z += o->vel.z * dt + tmpz * dt / 2.0;
		o->theta += o->rotvel * dt + tmpr * dt / 2.0;

		o->vel.x += tmpx;
		o->vel.y += tmpy;
		o->vel.z += tmpz;
		o->rotvel += tmpr;
	}
}

/** Check to see how many objects exist. Call at the end to see if some objects
 * have not been accounted for.
 */
void CheckObjs(void)
{
	if(slist_length(objs))
	{
		ELog(("Error: %i objects remaining!\n", slist_length(objs)));
	}
}
