#include "SDL.h"
#include "SDL_opengl.h"

#include "fps.h"
#include "glfunc.h"
#include "timer.h"

Uint32 fpsticks, frames;
float fps;

void InitFPS(void)
{
	frames = 0;
	fpsticks = 0;
	fps = 0.0;
}

void PrintFPS(void)
{
	SetFontSize(.75);
	glColor3f(0.0, 1.0, 1.0);
	if(fps < 100.0) PrintGL(DisplayWidth() - 75, 5, "FPS: %2.1f", fps);
	else PrintGL(DisplayWidth() - 75, 5, "FPS:%3.1f", fps);
	SetFontSize(1.0);
}

void UpdateFPS(void)
{
	frames++;
	fpsticks += ticDiff;
	if(fpsticks >= 1000) /* one second */
	{
		fps = (float)frames*1000.0/(float)(fpsticks);
		frames = 0;
		fpsticks = 0;
	}
}
