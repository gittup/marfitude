#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SDL_mixer.h"
#include "mikmod.h"
#include "mikmod_internals.h"

#include "wam.h"
#include "cfg.h"
#include "log.h"
#include "module.h"

#include "fatalerror.h"
#include "memtest.h"
#include "textprogress.h"

#define GRP_SIZE 32

typedef struct {
	int note;
	int ins;
	int vol;
	} Sample;

typedef struct {
	Sample *samples;	/* list of samples. length is the same */
				/* for all tracks, so it is kept in the function */
				/* and not in the struct */
	int *notes;		/* some temporary space to find the best */
				/* tracks in a pattern */
	int interest;		/* how "interesting" this track is :) */
	int singleIns;		/* if there is only one instrument, this is */
				/* set to correspond to that instrument. */
				/* if there are multiple instruments or the  */
				/* track is blank, this is -1 */
	int isEmpty;		/* if this track has no notes, isEmpty is set */
	int *channels;		/* length mod->numchn, reflects which channels */
				/* have been merged */
	int numChannels;	/* valid channels in list above.  Starts out as */
				/* 1 then grows as tracks are merged */
	int lastCol;		/* last column this track was placed in */
	} Track;

static Wam *LoadTrackData(void);
static char *BaseFileName(char *file);
static char *Mod2Wam(char *modFile);
static int GetNote(UBYTE *trk, UWORD row);
static int GetInstrument(UBYTE *trk, UWORD row);
static void Handler(void);
static void CombineSingleInsTracks(Track *t1, Track *t2, int trklen);
static int TracksIntersect(Track *t1, Track *t2, int trklen);
static int GenTrackData(Track *t, int trklen, int pos);
static int BestTrack(Track *t);
static void SetColumn(Column *col, Track *trk, Wam *wam, int colnum, int patnum, int trklen, int startRow);
static int EmptyCol(Column *cols, int numCols, Column **retptr);
static void UpdateRowData(Track *t, int trklen, Wam *wam, int startRow, int patnum);
static void ClearTrack(Track *t, int chan);
static int SetSample(Sample *s, int chan);
static void WriteCol(int fno, Column *col);
static int SaveWam(Wam *wam, char *wamFile);
static void ReadCol(int fno, Column *col);
static Wam *LoadWamWrite(char *modFile, int wamwrite);

char *BaseFileName(char *file)
{
	int x;
	int start = 0;
	char *ret;

	x = 0;
	while(file[x])
	{
		if(file[x] == '/') start = x + 1;
		x++;
	}
	ret = (char*)malloc(sizeof(char) * (x - start + 1));
	strcpy(ret, file+start);
	return ret;
}

char *Mod2Wam(char *modFile)
{
	char *s;
	char *base;
	int len;
	base = BaseFileName(modFile);
	Log(("BASE: %s\n", base));
	len = strlen(base);
	while(len >= 0 && base[len] != '.') len--;
	if(len == 0) len = strlen(base);
	s = (char*)calloc(sizeof(char), len+9);	/* 9 = "wam/"+".wam\0" */
	Log(("Alloc: %i\n", len+9));
	strcpy(s, "wam/");
	strncat(s, base, len);
	strcat(s, ".wam");
	free(base);
	Log(("Wam name generated\n"));
	return s;
}

int GetNote(UBYTE *trk, UWORD row)
{
	UBYTE c, note;
	UBYTE *rowPtr = UniFindRow(trk, row);
	UniSetRow(rowPtr);
	while((c = UniGetByte()))
	{
		switch(c)
		{
			case UNI_NOTE:
				note = UniGetByte();
				return note;
				break;
			default:
				UniSkipOpcode(c);
				break;
		}
	}
	return 0;
}

int GetInstrument(UBYTE *trk, UWORD row)
{
	UBYTE c, instrument;
	UBYTE *rowPtr = UniFindRow(trk, row);
	UniSetRow(rowPtr);
	while((c = UniGetByte()))
	{
		switch(c)
		{
			case UNI_INSTRUMENT:
				instrument = UniGetByte();
				if(instrument >= mod->numins)
				{
					/* don't know why this happens, but it's */
					/* in the libmikmod code and cures */
					/* a segfault :) */
					return 0;
				}
				return instrument;
				break;
			default:
				UniSkipOpcode(c);
				break;
		}
	}
	return 0;
}

