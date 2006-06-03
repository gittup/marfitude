#include <stdlib.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "explode.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/glfunc.h"
#include "gmae/particles.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

#include "util/memtest.h"
#include "util/myrand.h"
#include "util/slist.h"

static void explode(const void *);
static void victory(const void *);
static void explosion_particle(const struct marfitude_pos *pos, const float *c);
static void draw_particle(const struct particle *);

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
	const struct marfitude_pos *pos = data;
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

	explosion_particle(pos, col);
}

void victory(const void *data)
{
	const struct slist *list = data;
	const struct slist *t;
	struct marfitude_pos pos;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_player *ps;
	int num = 0;
	int x;

	num = slist_length(list);

	slist_foreach(t, list) {
		ps = t->data;
		for(x=0; x<48 / num; x++) {
			const float *c = get_player_color(ps->num);
			pos.tic = wam_row(wam, wam->num_rows)->ticpos;
			pos.channel = x % wam->num_cols;
			explosion_particle(&pos, c);
		}
	}
}

void explosion_particle(const struct marfitude_pos *pos, const float *c)
{
	struct particle *p = create_particle(draw_particle);
	if(!p) return;

	p->o.pos.v[0] = pos->channel;
	p->o.pos.v[2] = pos->tic;
	p->o.vel.v[0] = rand_float() - 0.5;
	p->o.vel.v[1] = 2.0 + rand_float();
	p->o.vel.v[2] = 13.0 + rand_float();
	p->o.rotvel = rand_float() * 720.0 - 360.0;
	p->o.acc.v[1] = -3.98;
	p->c[0] = c[0];
	p->c[1] = c[1];
	p->c[2] = c[2];
	p->c[3] = c[3];

	marfitude_evalv(&p->o.pos);
}

void draw_particle(const struct particle *p)
{
	glPushMatrix();
	glTranslated(p->o.pos.v[0], p->o.pos.v[1], p->o.pos.v[2]);
	setup_billboard();
	glRotatef(p->o.theta, p->o.axis.v[0], p->o.axis.v[1], p->o.axis.v[2]);

	glColor4fv(p->c);

	glBindTexture(GL_TEXTURE_2D, texture_num("StarBurst.png"));
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
	} glEnd();

	glBindTexture(GL_TEXTURE_2D, texture_num("StarCenter.png"));
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
	} glEnd();

	glPopMatrix();
}
