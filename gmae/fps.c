#include "SDL.h"

#include "GL/gl.h"

#include "glfunc.h"
#include "timer.h"

Uint32 fpsticks, frames;
float fps;

void InitFPS()
{
	frames = 0;
	fpsticks = 0;
	fps = 0.0;
}

void PrintFPS()
{
	SetFontSize(.75);
	glColor3f(0.0, 1.0, 1.0);
	if(fps < 100.0) PrintGL(DisplayWidth() - 75, 5, "FPS: %2.1f", fps);
	else PrintGL(DisplayWidth() - 75, 5, "FPS:%3.1f", fps);
	SetFontSize(1.0);
}

void UpdateFPS()
{
	frames++;
	fpsticks += timeDiff;
	if(fpsticks >= 1000) // one second
	{
		fps = (float)frames*1000.0/(float)(fpsticks);
		frames = 0;
		fpsticks = 0;
	}
}