void Handler(void)
{
	/* This is the tick handler for when we load the song initially. */
	/* Since we don't want the song to play while we load it, */
	/* we do nothing with the driver generated tick */
}

/* copy t2 over t1, and then void t2 by setting isEmpty to 1 and */
/* clearing its channels */
/* t1 becomes the intersection of t1 and t2, with notes averaged */
/* volumes are copied, unaveraged */
void CombineSingleInsTracks(Track *t1, Track *t2, int trklen)
{
	int x;
	/* keep the singleIns field up-to-date */
	if(t1->singleIns != t2->singleIns) t1->singleIns = -1;

	/* copy all the channels used by t2 into t1 */
	for(x=0;x<t2->numChannels;x++)
	{
		t1->channels[t1->numChannels] = t2->channels[x];
		t1->numChannels++;
	}

	/* now copy all the notes/instruments from t2 into t1 */
	/* if t1 doesn't have a note there, copy t2 over it */
	for(x=0;x<trklen;x++)
	{
		if(t2->samples[x].note)
		{
			if(t1->samples[x].note)
			{
				t1->samples[x].note += t2->samples[x].note;
			}
			else
			{
				/* multiply by numChannels to discount */
				/* averaging */
				t1->samples[x].note = t1->numChannels * t2->samples[x].note;
				t1->samples[x].ins = t2->samples[x].ins;
				t1->samples[x].vol = t2->samples[x].vol;
			}
		}
	}
	t2->isEmpty = 1;
	t2->numChannels = 0;
}

/* if both t1 and t2 have a note in the same row, they 'intersect' */
/* and a 1 is returned. otherwise, 0 */
int TracksIntersect(Track *t1, Track *t2, int trklen)
{
	int x;
	for(x=0;x<trklen;x++)
	{
		if(t1->samples[x].note && t2->samples[x].note) return 1;
	}
	return 0;
}

int GenTrackData(Track *t, int trklen, int pos)
{
	int x;
	int mishaps = 0;
	Uint8 oldinfo[4];
	Uint8 newinfo[4];
	Uint32 *oldnote = (Uint32*)oldinfo;
	Uint32 *newnote = (Uint32*)newinfo;
	*oldnote = 0;
	*newnote = 0;

	for(x=0;x<trklen;x++)
	{
		if(t->samples[x].note)
		{
			newinfo[0] = (Uint8)t->samples[x].vol;
			newinfo[1] = (Uint8)t->samples[x].note;
			newinfo[2] = (Uint8)t->samples[x].ins;
			if(*oldnote)
			{
				if(*newnote > *oldnote)
				{
					if(pos == MAX_NOTE)
					{
						mishaps++;
					}
					else
					{
						pos <<= 1;
					}
				}
				else if(*newnote < *oldnote)
				{
					if(pos == 1)
					{
						mishaps++;
					}
					else
					{
						pos >>= 1;
					}
				}
				/* no else, position is constant if the */
				/* consecutive notes are the same */
			}
			t->interest += (*oldnote != *newnote) * (t->samples[x].vol);
			*oldnote = *newnote;
			t->notes[x] = pos;
		}
		else t->notes[x] = 0;
	}
	return mishaps;
}

int BestTrack(Track *t)
{
	int x;
	int bestTrack = -1;
	int bestInterest = 0;
	for(x=0;x<mod->numchn;x++)
	{
		if(t[x].isEmpty) continue;
		if(bestTrack == -1 || bestInterest < t[x].interest)
		{
			bestTrack = x;
			bestInterest = t[x].interest;
		}
	}
	return bestTrack;
}

void SetColumn(Column *col, Track *trk, Wam *wam, int colnum, int patnum, int trklen, int startRow)
{
	int y;
	trk->isEmpty = 2;
	trk->lastCol = colnum;
	col->numchn = trk->numChannels;
	col->chan = (int*)malloc(sizeof(int) * col->numchn);
	memcpy(col->chan, trk->channels, sizeof(int) * col->numchn);
	for(y=0;y<trklen;y++)
	{
		wam->rowData[startRow+y].notes[colnum] = trk->notes[y];
		wam->rowData[startRow+y].patnum = patnum;
	}
}

int EmptyCol(Column *cols, int numCols, Column **retptr)
{
	int x;
	for(x=0;x<numCols;x++)
	{
		if(cols[x].chan == NULL)
		{
			*retptr = &cols[x];
			return x;
		}
	}
	*retptr = NULL;
	return -1;
}

