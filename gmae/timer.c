#include <stdio.h>
#include "SDL.h"

Uint32 curTime;  // current time since initialization
Uint32 timeDiff; // time between frames

void InitTimer()
{
	curTime = SDL_GetTicks();
	timeDiff = 0;
}

void UpdateTimer()
{
	Uint32 tmp;
	tmp = SDL_GetTicks();
	timeDiff = tmp - curTime;
	curTime = tmp;
}
