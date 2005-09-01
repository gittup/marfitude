/*
   Marfitude
   Copyright (C) 2005 Mike Shal

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "sdl_mixer/SDL_mixer.h"
#include "sdl_mixer/mikmod/mikmod.h"
#include "sdl_mixer/mikmod/mikmod_internals.h"

#include "wam.h"
#include "cfg.h"
#include "log.h"
#include "module.h"

#include "util/fatalerror.h"
#include "util/memtest.h"
#include "util/textprogress.h"

/** The maximum number of tracks to play. */
#define WAM_MAX_TRACKS 7

/** The magic at the top of the wam. */
#define WAM_MAGIC "wam-1.0"

/** Length of the magic wam string. */
#define WAM_MAGIC_LEN (sizeof(WAM_MAGIC) / sizeof(WAM_MAGIC[0]))

/** The max number of rows in a group */
#define GRP_SIZE 32

/** Defined for windows compatibility. Stupid windows. Who wouldn't want to
 * read in binary data? That's right. Nobody. stupid.
 */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/** @file
 * Creates, loads, and provides access to WAM files. I just made these up
 * to squish a Mod file down into like 3 notes.
 */

/** Describes a sample. The difficulty value is calculated based on a
 * completely silly and subjective algorithm. The rest of the information
 * is straight from MikMod.
 */
struct sample {
	int note; /**< The note that is played */
	int ins;  /**< The instrument playing */
	int vol;  /**< The volume of the sample */
	int difficulty; /**< How tough this note is to play */
};

/** Describes a track, which is one or more channels of a mod in a single
 * pattern.
 */
struct track {
	struct sample *samples;	/**< list of samples. Size numSamples, valid
				 * length trklen
				 */
	int trklen;             /**< Length of the sample data above. This
				 * is actually the same for all tracks.
				 */
	int numSamples;         /**< The size of the samples array */
	int *notes;		/**< some temporary space to find the best
				 * tracks in a pattern
				 */
	int interest;		/**< how "interesting" this track is :) */
	int singleIns;		/**< if there is only one instrument, this is
				 * set to correspond to that instrument.
				 * if there are multiple instruments or the 
				 * track is blank, this is -1
				 */
	int isEmpty;		/**< set if this track has no notes */
	int *channels;		/**< length mod->numchn, reflects which channels
				 * have been merged
				 */
	int numChannels;	/**< valid channels in list above.  Starts out
				 * as 1 then grows as tracks are merged
				 */
	int lastCol;		/**< last column this track was placed in */
};

static struct wam *LoadTrackData(void);
static char *BaseFileName(const char *file);
static char *Mod2Wam(const char *modFile);
static int GetNote(UBYTE *trk, UWORD row);
static int GetInstrument(UBYTE *trk, UWORD row);
static void Handler(void);
static void calculate_difficulty(struct track *t, struct wam *wam, int row);
static void CombineSingleInsTracks(struct track *t1, struct track *t2);
static int TracksIntersect(struct track *t1, struct track *t2);
static int NextPos(int pos, int movinup, Uint32 note, Uint32 *mem);
static int GenTrackData(struct track *t, int pos);
static int BestTrack(struct track *t);
static void SetColumn(struct column *col, struct track *trk, struct wam *wam, int colnum, int patnum, int startRow);
static int EmptyCol(struct column *cols, int numCols, struct column **retptr);
static void UpdateRowData(struct track *t, struct wam *wam, int startRow, int patnum);
static void ClearTrack(struct track *t, int chan);
static int SetSample(struct sample *s, int chan);
static void WriteCol(int fno, struct column *col);
static int SaveWam(struct wam *wam, const char *wamFile);
static void ReadCol(int fno, struct column *col);
static struct wam *ReadWam(const char *wamFile);
static struct wam *CreateWam(const char *modFile);

char *BaseFileName(const char *file)
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
	ret = malloc(sizeof(char) * (x - start + 1));
	strcpy(ret, file+start);
	return ret;
}

