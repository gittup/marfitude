#include <stdio.h>
#include <string.h>

#include "SDL_mixer.h"
#include "mikmod.h"
#include "mikmod_internals.h"

#include "module.h"
#include "cfg.h"
#include "log.h"
#include "../util/memtest.h"
#include "../util/sdlfatalerror.h"

typedef struct {
	int note;
	int ins;
	int vol;
	} Sample;

typedef struct {
	Sample *samples;	// list of samples. length is the same
				// for all tracks, so it is kept in the function
				// and not in the struct
	int *notes;		// some temporary space to find the best
				// tracks in a pattern
	int interest;		// how "interesting" this track is :)
	int singleIns;		// if there is only one instrument, this is
				// set to correspond to that instrument.
				// if there are multiple instruments or the 
				// track is blank, this is -1
	int isEmpty;		// if this track has no notes, isEmpty is set
	int *channels;		// length mod->numchn, reflects which channels
				// have been merged
	int numChannels;	// valid channels in list above.  Starts out as
				// 1 then grows as tracks are merged
	} Track;

Mix_Music *modMusic = NULL;
MODULE *mod = NULL;
ModRow *rowData = NULL;
int numRows = 0;
int numTics = 0;
int numChannels = 0;
int volThreshold; // grabbed from config file

int PatternForTrack(int trk)
{
	int p, c;
	for(p=0;p<mod->numpat;p++)
	{
		for(c=0;c<mod->numchn;c++)
		{
			if(mod->patterns[(p*mod->numchn)+c] == trk) return p;
		}
	}
	return -1;
}

