#include <stdio.h>

#include "phys.h"
#include "log.h"

void UpdateObj(Obj *o, double t)
{
	double tmpx, tmpy, tmpz, tmpr;
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