char *Mod2Wam(const char *modFile)
{
	char *s;
	char *base;
	int len;

	base = BaseFileName(modFile);
	Log(("BASE: %s\n", base));
	len = strlen(base);
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
					/* don't know why this happens, but it's
					 * in the libmikmod code and cures
					 * a segfault :)
					 */
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
	/* This is the tick handler for when we load the song initially.
	 * Since we don't want the song to play while we load it,
	 * we do nothing with the driver generated tick
	 */
}

/* copy t2 over t1, and then void t2 by setting isEmpty to 1 and
 * clearing its channels
 * t1 becomes the intersection of t1 and t2, with notes averaged
 * volumes are copied, unaveraged
 */
void CombineSingleInsTracks(struct track *t1, struct track *t2)
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

	/* now copy all the notes/instruments from t2 into t1
	 * if t1 doesn't have a note there, copy t2 over it
	 */
	for(x=0;x<t1->trklen;x++)
	{
		if(t2->samples[x].note)
		{
			if(t1->samples[x].note)
			{
				t1->samples[x].note += t2->samples[x].note;
			}
			else
			{
				/* multiply by numChannels to discount
				 * averaging
				 */
				t1->samples[x].note = t1->numChannels * t2->samples[x].note;
				t1->samples[x].ins = t2->samples[x].ins;
				t1->samples[x].vol = t2->samples[x].vol;
			}
		}
	}
	t2->isEmpty = 1;
	t2->numChannels = 0;
}

/* if both t1 and t2 have a note in the same row, they 'intersect'
 * and a 1 is returned. otherwise, 0
 */
int TracksIntersect(struct track *t1, struct track *t2)
{
	int x;
	for(x=0;x<t1->trklen;x++)
	{
		if(t1->samples[x].note && t2->samples[x].note) return 1;
	}
	return 0;
}

/* returns pos<<1 if movinup is 1 and pos>>1 if movinup is zero,
 * unless of course note is in mem, in which case pos in mem[pos]==note
 * is returned
 */
int NextPos(int pos, int movinup, Uint32 note, Uint32 *mem)
{
	int x;
	for(x=1;x<=MAX_NOTE;x<<=1)
	{
		if(mem[x] == note)
			return x;
	}

	if(movinup)
		return pos<<1;
	return pos>>1;
}

int GenTrackData(struct track *t, int pos)
{
	int x;
	int mishaps = 0;
	Uint8 oldinfo[4];
	Uint8 newinfo[4];
	Uint32 *oldnote = (Uint32*)oldinfo;
	Uint32 *newnote = (Uint32*)newinfo;
	Uint32 mem[MAX_NOTE+1] = {0};
	*oldnote = 0;
	*newnote = 0;

	for(x=0;x<t->trklen;x++)
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
						pos = NextPos(pos, 1, *newnote, mem);
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
						pos = NextPos(pos, 0, *newnote, mem);
					}
				}
				/* no else, position is constant if the
				 * consecutive notes are the same
				 */
			}
			t->interest += (*oldnote != *newnote) * (t->samples[x].vol);
			*oldnote = *newnote;
			t->notes[x] = pos;
			mem[pos] = *newnote;
		}
		else t->notes[x] = 0;
	}
	return mishaps;
}

int BestTrack(struct track *t)
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

void SetColumn(struct column *col, struct track *trk, struct wam *wam, int colnum, int patnum, int startRow)
{
	int y;
	trk->isEmpty = 2;
	trk->lastCol = colnum;
	col->numchn = trk->numChannels;
	col->chan = malloc(sizeof(int) * col->numchn);
	memcpy(col->chan, trk->channels, sizeof(int) * col->numchn);
	for(y=0;y<trk->trklen;y++)
	{
		wam->rowData[startRow+y].notes[colnum] = trk->notes[y];
		wam->rowData[startRow+y].difficulty[colnum] = trk->samples[y].difficulty;
		wam->rowData[startRow+y].patnum = patnum;
	}
}

