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

#define MAX_LIFE 3.0

/** @file
 * Creates and draws particles
 */

/** Contains a particle and the information needed to draw it. */
struct particle_info {
	struct particle p; /**< The underlying particle */
	void (*draw)(const struct particle *); /**< The user draw function */
	int active; /**< 1 = drawn, 0 not drawn */
};

static int particlesInited = 0;
static int numParticles;
static int curParticle;
static struct particle_info *particles;
static void free_particle(struct particle_info *pi);

/** Allocates all memory for the particle engine */
int init_particles(void)
{
	printf("Init particles\n");
	numParticles = cfg_get_int("video", "particles", 128);
	curParticle = 0;
	particles = calloc(numParticles, sizeof(*particles));
	particlesInited = 1;
	return 0;
}

/** Frees the particle engine */
void quit_particles(void)
{
	clear_particles();
	free(particles);
	printf("Particles shutdown\n");
	return;
}

/** Draws all of the active particles */
void draw_particles(void)
{
	int x;
	Log(("Draw Particles()\n"));
	for(x=0;x<numParticles;x++)
	{
		struct particle_info *pi = &particles[x];
		if(pi->active) {
			pi->draw(&pi->p);

			pi->p.life -= timeDiff / MAX_LIFE;
			if(pi->p.life <= 0.0) {
				free_particle(pi);
			}
		}
	}
	Log(("Draw Particles done\n"));
}

/** Create a particle using the object definition @a o.
 * @param data The user data to pass to the draw function.
 * @param draw The user's draw function. Will get the data passed in.
 */
struct particle *create_particle(void (*draw)(const struct particle *))
{
	struct particle_info *pi;

	Log(("Create Particle()\n"));
	if(numParticles <= 0) return 0;

	pi = &particles[curParticle];
	if(pi->active) {
		free_particle(pi);
	}

	new_obj(&pi->p.o);
	pi->p.c[0] = 1.0;
	pi->p.c[1] = 1.0;
	pi->p.c[2] = 1.0;
	pi->p.c[3] = 1.0;
	pi->p.life = 1.0;
	pi->draw = draw;
	pi->active = 1;

	curParticle++;
	if(curParticle >= numParticles)
		curParticle = 0;

	Log(("Create Particle done\n"));
	return &pi->p;
}

/** Deletes all of the objects associated with the particles */
void clear_particles(void)
{
	int x;
	Log(("clear_particles()\n"));
	for(x=0;x<numParticles;x++) {
		if(particles[x].active) {
			free_particle(&particles[x]);
		}
	}
	Log(("clear_particles done\n"));
}

/** Frees the particle and makes it inactive. */
void free_particle(struct particle_info *pi)
{
	pi->active = 0;
	delete_obj(&pi->p.o);
}
