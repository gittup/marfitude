/*
   Marfitude
   Copyright (C) 2006 Mike Shal

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
#define WAM_MAGIC "wam-1.2"

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
	struct sample *samples; /**< list of samples. Size numSamples, valid
	                         * length trklen
	                         */
	int trklen;             /**< Length of the sample data above. This
	                         * is actually the same for all tracks.
	                         */
	int numSamples;         /**< The size of the samples array */
	int *notes;             /**< some temporary space to find the best
	                         * tracks in a pattern
	                         */
	int interest;           /**< how "interesting" this track is :) */
	int singleIns;          /**< if there is only one instrument, this is
	                         * set to correspond to that instrument.
	                         * if there are multiple instruments or the 
	                         * track is blank, this is -1
	                         */
	int is_empty;           /**< set if this track has no notes */
	int *channels;          /**< length mod->numchn, reflects which channels
	                         * have been merged
	                         */
	int numChannels;        /**< valid channels in list above.  Starts out
	                         * as 1 then grows as tracks are merged
	                         */
	int lastCol;            /**< last column this track was placed in */
};

static struct wam *load_track_data(void);
static void remove_empty_tracks(struct wam *wam);
static char *base_file_name(const char *file);
static char *mod_to_wam(const char *file);
static int get_note(UBYTE *trk, UWORD row);
static int get_instrument(UBYTE *trk, UWORD row);
static void handler(void);
static void calculate_difficulty(struct track *t, struct wam *wam, int row);
static void combine_single_ins_tracks(struct track *t1, struct track *t2);
static int tracks_intersect(struct track *t1, struct track *t2);
static int next_pos(int pos, int movinup, Uint32 note, Uint32 *mem);
static int gen_track_data(struct track *t, int pos);
static void check_notes_unused(struct track *t);
static int best_track(struct track *t);
static void set_column(struct column *col, struct track *trk, struct wam *wam, int colnum, int patnum, int startRow);
static int empty_col(struct column *cols, int num_cols, struct column **retptr);
static void update_row_data(struct track *t, struct wam *wam, int startRow, int patnum);
static void clear_track(struct track *t, int chan);
static int set_sample(struct sample *s, int chan);
static void write_col(int fno, struct column *col);
static int save_wam(struct wam *wam, const char *wam_file);
static void read_col(int fno, struct column *col);
static struct wam *read_wam(const char *wam_file);
static struct wam *create_wam(const char *file);

char *base_file_name(const char *file)
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

char *mod_to_wam(const char *file)
{
	char *s;
	char *base;
	int len;

	base = base_file_name(file);
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

int get_note(UBYTE *trk, UWORD row)
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
				UniSkipOpcode();
				break;
		}
	}
	return 0;
}

int get_instrument(UBYTE *trk, UWORD row)
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
				UniSkipOpcode();
				break;
		}
	}
	return 0;
}

void handler(void)
{
	/* This is the tick handler for when we load the song initially.
	 * Since we don't want the song to play while we load it,
	 * we do nothing with the driver generated tick
	 */
}

/* copy t2 over t1, and then void t2 by setting is_empty to 1 and
 * clearing its channels
 * t1 becomes the intersection of t1 and t2, with notes averaged
 * volumes are copied, unaveraged
 */
void combine_single_ins_tracks(struct track *t1, struct track *t2)
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
	t2->is_empty = 1;
	t2->numChannels = 0;
}

/* if both t1 and t2 have a note in the same row, they 'intersect'
 * and a 1 is returned. otherwise, 0
 */
int tracks_intersect(struct track *t1, struct track *t2)
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
int next_pos(int pos, int movinup, Uint32 note, Uint32 *mem)
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

int gen_track_data(struct track *t, int pos)
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
						pos = next_pos(pos, 1, *newnote, mem);
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
						pos = next_pos(pos, 0, *newnote, mem);
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

