#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

void __attribute__ ((constructor)) fireball_init(void);
void __attribute__ ((destructor)) fireball_exit(void);
static void fireball_draw(const void *);

static int fireball_tex;
float fireball[4];

void fireball_init(void)
{
	fireball_tex = texture_num("Fireball.png");
	fireball[0] = 0.0;
	fireball[1] = 0.5;
	fireball[2] = 0.0;
	fireball[3] = 1.0;

	register_event("draw transparent", fireball_draw, EVENTTYPE_MULTI);
}

void fireball_exit(void)
{
	deregister_event("draw transparent", fireball_draw);
}

void fireball_draw(const void *data)
{
	struct marfitude_pos p;
	float sintmp;
	float bounceTime;

	if(data) {}
	marfitude_get_pos(&p);

	bounceTime = 2.0 * 3.1415 * ((double)p.row->ticprt + p.tic - (double)p.row->ticpos) / (double)p.row->ticgrp;
	sintmp = sin(bounceTime);
	fireball[0] = -BLOCK_WIDTH * p.channel + cos(bounceTime);
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
