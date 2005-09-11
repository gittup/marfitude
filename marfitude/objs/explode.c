#include "SDL_opengl.h"

#include "marfitude.h"
#include "explode.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/particles.h"
#include "gmae/wam.h"

#include "util/myrand.h"

static void explode(const void *);
static void victory(const void *);
static void explosion_particle(const struct marfitude_pos *p, const float *c);

void explode_init(void)
{
	register_event("row explosion", explode);
	register_event("victory", victory);
}

void explode_exit(void)
{
	deregister_event("victory", victory);
	deregister_event("row explosion", explode);
}

void explode(const void *data)
{
	const struct marfitude_pos *p = data;
	float col[4];
	int x;

	x = rand_int(7) + 1;
	col[ALPHA] = 1.0;
	col[RED] = 0.0;
	col[GREEN] = 0.0;
	col[BLUE] = 0.0;

	if(x&1) col[RED] = 1.0;
	if(x&2) col[GREEN] = 1.0;
	if(x&4) col[BLUE] = 1.0;

	explosion_particle(p, col);
}

void victory(const void *data)
{
	int score = *((const int*)data);
	struct marfitude_pos pos;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_player *ps;
	int num = 0;
	int x;

	marfitude_foreach_player(ps) {
		if(ps->score.score == score)
			num++;
	}

	marfitude_foreach_player(ps) {
		if(ps->score.score == score) {
			for(x=0; x<48 / num; x++) {
				const float *c = get_player_color(ps->num);
				pos.tic = wam->row_data[wam->num_rows - 1].ticpos;
				pos.channel = x % wam->num_cols;
				explosion_particle(&pos, c);
			}
		}
	}
}

void explosion_particle(const struct marfitude_pos *p, const float *c)
{
	struct obj *o;

	o = new_obj();
	o->pos.x = -p->channel * 2.0;
	o->pos.z = TIC_HEIGHT * p->tic;
	o->vel.x = rand_float() - 0.5;
	o->vel.y = 2.0 + rand_float();
	o->vel.z = 13.0 + rand_float();
	o->rotvel = rand_float() * 720.0 - 360.0;
	o->acc.y = -3.98;

	create_particle(o, c, P_StarBurst, 1.0);
}
