#ifndef _MIKMOD_H_
	#include "mikmod.h"
#endif

// notes are 1, 2, 4 (they are flags, in case we decide to do multiple notes
// in one row/channel)
// upping this to 8 or 16 or whatever would allow more than just the 3 dots
// with some changes in the scene, as well
#define MAX_NOTE 4

typedef struct {
	int note;	// 0 = no note, 1, 2, 4 = position of note
	int struck;	// note was struck by user
	int *modChannels; // actual channels in mod this reflects
	int numModChannels;
	} ModChannel;

typedef struct {
	int bpm;	// bpm for this row
	int ticpos;	// tick count up to this point
	int sngspd;	// sngspd for this row
	int patpos;	// this row number (position in pattern)
	int sngpos;	// position in song
	int line;	// 0 = no line, 1 = regular line, 2 = start of new patt
	ModChannel *chans;	// len numChannels;
//	int *notes;	// len numChannels
			// 0 = no note, 1, 2, 3 = position of note
//	int *struck;	// note was struck by user
	} ModRow;

int StartModule(char *modFile);
void StopModule();

extern int PatternForTrack(int trk);
extern int PatternHasTrack(int pat, int trk);

extern MODULE *mod;
extern unsigned char **trkData;
extern ModRow *rowData;
extern int numRows;
extern int numTics;
extern int numChannels;

#define NumPatternsAtSngPos(s) mod->pattrows[mod->positions[s]]
#define TrackAtSngPosForChannel(s, c) mod->patterns[(mod->positions[s]*mod->numchn)+c]
#define RowsInTrack(t) mod->pattrows[PatternForTrack(t)]