int PatternHasTrack(int pat, int trk)
{
	int c;
	for(c=0;c<mod->numchn;c++)
	{
		if(mod->patterns[(pat*mod->numchn)+c] == trk) return 1;
	}
	return 0;
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
					// don't know why this happens, but it's
					// in the libmikmod code and cures
					// a segfault :)
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

int IsSingleInstrumentTrack(int trk)
{
	int x;
	int inst = 0;
	int newinst;
	for(x=0;x<RowsInTrack(trk);x++)
	{
		newinst = GetInstrument(mod->tracks[trk], x);
		if(inst == 0) inst = newinst;
		else if(newinst && inst != newinst) return 0;
	}
	return 1;
}

int IsMultiInstrumentSingleNoteTrack(int trk, int *insts)
{
	int x;
	int newnote;
	int newinst;
	for(x=0;x<mod->numins;x++) insts[x] = 0;
	for(x=0;x<RowsInTrack(trk);x++)
	{
		newinst = GetInstrument(mod->tracks[trk], x);
		if(newinst)
		{
			newnote = GetNote(mod->tracks[trk], x);
			if(insts[newinst] == 0) insts[newinst] = newnote;
			else if(insts[newinst] != newnote) return 0;
		}
	}
	return 1;
}

int NumInstruments(int trk, int *insts)
{
	int x;
	int ins = 0;
	int newinst;
	for(x=0;x<mod->numins;x++) insts[x] = 0;
	for(x=0;x<RowsInTrack(trk);x++)
	{
		newinst = GetInstrument(mod->tracks[trk], x);
		if(newinst && !insts[newinst])
		{
			insts[newinst] = 1;
			ins++;
		}
	}
	return ins;
}

int ClipPos(int newpos)
{
	if(newpos > 3) return 3;
	if(newpos < 1) return 1;
	return newpos;
}

void SingleInstrumentTrack(unsigned char *trkData, int trk)
{
	int cur = -1;
	int pos = 1;
	int x;
	int new;
	for(x=0;x<RowsInTrack(trk);x++)
	{
		new = GetNote(mod->tracks[trk], x);
		if(new)
		{
			if(cur == -1)
			{
				cur = new;
				trkData[x] = pos;
			}
			else
			{
				if(new > cur)
				{
					pos = ClipPos(pos+1);
				}
				else if(new < cur)
				{
					pos = ClipPos(pos-1);
				}
				// else pos doesn't change
				trkData[x] = pos;
				cur = new;
			}
		}
		else trkData[x] = 0;
	}
}

void GenTrackDataold(unsigned char *trkData, int trk)
{
	int *insts;
	int x;
	insts = (int*)malloc(sizeof(int)*mod->numins);
	if(IsSingleInstrumentTrack(trk))
	{
		SingleInstrumentTrack(trkData, trk);
//		printf("Track %i is single instrument\n", trk);
	}
	else if(IsMultiInstrumentSingleNoteTrack(trk, insts))
	{
		int numInst = NumInstruments(trk, insts);
		int *instPtr = (int*)malloc(sizeof(int)*numInst);
		int curInst = 0;
		int inst;
		int y;
//		printf("Track %i has %i instruments.\n", trk, numInst);
		for(x=0;x<RowsInTrack(trk);x++)
		{
			inst = GetInstrument(mod->tracks[trk], x);
			if(inst)
			{
				trkData[x] = 0;
				for(y=0;y<curInst;y++)
				{
					if(instPtr[y] == inst)
					{
						trkData[x] = (y%3) + 1;
						break;
					}
				}
				if(!trkData[x])
				{
					instPtr[curInst] = inst;
					trkData[x] = (curInst%3) + 1;
					curInst++;
				}
			}
			else trkData[x] = 0;
		}
		free(instPtr);
	}
	else
	{
//		printf("Unsure how to process track %i\n", trk);
		for(x=0;x<RowsInTrack(trk);x++)
		{
			if(GetNote(mod->tracks[trk], x)) trkData[x] = 2;
			else trkData[x] = 0;
		}
	}
	free(insts);
}

// handler should stop multiple conflicting Player_HandleTick()'s to skip ticks
// and sometimes segfault
MikMod_player_t oldHand;

void Handler(void)
{
	// This is the tick handler for when we load the song initially.
	// Since we don't want the song to play while we load it,
	// we do nothing with the driver generated tick
}

// check whether two tracks are "equal"
// this means whether or not they both strike notes in the same places, not
// the fact that they are redundant tracks.
// They could have different instruments and notes, but if the notes
// all occur in the same place in the track, they are considered "equal"
int TracksAreEqual(Sample *s1, Sample *s2, int trklen)
{
	int x;
	for(x=0;x<trklen;x++)
	{
		// if one track has a note and the other doesn't, they don't
		// match!
		if(s1[x].note > 0 && s2[x].note == 0) return 0;
		if(s1[x].note == 0 && s2[x].note > 0) return 0;
	}
	return 1;
}

// add s1 and s2 into dest
void CombineSamples(Sample *dest, Sample *s1, Sample *s2)
{
	dest->note = s1->note + s2->note;
	dest->ins = s1->ins + s2->ins;
	dest->vol = s1->vol + s2->vol;
}

// copy t2 over t1, and then void t2 by setting isEmpty to 1 and
// clearing its channels
// t1 becomes the intersection of t1 and t2, with notes averaged
// volumes are copied, unaveraged
void CombineSingleInsTracks(Track *t1, Track *t2, int trklen)
{
	int x;
	// keep the singleIns field up-to-date
	if(t1->singleIns != t2->singleIns) t1->singleIns = -1;

	// copy all the channels used by t2 into t1
	for(x=0;x<t2->numChannels;x++)
	{
		t1->channels[t1->numChannels] = t2->channels[x];
		t1->numChannels++;
	}

	// now copy all the notes/instruments from t2 into t1
	// if t1 doesn't have a note there, copy t2 over it
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
				// multiply by numChannels to discount
				// averaging
				t1->samples[x].note = t1->numChannels * t2->samples[x].note;
				t1->samples[x].ins = t2->samples[x].ins;
				t1->samples[x].vol = t2->samples[x].vol;
			}
		}
	}
	t2->isEmpty = 1;
	t2->numChannels = 0;
}

// if both t1 and t2 have a note in the same row, they 'intersect'
// and a 1 is returned. otherwise, 0
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
				// no else, position is constant if the
				// consecutive notes are the same
			}
			t->interest += (*oldnote != *newnote) * (t->samples[x].vol);
			*oldnote = *newnote;
			t->notes[x] = pos;
		}
		else t->notes[x] = 0;
	}
	return mishaps;
}

int BestTrack(Track *t, int trklen)
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

void UpdateRowData(Track *t, int trklen, ModRow *startRow)
{
	int x, y, trk;
	int bestStart, bestCount, tmpCount;

	// try to combine tracks that are one instrument line that is
	// split among several tracks. These can be found by checking if
	// two (or more) tracks all use the same instrument, but in different
	// rows
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
		// if the best data is already in memory, no need to
		// generate it again, so save a little time :)
		if(bestStart != MAX_NOTE) GenTrackData(&t[x], trklen, bestStart);
	}

	// now combine tracks that are duplicates of each other - no sense
	// having a bunch of tracks that are all the same thing
