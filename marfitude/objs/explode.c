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

#include "util/myrand.h"

struct explode {
	struct obj o;
	float c[4];
};

static void explode(const void *);
static void victory(const void *);
static void explosion_particle(const struct marfitude_pos *p, const float *c);
static void draw_particle(const void *);
static void free_particle(void *data);

static int tex1;
static int tex2;

void explode_init(void)
{
	tex1 = texture_num("StarBurst.png");
	tex2 = texture_num("StarCenter.png");
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
	struct explode *e;

	e = malloc(sizeof(struct explode));

	new_obj(&e->o);
	e->o.pos.v[0] = -p->channel * 2.0;
	e->o.pos.v[2] = TIC_HEIGHT * p->tic;
	e->o.vel.v[0] = rand_float() - 0.5;
	e->o.vel.v[1] = 2.0 + rand_float();
	e->o.vel.v[2] = 13.0 + rand_float();
	e->o.rotvel = rand_float() * 720.0 - 360.0;
	e->o.acc.v[1] = -3.98;
	e->c[0] = c[0];
	e->c[1] = c[1];
	e->c[2] = c[2];
	e->c[3] = c[3];

	create_particle(e, draw_particle, free_particle);
}

void draw_particle(const void *data)
{
	const struct explode *e = data;

	glPushMatrix();
	glTranslated(e->o.pos.v[0], e->o.pos.v[1], e->o.pos.v[2]);
	setup_billboard();
	glRotatef(e->o.theta, e->o.axis.v[0], e->o.axis.v[1], e->o.axis.v[2]);

	glColor4fv(e->c);

	glBindTexture(GL_TEXTURE_2D, tex1);
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
	} glEnd();

	glBindTexture(GL_TEXTURE_2D, tex2);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
	} glEnd();

	glPopMatrix();
}

void free_particle(void *data)
{
	struct explode *e = data;

	delete_obj(&e->o);
	free(e);
}