int best_track(struct track *t)
{
	int x;
	int bestTrack = -1;
	int bestInterest = 0;
	for(x=0;x<mod->numchn;x++)
	{
		if(t[x].is_empty) continue;
		if(bestTrack == -1 || bestInterest < t[x].interest)
		{
			bestTrack = x;
			bestInterest = t[x].interest;
		}
	}
	return bestTrack;
}

void set_column(struct column *col, struct track *trk, struct wam *wam, int colnum, int patnum, int startRow)
{
	int y;
	trk->is_empty = 2;
	trk->lastCol = colnum;
	col->numchn = trk->numChannels;
	col->chan = malloc(sizeof(int) * col->numchn);
	memcpy(col->chan, trk->channels, sizeof(int) * col->numchn);
	for(y=0;y<trk->trklen;y++)
	{
		wam->row_data[startRow+y].notes[colnum] = trk->notes[y];
		wam->row_data[startRow+y].difficulty[colnum] = trk->samples[y].difficulty;
		wam->row_data[startRow+y].patnum = patnum;
	}
}

int empty_col(struct column *cols, int num_cols, struct column **retptr)
{
	int x;
	for(x=0;x<num_cols;x++)
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
			double time = wam->row_data[row+x].time;
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

/* Check if there's only one or two note positions used in the track, where
 * those one or two positions should be.
 */
void check_notes_unused(struct track *t)
{
	int x;
	int used[5] = {0};
	int samples = 0;

	/* Find out which note positions are used. We only care about
	 * 1, 2, and 4.
	 */
	for(x=0; x<t->trklen; x++) {
		used[t->notes[x]] = 1;
		if(t->notes[x]) {
			/* Use the instrument of the notes as the seemingly
			 * random yet well-defined arbitrator of which tracks
			 * the two notes will go in to.
			 */
			samples += t->samples[x].ins;
		}
	}

	/* There are three choices,
	 * 1 note: left, middle, and right
	 * 2 notes: left-middle, middle-right, and left-right.
	 * We can use 0, 1, and 2 for these choices.
	 */
	samples = samples % 3;

	/* Default case of 0: use left-middle, or left which it already is. */
	if(!samples)
		return;

	/* Only one track, determine where it goes... */
	if(used[1] + used[2] + used[4] == 1) {
		for(x=0; x<t->trklen; x++) {
			if(t->notes[x]) {
				if(samples == 1) /* Middle */
					t->notes[x] = 2;
				else /* (samples == 2) Right */
					t->notes[x] = 4;
			}
		}
	}

	/* Only two tracks, determine where they go... */
	if(used[1] + used[2] + used[4] == 2) {
		for(x=0; x<t->trklen; x++) {
			/* Always move the middle note to the right, since
			 * the only cases yet to be taken care of are
			 * middle-right and left-right.
			 */
			if(t->notes[x] == 2)
				t->notes[x] = 4;

			/* Move the left note to the middle if we're in the
			 * middle-right case (1)
			 */
			if(t->notes[x] == 1 && samples == 1)
				t->notes[x] = 2;
		}
	}
}

void update_row_data(struct track *t, struct wam *wam, int startRow, int patnum)
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
		if(t[x].is_empty || t[x].singleIns == -1) continue;
		for(y=x+1;y<mod->numchn;y++)
		{
			if(t[y].is_empty) continue;
			if(t[x].singleIns == t[y].singleIns && !tracks_intersect(&t[x], &t[y]))
			{
				combine_single_ins_tracks(&t[x], &t[y]);
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
			tmpCount = gen_track_data(&t[x], y);
			if(!bestStart || tmpCount < bestCount)
			{
				bestStart = y;
				bestCount = tmpCount;
			}
		}
		/* if the best data is already in memory, no need to
		 * generate it again, so save a little time :)
		 */
		if(bestStart != MAX_NOTE) gen_track_data(&t[x], bestStart);
		check_notes_unused(&t[x]);
	}

	/* get a list of the best tracks
	 * mark them empty as we go through, best_track picks up the best
	 * "non-empty" track
	 */
	Log(("A\n"));
	for(x=0;x<wam->num_cols;x++)
	{
		bestTrks[x] = best_track(t);
		Log(("Best: %i, Old: %i\n", bestTrks[x], t[x].lastCol));
		if(bestTrks[x] != -1) t[bestTrks[x]].is_empty = 1;
		col = &pat->columns[x];
		col->numchn = 0;
		col->chan = NULL;
	}
	Log(("B\n"));

	/* place all the best tracks that can be placed in their previous slots
	 */
	for(x=0;x<wam->num_cols;x++)
	{
		if(bestTrks[x] == -1) continue;
		trk = &t[bestTrks[x]];
		if(trk->lastCol != -1)
		{
			col = &pat->columns[trk->lastCol];
			if(col->chan == NULL)
			{
				Log(("Replace: %i\n", trk->lastCol));
				set_column(col, trk, wam, trk->lastCol, patnum, startRow);
			}
		}
	}
	Log(("C\n"));

	/* now place all the tracks that didn't fit, or weren't in previously */
	for(x=0;x<wam->num_cols;x++)
	{
		if(bestTrks[x] == -1) continue;
		trk = &t[bestTrks[x]];
		if(trk->is_empty != 2)
		{
			y = empty_col(pat->columns, wam->num_cols, &col);
			Log(("Place: %i\n", y));
			if(y == -1)
			{
				ELog(("ERROR: No empty column!\n"));
			}
			set_column(col, trk, wam, y, patnum, startRow);
		}
	}
	/* set empty rowdata for unused columns */
	for(x=0;x<wam->num_cols;x++)
	{
		col = &pat->columns[x];
		if(col->chan == NULL)
		{
			Log(("Empty: %i\n", x));
			for(y=0;y<t->trklen;y++)
			{
				wam->row_data[startRow+y].notes[x] = 0;
				wam->row_data[startRow+y].patnum = patnum;
			}
		}
	}

	col = &pat->unplayed;
	col->numchn = 0;
	col->chan = NULL;
	for(x=0;x<mod->numchn;x++)
	{
		if(t[x].is_empty) continue;
		col->chan = (int*)realloc(col->chan, sizeof(int) * (col->numchn + t[x].numChannels));
		memcpy(&(col->chan[col->numchn]), t[x].channels, sizeof(int) * t[x].numChannels);
		col->numchn += t[x].numChannels;
	}
}