void UpdateRowData(Track *t, int trklen, Wam *wam, int startRow, int patnum)
{
	int x, y;
	int bestStart, bestCount, tmpCount;
	int bestTrks[MAX_COLS];
	Column *col;
	Pattern *pat;
	Track *trk;

	pat = &wam->patterns[patnum];
	/* try to combine tracks that are one instrument line that is */
	/* split among several tracks. These can be found by checking if */
	/* two (or more) tracks all use the same instrument, but in different */
	/* rows */
	for(x=0;x<mod->numchn-1;x++)
	{
		if(t[x].isEmpty || t[x].singleIns == -1) continue;
		for(y=x+1;y<mod->numchn;y++)
		{
			if(t[y].isEmpty) continue;
			if(t[x].singleIns == t[y].singleIns && !TracksIntersect(&t[x], &t[y], trklen))
			{
				CombineSingleInsTracks(&t[x], &t[y], trklen);
			}
		}
	}

	for(x=0;x<mod->numchn;x++)
	{
		bestStart = 0;
		bestCount = trklen;
		for(y=1;y<=MAX_NOTE;y*=2)
		{
			tmpCount = GenTrackData(&t[x], trklen, y);
			if(!bestStart || tmpCount < bestCount)
			{
				bestStart = y;
				bestCount = tmpCount;
			}
		}
		/* if the best data is already in memory, no need to */
		/* generate it again, so save a little time :) */
		if(bestStart != MAX_NOTE) GenTrackData(&t[x], trklen, bestStart);
	}

	/* now combine tracks that are duplicates of each other - no sense */
	/* having a bunch of tracks that are all the same thing */
/*	for(x=0;x<mod->numchn-1;x++)
	{
		if(t[x].isEmpty) continue;
		for(y=x+1;y<mod->numchn;y++)
		{
			if(t[y].isEmpty) continue;
			if(TracksAreEqual(t[x].samples, t[y].samples, trklen))
			{
				*CombineTracks(&t[x], &t[y], trklen); *
			}
		}
	}*/

	/* get a list of the best tracks */
	/* mark them empty as we go through, BestTrack picks up the best */
	/* "non-empty" track */
	Log(("A\n"));
	for(x=0;x<wam->numCols;x++)
	{
		bestTrks[x] = BestTrack(t);
		Log(("Best: %i, Old: %i\n", bestTrks[x], t[x].lastCol));
		if(bestTrks[x] != -1) t[bestTrks[x]].isEmpty = 1;
		col = &pat->columns[x];
		col->numchn = 0;
		col->chan = NULL;
	}
	Log(("B\n"));

	/* place all the best tracks that can be placed in their previous slots */
	for(x=0;x<wam->numCols;x++)
	{
		if(bestTrks[x] == -1) continue;
		trk = &t[bestTrks[x]];
		if(trk->lastCol != -1)
		{
			col = &pat->columns[trk->lastCol];
			if(col->chan == NULL)
			{
				Log(("Replace: %i\n", trk->lastCol));
				SetColumn(col, trk, wam, trk->lastCol, patnum, trklen, startRow);
			}
		}
	}
	Log(("C\n"));

	/* now place all the tracks that didn't fit, or weren't in previously */
	for(x=0;x<wam->numCols;x++)
	{
		if(bestTrks[x] == -1) continue;
		trk = &t[bestTrks[x]];
		if(trk->isEmpty != 2)
		{
			y = EmptyCol(pat->columns, wam->numCols, &col);
			Log(("Place: %i\n", y));
			if(y == -1)
			{
				ELog(("ERROR: No empty column!\n"));
			}
			SetColumn(col, trk, wam, y, patnum, trklen, startRow);
		}
	}
	/* set empty rowdata for unused columns */
	for(x=0;x<wam->numCols;x++)
	{
		col = &pat->columns[x];
		if(col->chan == NULL)
		{
			Log(("Empty: %i\n", x));
			for(y=0;y<trklen;y++)
			{
				wam->rowData[startRow+y].notes[x] = 0;
				wam->rowData[startRow+y].patnum = patnum;
			}
		}
	}

	col = &pat->unplayed;
	col->numchn = 0;
	col->chan = NULL;
	for(x=0;x<mod->numchn;x++)
	{
		if(t[x].isEmpty) continue;
		col->chan = (int*)realloc(col->chan, sizeof(int) * (col->numchn + t[x].numChannels));
		memcpy(&(col->chan[col->numchn]), t[x].channels, sizeof(int) * t[x].numChannels);
		col->numchn += t[x].numChannels;
	}
}

