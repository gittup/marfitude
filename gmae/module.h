#ifndef _MIKMOD_H_
	#include "mikmod.h"
#endif

int StartModule(char *modFile);
void StopModule();

extern MODULE *mod;

#define NumPatternsAtSngPos(s) mod->pattrows[mod->positions[s]]
#define TrackAtSngPosForChannel(s, c) mod->patterns[(mod->positions[s]*mod->numchn)+c]
#define RowsInTrack(t) mod->pattrows[PatternForTrack(t)]