/* resets default values in the Track struct for channel chan */
void clear_track(struct track *t, int chan)
{
	t->channels[0] = chan;
	t->numChannels = 1;
	t->is_empty = 1;
	t->singleIns = -1;
	t->interest = 0;
	t->trklen = 0;
}

/* sets the sample information in s from channel chan = [0..mod->numchn-1]
 * returns the instrument used, -1 if there is no note
 */
int set_sample(struct sample *s, int chan)
{
	/* can't use the 'sample' and 'note' values
	 * from the MP_CONTROL struct in
	 * mikmod_internals.h since they aren't
	 * necessarily set to 0 after the note is
	 * first "struck."  Instead we get the info
	 * from MikMod's internal format
	 */
	s->vol = mod->control[chan].volume;
	s->ins = get_instrument(mod->control[chan].row, 0);
	s->note = get_note(mod->control[chan].row, 0);

	if(!s->note)
		s->ins = -1;
	return s->ins;
}

/* returns a new WAM structure containing all data necessary for the game */
struct wam *load_track_data(void)
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
		clear_track(&tracks[x], x);
		tracks[x].lastCol = -1;
	}

	Player_Mute(MUTE_INCLUSIVE, 0, mod->numchn-1);
	oldHand = MikMod_RegisterPlayer(handler);
	oldSngPos = -1;
	lineCount = 0;
	lineTicker = 0;

	wam->num_cols = WAM_MAX_TRACKS;
	if(wam->num_cols >= mod->numchn) wam->num_cols = mod->numchn;
	if(wam->num_cols >= MAX_COLS) wam->num_cols = MAX_COLS;

	Player_TogglePause(); /* start the mod again so we can read in data */
	Log(("Starting row loop\n"));
	progress_meter("Creating WAM");
	while(mod->sngpos < mod->numpos)
	{
		if(!mod->vbtick)
		{
			if(wam->num_rows >= rowsAlloced)
			{
				if(rowsAlloced == 0)
				{
					rowsAlloced = 1024;
				}
				else
				{
					rowsAlloced <<= 2;
				}
				wam->row_data = (struct row*)realloc(wam->row_data, sizeof(struct row) * rowsAlloced);
			}

			/* only designate one place the start of a
			 * pattern so if we loop there aren't multiple
			 * 'starts'
			 */
			if(!mod->patpos && mod->sngpos != oldSngPos)
			{
				wam->row_data[wam->num_rows].line = 2;
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
						wam->row_data[wam->num_rows-x-1].ticgrp = grpCount;
					}
					numgrps = 0;
					grpCount = 0;

					wam->patterns = (struct pattern*)realloc(wam->patterns, sizeof(struct pattern) * (wam->num_pats+1));
					update_row_data(tracks, wam, startRow, wam->num_pats);
					update_progress(mod->sngpos, mod->numpos);
					wam->num_pats++;
					startRow = wam->num_rows;
					for(x=0;x<mod->numchn;x++)
					{
						clear_track(&tracks[x], x);
					}
				}
			}
			else if(lineTicker >= 20 && (lineCount&3) == 0) {
				wam->row_data[wam->num_rows].line = 1;
				lineCount = 0;
				lineTicker = 0;
			}
			else wam->row_data[wam->num_rows].line = 0;

			Log(("Sng: %i Pat: %i row: %i, alloc: %i\n", mod->sngpos, mod->patpos, wam->num_rows, rowsAlloced));
			wam->row_data[wam->num_rows].bpm = mod->bpm;
			wam->row_data[wam->num_rows].sngspd = mod->sngspd;
			wam->row_data[wam->num_rows].ticpos = tickCount;
			wam->row_data[wam->num_rows].ticprt = grpCount;
			wam->row_data[wam->num_rows].patpos = mod->patpos;
			wam->row_data[wam->num_rows].sngpos = mod->sngpos;
			wam->row_data[wam->num_rows].time = time;
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
					wam->row_data[wam->num_rows-x].ticgrp = grpCount;
				}
				numgrps = 0;
				grpCount = 0;
			}
			lineCount++;
			lineTicker += mod->sngspd;

			for(x=0;x<mod->numchn;x++)
			{
				ins = set_sample(tracks[x].samples+tracks[x].trklen, x);
				/* some confusing logic ahead :)
				 * basically it clears the is_empty var
				 * if an instrument is used, and ensures that
				 * singleIns is set to the single instrument
				 * used if there is only one instrument.  If
				 * more than one is used or the track is empty,
				 * it is set to -1 (clear_track() sets it to
				 * -1 when to start, and is not modified when
				 * it's empty)
				 */
				if(ins != -1)
				{
					if(tracks[x].is_empty)
					{
						tracks[x].is_empty = 0;
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
			wam->num_rows++;
		}
		Player_HandleTick();
		wam->num_tics++;
	}
	wam->patterns = (struct pattern*)realloc(wam->patterns, sizeof(struct pattern) * (wam->num_pats+1));
	update_row_data(tracks, wam, startRow, wam->num_pats);
	update_progress(mod->sngpos, mod->numpos);
	end_progress_meter();
	wam->num_pats++;
	wam->song_length = time;
	Log(("Row loop done\n"));

	oldHand = MikMod_RegisterPlayer(oldHand);
	for(x=0;x<mod->numchn;x++)
	{
		free(tracks[x].samples);
		free(tracks[x].notes);
		free(tracks[x].channels);
	}
	free(tracks);
	remove_empty_tracks(wam);
	Log(("Track data Ready\n"));
	return wam;
}

