
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

/** A structure of score information */
struct marfitude_score {
	int highscore;    /**< The previous high score */
	int score;        /**< The player's current score */
	int multiplier;   /**< The player's current multiplier */
};

/** A structure of time/position information */
struct marfitude_pos {
	double modtime;        /**< Time in seconds */
	double tic;            /**< Current tic position */
	const struct row *row; /**< Current row */
	int row_index;         /**< Row index */
	int channel;           /**< Which channel is currently played */
};

const struct wam *marfitude_get_wam(void);
const int *marfitude_get_offsets(void);
const struct marfitude_score *marfitude_get_score(void);
void marfitude_get_pos(struct marfitude_pos *);
