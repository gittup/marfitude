#include <stdlib.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/particles.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

#include "util/myrand.h"

struct burst {
	struct obj o;
	union vector origin;
	float c[4];
};

static void burst_init(void) __attribute__((constructor));
static void burst_exit(void) __attribute__((destructor));
static void row_burst(const void *);
static void note_burst(const void *);
static void victory(const void *);
static void burst_particle(const struct marfitude_pos *p, const float *c);
static void draw_particle(const void *);
static void free_particle(void *);

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

	col[ALPHA] = rand_float();
	col[RED] = pc[RED];
	col[GREEN] = pc[GREEN];
	col[BLUE] = pc[BLUE];

	burst_particle(p, col);
}

void note_burst(const void *data)
{
	const struct marfitude_note *sn = data;
	struct marfitude_pos pos;
	float col[4];

	pos.tic = sn->tic;
	pos.channel = sn->col;

	col[ALPHA] = rand_float();
	col[RED] = 1.0;
	col[GREEN] = 1.0;
	col[BLUE] = 1.0;

	burst_particle(&pos, col);
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
				burst_particle(&pos, c);
			}
		}
	}
}

void burst_particle(const struct marfitude_pos *p, const float *c)
{
	struct burst *b;

	b = malloc(sizeof(struct burst));
	new_obj(&b->o);

	b->o.pos.v[0] = -p->channel * 2.0;
	b->o.pos.v[2] = TIC_HEIGHT * p->tic;
	b->o.vel.v[0] = 9.0 * (rand_float() - 0.5);
	b->o.vel.v[1] = 9.0 * (rand_float() - 0.5);
	b->o.vel.v[2] = 9.0 * (rand_float() - 0.5);
	b->c[0] = c[0];
	b->c[1] = c[1];
	b->c[2] = c[2];
	b->c[3] = c[3];

	create_particle(b, draw_particle, free_particle);
}

void draw_particle(const void *data)
{
	const struct burst *b = data;

	glPushMatrix();
	glTranslated(b->o.pos.v[0], b->o.pos.v[1], b->o.pos.v[2]);
	glColor4fv(b->c);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POINTS); {
		glVertex3f(0.0, 0.0, 0.0);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void free_particle(void *data)
{
	struct burst *b = data;

	delete_obj(&b->o);
	free(b);
}