void remove_empty_tracks(struct wam *wam)
{
	int x;
	int y;

	for(x=wam->num_cols - 1; x>=0; x--) {
		for(y=0; y<wam->num_rows; y++) {
			if(wam->row_data[y].notes[x]) {
				/* As soon as a note is found, we're done.
				 * Since tracks are put in from 0 to num_cols-1,
				 * the empty tracks will always be at the high
				 * end. So once a non-empty track is found,
				 * there can't be any more empty ones in
				 * between.
				 */
				return;
			}
		}
		/* Track is empty, remove it */
		wam->num_cols--;
	}
}

static void mwrite(int fd, const void *buf, size_t count)
{
	if(write(fd, buf, count) < 0) {
		perror("write");
		exit(1);
	}
}

static void mread(int fd, void *buf, size_t count)
{
	if(read(fd, buf, count) < 0) {
		perror("read");
		exit(1);
	}
}

void write_col(int fno, struct column *col)
{
	mwrite(fno, &col->numchn, sizeof(int32_t));
	mwrite(fno, col->chan, sizeof(int32_t) * col->numchn);
}

int save_wam(struct wam *wam, const char *wam_file)
{
	int x;
	int y;
	int fno;
	fno = open(wam_file, O_CREAT | O_WRONLY | O_BINARY, 0644);
	if(fno == -1)
	{
		ELog(("Error opening '%s'\n", wam_file));
		Error("Opening Wam for write.");
		return 0;
	}
	mwrite(fno, WAM_MAGIC, WAM_MAGIC_LEN);
	mwrite(fno, &wam->num_cols, sizeof(int32_t));
	mwrite(fno, &wam->num_tics, sizeof(int32_t));
	mwrite(fno, &wam->num_pats, sizeof(int32_t));
	mwrite(fno, &wam->num_rows, sizeof(int32_t));
	mwrite(fno, &wam->song_length, sizeof(double));
	for(x=0;x<wam->num_pats;x++)
	{
		for(y=0;y<wam->num_cols;y++)
		{
			write_col(fno, &wam->patterns[x].columns[y]);
		}
		write_col(fno, &wam->patterns[x].unplayed);
	}
	mwrite(fno, wam->row_data, sizeof(struct row) * wam->num_rows);
	close(fno);
	return 1;
}

