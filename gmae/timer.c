#include <stdio.h>
#include "SDL.h"

Uint32 curTime;  // current time since initialization
Uint32 ticDiff;	 // number of ticks between frames
double timeDiff; // time (seconds) between frames

void InitTimer()
{
	curTime = SDL_GetTicks();
	timeDiff = 0;
}

void UpdateTimer()
{
	Uint32 tmp;
	tmp = SDL_GetTicks();
	ticDiff = tmp - curTime;
	timeDiff = (double)ticDiff / 1000.0;
	curTime = tmp;
}