/* resets default values in the Track struct for channel chan */
void ClearTrack(Track *t, int chan)
{
	t->channels[0] = chan;
	t->numChannels = 1;
	t->isEmpty = 1;
	t->singleIns = -1;
	t->interest = 0;
}

/* sets the sample information in s from channel chan = [0..mod->numchn-1] */
/* ignores notes that are too quiet (cutoff set by config file) */
/* returns the instrument used, -1 if there is no note */
int SetSample(Sample *s, int chan)
{
	int volThreshold;
	volThreshold = CfgI("main.volumethreshold");
	/* can't use the 'sample' and 'note' values */
	/* from the MP_CONTROL struct in */
	/* mikmod_internals.h since they aren't  */
	/* necessarily set to 0 after the note is */
	/* first "struck."  Instead we get the info */
	/* from MikMod's internal format */
	s->vol = mod->control[chan].outvolume;
	if(s->vol < volThreshold)
	{
		s->vol = 0;
		s->note = 0;
		s->ins = -1; /* 0 is a valid instrument, -1 is not */
	}
	else
	{
		s->ins = GetInstrument(mod->control[chan].row, 0);
		s->note = GetNote(mod->control[chan].row, 0);
	}
	if(s->note) return s->ins;
	else return -1;
}

/* returns a new WAM structure containing all data necessary for the game */
Wam *LoadTrackData(void)
{
	int x, oldSngPos, lineCount;
	int ins;
	int tickCount = 0;
	int grpCount = 0;
	int numgrps = 0;
	int startRow = 0;
	int rowsAlloced = 0;	/* number of rows allocated in WAM file */
	int numSamples = 64;	/* numSamples = how much we have allocated */
				/* start with 64, since that's the default */
				/* number of rows/pattern for most mods */
	int trklen = 0;		/* trklen is how many samples we are actually */
				/* using, since patterns can vary in length, */
				/* even in the same song */
	Track *tracks;		/* track data */
				/* we keep one pattern worth of samples */
				/* in memory to check for redundant tracks */
				/* and pick the best remaining tracks to play */
	Wam *wam;		/* the wam we'll create and return */
	MikMod_player_t oldHand; /* handler should stop multiple conflicting */
	/* Player_HandleTick()'s to skip ticks and sometimes segfault */

	/* use calloc to zero everything in the struct */
	wam = (Wam*)calloc(1, sizeof(Wam));

	tracks = (Track*)malloc(sizeof(Track) * mod->numchn);
	for(x=0;x<mod->numchn;x++)
	{
		tracks[x].samples = (Sample*)malloc(sizeof(Sample) * numSamples);
		tracks[x].notes = (int*)malloc(sizeof(int) * numSamples);
		tracks[x].channels = (int*)malloc(sizeof(int) * mod->numchn);
		ClearTrack(&tracks[x], x);
		tracks[x].lastCol = -1;
	}

	Player_Mute(MUTE_INCLUSIVE, 0, mod->numchn-1);
	oldHand = MikMod_RegisterPlayer(Handler);
	oldSngPos = -1;
	lineCount = 0;

	wam->numCols = CfgI("main.tracks");
	if(wam->numCols >= mod->numchn) wam->numCols = mod->numchn;
	if(wam->numCols >= MAX_COLS) wam->numCols = MAX_COLS;

	Player_TogglePause(); /* start the mod up again so we can read in data */
	Log(("Starting row loop\n"));
	ProgressMeter("Creating WAM");
	while(mod->sngpos < mod->numpos)
	{
		if(!mod->vbtick)
		{
			if(wam->numRows >= rowsAlloced)
			{
				if(rowsAlloced == 0)
				{
					rowsAlloced = 1024;
				}
				else
				{
					rowsAlloced <<= 2;
				}
				wam->rowData = (Row*)realloc(wam->rowData, sizeof(Row) * rowsAlloced);
			}

			/* only designate one place the start of a */
			/* pattern so if we loop there aren't multiple */
			/* 'starts' */
			if(!mod->patpos && mod->sngpos != oldSngPos)
			{
				wam->rowData[wam->numRows].line = 2;
				oldSngPos = mod->sngpos;
				lineCount = 0;
				/* now convert the track data to something */
				/* usable. mod->sngpos == 0 the first time */
				/* through, so there won't be any track data */
				/* then :) */
				if(mod->sngpos != 0)
				{
				Log(("b: %i\n", grpCount));
					/* reset groups on a line break */
					for(x=0;x<numgrps;x++)
					{
						/* extra -1 since we're holding */
						/* onto a note */
						wam->rowData[wam->numRows-x-1].ticgrp = grpCount;
					}
					numgrps = 0;
					grpCount = 0;

					wam->patterns = (Pattern*)realloc(wam->patterns, sizeof(Pattern) * (wam->numPats+1));
					UpdateRowData(tracks, trklen, wam, startRow, wam->numPats);
					UpdateProgress(mod->sngpos, mod->numpos);
					wam->numPats++;
					startRow = wam->numRows;
					trklen = 0;
					for(x=0;x<mod->numchn;x++)
					{
						ClearTrack(&tracks[x], x);
					}
				}
			}
			else if(!(lineCount&3)) wam->rowData[wam->numRows].line = 1;
			else wam->rowData[wam->numRows].line = 0;

			Log(("Sng: %i Pat: %i row: %i, alloc: %i\n", mod->sngpos, mod->patpos, wam->numRows, rowsAlloced));
			wam->rowData[wam->numRows].bpm = mod->bpm;
			wam->rowData[wam->numRows].sngspd = mod->sngspd;
			wam->rowData[wam->numRows].ticpos = tickCount;
			wam->rowData[wam->numRows].ticprt = grpCount;
			wam->rowData[wam->numRows].patpos = mod->patpos;
			wam->rowData[wam->numRows].sngpos = mod->sngpos;
			tickCount += mod->sngspd;
			grpCount += mod->sngspd;
			numgrps++;
			/* only break on a group mod 8 when we have enough */
			/* ticks in the group */
			/* or break if we've already got GRP_SIZE rows */
			if((grpCount >= GRP_SIZE && !(numgrps&7)) ||
					numgrps == GRP_SIZE)
			{
				Log(("A: %i\n", grpCount));
				for(x=0;x<numgrps;x++)
				{
					wam->rowData[wam->numRows-x].ticgrp = grpCount;
				}
				numgrps = 0;
				grpCount = 0;
			}
			lineCount++;

			for(x=0;x<mod->numchn;x++)
			{
				ins = SetSample(tracks[x].samples+trklen, x);
				/* some confusing logic ahead :) */
				/* basically it clears the isEmpty var */
				/* if an instrument is used, and ensures that */
				/* singleIns is set to the single instrument */
				/* used if there is only one instrument.  If */
				/* more than one is used or the track is empty, */
				/* it is set to -1 (ClearTrack() sets it to */
				/* -1 when to start, and is not modified when */
				/* it's empty) */
				if(ins != -1)
				{
					if(tracks[x].isEmpty)
					{
						tracks[x].isEmpty = 0;
						tracks[x].singleIns = ins;
					}
					else
					{
						if(tracks[x].singleIns != ins)
						{
							tracks[x].singleIns = -1;
						}
					}
				}
			}
			trklen++;
			if(trklen == numSamples)
			{
				numSamples *= 2;
				for(x=0;x<mod->numchn;x++)
				{
					tracks[x].samples = (Sample*)realloc(tracks[x].samples, sizeof(Sample) * numSamples);
					tracks[x].notes = (int*)realloc(tracks[x].notes, sizeof(int) * numSamples);
				}
			}
			wam->numRows++;
		}
		Player_HandleTick();
		wam->numTics++;
	}
	wam->patterns = (Pattern*)realloc(wam->patterns, sizeof(Pattern) * (wam->numPats+1));
	UpdateRowData(tracks, trklen, wam, startRow, wam->numPats);
	UpdateProgress(mod->sngpos, mod->numpos);
	EndProgressMeter();
	wam->numPats++;
	Log(("Row loop done\n"));

	oldHand = MikMod_RegisterPlayer(oldHand);
	for(x=0;x<mod->numchn;x++)
	{
		free(tracks[x].samples);
		free(tracks[x].notes);
		free(tracks[x].channels);
	}
	free(tracks);
	Log(("Track data Ready\n"));
	return wam;
}

