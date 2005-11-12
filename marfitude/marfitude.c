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

#include <math.h>
#include <string.h>

#include "marfitude.h"
#include "objs/bluenotes.h"
#include "objs/explode.h"
#include "objs/fireball.h"
#include "objs/greynotes.h"
#include "objs/laser.h"
#include "objs/lines.h"
#include "objs/rows.h"
#include "objs/scoreboard.h"
#include "objs/targets.h"
#include "objs/view.h"

#include "SDL_opengl.h"

#include "gmae/audio.h"
#include "gmae/cfg.h"
#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/input.h"
#include "gmae/log.h"
#include "gmae/mainscene.h"
#include "gmae/menu.h"
#include "gmae/module.h"
#include "gmae/particles.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#include "util/memtest.h"
#include "util/myrand.h"
#include "util/plugin.h"
#include "util/slist.h"

/** @file
 * The main marfitude source. Process the music and wam file in a super-fun
 * way.
 */

static Uint32 ticTime;
static struct wam *wam; /* note file */
static int difficulty = 0;

/** returns 1 if the row is valid, 0 otherwise */
#define IsValidRow(row) ((row >= 0 && row < wam->num_rows) ? 1 : 0)
/** Find the max of a and b. Usual macro warnings apply */
#define Max(a, b) ((a) > (b) ? (a) : (b))

/** Iterate through each player, for internal marfitude use. Essentially a
 * non-const version of marfitude_foreach_player.
 */
#define foreach_player(ps) for(ps=get_player(NULL); ps != NULL; ps=get_player(ps))

static int curTic; /* tick counter from 0 - total ticks in the song */
static int firstVb, curVb, lastVb;     /* tick counters for the three rows */
static int firstRow, rowIndex, lastRow;  /* first row on screen, current row
                                   * playing and the last row on screen
                                   */
static struct row *curRow;
static double modTime;
static double partialTic;

static void MoveBack(void);
static void ResetCol(void);
static void ResetAp(void);
static void ChannelUp(int);
static void ChannelDown(int);
static struct column *ColumnFromNum(int col);
static void Setmute(struct column *c, int mute);
static void UpdateModule(void);
static void AddNotes(int row);
static struct slist *RemoveList(struct slist *list, int tic);
static void RemoveNotes(int row);

static void Press(int button, int player);
static void MoveHitNotes(int tic, int col);
static void UpdateClearedCols(void);
static void UpdatePosition(void);
static int NearRow(void);
static struct marfitude_note *FindNote(struct slist *list, int tic, int col);
static struct marfitude_player *get_player(struct marfitude_player *player);
static int get_clear_column(int start, int skip_lines);
static void FixVb(int *vb, int *row);
static void TickHandler(void);
static void reinitializer(const void *);
static void CheckMissedNotes(void);
static void CheckColumn(int row);
static int NoteListTic(const void *snp, const void *tp);
static int SortByTic(const void *a, const void *b);
static void menu_handler(const void *data);
static void button_handler(const void *data);
static void leaver(const void *data);

static struct marfitude_attack_col ac[MAX_COLS];
static struct marfitude_note *notesOnScreen; /* little ring buffer of notes */
static struct slist *unusedList;	/* unused notes */
static struct slist *notesList;	/* notes on the screen, not hit */
static struct slist *hitList;	/* notes on the screen, hit */
static int numNotes;	/* max number of notes on screen (wam->num_cols * NUM_TICKS) */
static char *cursong;
static int highscore; /* Previous high score */
static int local_high; /* Current high score */

