#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

/* tmp */
extern int curTic;
extern double partialTic;
extern int channelFocus;
extern struct row *curRow;
/* end tmp */



void __attribute__ ((constructor)) fireball_init(void);
void __attribute__ ((destructor)) fireball_exit(void);
static void fireball_draw(const void *);

static int fireball_tex;
float fireball[4];

void fireball_init(void)
{
	fireball_tex = TextureNum("Fireball.png");
	fireball[0] = 0.0;
	fireball[1] = 0.5;
	fireball[2] = 0.0;
	fireball[3] = 1.0;

	RegisterEvent("draw transparent", fireball_draw, EVENTTYPE_MULTI);
}

void fireball_exit(void)
{
	DeregisterEvent("draw transparent", fireball_draw);
}

void fireball_draw(const void *data)
{
	float sintmp;
	float bounceTime;

	if(data) {}

	bounceTime = 2.0 * 3.1415 * ((double)curRow->ticprt + (double)curTic - (double)curRow->ticpos + partialTic) / (double)curRow->ticgrp;
	sintmp = sin(bounceTime);
	fireball[0] = -BLOCK_WIDTH * channelFocus + cos(bounceTime);
	fireball[1] = 1.0 + sintmp * sintmp;
	fireball[2] = TIC_HEIGHT * ((double)curTic + partialTic);
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
