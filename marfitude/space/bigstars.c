#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/textures.h"

#include "util/slist.h"

static void bigstars_init(void) __attribute__((constructor));
static void bigstars_exit(void) __attribute__((destructor));
static void draw_stars(const void *);
static void createstar0(unsigned char *p, int x, int y);
static void createstar1(unsigned char *p, int x, int y);
static void createstar(unsigned char *p, int x, int y, int rad);
static double distance(int xa, int ya, int xb, int yb);
static const int width = 128;
static const int height = 128;
static int stars[2];

void bigstars_init(void)
{
	create_texture(&stars[0], width, height, createstar0);
	create_texture(&stars[1], width, height, createstar1);
	register_event("draw transparent", draw_stars);
}

void bigstars_exit(void)
{
	deregister_event("draw transparent", draw_stars);
	delete_texture(&stars[1]);
	delete_texture(&stars[0]);
}

double distance(int xa, int ya, int xb, int yb)
{
	return sqrt((double)((xb-xa) * (xb-xa) + (yb-ya) * (yb-ya)));
}

void createstar0(unsigned char *p, int x, int y)
{
	createstar(p, x, y, 64);
}

void createstar1(unsigned char *p, int x, int y)
{
	createstar(p, x, y, 82);
}


void createstar(unsigned char *p, int x, int y, int rad)
{
	int u;
	int v;
	int cx;
	int cy;
	double var;

	u = width / 2 - x;
	v = height / 2 - y;

	p[RED] = 255;
	p[GREEN] = 255;
	p[BLUE] = 255;

	if(u == 0 && v == 0) {
		p[ALPHA] = 255;
		return;
	}

	if(u > 0)
		cx = width / 2;
	else
		cx = -width / 2;
	if(v > 0)
		cy = height / 2;
	else
		cy = -height / 2;

	var = distance(u, v, cx, cy);
	if(var < rad)
		p[ALPHA] = 0;
	else
		p[ALPHA] = (var - rad) * 255 / (90 - rad);
}

void draw_stars(const void *data)
{
	const struct slist *t;
	struct marfitude_pos p;

	if(data) {}
	marfitude_get_pos(&p);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	slist_foreach(t, marfitude_get_hitnotes()) {
		struct marfitude_note *sn = t->data;

		glPushMatrix();
		glTranslated(   sn->pos.v[0],
				sn->pos.v[1],
				sn->pos.v[2]+0.3);
		setup_billboard();

		glColor4f(1.0, 1.0, 1.0, 1.0);
		if(sn->tic - p.tic <= 0)
			glBindTexture(GL_TEXTURE_2D, stars[0]);
		else
			glBindTexture(GL_TEXTURE_2D, stars[1]);

		glBegin(GL_QUADS); {
			glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
			glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
			glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
			glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
		} glEnd();

		glPopMatrix();
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
