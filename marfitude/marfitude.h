
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

/** Data for the "shoot" event */
struct shoot_e {
	int pos; /**< The position of the shot. Currently 1, 2, or 4 */
};