int EmptyCol(struct column *cols, int numCols, struct column **retptr)
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

void calculate_difficulty(struct track *t, struct wam *wam, int row)
{
	int x;
	int difficulty = 0;
	double last = -5.0;
	const double toughness = .20;
	int count = 1;

	for(x=0; x<t->trklen; x++) {
		if(t->samples[x].ins >= 0) {
			double time = wam->rowData[row+x].time;
			double sor;

			sor = (double)count + (double)difficulty;
			if(time - last < toughness / sor) {
				difficulty += 2;
			} else if(time - last < toughness * 2.0 / sor) {
				difficulty++;
			} else if(time - last < toughness * 3.0 / sor) {
				/* No change */
			} else if(time - last < toughness * 4.0 / sor) {
				difficulty--;
			} else if(time - last < toughness * 5.0 / sor) {
				difficulty -= 2;
			} else {
				difficulty = 0;
			}

			if(difficulty < 0)
				difficulty = 0;
			if(difficulty > 3)
				difficulty = 3;

			last = time;
			if(difficulty == 0) {
				count = 1;
			} else {
				count += difficulty;
			}

			t->samples[x].difficulty = difficulty;
		}
	}
}

void UpdateRowData(struct track *t, struct wam *wam, int startRow, int patnum)
{
	int x, y;
	int bestStart, bestCount, tmpCount;
	int bestTrks[MAX_COLS];
	struct column *col;
	struct pattern *pat;
	struct track *trk;

	pat = &wam->patterns[patnum];
	/* try to combine tracks that are one instrument line that is
	 * split among several tracks. These can be found by checking if
	 * two (or more) tracks all use the same instrument, but in different
	 * rows
	 */
	for(x=0;x<mod->numchn-1;x++)
	{
		if(t[x].isEmpty || t[x].singleIns == -1) continue;
		for(y=x+1;y<mod->numchn;y++)
		{
			if(t[y].isEmpty) continue;
			if(t[x].singleIns == t[y].singleIns && !TracksIntersect(&t[x], &t[y]))
			{
				CombineSingleInsTracks(&t[x], &t[y]);
			}
		}
	}

	for(x=0;x<mod->numchn;x++)
	{
		calculate_difficulty(&t[x], wam, startRow);

		bestStart = 0;
		bestCount = t->trklen;
		for(y=1;y<=MAX_NOTE;y*=2)
		{
			tmpCount = GenTrackData(&t[x], y);
			if(!bestStart || tmpCount < bestCount)
			{
				bestStart = y;
				bestCount = tmpCount;
			}
		}
		/* if the best data is already in memory, no need to
		 * generate it again, so save a little time :)
		 */
		if(bestStart != MAX_NOTE) GenTrackData(&t[x], bestStart);
	}

	/* get a list of the best tracks
	 * mark them empty as we go through, BestTrack picks up the best
	 * "non-empty" track
	 */
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

	/* place all the best tracks that can be placed in their previous slots
	 */
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
				SetColumn(col, trk, wam, trk->lastCol, patnum, startRow);
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
			SetColumn(col, trk, wam, y, patnum, startRow);
		}
	}
	/* set empty rowdata for unused columns */
	for(x=0;x<wam->numCols;x++)
	{
		col = &pat->columns[x];
		if(col->chan == NULL)
		{
			Log(("Empty: %i\n", x));
			for(y=0;y<t->trklen;y++)
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
void ClearTrack(struct track *t, int chan)
{
	t->channels[0] = chan;
	t->numChannels = 1;
	t->isEmpty = 1;
	t->singleIns = -1;
	t->interest = 0;
	t->trklen = 0;
}

/* sets the sample information in s from channel chan = [0..mod->numchn-1]
 * returns the instrument used, -1 if there is no note
 */
int SetSample(struct sample *s, int chan)
{
	/* can't use the 'sample' and 'note' values
	 * from the MP_CONTROL struct in
	 * mikmod_internals.h since they aren't
	 * necessarily set to 0 after the note is
	 * first "struck."  Instead we get the info
	 * from MikMod's internal format
	 */
	s->vol = mod->control[chan].outvolume;
	s->ins = GetInstrument(mod->control[chan].row, 0);
	s->note = GetNote(mod->control[chan].row, 0);

	if(!s->note)
		s->ins = -1;
	return s->ins;
}

/* returns a new WAM structure containing all data necessary for the game */
struct wam *LoadTrackData(void)
{
	int x, oldSngPos, lineCount;
	int lineTicker;
	int ins;
	int tickCount = 0;
	int grpCount = 0;
	int numgrps = 0;
	int startRow = 0;
	int rowsAlloced = 0;	/* number of rows allocated in WAM file */
	struct track *tracks;	/* track data
				 * we keep one pattern worth of samples
				 * in memory to check for redundant tracks
				 * and pick the best remaining tracks to play
				 */
	struct wam *wam;	/* the wam we'll create and return */
	double time = 0.0;      /* time in seconds */
	MikMod_player_t oldHand; /* handler should stop multiple conflicting */
	/* Player_HandleTick()'s to skip ticks and sometimes segfault */

	/* use calloc to zero everything in the struct */
	wam = (struct wam*)calloc(1, sizeof(struct wam));

	tracks = malloc(sizeof(struct track) * mod->numchn);
	for(x=0;x<mod->numchn;x++)
	{
		tracks[x].numSamples = 64;
		tracks[x].samples = malloc(sizeof(struct sample) * tracks[x].numSamples);
		tracks[x].notes = malloc(sizeof(int) * tracks[x].numSamples);
		tracks[x].channels = malloc(sizeof(int) * mod->numchn);
		ClearTrack(&tracks[x], x);
		tracks[x].lastCol = -1;
	}

	Player_Mute(MUTE_INCLUSIVE, 0, mod->numchn-1);
	oldHand = MikMod_RegisterPlayer(Handler);
	oldSngPos = -1;
	lineCount = 0;
	lineTicker = 0;

	wam->numCols = WAM_MAX_TRACKS;
	if(wam->numCols >= mod->numchn) wam->numCols = mod->numchn;
	if(wam->numCols >= MAX_COLS) wam->numCols = MAX_COLS;

	Player_TogglePause(); /* start the mod again so we can read in data */
	Log(("Starting row loop\n"));
	progress_meter("Creating WAM");
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
				wam->rowData = (struct row*)realloc(wam->rowData, sizeof(struct row) * rowsAlloced);
			}

			/* only designate one place the start of a
			 * pattern so if we loop there aren't multiple
			 * 'starts'
			 */
			if(!mod->patpos && mod->sngpos != oldSngPos)
			{
				wam->rowData[wam->numRows].line = 2;
				oldSngPos = mod->sngpos;
				lineCount = 0;
				lineTicker = 0;
				/* now convert the track data to something
				 * usable. mod->sngpos == 0 the first time
				 * through, so there won't be any track data
				 * then :)
				 */
				if(mod->sngpos != 0)
				{
				Log(("b: %i\n", grpCount));
					/* reset groups on a line break */
					for(x=0;x<numgrps;x++)
					{
						/* extra -1 since we're holding
						 * onto a note
						 */
						wam->rowData[wam->numRows-x-1].ticgrp = grpCount;
					}
					numgrps = 0;
					grpCount = 0;

					wam->patterns = (struct pattern*)realloc(wam->patterns, sizeof(struct pattern) * (wam->numPats+1));
					UpdateRowData(tracks, wam, startRow, wam->numPats);
					update_progress(mod->sngpos, mod->numpos);
					wam->numPats++;
					startRow = wam->numRows;
					for(x=0;x<mod->numchn;x++)
					{
						ClearTrack(&tracks[x], x);
					}
				}
			}
			else if(lineTicker >= 20 && (lineCount&3) == 0) {
				wam->rowData[wam->numRows].line = 1;
				lineCount = 0;
				lineTicker = 0;
			}
			else wam->rowData[wam->numRows].line = 0;

			Log(("Sng: %i Pat: %i row: %i, alloc: %i\n", mod->sngpos, mod->patpos, wam->numRows, rowsAlloced));
			wam->rowData[wam->numRows].bpm = mod->bpm;
			wam->rowData[wam->numRows].sngspd = mod->sngspd;
			wam->rowData[wam->numRows].ticpos = tickCount;
			wam->rowData[wam->numRows].ticprt = grpCount;
			wam->rowData[wam->numRows].patpos = mod->patpos;
			wam->rowData[wam->numRows].sngpos = mod->sngpos;
			wam->rowData[wam->numRows].time = time;
			tickCount += mod->sngspd;
			grpCount += mod->sngspd;
			time += BpmToSec(mod->sngspd, mod->bpm);
			numgrps++;
			/* only break on a group mod 8 when we have enough
			 * ticks in the group
			 * or break if we've already got GRP_SIZE rows
			 */
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
			lineTicker += mod->sngspd;

			for(x=0;x<mod->numchn;x++)
			{
				ins = SetSample(tracks[x].samples+tracks[x].trklen, x);
				/* some confusing logic ahead :)
				 * basically it clears the isEmpty var
				 * if an instrument is used, and ensures that
				 * singleIns is set to the single instrument
				 * used if there is only one instrument.  If
				 * more than one is used or the track is empty,
				 * it is set to -1 (ClearTrack() sets it to
				 * -1 when to start, and is not modified when
				 * it's empty)
				 */
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
				tracks[x].trklen++;
				if(tracks[x].trklen == tracks[x].numSamples) {
					tracks[x].numSamples *= 2;
					tracks[x].samples = (struct sample*)realloc(tracks[x].samples, sizeof(struct sample) * tracks[x].numSamples);
					tracks[x].notes = (int*)realloc(tracks[x].notes, sizeof(int) * tracks[x].numSamples);
				}
			}
			wam->numRows++;
		}
		Player_HandleTick();
		wam->numTics++;
	}
	wam->patterns = (struct pattern*)realloc(wam->patterns, sizeof(struct pattern) * (wam->numPats+1));
	UpdateRowData(tracks, wam, startRow, wam->numPats);
	update_progress(mod->sngpos, mod->numpos);
	end_progress_meter();
	wam->numPats++;
	wam->songLength = time;
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

