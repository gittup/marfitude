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

#include <stdio.h>
#include <math.h>

#include "SDL_opengl.h"

#include "scene.h"
#include "log.h"
#include "main.h"
#include "mainscene.h"

static int NullInit(void);
static void NullScene(void);
static void NullQuit(void);
static int IntroInit(void);
static void IntroScene(void);
static void IntroQuit(void);

#define NUMSCENES 3
static struct scene scenes[NUMSCENES] = {
	{NullInit, NullQuit, NullScene},
	{IntroInit, IntroQuit, IntroScene},
	{MainInit, MainQuit, MainScene}
};

int SwitchScene(int scene)
{
	if(scene < 0 || scene >= NUMSCENES) return 0;
	Log(("Switching scene: %i\n", scene));
	if(activeScene) activeScene->QuitScene();
	if(scenes[scene].InitScene()) {
		activeScene = &(scenes[NULLSCENE]);
		ELog(("Scene switch failed\n"));
		return 1;
	}
	activeScene = &(scenes[scene]);
	Log(("Scene switched\n"));
	return 0;
}

int SceneActive(int scene)
{
	if(activeScene == &(scenes[scene])) return 1;
	return 0;
}

void NullScene(void) {}
void NullQuit(void) {}
int NullInit() {return 0;}

int IntroInit()
{
	return 0;
}

void IntroQuit(void)
{
}

void IntroScene(void)
{
	static float theta = 0.0;
  glLoadIdentity();				/* Reset The View */

  glTranslatef(-1.5f,0.0f,-6.0f);		/* Move Left 1.5 Units And Into The Screen 6.0 */
	
  glDisable(GL_TEXTURE_2D);
  /* draw a triangle */
  glBegin(GL_POLYGON);				/* start drawing a polygon */
  glTexCoord2f(0.0, 0.0); glVertex3f( 0.0f, 1.0f, 0.0f);		/* Top */
  glTexCoord2f(1.0, 1.0); glVertex3f( 1.0f,-1.0f, 0.0f);		/* Bottom Right */
  glTexCoord2f(0.0, 1.0); glVertex3f(-1.0f,-1.0f, 0.0f);		/* Bottom Left	 */
  glEnd();					/* we're done with the polygon */

  glTranslatef(3.0f,0.0f,0.0f);		        /* Move Right 3 Units */
	
  /* draw a square (quadrilateral) */
  glBegin(GL_QUADS);				/* start drawing a polygon (4 sided) */
  glTexCoord2f(0.0, 0.0); glVertex3f(-cos(theta), cos(theta), sin(theta));	/* Top Left */
  glTexCoord2f(1.0, 0.0); glVertex3f( cos(theta), cos(theta), sin(theta));	/* Top Right */
  glTexCoord2f(1.0, 1.0); glVertex3f( cos(theta),-cos(theta), sin(theta));	/* Bottom Right */
  glTexCoord2f(0.0, 1.0); glVertex3f(-cos(theta),-cos(theta), sin(theta));	/* Bottom Left	 */
  glEnd();					/* done with the polygon */
  glEnable(GL_TEXTURE_2D);
  theta += .01;
  /* swap buffers to display, since we're double buffered. */
}
