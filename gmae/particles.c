#include <stdio.h>
#include <stdlib.h>

#include "GL/gl.h"

#include "particles.h"
#include "cfg.h"
#include "log.h"
#include "textures.h"
#include "timer.h"
#include "../util/memtest.h"

int particlesInited = 0;
int numParticles;
int curParticle;
GLuint plist;
Particle *particles;
ParticleType particleTypes[] = {
	{PT_POINT, 0, 0, 0},
	{PT_LINE, 0, 0, 0},
	{PT_TQUAD, 1, T_BlueNova, 0},
	{PT_TQUAD, 1, T_BlueStar, 0},
	{PT_TQUAD, 1, T_Fireball, 0},
	{PT_2TQUAD, 1, T_StarBurst, T_StarCenter},
	{PT_2TQUAD, 1, T_SunBurst, T_SunCenter}
	};

void GenPoint()
{
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POINTS);
	{
		glVertex3f(0.0, 0.0, 0.0);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void GenLine()
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

int InitParticles()
{
	int x;
	int numpTypes;
	ParticleType *pt;

	numpTypes = sizeof(particleTypes) / sizeof(ParticleType);
	printf("Init particles\n");
	numParticles = CfgI("video.particles");
	curParticle = 0;
	particles = (Particle*)calloc(numParticles, sizeof(Particle));
	plist = glGenLists(numpTypes);
	particlesInited = 1;
	for(x=0;x<numpTypes;x++)
	{
		pt = &particleTypes[x];

		// these are defined at runtime, so we set the true texture
		// values now
		pt->tex1 = GLTexture[pt->tex1];
		pt->tex2 = GLTexture[pt->tex2];

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
				GenTQuad(pt->tex1);
				break;
			case PT_2TQUAD:
				Gen2TQuad(pt->tex1, pt->tex2);
				break;
			default:
				ELog("Error: Invalid particle type\n");
				return 0;
		}
		glEndList();
	}
	return 1;
}

void QuitParticles()
{
	free(particles);
	glDeleteLists(plist, sizeof(particleTypes) / sizeof(Particle));
	printf("Particles Shutdown\n");
	return;
}

void DrawParticle(Particle *p)
{
	float mat[16];
	int i, j;
	ParticleType *pt;

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
		DeleteObj(p->o);
	}
}

void DrawParticles()
{
	int x;
	Log("Draw Particles()\n");
	for(x=0;x<numParticles;x++)
	{
		if(particles[x].active) DrawParticle(&particles[x]);
	}
	Log("Draw Particles done\n");
}

void StartParticles()
{
	glDisable(GL_LIGHTING);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
}

void StopParticles()
{
	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);
}

void CreateParticle(Obj *o, float col[4], int type, float size)
{
	Particle *p;
	Log("Create Particle()\n");
	if(numParticles <= 0) return;
	p = &particles[curParticle];
	if(p->active) DeleteObj(p->o);
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
	Log("Create Particle done\n");
}

void ClearParticles()
{
	int x;
	Log("ClearParticles()\n");
	for(x=0;x<numParticles;x++)
	{
		if(particles[x].active)
		{
			particles[x].active = 0;
			DeleteObj(particles[x].o);
		}
	}
	Log("ClearParticles done\n");
}