void WriteCol(int fno, Column *col)
{
	write(fno, &col->numchn, sizeof(int));
	write(fno, col->chan, sizeof(int) * col->numchn);
}

int SaveWam(Wam *wam, char *wamFile)
{
	int x, y, fno;
	FILE *f;
	f = fopen(wamFile, "w");
	if(f == NULL)
	{
		ELog(("Error opening '%s'\n", wamFile));
		Error("Opening Wam for write.");
		return 0;
	}
	fno = f->_fileno;
	write(fno, &wam->numCols, sizeof(int));
	write(fno, &wam->numTics, sizeof(int));
	write(fno, &wam->numPats, sizeof(int));
	write(fno, &wam->numRows, sizeof(int));
	for(x=0;x<wam->numPats;x++)
	{
		for(y=0;y<wam->numCols;y++)
		{
			WriteCol(fno, &wam->patterns[x].columns[y]);
		}
		WriteCol(fno, &wam->patterns[x].unplayed);
	}
	write(fno, wam->rowData, sizeof(Row) * wam->numRows);
	fclose(f);
	return 1;
}

int WriteWam(char *modFile)
{
	Wam *wam;
	char *wamFile;
	wamFile = Mod2Wam(modFile);
	Log(("MOD: %s\nWAM: %s\n", modFile, wamFile));
	if(!StartModule(modFile))
	{
		ELog(("Error: Couldn't start module.\n"));
		return 0;
	}
	Log(("Loading track data...\n"));
	wam = LoadTrackData();
	Log(("Saving Wam...\n"));
	if(!SaveWam(wam, wamFile))
	{
		ELog(("Error: Couldn't save wam\n"));
		return 0;
	}
	free(wamFile);
	FreeWam(wam);
	Log(("Wam written.\n"));
	return 1;
}