struct wam *create_wam(const char *file)
{
	struct wam *wam;

	if(start_module(file))
	{
		ELog(("Error: Couldn't start module.\n"));
		return NULL;
	}
	Log(("Loading track data...\n"));
	wam = load_track_data();
	stop_module();
	return wam;
}

void read_col(int fno, struct column *col)
{
	mread(fno, &col->numchn, sizeof(int32_t));
	if(col->numchn)
	{
		col->chan = malloc(sizeof(int32_t) * col->numchn);
		mread(fno, col->chan, sizeof(int32_t) * col->numchn);
	}
	else
	{
		col->chan = NULL;
	}
}

/** Loads the @a file and creates and writes the appropriate WamFile
 * @param file filename of the mod
 * @returns 1 if successful, 0 on failure
 */
int write_wam(const char *file)
{
	struct wam *wam;
	char *wam_file;

	wam = create_wam(file);
	if(wam == NULL) {
		ELog(("Error: Couldn't create wam file.\n"));
		return 0;
	}

	wam_file = mod_to_wam(file);
	Log(("Write MOD: %s\nWAM: %s\n", file, wam_file));
	if(!save_wam(wam, wam_file)) {
		ELog(("Error: Couldn't save wam. Make sure the wam/ directory in the data directory: '%s' is writeable\n", MARFDATADIR));
		free(wam_file);
		return 0;
	}

	free(wam_file);
	return 1;
}

