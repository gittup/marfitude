#include <stdio.h>
#include "SDL.h"

#include "timer.h"

Uint32 curTime;  /* current time since initialization */
Uint32 ticDiff;	 /* number of ticks between frames */
double timeDiff; /* time (seconds) between frames */

void InitTimer(void)
{
	curTime = SDL_GetTicks();
	timeDiff = 0;
}

void UpdateTimer(void)
{
	Uint32 tmp;
	tmp = SDL_GetTicks();
	ticDiff = tmp - curTime;
	timeDiff = (double)ticDiff / 1000.0;
	curTime = tmp;
}