void ReadCol(int fno, Column *col)
{
	read(fno, &col->numchn, sizeof(int));
	if(col->numchn)
	{
		col->chan = (int*)malloc(sizeof(int) * col->numchn);
		read(fno, col->chan, sizeof(int) * col->numchn);
	}
	else
	{
		col->chan = NULL;
	}
}

Wam *LoadWamWrite(char *modFile, int wamwrite)
{
	int x, y, fno;
	char *wamFile;
	FILE *f;
	Wam *wam;

	wamFile = Mod2Wam(modFile);
	f = fopen(wamFile, "r");
	free(wamFile);
	if(f == NULL)
	{
		if(wamwrite)
		{
			/* if the file doesn't exist, create one */
			Log(("WAM not found, creating...\n"));
			WriteWam(modFile);
			/* next time we try to load, don't try to create again */
			return LoadWamWrite(modFile, 0);
		}
		else
		{
			ELog(("Error: Couldn't load WAM file after creation!\n"));
			return NULL;
		}
	}
	Log(("Loading wam\n"));
	fno = f->_fileno;
	wam = (Wam*)malloc(sizeof(Wam));
	read(fno, &wam->numCols, sizeof(int));
	read(fno, &wam->numTics, sizeof(int));
	read(fno, &wam->numPats, sizeof(int));
	read(fno, &wam->numRows, sizeof(int));
	Log(("Main info read: %i cols, %i tics, %i pats, %i rows\n", wam->numCols, wam->numTics, wam->numPats, wam->numRows));
	wam->patterns = (Pattern*)malloc(sizeof(Pattern) * wam->numPats);
	wam->rowData = (Row*)malloc(sizeof(Row) * wam->numRows);
	for(x=0;x<wam->numPats;x++)
	{
		for(y=0;y<wam->numCols;y++)
		{
			ReadCol(fno, &wam->patterns[x].columns[y]);
		}
		ReadCol(fno, &wam->patterns[x].unplayed);
	}
	Log(("Patterns read\n"));
	read(fno, wam->rowData, sizeof(Row) * wam->numRows);
	Log(("Rows read\n"));

	return wam;
}

Wam *LoadWam(char *modFile)
{
	/* the first time we try to load the file we create if it doesn't exist */
	return LoadWamWrite(modFile, 1);
}

void FreeWam(Wam *wam)
{
	int x, y;
	Log(("Freeing Wam\n"));
	for(x=0;x<wam->numPats;x++)
	{
		for(y=0;y<wam->numCols;y++)
		{
			if(wam->patterns[x].columns[y].numchn)
				free(wam->patterns[x].columns[y].chan);
		}
		if(wam->patterns[x].unplayed.numchn)
			free(wam->patterns[x].unplayed.chan);
	}
	free(wam->patterns);
	free(wam->rowData);
	free(wam);
	Log(("Wam freed\n"));
}