void WriteCol(int fno, struct column *col)
{
	write(fno, &col->numchn, sizeof(int));
	write(fno, col->chan, sizeof(int) * col->numchn);
}

int SaveWam(struct wam *wam, const char *wamFile)
{
	int x;
	int y;
	int fno;
	fno = open(wamFile, O_CREAT | O_WRONLY | O_BINARY, 0644);
	if(fno == -1)
	{
		ELog(("Error opening '%s'\n", wamFile));
		Error("Opening Wam for write.");
		return 0;
	}
	write(fno, WAM_MAGIC, WAM_MAGIC_LEN);
	write(fno, &wam->numCols, sizeof(int));
	write(fno, &wam->numTics, sizeof(int));
	write(fno, &wam->numPats, sizeof(int));
	write(fno, &wam->numRows, sizeof(int));
	write(fno, &wam->songLength, sizeof(double));
	for(x=0;x<wam->numPats;x++)
	{
		for(y=0;y<wam->numCols;y++)
		{
			WriteCol(fno, &wam->patterns[x].columns[y]);
		}
		WriteCol(fno, &wam->patterns[x].unplayed);
	}
	write(fno, wam->rowData, sizeof(struct row) * wam->numRows);
	close(fno);
	return 1;
}

struct wam *CreateWam(const char *modFile)
{
	struct wam *wam;

