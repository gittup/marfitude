/*
   Marfitude
   Copyright (C) 2004 Mike Shal

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

#include "phys.h"

enum particleTypes {
	P_Point = 0,
	P_Line,
	P_BlueNova,
	P_BlueStar,
	P_Fireball,
	P_StarBurst,
	P_SunBurst,
	P_LAST
};

enum particleListTypes {
	PT_POINT = 0,
	PT_LINE,
	PT_TLINE,
	PT_TQUAD,
	PT_2TQUAD,
	PT_LAST
};

struct particle {
	struct obj *o;	/* used for physics stuff */
	float col[4];	/* RGBA colors */
	int type;	/* one of ParticleTypes above */
	float size;	/* dimension of particle */
	int active;	/* 1 = drawn, 0 not drawn */
	float life;	/* TTL in seconds */
};

struct particleType {
	int type;		/* one of ParticleListTypes above */
	int billboard;		/* 1 = billboarded, 0 = not billboarded */
	const char *tex1;	/* first texture (if necessary) */
	const char *tex2;	/* second texture (if necessary) */
};

typedef int (*PTestFunc)(struct particle *);

int InitParticles(void);
void QuitParticles(void);
void DrawParticles(void);
void DrawParticlesTest(PTestFunc p);
void StartParticles(void);	/* set up OpenGL variables */
void StopParticles(void);	/* reset OpenGL variables */
void CreateParticle(struct obj *o, float col[4], int type, float size);
void ClearParticles(void);

extern GLuint plist;