struct wam *read_wam(const char *wam_file)
{
	int x;
	int y;
	int fno;
	char magic[WAM_MAGIC_LEN];
	struct wam *wam;

	fno = open(wam_file, O_RDONLY | O_BINARY);
	if(fno == -1)
		return NULL;
	mread(fno, magic, WAM_MAGIC_LEN);
	if(strcmp(magic, WAM_MAGIC) != 0) {
		close(fno);
		return NULL;
	}
	Log(("Loading wam\n"));
	wam = malloc(sizeof(struct wam));
	mread(fno, &wam->num_cols, sizeof(int32_t));
	mread(fno, &wam->num_tics, sizeof(int32_t));
	mread(fno, &wam->num_pats, sizeof(int32_t));
	mread(fno, &wam->num_rows, sizeof(int32_t));
	mread(fno, &wam->song_length, sizeof(double));
	Log(("Main info read: %i cols, %i tics, %i pats, %i rows\n", wam->num_cols, wam->num_tics, wam->num_pats, wam->num_rows));
	wam->patterns = malloc(sizeof(struct pattern) * wam->num_pats);
	wam->row_data = malloc(sizeof(struct row) * wam->num_rows);
	for(x=0;x<wam->num_pats;x++)
	{
		for(y=0;y<wam->num_cols;y++)
		{
			read_col(fno, &wam->patterns[x].columns[y]);
		}
		read_col(fno, &wam->patterns[x].unplayed);
	}
	Log(("Patterns read\n"));
	mread(fno, wam->row_data, sizeof(struct row) * wam->num_rows);
	Log(("Rows read\n"));
	close(fno);

	return wam;
}

/** Loads the wam related to the @a file and returns it.  Must be freed
 * later by free_wam
 * @param file The filename of the mod
 * @return The wam for the mod file
 */
struct wam *load_wam(const char *file)
{
	struct wam *wam;
	char *wam_file;

	wam_file = mod_to_wam(file);
	Log(("Load MOD: %s\nWAM: %s\n", file, wam_file));

	/* the first time we try to load the file we create it
	 * if it doesn't exist
	 */
	wam = read_wam(wam_file);
	if(wam == NULL) {
		/* If we couldn't read the wam, create one in memory and try
		 * to save it. Failure to save is not an error, since we can
		 * just recreate the wam next time (it will just be slower)
		 */
		wam = create_wam(file);
		if(wam == NULL) {
			ELog(("Error: Couldn't create wam file\n"));
			free(wam_file);
			return 0;
		}
		if(!save_wam(wam, wam_file)) {
			ELog(("Error: Couldn't save wam. Make sure the wam/ directory in the data directory: '%s' is writeable\n", MARFDATADIR));
		}
	}

	free(wam_file);
	return wam;
}

/** Frees the @a wam from that was created by load_wam */
void free_wam(struct wam *wam)
{
	int x, y;
	Log(("Freeing Wam\n"));
	for(x=0;x<wam->num_pats;x++)
	{
		for(y=0;y<wam->num_cols;y++)
		{
			if(wam->patterns[x].columns[y].numchn)
				free(wam->patterns[x].columns[y].chan);
		}
		if(wam->patterns[x].unplayed.numchn)
			free(wam->patterns[x].unplayed.chan);
	}
	free(wam->patterns);
	free(wam->row_data);
	free(wam);
	Log(("Wam freed\n"));
}

/** Get a valid row index from the wam. If the row is less than 0, 0 is
 * returned. If the row is past the last row, the last row index is returned.
 * Otherwise the input row is returned.
 */
int wam_rowindex(const struct wam *wam, int row)
{
	if(row < 0)
		return 0;
	if(row >= wam->num_rows)
		return wam->num_rows - 1;
	return row;
}
