#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "fireball.h"
#include "view.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

static void fireball_draw(const void *);

static union vector fireball;
static float origin[4] = {0.0, 0.0, 0.0, 1.0};

void fireball_init(void)
{
	fireball.v[0] = 0.0;
	fireball.v[1] = 0.5;
	fireball.v[2] = 0.0;
	fireball.v[3] = 1.0;

	register_event("draw transparent", fireball_draw);
}

void fireball_exit(void)
{
	deregister_event("draw transparent", fireball_draw);
}

void fireball_draw(const void *data)
{
	const struct wam *wam;
	struct marfitude_pos pos;
	struct row *row;
	float sintmp;
	float bounceTime;

	if(data) {}
	if(marfitude_num_players() != 1)
		return;
	marfitude_get_pos(&pos);

	wam = marfitude_get_wam();
	row = wam_row(wam, pos.row_index);
	bounceTime = 2.0 * 3.1415 * ((double)row->ticprt + pos.tic - (double)row->ticpos) / (double)row->ticgrp;
	sintmp = sin(bounceTime);
	fireball.v[0] = get_view_focus() + cos(bounceTime) / 2.0;
	fireball.v[1] = 1.0 + sintmp * sintmp;
	fireball.v[2] = pos.tic;

	glPushMatrix();
	marfitude_translatev(&fireball);
	glLightfv(GL_LIGHT1, GL_POSITION, origin);

	glBindTexture(GL_TEXTURE_2D, texture_num("Fireball.png"));
	glNormal3f(0.0, 1.0, 0.0);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, +0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(+0.5, +0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(+0.5, -0.5, 0.0);
	} glEnd();
	glPopMatrix();
}

const union vector *fireball_get_pos(void)
{
	return &fireball;
}
