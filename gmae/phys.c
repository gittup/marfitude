#include <stdio.h>
#include <stdlib.h>

#include "phys.h"
#include "log.h"

#include "slist.h" 

static void FreeObj(void *data, void *not_used);
static void UpdateObj(void *op, void *tp);

struct slist *objs = NULL; 

struct obj *NewObj(void)
{
	struct obj *o;
	o = (struct obj*)calloc(1, sizeof(struct obj));
	o->axis.z = 1.0;
	o->mass = 1.0;
	objs = slist_append(objs, (void *)o);
	return o;
}

void DeleteObj(struct obj *o)
{
	free(o);
	objs = slist_remove(objs, (void *)o);
}

void FreeObj(void *data, void *not_used)
{
	if(not_used) {}
	free(data);
}

void ClearObjs(void)
{
	slist_foreach(objs, FreeObj, NULL);
}

void UpdateObj(void *op, void *tp)
{
	double tmpx, tmpy, tmpz, tmpr;
	double t;
	struct obj *o;
	t = *(double*)tp;
	o = (struct obj*)op;

	tmpx = o->acc.x * t;
	tmpy = o->acc.y * t;
	tmpz = o->acc.z * t;
	tmpr = o->rotacc * t;

	o->pos.x += o->vel.x * t + tmpx * t / 2.0;
	o->pos.y += o->vel.y * t + tmpy * t / 2.0;
	o->pos.z += o->vel.z * t + tmpz * t / 2.0;
	o->theta += o->rotvel * t + tmpr * t / 2.0;

	o->vel.x += tmpx;
	o->vel.y += tmpy;
	o->vel.z += tmpz;
	o->rotvel += tmpr;
}

void UpdateObjs(double dt)
{
	slist_foreach(objs, UpdateObj, &dt);
}

void CheckObjs(void)
{
	if(slist_length(objs))
	{
		ELog(("Error: %i objects remaining!\n", slist_length(objs)));
	}
}
