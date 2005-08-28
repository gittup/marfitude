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

/** @file
 * A particle manager
 */

struct obj;

/** Describes the different particles that can be drawn */
enum particleTypes {
	P_Point = 0, /**< A simple point particle */
	P_Line,      /**< A simple line particle */
	P_BlueNova,  /**< A textured blue nova particle */
	P_BlueStar,  /**< A textured blue start particle */
	P_Fireball,  /**< A textured fireball particle */
	P_StarBurst, /**< A 2x-textured starburst particle */
	P_SunBurst,  /**< A 2x-textured sunburst particle */
	P_LAST
};

/** Describes how particles are drawn */
enum particleListTypes {
	PT_POINT = 0, /**< Draw as GL_POINTS */
	PT_LINE,      /**< Draw as GL_LINES */
	PT_TLINE,     /**< Draw as a textured line */
	PT_TQUAD,     /**< Draw as a textured quad */
	PT_2TQUAD,    /**< Draw two textured quads */
	PT_LAST
};

/** Describes a single particle */
struct particle {
	struct obj *o; /**< used for physics stuff */
	float col[4];  /**< RGBA colors */
	int type;      /**< one of ParticleTypes enum */
	float size;    /**< dimension of particle */
	int active;    /**< 1 = drawn, 0 not drawn */
	float life;    /**< TTL in seconds */
};

/** Links a particle list type with textures and billboarding, if used */
struct particleType {
	int type;         /**< one of ParticleListTypes above */
	int billboard;    /**< 1 = billboarded, 0 = not billboarded */
	const char *tex1; /**< first texture (if necessary) */
	const char *tex2; /**< second texture (if necessary) */
};

/** A function pointer to test a particle for drawing */
typedef int (*PTestFunc)(struct particle *);

GLuint particle(enum particleTypes p);
int init_particles(void);
void quit_particles(void);
void draw_particles(void);
void draw_particles_test(PTestFunc p);
void create_particle(struct obj *o, const float col[4], int type, float size);
void clear_particles(void);