/*	for(x=0;x<mod->numchn-1;x++)
	{
		if(t[x].isEmpty) continue;
		for(y=x+1;y<mod->numchn;y++)
		{
			if(t[y].isEmpty) continue;
			if(TracksAreEqual(t[x].samples, t[y].samples, trklen))
			{
				//CombineTracks(&t[x], &t[y], trklen);
			}
		}
	}*/

	for(x=0;x<numChannels;x++)
	{
		trk = BestTrack(t, trklen);
		if(trk == -1)
		{
			for(y=0;y<trklen;y++)
			{
				startRow[y].chans[x].note = 0;
				startRow[y].chans[x].struck = 0;
				startRow[y].chans[x].modChannels = NULL;
				startRow[y].chans[x].numModChannels = 0;
			}
		}
		else
		{
			t[trk].isEmpty = 1;
			for(y=0;y<trklen;y++)
			{
				startRow[y].chans[x].note = t[trk].notes[y];
				startRow[y].chans[x].struck = 0;
				startRow[y].chans[x].numModChannels = t[trk].numChannels;
				startRow[y].chans[x].modChannels = (int*)malloc(sizeof(int) * t[trk].numChannels);
				memcpy(startRow[y].chans[x].modChannels, t[trk].channels, sizeof(int) * t[trk].numChannels);
			}
		}
	}
	for(x=0;x<mod->numchn;x++)
	{
		if(t[x].isEmpty) continue;
		printf("Track: %i: Interest: %i Channels: ", x, t[x].interest);
		for(y=0;y<t[x].numChannels;y++) printf("%i ", t[x].channels[y]);
		printf("\n");
	}
}

// resets default values in the Track struct for channel chan
void ClearTrack(Track *t, int chan)
{
	t->channels[0] = chan;
	t->numChannels = 1;
	t->isEmpty = 1;
	t->singleIns = -1;
	t->interest = 0;
}

// sets the sample information in s from channel chan = [0..mod->numchn-1]
// ignores notes that are too quiet (cutoff set by config file)
// returns the instrument used, -1 if there is no note
int SetSample(Sample *s, int chan)
{
	// can't use the 'sample' and 'note' values
	// from the MP_CONTROL struct in
	// mikmod_internals.h since they aren't 
	// necessarily set to 0 after the note is
	// first "struck."  Instead we get the info
	// from MikMod's internal format
	s->vol = mod->control[chan].outvolume;
	if(s->vol < volThreshold)
	{
		s->vol = 0;
		s->note = 0;
		s->ins = -1; // 0 is a valid instrument, -1 is not
	}
	else
	{
		s->ins = GetInstrument(mod->control[chan].row, 0);
		s->note = GetNote(mod->control[chan].row, 0);
	}
	if(s->note) return s->ins;
	else return -1;
}

