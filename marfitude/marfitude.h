
#define BLOCK_HEIGHT .75
#define TIC_HEIGHT .25
#define BLOCK_WIDTH 2.0
#define NOTE_WIDTH .75

#define NEGATIVE_TICKS 7*6
#define POSITIVE_TICKS 57*6
#define NUM_TICKS 64*6
#define LINES_PER_AP 8
#define ROWS_PER_LINE 4

#define UNMUTE 0
#define MUTE 1

struct marfitude_score {
	int highscore;    /**< The previous high score */
	int score;        /**< The player's current score */
	int multiplier;   /**< The player's current multiplier */
};

struct wam *marfitude_get_wam(void);
void marfitude_get_score(struct marfitude_score *);