/* Array of players. Default player 1 to active. Yeah, kinda ugly. */
static struct marfitude_player ps[MAX_PLAYERS] = {
	{{0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, 1, 0, 0},
	{{0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 1},
	{{0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 2},
	{{0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 3}
};
static struct marfitude_player *curp; /* The current player */

static void *plugin = NULL;
static int *noteOffset;
static MikMod_player_t oldHand;
static int tickCounter;
static double timerCounter;
static int songStarted;	/* this is set once we get to the first row, and the
			 * song is unpaused
			 */

void menu_handler(const void *data)
{
	const struct menu_e *m = data;

	if(m->active ^ Player_Paused()) {
		Player_TogglePause();
	}
}

void button_handler(const void *data)
{
	static int lastkeypressed[MAX_PLAYERS][2] = {
		{1, 1},
		{1, 1},
		{1, 1},
		{1, 1}
	};
	const struct button_e *b = data;
	int p;

	curp = &ps[b->player];
	p = b->player;

	/* Activate the player if they're not yet in the game */
	if(b->button == B_MENU && curp->active == 0) {
		curp->active = 1;
		MoveBack();
		ResetAp();
		return;
	}

	if(is_menu_active()) return;

	switch(b->button) {
		case B_RIGHT:
			ChannelUp(b->shift);
			break;
		case B_LEFT:
			ChannelDown(b->shift);
			break;
		case B_BUTTON1:
			if(b->shift) {
				lastkeypressed[p][0] = 1;
				lastkeypressed[p][1] = 1;
				Press(1, p);
				Press(1, p);
			} else {
				lastkeypressed[p][0] = lastkeypressed[p][1];
				lastkeypressed[p][1] = 1;
				Press(1, p);
			}
			break;
		case B_BUTTON2:
			if(b->shift) {
				lastkeypressed[p][0] = 2;
				lastkeypressed[p][1] = 2;
				Press(2, p);
				Press(2, p);
			} else {
				lastkeypressed[p][0] = lastkeypressed[p][1];
				lastkeypressed[p][1] = 2;
				Press(2, p);
			}
			break;
		case B_BUTTON3:
			/* yes, this is really 4 (3rd bit) */
			if(b->shift) {
				lastkeypressed[p][0] = 4;
				lastkeypressed[p][1] = 4;
				Press(4, p);
				Press(4, p);
			} else {
				lastkeypressed[p][0] = lastkeypressed[p][1];
				lastkeypressed[p][1] = 4;
				Press(4, p);
			}
			break;
		case B_BUTTON4:
			if(b->shift) {
				Press(lastkeypressed[p][0], p);
				Press(lastkeypressed[p][1], p);
			} else {
				lastkeypressed[p][0] = lastkeypressed[p][1];
				Press(lastkeypressed[p][1], p);
			}
			break;
		case B_MENU:
			show_menu(p);
			break;
		case B_UP:
		case B_DOWN:
		default:
			break;
	}
}

void leaver(const void *data)
{
	int p = *((const int*)data);
	ps[p].active = 0;
	ac[ps[p].channel].ps = slist_remove(ac[ps[p].channel].ps, &ps[p]);
}

int main_init()
{
	int x;
	int p;
	const char *scene;

	Log(("Load Wam\n"));
	cursong = cfg_copy("main", "song", "null");

	wam = load_wam(cursong);
	if(wam == NULL) {
		ELog(("Error: Couldn't load WAM file\n"));
		return 1;
	}
	Log(("Start module\n"));
	if(start_module(cursong)) {
		ELog(("Error: Couldn't start module\n"));
		return 2;
	}

	/* module and module data (where to place the notes) are now loaded,
	 * and the module is paused
	 */
	Log(("Module ready\n"));

	difficulty = cfg_get_int("main", "difficulty", 1);
	if(difficulty < 0)
		difficulty = 0;
	if(difficulty > 3)
		difficulty = 3;
	highscore = cfg_get_int("highscore", cursong, 0);

	tickCounter = 0;
	timerCounter = 0.0;
	songStarted = 0;
	modTime = -5.0;
	oldHand = MikMod_RegisterPlayer(TickHandler);
	noteOffset = malloc(sizeof(int) * (MAX_NOTE+1));

	noteOffset[1] = -1;
	noteOffset[2] = 0;
	noteOffset[4] = 1;

	curRow = wam_row(wam, 0);

	ticTime = 0;

	if(marfitude_get_player(NULL) == NULL) {
		p = 0;
	} else {
		p = marfitude_get_player(NULL)->num;
	}
	for(x=0;x<wam->num_cols;x++) {
		int y;
		ac[x].part = 0.0;
		ac[x].cleared = 0;
		for(y=0; y<LINES_PER_AP*(x / marfitude_num_players()); y++) {
			ac[x].cleared++;
			while(ac[x].cleared < wam->num_rows && wam->row_data[ac[x].cleared].line == 0)
				ac[x].cleared++;
		}
		ac[x].minRow = ac[x].cleared;
		ac[x].hit = -1;
		ac[x].miss = -2;
		ac[x].player = p;
		ac[x].ps = NULL;
	}

	/* start back about 3.5 seconds worth of ticks
	 * doesn't need to be exact, we just need some time for
	 * the player to get ready
	 */
	curTic = (int)(-3500.0 * (double)wam->row_data[0].bpm / 2500.0);
	/* set the tic positions of where we can see and where we're looking */
	firstVb = curTic - NEGATIVE_TICKS;
	curVb = curTic;
	lastVb = curTic + POSITIVE_TICKS;
	/* convert tic values to rows, now the ticks are all within */
	/* [0 .. wam->row_data[xRow].sngspd - 1] */
	FixVb(&firstVb, &firstRow);
	FixVb(&curVb, &rowIndex);
	FixVb(&lastVb, &lastRow);

	partialTic = 0.0;

	numNotes = wam->num_cols * NUM_TICKS;
	notesOnScreen = malloc(sizeof(struct marfitude_note) * numNotes);
	unusedList = NULL;
	notesList = NULL;
	hitList = NULL;
	for(x=0;x<numNotes;x++) {
		unusedList = slist_append(unusedList, (void *)&notesOnScreen[x]);
		notesOnScreen[x].ins = __LINE__;
	}
	for(x=0;x<=lastRow;x++) AddNotes(x);

	register_event("sound re-init", reinitializer);
	register_event("button", button_handler);
	register_event("leave", leaver);

	scene = cfg_get("main", "scene", "scenes/default");
	if(strcmp(scene, "scenes/default") == 0) {
		bluenotes_init();
		explode_init();
		fireball_init();
		greynotes_init();
		laser_init();
		lines_init();
		rows_init();
		scoreboard_init();
		targets_init();
		view_init();
		plugin = NULL;
	} else {
		plugin = load_plugin(scene);
	}

	local_high = 0;
	x = 0;
	for(p=0; p<MAX_PLAYERS; p++) {
		curp = &ps[p];
		curp->score.score = 0;
		curp->score.multiplier = 1;
		curp->channel = x % wam->num_rows;
		curp->ap.nextStartRow = -1;
		if(curp->active) {
			x++;
			MoveBack();
			ResetAp();
		}
	}

	for(x=0;x<=lastRow;x++) fire_event("row", &wam->row_data[x]);

	init_timer();
	Log(("Lists created\n"));
	return 0;
}

void main_quit(void)
{
	int i;
	struct marfitude_player *p;

	Log(("Main Scene quit\n"));
	foreach_player(p) {
		if(p->score.score > highscore) {
			cfg_set_int("highscore", cursong, p->score.score);
			highscore = p->score.score;
		}
	}
	for(i=0; i<wam->num_cols; i++) {
		slist_free(ac[i].ps);
	}
	if(plugin == NULL) {
		view_exit();
		targets_exit();
		scoreboard_exit();
		rows_exit();
		lines_exit();
		laser_exit();
		greynotes_exit();
		fireball_exit();
		explode_exit();
		bluenotes_exit();
	} else {
		free_plugin(plugin);
	}
	free(cursong);
	oldHand = MikMod_RegisterPlayer(oldHand);
	Log(("A\n"));
	if(songStarted) {
		deregister_event("menu", menu_handler);
	}
	Log(("A\n"));
	songStarted = 0;
	deregister_event("leave", leaver);
	deregister_event("button", button_handler);
	deregister_event("sound re-init", reinitializer);
	Log(("A\n"));
	free(notesOnScreen);
	Log(("A\n"));
	free(noteOffset);
	Log(("A\n"));
	clear_particles();
	Log(("A\n"));
	check_objs();
	Log(("A\n"));
	free_wam(wam);
	Log(("A\n"));
	stop_module();
	slist_free(hitList);
	slist_free(notesList);
	slist_free(unusedList);
	Log(("Main scene quit finished\n"));
}

void main_scene(void)
{
	Log(("main_scene\n"));

	/* Set key repeating here in case the configuration changes mid-game */
	set_key_repeat(B_BUTTON1, 0);
	set_key_repeat(B_BUTTON2, 0);
	set_key_repeat(B_BUTTON3, 0);
	set_key_repeat(B_BUTTON4, 0);

	if(rowIndex != wam->num_rows) UpdatePosition();
	update_objs(timeDiff);
	curp = &ps[0];

	fire_event("set view", NULL);

	fire_event("draw opaque", NULL);

	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);
	fire_event("draw transparent", NULL);

	/* Hehe, this should help when I screw it up again in the future :) */
	if(glIsEnabled(GL_LIGHTING)) {
		ELog(("Someone left the lights on in transparency mode!\n"));
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	draw_particles();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(GL_TRUE);
	glEnable(GL_LIGHTING);

	set_ortho_projection();
	fire_event("draw ortho", NULL);
	reset_projection();

	Log(("endMainScene\n"));
}

void ChannelUp(int shift)
{
	if(curp->channel + 1 < wam->num_cols) {
		ac[curp->channel].ps = slist_remove(ac[curp->channel].ps, curp);
		if(shift)
			curp->channel = wam->num_cols - 1;
		else
			curp->channel++;
		ac[curp->channel].ps = slist_append(ac[curp->channel].ps, curp);
		if(curp->ap.notesHit > 0) curp->score.multiplier = 1;
		ResetAp();
		if(ac[curp->channel].ps && ac[curp->channel].ps->data == curp)
			ac[curp->channel].hit = curp->ap.startTic - 1;
	}
}

void ChannelDown(int shift)
{
	if(curp->channel != 0) {
		ac[curp->channel].ps = slist_remove(ac[curp->channel].ps, curp);
		if(shift)
			curp->channel = 0;
		else
			curp->channel--;
		if(curp->ap.notesHit > 0) curp->score.multiplier = 1;
		ac[curp->channel].ps = slist_append(ac[curp->channel].ps, curp);
		ResetAp();
		if(ac[curp->channel].ps && ac[curp->channel].ps->data == curp)
			ac[curp->channel].hit = curp->ap.startTic - 1;
	}
}

struct column *ColumnFromNum(int col)
{
	return &wam->patterns[curRow->patnum].columns[col];
}

void Setmute(struct column *c, int mute)
{
	int x;
	for(x=0;x<c->numchn;x++) {
		if(mute) Player_Mute(c->chan[x]);
		else Player_Unmute(c->chan[x]);
	}
}

/* update which channels are playing based on marfitude_attack_col contents */
void UpdateModule(void)
{
	int x;
	for(x=0;x<wam->num_cols;x++) {
		if(ac[x].cleared || ac[x].hit > ac[x].miss)
			Setmute(ColumnFromNum(x), UNMUTE);
		else
			Setmute(ColumnFromNum(x), MUTE);
	}
	Setmute(&wam->patterns[curRow->patnum].unplayed, UNMUTE);
}

void AddNotes(int row)
{
	int x;
	int tic;
	struct marfitude_note *sn;
	if(row < 0 || row >= wam->num_rows) return;
	tic = wam->row_data[row].ticpos;
	for(x=0;x<wam->num_cols;x++) {
		if(marfitude_get_note(row, x)) {
			sn = unusedList->data;
			if(row < ac[x].minRow) {
				hitList = slist_append(hitList, sn);
				sn->ins = __LINE__;
			} else {
				notesList = slist_append(notesList, sn);
				sn->ins = __LINE__;
			}
			unusedList = slist_remove(unusedList, sn);
			marfitude_get_notepos(&sn->pos, row, x);
			sn->tic = tic;
			sn->time = wam->row_data[row].time;
			sn->col = x;
			sn->difficulty = wam->row_data[row].difficulty[x];
		}
	}
}

struct slist *RemoveList(struct slist *list, int tic)
{
	struct marfitude_note *sn;
	while(list) {
		sn = (struct marfitude_note*)list->data;
		if(sn->tic != tic) break;
		unusedList = slist_append(unusedList, list->data);
		list = slist_remove(list, list->data);
		sn->ins = __LINE__;
	}
	return list;
}

void RemoveNotes(int row)
{
	int tic;
	if(row < 0 || row >= wam->num_rows) return;
	tic = wam->row_data[row].ticpos;
	Log(("RM\n"));
	notesList = RemoveList(notesList, tic);
	Log(("RMHit: %i\n", hitList));
	hitList = RemoveList(hitList, tic);
	Log(("done: %i\n", hitList));
}

struct marfitude_note *FindNote(struct slist *list, int tic, int col)
{
	struct marfitude_note *sn;
	while(list) {
		sn = (struct marfitude_note*)list->data;
		if(sn->tic == tic && sn->col == col) return sn;
		list = slist_next(list);
	}
	return NULL;
}

/** Convert the column @a start plus @a lines line columns to the next line'd
 * column. Also make sure there is enough space at the end for another attack
 * pattern.
 */
int get_clear_column(int start, int skip_lines)
{
	int lines = 0;
	int tmp;

	while(start < wam->num_rows && skip_lines > 0) {
		if(wam->row_data[start].line != 0)
			skip_lines--;
		start++;
	}
	/* clear to a line break */
	while(start < wam->num_rows && wam->row_data[start].line == 0)
		start++;
	tmp = start;
	while(tmp < wam->num_rows && lines < LINES_PER_AP) {
		if(wam->row_data[tmp].line != 0)
			lines++;
		tmp++;
	}

	/* Not enough space to start a new AP, set to the end of the song */
	if(lines < LINES_PER_AP)
		start = wam->num_rows;
	return start;
}

void Press(int button, int player)
{
	int i;
	int noteHit = 0;
	int rowStart;
	int rowStop;
	struct marfitude_note *sn;
	struct row *r;
	struct button_e b;

	b.button = button;
	b.player = player;
	b.shift = 0;
	fire_event("shoot", &b);

	if(!curp->ap.active) return;

	rowStart = rowIndex;
	while(rowStart > 0 && curRow->time - wam_row(wam, rowStart)->time < MARFITUDE_TIME_ERROR)
		rowStart--;

	rowStop = rowIndex;
	while(rowStop < wam->num_rows && wam_row(wam, rowStop)->time - curRow->time < MARFITUDE_TIME_ERROR)
		rowStop++;

	for(i=rowStart;i<=rowStop;i++) {
		if(i >= 0 && i < wam->num_rows) {
			r = wam_row(wam, i);
			/* if we're in the attackpattern limits, and
			 * if we didn't already play this row, and this row 
			 * is within our acceptable error, and we hit the
			 * right note, and it's within the difficulty range,
			 * then yay
			 */
			if(	
				r->ticpos >= curp->ap.startTic &&
				r->ticpos < curp->ap.stopTic &&
				r->ticpos > curp->ap.lastTic &&
				fabs(r->time - modTime) <= MARFITUDE_TIME_ERROR &&
				button == r->notes[curp->channel] &&
				r->difficulty[curp->channel] <= difficulty) {

				sn = FindNote(notesList, r->ticpos, curp->channel);
				if(!sn) {
					ELog(("Error: Struck note not found!\n"));
					sn = FindNote(hitList, r->ticpos, curp->channel);
					if(sn) {
						ELog(("note found in hitList: %i\n", sn->ins));
					}
					sn = FindNote(unusedList, r->ticpos, curp->channel);
					if(sn) {
						ELog(("note found in unusedList: %i\n", sn->ins));
					}
					break;
				}
				fire_event("note shot", sn);
				notesList = slist_remove(notesList, (void *)sn);
				unusedList = slist_append(unusedList, (void *)sn);
				sn->ins = __LINE__;
				curp->ap.lastTic = r->ticpos;
				curp->ap.notesHit++;
				if(curp->ap.notesHit == curp->ap.notesTotal) {
					curp->ap.nextStartRow = curp->ap.stopRow;
					ac[curp->channel].cleared = get_clear_column(curp->ap.stopRow, LINES_PER_AP * wam->num_cols / marfitude_num_players());
					ac[curp->channel].minRow = rowIndex;
					ac[curp->channel].part = 0.0;
					ac[curp->channel].player = curp->num;
					curp->score.score += curp->ap.notesHit * curp->score.multiplier;
					if(curp->score.score > local_high)
						local_high = curp->score.score;
					if(curp->score.multiplier < 8)
						curp->score.multiplier++;
					curp->ap.notesHit = 0;
				}
				ac[curp->channel].hit = r->ticpos;
				noteHit = 1;
				r->notes[curp->channel] = 0;
				break;
			}
		}
	}

	if(!noteHit && curRow->ticpos >= curp->ap.startTic && curRow->ticpos < curp->ap.stopTic) {
		/* oops, we missed! */
		if(curp->ap.notesHit > 0) {
			curp->score.multiplier = 1;
			MoveBack();
		}
		if(IsValidRow(rowIndex) && curRow->ticpos >= curp->ap.startTic && curRow->ticpos < curp->ap.stopTic)
			ac[curp->channel].miss = curRow->ticpos;
		ResetCol();
	}
	UpdateModule();
}

void FixVb(int *vb, int *row)
{
	*row = 0;
	while(1) {
		if(*row < 0 && *vb >= wam->row_data[0].sngspd) {
			(*row)++;
			*vb -= wam->row_data[0].sngspd;
		} else if(*row >= 0 && *vb >= wam->row_data[*row].sngspd) {
			(*row)++;
			*vb -= wam->row_data[*row].sngspd;
		} else if(*vb < 0) {
			(*row)--;
			*vb += wam->row_data[0].sngspd;
		}
		else break;
	}
}

/* keep track of the number of actual ticks played by the mod player,
 * so we know if we're off
 */
void TickHandler(void)
{
	if(songStarted) {
		if(!Player_Paused()) {
			tickCounter++;
			timerCounter += 2.5 / curRow->bpm;
		}
	}
	oldHand();
}

/* Reinitialize the module */
void reinitializer(const void *data)
{
	Log(("Reinit: Start module\n"));
	if(data) {}
	if(start_module(cursong)) {
		ELog(("Error (reinit): Couldn't start module\n"));
		return;
	}
	seek_module(tickCounter);
}

/* gets either rowIndex or rowIndex+1, depending on which one row is closest
 * based on the time (eg within acceptable error)
 */
int NearRow(void)
{
	if(modTime - curRow->time < MARFITUDE_TIME_ERROR)
		return rowIndex;
	return rowIndex+1;
}

void MoveBack(void)
{
	/* Remove from the list and append to the end, so curp is at the back
	 * of the queue.
	 */
	ac[curp->channel].ps = slist_remove(ac[curp->channel].ps, curp);
	ac[curp->channel].ps = slist_append(ac[curp->channel].ps, curp);
}

void ResetCol(void)
{
	struct marfitude_player *old_curp = curp;
	struct slist *t;

	slist_foreach(t, ac[curp->channel].ps) {
		curp = t->data;
		ResetAp();
	}

	curp = old_curp;
}

void ResetAp(void)
{
	int p;
	int start;
	int end;
	int apLines = 0;

	for(p=0; p<MAX_PLAYERS; p++)
		ps[p].ap.active = 0;
	for(p=0; p<wam->num_cols; p++)
		if(ac[p].ps)
			((struct marfitude_player*)ac[p].ps->data)->ap.active = 1;

	curp->ap.notesHit = 0;
	curp->ap.notesTotal = 0;

	start = wam_rowindex(wam, Max(Max(NearRow(), curp->ap.nextStartRow), ac[curp->channel].cleared));
	while(start < wam->num_rows && (wam->row_data[start].line == 0 || wam->row_data[start].ticpos <= ac[curp->channel].miss))
		start++;

	end = start;

	/* Make sure there are at least LINES_PER_AP lines in the AP */
	while(apLines < LINES_PER_AP && end < wam->num_rows) {
		/* don't count the note on the last row, since that will
		 * be the beginning of the next "AttackPattern"
		 */
		if(marfitude_get_note(end, curp->channel))
			curp->ap.notesTotal++;
		end++;
		if(wam->row_data[end].line != 0) apLines++;
	}

	/* Make sure there is at least one note in the AP */
	while(curp->ap.notesTotal == 0 && end < wam->num_rows) {
		if(marfitude_get_note(end, curp->channel))
			curp->ap.notesTotal++;
		end++;
	}

	/* Make sure the AP ends on a line */
	while(wam->row_data[end].line == 0 && end < wam->num_rows) {
		if(marfitude_get_note(end, curp->channel))
			curp->ap.notesTotal++;
		end++;
	}

	if(curp->ap.notesTotal == 0) {
		/* at the end of the song we can't find a valid pattern to do */
		curp->ap.startTic = -1;
		curp->ap.stopTic = -1;
		curp->ap.lastTic = -1;
		curp->ap.startRow = -1;
		curp->ap.stopRow = -1;
		curp->ap.active = 0;
	} else {
		Log(("StarT: %i, End: %i\n", start, end));
		if(end == wam->num_rows)
			curp->ap.stopTic = wam->num_tics;
		else
			curp->ap.stopTic = wam->row_data[end].ticpos;
		curp->ap.startTic = wam->row_data[start].ticpos;
		curp->ap.lastTic = wam->row_data[start].ticpos - 1;
		curp->ap.startRow = start;
		curp->ap.stopRow = end;
	}
	fire_event("reset AP", curp);
	return;
}

void CheckMissedNotes(void)
{
	struct marfitude_note *sn;
	struct slist *list;

	list = notesList;
	while(list) {
		sn = (struct marfitude_note*)list->data;
		if(modTime - sn->time > MARFITUDE_TIME_ERROR && sn->tic > ac[sn->col].miss) {
			ac[sn->col].miss = sn->tic;
			foreach_player(curp) {
				if(ac[sn->col].ps && ac[sn->col].ps->data == curp) {
					if(sn->col == curp->channel && sn->tic >= curp->ap.startTic && sn->tic < curp->ap.stopTic) {
						if(curp->ap.notesHit > 0) {
							curp->score.multiplier = 1;
							MoveBack();
						}
						ResetCol();
					}
					break;
				}
			}
		}
		list = slist_next(list);
	}
}

void CheckColumn(int row)
{
	int x;
	for(x=0;x<wam->num_cols;x++) {
		if(ac[x].cleared == row) ac[x].cleared = 0;
	}
}

int NoteListTic(const void *snp, const void *tp)
{
	int tic = (int)tp;
	const struct marfitude_note *sn = (const struct marfitude_note*)snp;
	return sn->tic - tic;
}

int SortByTic(const void *a, const void *b)
{
	const struct marfitude_note *an = (const struct marfitude_note*)a;
	const struct marfitude_note *bn = (const struct marfitude_note*)b;
	return an->tic - bn->tic;
}

void MoveHitNotes(int tic, int col)
{
	struct marfitude_note *sn;
	struct slist *tmp;
	struct slist *holder = NULL;
	Log(("moveHIt\n"));
	tmp = slist_find_custom(notesList, (void *)tic, NoteListTic);
	while(tmp) {
		sn = (struct marfitude_note*)tmp->data;

		if(sn->tic == tic && sn->col == col) {
			fire_event("note explosion", sn);
			hitList = slist_insert_sorted(hitList, sn, SortByTic);
			holder = slist_append(holder, sn);
			sn->ins = __LINE__;
		}
		tmp = slist_next(tmp);
	}
	tmp = holder;
	while(tmp) {
		notesList = slist_remove(notesList, tmp->data);
		tmp = slist_next(tmp);
	}
	slist_free(holder);
	Log(("movehit\n"));
}

void UpdateClearedCols(void)
{
	int x;
	int tic;
	struct row *r;

	for(x=0;x<wam->num_cols;x++) {
		r = wam_row(wam, ac[x].minRow);
		/* Yes I realize this is a bunch of magic numbers. Sue me. */
		ac[x].part += (double)ticDiff * (double)r->bpm / (833.0 * (double)r->sngspd);
		while(ac[x].part >= 1.0 && ac[x].minRow < ac[x].cleared && ac[x].minRow < wam->num_rows) {
			struct marfitude_pos p;

			tic = r->ticpos;
			MoveHitNotes(tic, x);

			p.modtime = modTime;
			p.tic = tic;
			p.row_index = wam_rowindex(wam, ac[x].minRow);
			p.channel =  x;
			fire_event("row explosion", &p);

			ac[x].part -= 1.0;
			ac[x].minRow++;
			r++;
		}
	}
}

struct marfitude_player *get_player(struct marfitude_player *player)
{
	int i = 0;
	if(player != NULL) {
		for(i=0; i<MAX_PLAYERS; i++) {
			if(player == &ps[i]) {
				i++;
				break;
			}
		}
	}
	for(; i<MAX_PLAYERS; i++) {
		if(ps[i].active)
			return &ps[i];
	}
	return NULL;
}

void UpdatePosition(void)
{
	/* calculate the amount of ticTime elapsed
	 * every 2500 ticTime is one tick
	 */
	if(!is_menu_active()) ticTime += ticDiff * curRow->bpm;

	/* adjust the ticTime if our time is different from the
	 * song time.  This is needed in case a little blip in the process
	 * causes the song to be ahead or behind the game, so we can
	 * right ourselves.
	 */
	if(songStarted) {
		int tmpAdj;
		double tdiff;

		tdiff = timerCounter - modTime - audio_delay();
		/* If the current modtime is not in between the timer counter
		 * and the timer at the next tic, then make some adjustments.
		 */
		if(tdiff > 0.0) {
			tmpAdj = (int)(tdiff * 1024.0);
		} else if(tdiff < -2.5 / curRow->bpm) {
			tmpAdj = (int)((tdiff + 2.5 / curRow->bpm) * 1024.0);
		} else {
			tmpAdj = 0;
		}
		Log(("Adj: %i\n", tmpAdj));

		/* Make sure we don't actually go back rows, as that would
		 * be massively confusing (for my code, not necessarily the
		 * player). If we get too far ahead of the song somehow, the
		 * game will just pause until it catches up.
		 */
		if((signed)ticTime + tmpAdj < 0) ticTime = 0;
		else ticTime += tmpAdj;
	}

	while(ticTime >= 2500) {
		struct row *r;

		ticTime -= 2500;
		curTic++;
		curVb++;
		firstVb++;
		lastVb++;
		CheckMissedNotes();
		if(curVb >= curRow->sngspd) {
			curVb -= curRow->sngspd;
			rowIndex++;
			curRow = wam_row(wam, rowIndex);
			foreach_player(curp) {
				if(curp->ap.active && rowIndex > curp->ap.stopRow) {
					MoveBack();
					ResetCol();
				}
			}
			CheckColumn(wam_rowindex(wam, rowIndex));
			if(rowIndex == wam->num_rows) {
				fire_event("victory", &local_high);
			}
		}
		modTime = curRow->time + (curTic - curRow->ticpos) * BpmToSec(curRow->sngspd, curRow->bpm) / curRow->sngspd;
		if(!songStarted && modTime + audio_delay() >= 0.0) {
			/* start the song! */
			Player_TogglePause();
			songStarted = 1;
			register_event("menu", menu_handler);
		}
		UpdateModule();

		r = wam_row(wam, firstRow);
		if(firstVb >= r->sngspd) {
			firstVb -= r->sngspd;
			/* the remove functions check to make sure 
			 * the row is valid, so it's ok to pass firstRow
			 */
			RemoveNotes(firstRow);
			if(firstRow >= 0 && firstRow < wam->num_rows)
				fire_event("de-row", &wam->row_data[firstRow]);
			firstRow++;
		}

		r = wam_row(wam, lastRow);
		if(lastVb >= r->sngspd) {
			lastVb -= r->sngspd;
			lastRow++;
			/* the add functions check to make sure the
			 * row is valid, so it's ok to pass lastRow
			 */
			AddNotes(lastRow);
			if(lastRow >= 0 && lastRow < wam->num_rows)
				fire_event("row", &wam->row_data[lastRow]);
		}
	}
	UpdateClearedCols();
	partialTic = (double)ticTime / 2500.0;
}

/** Gets the wam structure for the current song.
 * @return The wam struct.
 */
const struct wam *marfitude_get_wam(void)
{
	return wam;
}

/** Gets the note offsets, size MAX_NOTE+1 */
const int *marfitude_get_offsets(void)
{
	return noteOffset;
}

/** Gets the current score structure and stores it in @a s */
const struct marfitude_score *marfitude_get_score(int player)
{
	return &ps[player].score;
}

/** Gets the previous high score of the current song */
int marfitude_get_highscore(void)
{
	return highscore;
}

/** Gets the current high score among all players */
int marfitude_get_local_highscore(void)
{
	return local_high;
}

/** Gets the number of active players */
int marfitude_num_players(void)
{
	int i;
	int x = 0;
	for(i=0; i<MAX_PLAYERS; i++) {
		if(ps[i].active)
			x++;
	}
	return x;
}

/** Gets the current difficulty */
int marfitude_get_difficulty(void)
{
	return difficulty;
}

/** Returns a slist of the notes to play */
const struct slist *marfitude_get_notes(void)
{
	return notesList;
}

/** Returns a slist of the exploded notes */
const struct slist *marfitude_get_hitnotes(void)
{
	return hitList;
}

/** Returns an array of attack columns. Size is MAX_COLS, only wam->num_cols
 * are populated with stuff.
 */
const struct marfitude_attack_col *marfitude_get_ac(void)
{
	return ac;
}

/** Gets the current attack pattern. */
const struct marfitude_attack_pat *marfitude_get_ap(int player)
{
	return &ps[player].ap;
}

/** Gets the next player. If @a player is NULL, gets the first player.
 * If @a player is the last player, then returns NULL.
 */
const struct marfitude_player *marfitude_get_player(const struct marfitude_player *player)
{
	int i = 0;
	if(player != NULL) {
		for(i=0; i<MAX_PLAYERS; i++) {
			if(player == &ps[i]) {
				i++;
				break;
			}
		}
	}
	for(; i<MAX_PLAYERS; i++) {
		if(ps[i].active)
			return &ps[i];
	}
	return NULL;
}

/** Gets the current module position information, and store it in @a p */
void marfitude_get_pos(struct marfitude_pos *p)
{
	p->modtime = modTime;
	p->tic = (double)curTic + partialTic;
	p->row_index = rowIndex;
	p->channel = curp->channel;
}

/** Gets the note at the specified @a row, @a col. If there is no note, 0 is
 * returned. Otherwise, 1, 2, or 4 is returned depending on what the note is.
 */
int marfitude_get_note(int row, int col)
{
	if(wam->row_data[row].notes[col] &&
			wam->row_data[row].difficulty[col] <= difficulty)
		return wam->row_data[row].notes[col];
	return 0;
}

/** Fill the destination vector @a dest with the coordinates of the note at
 * @a row, @a col. If there is no note at that position, @a dest is unmodified.
 * You should probably make sure there is a note there first by using
 * marfitude_get_note();
 */
void marfitude_get_notepos(union vector *dest, int row, int col)
{
	dest->v[0] = -col * BLOCK_WIDTH - NOTE_WIDTH * (double)noteOffset[(int)wam->row_data[row].notes[col]];
	dest->v[1] = 0.0;
	dest->v[2] = TIC_HEIGHT * (double)wam->row_data[row].ticpos;
}
