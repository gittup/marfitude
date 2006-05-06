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

#include "util/math/vector.h"

/** @file
 * Provide access to marfitude's phatty boom beat-matching capabilities.
 */

#define TIC_HEIGHT .25   /**< Height (into the screen) of a tic */
#define BLOCK_WIDTH 2.0  /**< Width (left/right) of a block */
#define NOTE_WIDTH .75   /**< Width positioning (left/right) of a note */

#define NEGATIVE_TICKS 7*6  /**< How many ticks before the player to display */
#define POSITIVE_TICKS 57*6 /**< How many ticks after the player to display */
#define NUM_TICKS 64*6      /**< The size of POSITIVE_TICKS + NEGATIVE_TICKS */
#define LINES_PER_AP 8      /**< How many lines are in an attack pattern */

#define UNMUTE 0 /**< Unmute a channel */
#define MUTE 1   /**< Mute a channel */

#define MARFITUDE_TIME_ERROR 0.1 /**< How far off in seconds the player can miss
				   * the note. The total time the player has to
				   * play a note is 2 * MARFITUDE_TIME_ERROR
				   */

/** A structure of score information */
struct marfitude_score {
	int score;        /**< The player's current score */
	int multiplier;   /**< The player's current multiplier */
};

/** A structure of time/position information */
struct marfitude_pos {
	double modtime;        /**< Time in seconds */
	double tic;            /**< Current tic position */
	int row_index;         /**< Row index */
	int channel;           /**< Which channel is currently played */
};

/** A note on the screen */
struct marfitude_note {
	union vector pos; /**< The coordinates of where to draw the note */
	int tic;          /**< The mod tic this note is on */
	double time;      /**< The time this note is on */
	int col;          /**< The column the note is in */
	int difficulty;   /**< The difficulty of this note */
	int ins;          /**< DEBUG - the line in the src where this note was
			   * added
			   */
};

/** Keep track of clearing information for each column */
struct marfitude_attack_col {
	double part;	  /**< cumulative row adder, when >= 1.0 inc minRow */
	int minRow;	  /**< equal to cleared, but doesn't get set to 0
			   * after the column is recreated
			   */
	int cleared;	  /**< equals the last row this col is cleared to, 0 if
			   * not cleared
			   */
	int hit;	  /**< equals the tic of the last hit note */
	int miss;	  /**< equals the tic of the last missed note */
	int player;       /**< The player who last cleared this AC */
	struct slist *ps; /**< The players on this AC */
};

/** Keep track of information needed for the column that is being played */
struct marfitude_attack_pat {
	int realStartTic; /**< The "real" start. Any space between the "real"
			   * start and the start has no effect on the game.
			   * Resetting the AP between realStart and start will
			   * keep the same start/stop, and only update
			   * realStart.
			   */
	int startTic;     /**< first tic that we need to play */
	int stopTic;      /**< last tic that we need to play */
	int realStartRow; /**< corresponding row to realStartTic */
	int startRow;     /**< corresponding row to startTic */
	int stopRow;      /**< corresponding row to stopTic */
	int nextStartRow; /**< which row the game is cleared to. */
	int lastTic;      /**< last note played is in lastTic */
	int notesHit;     /**< number of notes we hit so far */
	int notesTotal;   /**< total number of notes we need to play */
	int active;       /**< 1 means we can play, 0 means we're waiting */
};

/** This is all the information that is specific to a particular player in
 * the game.
 */
struct marfitude_player {
	struct marfitude_score score;   /**< The player's score info */
	struct marfitude_attack_pat ap; /**< The player's attack pattern info */
	int active;                     /**< 1 = in the game, 0 not in */
	int channel;                    /**< Which channel they're in */
	int num;                        /**< The player's unique ID 0-3 */
};

const struct wam *marfitude_get_wam(void);
const double *marfitude_get_offsets(void);
const struct marfitude_score *marfitude_get_score(int player);
int marfitude_get_highscore(void);
int marfitude_get_local_highscore(void);
int marfitude_num_players(void);
int marfitude_get_difficulty(void);
const struct slist *marfitude_get_notes(void);
const struct slist *marfitude_get_hitnotes(void);
const struct marfitude_attack_col *marfitude_get_ac(void);
const struct marfitude_attack_pat *marfitude_get_ap(int player);
const struct marfitude_player *marfitude_get_player(const struct marfitude_player *);
void marfitude_get_pos(struct marfitude_pos *);
int marfitude_get_note(int row, int col);
void marfitude_get_notepos(union vector *dest, int row, int col);
void marfitude_evalv(union vector *v);
void marfitude_translate3d(double, double, double);
void marfitude_translatev(const union vector *);

#define marfitude_foreach_player(ps) for(ps=marfitude_get_player(0); ps != 0; ps=marfitude_get_player(ps))
