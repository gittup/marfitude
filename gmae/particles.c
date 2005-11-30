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
#include "glfunc.h"
#include "phys.h"
#include "textures.h"
#include "timer.h"

#include "util/memtest.h"

/** @file
 * Creates and draws particles
 */

/** Describes a single particle */
struct particle {
	void *data;                 /**< User data to draw the particle */
	void (*draw)(const void *); /**< The user draw function */
	void (*del)(void *);        /**< The user delete function */
	int active;    /**< 1 = drawn, 0 not drawn */
	float life;    /**< TTL in seconds */
};

static void draw_particle(struct particle *p);

static int particlesInited = 0;
static int numParticles;
static int curParticle;
static struct particle *particles;

/** Allocates all memory for the particle engine */
int init_particles(void)
{
	printf("Init particles\n");
	numParticles = cfg_get_int("video", "particles", 100);
	curParticle = 0;
	particles = (struct particle*)calloc(numParticles, sizeof(struct particle));
	particlesInited = 1;
	return 1;
}

/** Frees the particle engine */
void quit_particles(void)
{
	clear_particles();
	free(particles);
	printf("Particles shutdown\n");
	return;
}

void draw_particle(struct particle *p)
{
	p->draw(p->data);

	p->life -= timeDiff;
	if(p->life < 0.0) {
	       	p->active = 0;
		p->del(p->data);
	}
}

/** Draws all of the active particles */
void draw_particles(void)
{
	int x;
	Log(("Draw Particles()\n"));
	for(x=0;x<numParticles;x++)
	{
		if(particles[x].active)
			draw_particle(&particles[x]);
	}
	Log(("Draw Particles done\n"));
}

/** Create a particle using the object definition @a o.
 * @param data The user data to pass to the draw function.
 * @param draw The user's draw function. Will get the data passed in.
 */
void create_particle(void *data, void (*draw)(const void *), void (*del)(void *))
{
	struct particle *p;

	Log(("Create Particle()\n"));
	if(numParticles <= 0) return;

	p = &particles[curParticle];
	if(p->active)
		p->del(p->data);

	p->data = data;
	p->draw = draw;
	p->del = del;
	p->active = 1;
	p->life = 3.00;

	curParticle++;
	if(curParticle >= numParticles)
		curParticle = 0;

	Log(("Create Particle done\n"));
}

/** Deletes all of the objects associated with the particles */
void clear_particles(void)
{
	int x;
	Log(("clear_particles()\n"));
	for(x=0;x<numParticles;x++) {
		if(particles[x].active) {
			particles[x].active = 0;
			particles[x].del(particles[x].data);
		}
	}
	Log(("clear_particles done\n"));
}
