#include "SDL.h"

void InitTimer();
void UpdateTimer();

extern Uint32 curTime;  // current time
extern Uint32 ticDiff;	// ticks between frames
extern double timeDiff; // elapsed time (seconds) between frames
