#include "SDL_opengl.h"

#include "marfitude.h"
#include "explode.h"

#include "gmae/event.h"
#include "gmae/particles.h"

#include "util/myrand.h"

static void explode(const void *);

void explode_init(void)
{
	register_event("row explosion", explode, EVENTTYPE_MULTI);
}

void explode_exit(void)
{
	deregister_event("row explosion", explode);
}

void explode(const void *data)
{
	const struct marfitude_pos *p = data;
	struct obj *o;
	float col[4];
	int x;

	o = new_obj();
	o->pos.x = -p->channel * 2.0;
	o->pos.z = TIC_HEIGHT * p->tic;
	o->vel.x = rand_float() - 0.5;
	o->vel.y = 2.0 + rand_float();
	o->vel.z = 13.0 + rand_float();
	o->rotvel = rand_float() * 720.0 - 360.0;
	o->acc.y = -3.98;

	x = rand_int(7);
	col[ALPHA] = 1.0;
	col[RED] = 0.0;
	col[GREEN] = 0.0;
	col[BLUE] = 0.0;
	if(x&1) col[RED] = 1.0;
	if(x&2) col[GREEN] = 1.0;
	if(x&4) col[BLUE] = 1.0;

	create_particle(o, col, P_StarBurst, 1.0);
}
