#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "phys.h"
#include "log.h"

GSList *objs = NULL;

Obj *NewObj()
{
	Obj *o;
	o = (Obj*)calloc(1, sizeof(Obj));
	o->axis.z = 1.0;
	o->mass = 1.0;
	objs = g_slist_append(objs, (gpointer)o);
	return o;
}

void DeleteObj(Obj *o)
{
	free(o);
	objs = g_slist_remove(objs, (gpointer)o);
}

void FreeObj(gpointer data, gpointer user_data)
{
	free(data);
}

void ClearObjs()
{
	g_slist_foreach(objs, FreeObj, NULL);
}

void UpdateObj(gpointer op, gpointer tp)
{
	double tmpx, tmpy, tmpz, tmpr;
	double t;
	Obj *o;
	t = (double)(*(int*)tp) / 1000.0;
	o = (Obj*)op;

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

void UpdateObjs(int dt)
{
	g_slist_foreach(objs, UpdateObj, &dt);
}

void CheckObjs()
{
	if(g_slist_length(objs))
		ELog("Error: %i objects remaining!\n", g_slist_length(objs));
}