	if(start_module(modFile))
	{
		ELog(("Error: Couldn't start module.\n"));
		return NULL;
	}
	Log(("Loading track data...\n"));
	wam = LoadTrackData();
	return wam;
}

void ReadCol(int fno, struct column *col)
{
	read(fno, &col->numchn, sizeof(int));
	if(col->numchn)
	{
		col->chan = malloc(sizeof(int) * col->numchn);
		read(fno, col->chan, sizeof(int) * col->numchn);
	}
	else
	{
		col->chan = NULL;
	}
}

/** Loads the @a modFile and creates and writes the appropriate WamFile
 * @param modFile filename of the mod
 * @returns 1 if successful, 0 on failure
 */
int write_wam(const char *modFile)
{
	struct wam *wam;
	char *wamFile;

	wam = CreateWam(modFile);
	if(wam == NULL) {
		ELog(("Error: Couldn't create wam file.\n"));
		return 0;
	}

	wamFile = Mod2Wam(modFile);
	Log(("Write MOD: %s\nWAM: %s\n", modFile, wamFile));
	if(!SaveWam(wam, wamFile)) {
		ELog(("Error: Couldn't save wam. Make sure the wam/ directory in the data directory: '%s' is writeable\n", MARFDATADIR));
		free(wamFile);
		return 0;
	}

	free(wamFile);
	return 1;
}

