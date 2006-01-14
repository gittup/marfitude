#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "view.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/glfunc.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#include "util/math/vector.h"

static void set_main_view(const void *);
static double board_middle(void);
static union vector eye;
static union vector view;

#define MULTI_Y_EYE 7.0
#define MULTI_Z_EYE -14.0
#define MULTI_Y_VIEW 2.8
#define MULTI_Z_VIEW 0.0

void view_init(void)
{
	register_event("set view", set_main_view);
	if(marfitude_num_players() == 1) {
		eye.v[0] = 0.0;
		eye.v[1] = 3.0;
		eye.v[2] = -8.0;
		view.v[0] = 0.0;
		view.v[1] = 0.8;
		view.v[2] = 0.0;
	} else {
		eye.v[0] = board_middle();
		eye.v[1] = MULTI_Y_EYE;
		eye.v[2] = MULTI_Z_EYE;
		view.v[0] = eye.v[0];
		view.v[1] = MULTI_Y_VIEW;
		view.v[2] = MULTI_Z_VIEW;
	}
}

void view_exit(void)
{
	deregister_event("set view", set_main_view);
}

void set_main_view(const void *data)
{
	union vector dest;
	const struct marfitude_player *ps;
	struct marfitude_pos pos;

	if(data) {}
	marfitude_get_pos(&pos);

	if(marfitude_num_players() == 1) {
		/* Get the active player */
		ps = marfitude_get_player(0);

		dest.v[0] = -(double)ps->channel * BLOCK_WIDTH;
		dest.v[1] = 3.0;
		dest.v[2] = TIC_HEIGHT * pos.tic - 8.0;
		eye.v[2] = dest.v[2];
		vector_transition(&eye, &dest, timeDiff * 8.0, 0.003);

		/* v[0] stays the same */
		dest.v[1] = 0.8;
		dest.v[2] = TIC_HEIGHT * pos.tic;
		view.v[2] = dest.v[2];
		vector_transition(&view, &dest, timeDiff * 8.0, 0.003);
	} else {
		dest.v[0] = board_middle();
		dest.v[1] = MULTI_Y_EYE;
		dest.v[2] = MULTI_Z_EYE + TIC_HEIGHT * pos.tic;
		eye.v[2] = dest.v[2];
		vector_transition(&eye, &dest, timeDiff * 8.0, 0.003);

		dest.v[1] = MULTI_Y_VIEW;
		dest.v[2] = MULTI_Z_VIEW + TIC_HEIGHT * pos.tic;
		view.v[2] = dest.v[2];
		vector_transition(&view, &dest, timeDiff * 8.0, 0.003);
	}

	glLoadIdentity();
	look_at(eye.v[0], eye.v[1], eye.v[2],
		view.v[0], view.v[1], view.v[2],
		0.0, 1.0, 0.0);
}

double get_view_focus(void)
{
	return eye.v[0];
}

double board_middle(void)
{
	const struct wam *wam;

	wam = marfitude_get_wam();
	return -BLOCK_WIDTH * ((double)wam->num_cols / 2.0 - 0.5);
}
