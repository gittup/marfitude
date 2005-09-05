/*
   Marfitude
   Copyright (C) 2005 Mike Shal

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>

#include "SDL_opengl.h"

#include "particles.h"
#include "cfg.h"
#include "log.h"
#include "phys.h"
#include "textures.h"
#include "timer.h"

#include "util/memtest.h"

/** @file
 * Creates and draws particles
 */

static void GenPoint(void);
static void GenLine(void);
static void GenTQuad(GLuint tex);
static void Gen2TQuad(GLuint tex1, GLuint tex2);
static void DrawParticle(struct particle *p);

static GLuint plist;
static int particlesInited = 0;
static int numParticles;
static int curParticle;
static struct particle *particles;
static struct particleType particleTypes[] = {
	{PT_POINT, 0, 0, 0},
	{PT_LINE, 0, 0, 0},
	{PT_TQUAD, 1, "BlueNova.png", 0},
	{PT_TQUAD, 1, "BlueStar.png", 0},
	{PT_TQUAD, 1, "Fireball.png", 0},
	{PT_2TQUAD, 1, "StarBurst.png", "StarCenter.png"},
	{PT_2TQUAD, 1, "SunBurst.png", "SunCenter.png"}
};

void GenPoint(void)
{
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POINTS);
	{
		glVertex3f(0.0, 0.0, 0.0);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void GenLine(void)
{
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	{
		glVertex3f(0.0, 0.5, 0.0);
		glVertex3f(0.0, -0.5, 0.0);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void GenTQuad(GLuint tex)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
	} glEnd();

	glPopMatrix();
}

void Gen2TQuad(GLuint tex1, GLuint tex2)
{
	glBindTexture(GL_TEXTURE_2D, tex1);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
	} glEnd();

	glBindTexture(GL_TEXTURE_2D, tex2);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
	} glEnd();

	glPopMatrix();
}

/** Allocates all memory for the particle engine */
int init_particles(void)
{
	int x;
	int numpTypes;
	int t1, t2;
	struct particleType *pt;

	numpTypes = sizeof(particleTypes) / sizeof(struct particleType);
	printf("Init particles\n");
	numParticles = cfg_get_int("video", "particles", 100);
	curParticle = 0;
	particles = (struct particle*)calloc(numParticles, sizeof(struct particle));
	plist = glGenLists(numpTypes);
	particlesInited = 1;
	for(x=0;x<numpTypes;x++)
	{
		pt = &particleTypes[x];

		/* these are defined at runtime, so we set the true texture
		 * values now
		 */
		t1 = texture_num(pt->tex1);
		t2 = texture_num(pt->tex2);

		glNewList(plist+x, GL_COMPILE);
		switch(pt->type)
		{
			case PT_POINT:
				GenPoint();
				break;
			case PT_LINE:
				GenLine();
				break;
			case PT_TLINE:
			case PT_TQUAD:
				GenTQuad(t1);
				break;
			case PT_2TQUAD:
				Gen2TQuad(t1, t2);
				break;
			default:
				ELog(("Error: Invalid particle type\n"));
				return 0;
		}
		glEndList();
	}
	return 1;
}

/** Frees the particle engine */
void quit_particles(void)
{
	clear_particles();
	free(particles);
	glDeleteLists(plist, sizeof(particleTypes) / sizeof(struct particle));
	printf("Particles shutdown\n");
	return;
}

void DrawParticle(struct particle *p)
{
	float mat[16];
	int i, j;
	struct particleType *pt;

	pt = &particleTypes[p->type];
	glPushMatrix();
	glTranslated(p->o->pos.x, p->o->pos.y, p->o->pos.z);
	if(pt->billboard)
	{
		glGetFloatv(GL_MODELVIEW_MATRIX, mat);
		for(i=0;i<3;i++)
			for(j=0;j<3;j++)
			{
				if(i == j) mat[i+j*4] = 1.0;
				else mat[i+j*4] = 0.0;
			}
		glLoadMatrixf(mat);
	}
	glRotatef(p->o->theta, p->o->axis.x, p->o->axis.y, p->o->axis.z);
	if(pt->type == PT_POINT)
		glPointSize(p->size);
	else
		glScalef(p->size, p->size, p->size);
	glColor4fv(p->col);
	glCallList(plist+p->type);
	p->life -= timeDiff;
	if(p->life < 0.0)
	{
	       	p->active = 0;
		delete_obj(p->o);
	}
}

/** Draws all of the active particles */
void draw_particles(void)
{
	int x;
	Log(("Draw Particles()\n"));
	for(x=0;x<numParticles;x++)
	{
		if(particles[x].active) DrawParticle(&particles[x]);
	}
	Log(("Draw Particles done\n"));
}

/** Draws all particles that pass the testing function @a p */
void draw_particles_test(PTestFunc p)
{
	int x;
	Log(("draw_particles_test()\n"));
	for(x=0;x<numParticles;x++)
	{
		if(particles[x].active && p(&particles[x])) DrawParticle(&particles[x]);
	}
	Log(("draw_particles_test() done\n"));
}

/** Create a particle using the object definition @a o.
 * @param o The physics object that allows this particle to move
 * @param col The color of the particle
 * @param type Which particle to draw
 * @param size How big the particle is (somewhat dependent on the type)
 */
void create_particle(struct obj *o, const float col[4], int type, float size)
{
	struct particle *p;
	Log(("Create Particle()\n"));
	if(numParticles <= 0) return;
	p = &particles[curParticle];
	if(p->active) delete_obj(p->o);
	p->o = o;
	p->col[0] = col[0];
	p->col[1] = col[1];
	p->col[2] = col[2];
	p->col[3] = col[3];
	p->type = type;
	p->size = size;
	p->active = 1;
	p->life = 3.00;
	curParticle++;
	if(curParticle >= numParticles) curParticle = 0;
	Log(("Create Particle done\n"));
}

/** Deletes all of the objects associated with the particles */
void clear_particles(void)
{
	int x;
	Log(("clear_particles()\n"));
	for(x=0;x<numParticles;x++)
	{
		if(particles[x].active)
		{
			particles[x].active = 0;
			delete_obj(particles[x].o);
		}
	}
	Log(("clear_particles done\n"));
}

/** Gets the particle texture associated with particle type @a p */
GLuint particle(enum particleTypes p)
{
	return plist+p;
}