void LoadTrackData(void)
{
	int x, oldSngPos, lineCount;
	int ins;
	int tickCount = 0;
	int startRow = 0;
	int numSamples = 64;	// numSamples = how much we have allocated
				// start with 64, since that's the default
				// number of rows/pattern for most mods
	int trklen = 0;		// trklen is how many samples we are actually
				// using, since patterns can vary in length,
				// even in the same song
	Track *tracks;		// track data
				// we keep one pattern worth of samples
				// in memory to check for redundant tracks
				// and pick the best remaining tracks to play

	tracks = (Track*)malloc(sizeof(Track) * mod->numchn);
	for(x=0;x<mod->numchn;x++)
	{
		tracks[x].samples = (Sample*)malloc(sizeof(Sample) * numSamples);
		tracks[x].notes = (int*)malloc(sizeof(int) * numSamples);
		tracks[x].channels = (int*)malloc(sizeof(int) * mod->numchn);
		ClearTrack(&tracks[x], x);
	}

	Player_Mute(MUTE_INCLUSIVE, 0, mod->numchn-1);
	oldHand = MikMod_RegisterPlayer(Handler);
	oldSngPos = -1;
	lineCount = 0;

	Player_TogglePause(); // start the mod up again so we can read in data
	Log("Starting row loop\n");
	while(mod->sngpos < mod->numpos)
	{
		if(!mod->vbtick)
		{
			Log("Sng: %i row: %i\n", mod->sngpos, numRows);
			rowData = (ModRow*)realloc(rowData, sizeof(ModRow) * (numRows+1));
			numChannels = CfgI("main.tracks");
			if(mod->numchn < numChannels) numChannels = mod->numchn;
			rowData[numRows].chans = (ModChannel*)malloc(sizeof(ModChannel) * numChannels);
			rowData[numRows].bpm = mod->bpm;
			rowData[numRows].sngspd = mod->sngspd;
			rowData[numRows].ticpos = tickCount;
			rowData[numRows].patpos = mod->patpos;
			rowData[numRows].sngpos = mod->sngpos;
			tickCount += mod->sngspd;

			// only designate one place the start of a
			// pattern so if we loop there aren't multiple
			// 'starts'
			if(!mod->patpos && mod->sngpos != oldSngPos)
			{
				rowData[numRows].line = 2;
				oldSngPos = mod->sngpos;
				lineCount = 0;
				// now convert the track data to something
				// usable. mod->sngpos == 0 the first time
				// through, so there won't be any track data
				// then :)
				if(mod->sngpos != 0)
				{
					UpdateRowData(tracks, trklen, &rowData[startRow]);
					startRow = numRows;
					trklen = 0;
					for(x=0;x<mod->numchn;x++)
					{
						ClearTrack(&tracks[x], x);
					}
				}
			}
			else if(!(lineCount&3)) rowData[numRows].line = 1;
			else rowData[numRows].line = 0;
			lineCount++;

			for(x=0;x<mod->numchn;x++)
			{
				ins = SetSample(tracks[x].samples+trklen, x);
				// some confusing logic ahead :)
				// basically it clears the isEmpty var
				// if an instrument is used, and ensures that
				// singleIns is set to the single instrument
				// used if there is only one instrument.  If
				// more than one is used or the track is empty,
				// it is set to -1 (ClearTrack() sets it to
				// -1 when to start, and is not modified when
				// it's empty)
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
//				Log("Chan: %i\n", x);
//				rowData[numRows].struck[x] = 0;
//				if(GetNote(mod->control[x].row, 0))
//					rowData[numRows].notes[x] = 2;
//				else rowData[numRows].notes[x] = 0;
//				Log("Chan done\n");
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
			numRows++;
		}
		Log("Handle tick\n");
		Player_HandleTick();
		numTics++;
		Log("handled\n");
	}
	Player_Unmute(MUTE_INCLUSIVE, 0, mod->numchn-1);
	Player_SetPosition(0);
	Player_TogglePause(); 	// pause the module since we see the screen
				// before it plays
	UpdateRowData(tracks, trklen, &rowData[startRow]);
	Log("Row loop done\n");
	printf("POS: %i, %i\n", mod->sngpos, Player_Paused());

//	Player_Stop();
//	Player_Start(mod);
	printf("POS: %i, %i\n", mod->sngpos, Player_Paused());
	oldHand = MikMod_RegisterPlayer(oldHand);
	for(x=0;x<mod->numchn;x++)
	{
		free(tracks[x].samples);
		free(tracks[x].notes);
		free(tracks[x].channels);
	}
	free(tracks);
	Log("Track data Ready\n");
}

int StartModule(char *modFile)
{
	Log("Loading module...\n");
	modMusic = Mix_LoadMUS(modFile);

	if(!modMusic)
	{
		SDLError("Loading mod file");
		return 0;
	}
	Log("Mod loaded\n");
	if(Mix_PlayMusic(modMusic, 1) == -1)
	{
		SDLError("Playing mod file");
		return 0;
	}
	Log("Mod playing\n");
	Player_TogglePause();
	mod = Player_GetModule();
	mod->loop = 0; // don't want to keep looping forever!

	volThreshold = CfgI("main.volumethreshold");
	// if the configuration option doesn't exist, volThreshold = 0
	// which is a valid threshold

	Log("Loading track data...\n");
	LoadTrackData();
	Log("Module ready.\n");

	if(CfgEq("sound.interpolation", "yes"))
		md_mode |= DMODE_INTERP;
	else
		md_mode &= !DMODE_INTERP;
	return 1;
}

void StopModule()
{
	int x, y;
	if(!modMusic) return;
	Log("Freeing note mem\n");
	printf("Freeing note mem\n");
	for(x=0;x<numRows;x++)
	{
		for(y=0;y<numChannels;y++)
		{
			if(rowData[x].chans[y].numModChannels) free(rowData[x].chans[y].modChannels);
		}
		free(rowData[x].chans);
	}
	printf("Freeing rowdata\n");
	Log("Freeing rowdata\n");
	free(rowData);
	numRows = 0;
	numTics = 0;
	rowData = NULL;
	Log("Freeing Mod music\n");
	Mix_FreeMusic(modMusic);
	Log("Module stopped\n");
}
