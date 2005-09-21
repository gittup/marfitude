#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "fireball.h"
#include "view.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

static void fireball_draw(const void *);

static int fireball_tex;
static float fireball[4];

void fireball_init(void)
{
	fireball_tex = texture_num("Fireball.png");
	fireball[0] = 0.0;
	fireball[1] = 0.5;
	fireball[2] = 0.0;
	fireball[3] = 1.0;

	register_event("draw transparent", fireball_draw);
}

void fireball_exit(void)
{
	deregister_event("draw transparent", fireball_draw);
}

void fireball_draw(const void *data)
{
	const struct wam *wam;
	struct marfitude_pos p;
	struct row *row;
	float sintmp;
	float bounceTime;

	if(data) {}
	marfitude_get_pos(&p);

	wam = marfitude_get_wam();
	row = wam_row(wam, p.row_index);
	bounceTime = 2.0 * 3.1415 * ((double)row->ticprt + p.tic - (double)row->ticpos) / (double)row->ticgrp;
	sintmp = sin(bounceTime);
	fireball[0] = -BLOCK_WIDTH * get_view_focus() + cos(bounceTime);
	fireball[1] = 1.0 + sintmp * sintmp;
	fireball[2] = TIC_HEIGHT * p.tic;
	glLightfv(GL_LIGHT1, GL_POSITION, fireball);

	glBindTexture(GL_TEXTURE_2D, fireball_tex);
	glNormal3f(0.0, 1.0, 0.0);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0);
		glVertex3f(fireball[0]-.5, fireball[1]-.5, fireball[2]);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(fireball[0]+.5, fireball[1]-.5, fireball[2]);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(fireball[0]+.5, fireball[1]+.5, fireball[2]);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(fireball[0]-.5, fireball[1]+.5, fireball[2]);
	} glEnd();
}

const float *fireball_get_pos(void)
{
	return fireball;
}
