#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_image.h"
#include "GL/gl.h"
#include "GL/glu.h"

#include "main.h"
#include "audio.h"
#include "cfg.h"
#include "event.h"
#include "glfunc.h"
#include "joy.h"
#include "log.h"
#include "menu.h"
#include "scene.h"
#include "sounds.h"
#include "textures.h"
#include "timer.h"

#include "../util/memtest.h"
#include "../util/fatalerror.h"
#include "../util/sdlfatalerror.h"
#include "../util/savetga.h"

int quit = 0;
Scene *activeScene = NULL;
Menu *activeMenu = NULL;

void Shutdown()
{
	SwitchScene(NULLSCENE);
	SwitchMenu(NULLMENU);
//	QuitMusic();
	QuitSounds();
	QuitTextures();
	QuitJoystick();
	QuitGL();
	QuitAudio();
	QuitConfig();
	QuitLog();
}

int main(int argc, char **argv)
{
	SDL_Surface *screen;

	if(!InitLog())
	{
		printf("Error creating log file!");
		Shutdown();
		return 1;
	}

	if(!InitConfig())
	{
		Log("ERROR: Couldn't set configuration options!\n");
		Shutdown();
		return 1;
	}

	if(!InitAudio())
	{
		Log("ERROR: Couldn't start audio!\n");
		Shutdown();
		return 1;
	}

	screen = InitGL();
	if(screen == NULL)
	{
		Log("Error setting up the screen!\n");
		Shutdown();
		return 1;
	}

	if(!InitTextures())
	{
		Log("ERROR: Couldn't load textures!\n");
		Shutdown();
		return 1;
	}

	if(!InitSounds())
	{
		Log("ERROR: Couldn't load sounds!\n");
		Shutdown();
		return 1;
	}

	InitJoystick();

	SDL_EnableKeyRepeat(0, 0); // disable key repeating
	SDL_ShowCursor(SDL_DISABLE);
	ConfigureJoyKey();

	SwitchScene(INTROSCENE);
	if(!SwitchScene(MAINSCENE))
	{
		Log("Error switching to main scene.\n");
		Shutdown();
		return 1;
	}
	if(!SwitchMenu(NOMENU))
	{
		Log("Error enabling the menu.\n");
		Shutdown();
		return 1;
	}

	ClearEvents(); // clears event cue, has nothing to do with registering

	while(!quit)
	{
		Log("EventLoop\n");
		EventLoop();
		Log("UpdateTimer\n");
		UpdateTimer();
		Log("Scene Render\n");
		activeScene->Render();
		Log("Menu Render\n");
		activeMenu->Render();
		Log("Update Screen\n");
		UpdateScreen();
		Log("Next loop\n");
	}
	
	Shutdown();
	return 0;
}
