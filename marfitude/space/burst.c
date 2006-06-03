#include <stdlib.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/particles.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

#include "util/memtest.h"
#include "util/myrand.h"
#include "util/slist.h"

static void burst_init(void) __attribute__((constructor));
static void burst_exit(void) __attribute__((destructor));
static void row_burst(const void *);
static void note_burst(const void *);
static void victory(const void *);
static void burst_particle(const struct marfitude_pos *pos, const float *c);
static void dust_particle(const struct marfitude_pos *pos, const float *c);
static void draw_dust(const struct particle *);
static void draw_burst(const struct particle *);

void burst_init(void)
{
	register_event("row explosion", row_burst);
	register_event("note explosion", note_burst);
	register_event("victory", victory);
}

void burst_exit(void)
{
	deregister_event("victory", victory);
	deregister_event("note explosion", note_burst);
	deregister_event("row explosion", row_burst);
}

void row_burst(const void *data)
{
	const struct marfitude_pos *pos = data;
	float col[4];
	const float *pc;

	pc = get_player_color(marfitude_get_ac()[pos->channel].player);

	col[ALPHA] = 1.0;
	col[RED] = pc[RED];
	col[GREEN] = pc[GREEN];
	col[BLUE] = pc[BLUE];

	dust_particle(pos, col);
}

void note_burst(const void *data)
{
	const struct marfitude_note *sn = data;
	const float *pc;
	struct marfitude_pos pos;

	pos.tic = sn->tic;
	pos.channel = sn->col;

	/* note -> column -> player -> color... werd */
	pc = get_player_color(marfitude_get_ac()[sn->col].player);

	dust_particle(&pos, pc);
}

void victory(const void *data)
{
	struct marfitude_pos pos;
	const struct slist *list = data;
	const struct slist *t;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_player *ps;
	int num = 0;
	int x;

	num = slist_length(list);

	slist_foreach(t, list) {
		ps = t->data;
		for(x=0; x<48 / num; x++) {
			const float *c = get_player_color(ps->num);
			pos.tic = wam->num_tics;
			pos.channel = x % wam->num_cols;
			burst_particle(&pos, c);
		}
	}
}

void burst_particle(const struct marfitude_pos *pos, const float *c)
{
	struct particle *p = create_particle(draw_burst);
	if(!p) return;

	p->o.pos.v[0] = pos->channel;
	p->o.pos.v[1] = 0.0;
	p->o.pos.v[2] = pos->tic;
	p->o.vel.v[0] = 9.0 * (rand_float() - 0.5);
	p->o.vel.v[1] = 9.0 * (rand_float() - 0.5);
	p->o.vel.v[2] = 9.0 * (rand_float() - 0.5);
	p->c[0] = c[0];
	p->c[1] = c[1];
	p->c[2] = c[2];
	p->c[3] = c[3];

	marfitude_evalv(&p->o.pos);
}

void dust_particle(const struct marfitude_pos *pos, const float *c)
{
	struct particle *p = create_particle(draw_dust);
	if(!p) return;

	p->o.pos.v[0] = pos->channel;
	p->o.pos.v[1] = 0.0;
	p->o.pos.v[2] = pos->tic;
	p->o.vel.v[0] = 15.0 * (rand_float() - 0.5);
	p->o.vel.v[1] = 15.0 * (rand_float() - 0.5);
	p->o.vel.v[2] = 15.0 * (rand_float() - 0.5);
	p->c[0] = c[0];
	p->c[1] = c[1];
	p->c[2] = c[2];
	p->c[3] = c[3];

	marfitude_evalv(&p->o.pos);
}

void draw_burst(const struct particle *p)
{
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES); {
		union vector v;

		v.v[0] = p->o.pos.v[0] - p->o.vel.v[0] * 0.3;
		v.v[1] = p->o.pos.v[1] - p->o.vel.v[1] * 0.3;
		v.v[2] = p->o.pos.v[2] - p->o.vel.v[2] * 0.3;

		glColor4f(p->c[0], p->c[1], p->c[2], p->life);
		glVertex3dv(p->o.pos.v);
		glColor4f(1.0, 1.0, 1.0, 0.5 - p->life / 2.0);
		glVertex3dv(v.v);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void draw_dust(const struct particle *p)
{
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POINTS); {
		glColor4f(p->c[0], p->c[1], p->c[2], p->life);
		glVertex3dv(p->o.pos.v);
	}; glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}
