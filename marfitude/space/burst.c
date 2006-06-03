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

struct burst {
	struct obj o;
	float c[4];
};

static void burst_init(void) __attribute__((constructor));
static void burst_exit(void) __attribute__((destructor));
static void row_burst(const void *);
static void note_burst(const void *);
static void victory(const void *);
static void burst_particle(const struct marfitude_pos *p, const float *c);
static void dust_particle(const struct marfitude_pos *p, const float *c);
static void draw_dust(const void *, float);
static void draw_burst(const void *, float);
static void free_burst(void *);

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
	const struct marfitude_pos *p = data;
	float col[4];
	const float *pc;

	pc = get_player_color(marfitude_get_ac()[p->channel].player);

	col[ALPHA] = 1.0;
	col[RED] = pc[RED];
	col[GREEN] = pc[GREEN];
	col[BLUE] = pc[BLUE];

	dust_particle(p, col);
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

void burst_particle(const struct marfitude_pos *p, const float *c)
{
	struct burst *b;

	b = malloc(sizeof(struct burst));
	new_obj(&b->o);

	b->o.pos.v[0] = -2.0 * p->channel;
	b->o.pos.v[1] = 0.0;
	b->o.pos.v[2] = TIC_HEIGHT * p->tic;
	b->o.vel.v[0] = 9.0 * (rand_float() - 0.5);
	b->o.vel.v[1] = 9.0 * (rand_float() - 0.5);
	b->o.vel.v[2] = 9.0 * (rand_float() - 0.5);
	b->c[0] = c[0];
	b->c[1] = c[1];
	b->c[2] = c[2];
	b->c[3] = c[3];

	create_particle(b, draw_burst, free_burst);
}

void dust_particle(const struct marfitude_pos *p, const float *c)
{
	struct burst *b;

	b = malloc(sizeof(struct burst));
	new_obj(&b->o);

	b->o.pos.v[0] = -2.0 * p->channel;
	b->o.pos.v[1] = 0.0;
	b->o.pos.v[2] = TIC_HEIGHT * p->tic;
	b->o.vel.v[0] = 15.0 * (rand_float() - 0.5);
	b->o.vel.v[1] = 15.0 * (rand_float() - 0.5);
	b->o.vel.v[2] = 15.0 * (rand_float() - 0.5);
	b->c[0] = c[0];
	b->c[1] = c[1];
	b->c[2] = c[2];
	b->c[3] = c[3];

	create_particle(b, draw_dust, free_burst);
}

void draw_burst(const void *data, float life)
{
	const struct burst *b = data;

	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES); {
		union vector v;

		v.v[0] = b->o.pos.v[0] - b->o.vel.v[0] * 0.3;
		v.v[1] = b->o.pos.v[1] - b->o.vel.v[1] * 0.3;
		v.v[2] = b->o.pos.v[2] - b->o.vel.v[2] * 0.3;

		glColor4f(b->c[0], b->c[1], b->c[2], life);
		glVertex3dv(b->o.pos.v);
		glColor4f(1.0, 1.0, 1.0, 0.5 - life / 2.0);
		glVertex3dv(v.v);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void free_burst(void *data)
{
	struct burst *b = data;

	delete_obj(&b->o);
	free(b);
}

void draw_dust(const void *data, float life)
{
	const struct burst *b = data;

	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POINTS); {
		glColor4f(b->c[0], b->c[1], b->c[2], life);
		glVertex3dv(b->o.pos.v);
	}; glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}
