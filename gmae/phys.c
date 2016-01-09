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

#include "phys.h"
#include "log.h"
#include "event.h"

#include "util/slist.h" 

#include <stdio.h>
#include <stdlib.h>

/** @file
 * Keeps track of objects and updates them based on some uber fake physics
 */

static struct slist *objs = NULL; 
static void update_objs(const void *data);

/** Initialize physics engine. */
void phys_init(void)
{
	register_event("timer delta", update_objs);
}

/** Check to see how many objects exist. Call at the end to see if some objects
 * have not been accounted for.
 */
void phys_quit(void)
{
	deregister_event("timer delta", update_objs);
	if(slist_length(objs))
	{
		ELog(("Error: %i objects remaining!\n", slist_length(objs)));
	}
}

/** Creates a new object structure and returns it. All parameters are zero
 * except the rotation axis, which is (0.0, 0.0, 1.0) and the mass, which is
 * 1.0
 * @return A pointer to an object structure
 */
void new_obj(struct obj *o)
{
	o->pos.v[0] = 0.0;
	o->pos.v[1] = 0.0;
	o->pos.v[2] = 0.0;
	o->vel.v[0] = 0.0;
	o->vel.v[1] = 0.0;
	o->vel.v[2] = 0.0;
	o->acc.v[0] = 0.0;
	o->acc.v[1] = 0.0;
	o->acc.v[2] = 0.0;
	o->axis.v[0] = 0.0;
	o->axis.v[1] = 0.0;
	o->axis.v[2] = 1.0;
	o->theta = 0.0;
	o->rotvel = 0.0;
	o->rotacc = 0.0;
	o->mass = 1.0;
	objs = slist_append(objs, (void *)o);
}

/** Deletes the object structure @a o
 * @param o The object to delete
 */
void delete_obj(struct obj *o)
{
	objs = slist_remove(objs, (void *)o);
}

/** Update the positions of objects based on velocity/acceleration/rotation
 * parameters.
 */
void update_objs(const void *data)
{
	double dt = *((const double*)data);
	struct slist *t;

	slist_foreach(t, objs) {
		struct obj *o = t->data;
		double tmpx, tmpy, tmpz, tmpr;

		tmpx = o->acc.v[0] * dt;
		tmpy = o->acc.v[1] * dt;
		tmpz = o->acc.v[2] * dt;
		tmpr = o->rotacc * dt;

		o->pos.v[0] += o->vel.v[0] * dt + tmpx * dt / 2.0;
		o->pos.v[1] += o->vel.v[1] * dt + tmpy * dt / 2.0;
		o->pos.v[2] += o->vel.v[2] * dt + tmpz * dt / 2.0;
		o->theta += o->rotvel * dt + tmpr * dt / 2.0;

		o->vel.v[0] += tmpx;
		o->vel.v[1] += tmpy;
		o->vel.v[2] += tmpz;
		o->rotvel += tmpr;
	}
}