struct wam *ReadWam(const char *wamFile)
{
	int x;
	int y;
	int fno;
	char magic[WAM_MAGIC_LEN];
	struct wam *wam;

	fno = open(wamFile, O_RDONLY | O_BINARY);
	if(fno == -1)
		return NULL;
	read(fno, magic, WAM_MAGIC_LEN);
	if(strcmp(magic, WAM_MAGIC) != 0) {
		close(fno);
		return NULL;
	}
	Log(("Loading wam\n"));
	wam = malloc(sizeof(struct wam));
	read(fno, &wam->numCols, sizeof(int));
	read(fno, &wam->numTics, sizeof(int));
	read(fno, &wam->numPats, sizeof(int));
	read(fno, &wam->numRows, sizeof(int));
	read(fno, &wam->songLength, sizeof(double));
	Log(("Main info read: %i cols, %i tics, %i pats, %i rows\n", wam->numCols, wam->numTics, wam->numPats, wam->numRows));
	wam->patterns = malloc(sizeof(struct pattern) * wam->numPats);
	wam->rowData = malloc(sizeof(struct row) * wam->numRows);
	for(x=0;x<wam->numPats;x++)
	{
		for(y=0;y<wam->numCols;y++)
		{
			ReadCol(fno, &wam->patterns[x].columns[y]);
		}
		ReadCol(fno, &wam->patterns[x].unplayed);
	}
	Log(("Patterns read\n"));
	read(fno, wam->rowData, sizeof(struct row) * wam->numRows);
	Log(("Rows read\n"));
	close(fno);

	return wam;
}

/** Loads the wam related to the @a modFile and returns it.  Must be freed
 * later by free_wam
 * @param modFile The filename of the mod
 * @return The wam for the mod file
 */
struct wam *load_wam(const char *modFile)
{
	struct wam *wam;
	char *wamFile;

	wamFile = Mod2Wam(modFile);
	Log(("Load MOD: %s\nWAM: %s\n", modFile, wamFile));

	/* the first time we try to load the file we create it
	 * if it doesn't exist
	 */
	wam = ReadWam(wamFile);
	if(wam == NULL) {
		/* If we couldn't read the wam, create one in memory and try
		 * to save it. Failure to save is not an error, since we can
		 * just recreate the wam next time (it will just be slower)
		 */
		wam = CreateWam(modFile);
		if(wam == NULL) {
			ELog(("Error: Couldn't create wam file\n"));
			free(wamFile);
			return 0;
		}
		if(!SaveWam(wam, wamFile)) {
			ELog(("Error: Couldn't save wam. Make sure the wam/ directory in the data directory: '%s' is writeable\n", MARFDATADIR));
		}
	}

	free(wamFile);
	return wam;
}

/** Frees the @a wam from that was created by load_wam */
void free_wam(struct wam *wam)
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
