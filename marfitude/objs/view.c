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
static void update_view(const void *);
static double board_middle(void);
static union vector eye;
static union vector view;

#define MULTI_Y_EYE 7.0
#define MULTI_Z_EYE -14.0
#define MULTI_Y_VIEW 2.8
#define MULTI_Z_VIEW 0.0

void view_init(void)
{
	/* Make sure we use "timer delta" since we use the position from
	 * marfitude_get_pos, which is updated in the "timer tic delta"
	 * event.
	 */
	register_event("set view", set_main_view);
	register_event("timer delta", update_view);
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
	deregister_event("timer delta", update_view);
	deregister_event("set view", set_main_view);
}

void set_main_view(const void *data)
{
	double ex, ey, ez, vx, vy, vz;

	if(data) {}

	glLoadIdentity();
	ex = eye.v[0];
	ey = eye.v[1];
	ez = eye.v[2];
	vx = view.v[0];
	vy = view.v[1];
	vz = view.v[2];
	marfitude_eval3d(&ex, &ey, &ez);
	marfitude_eval3d(&vx, &vy, &vz);
	look_at(ex, ey, ez,
		vx, vy, vz,
		0.0, 1.0, 0.0);
}

void update_view(const void *data)
{
	double dt = *((const double *)data);
	const struct marfitude_player *ps;
	struct marfitude_pos pos;
	union vector dest;

	marfitude_get_pos(&pos);

	if(marfitude_num_players() == 1) {
		/* Get the active player */
		ps = marfitude_get_player(0);

		dest.v[0] = (double)ps->channel;
		dest.v[1] = 3.0;
		dest.v[2] = pos.tic - 32.0;
		eye.v[2] = dest.v[2];
		vector_transition(&eye, &dest, dt * 8.0, 0.003);

		dest.v[0] = (double)ps->channel;
		dest.v[1] = 0.8;
		dest.v[2] = pos.tic;
		view.v[2] = dest.v[2];
		vector_transition(&view, &dest, dt * 8.0, 0.003);
	} else {
		dest.v[0] = board_middle();
		dest.v[1] = MULTI_Y_EYE;
		dest.v[2] = MULTI_Z_EYE + TIC_HEIGHT * pos.tic;
		eye.v[2] = dest.v[2];
		vector_transition(&eye, &dest, dt * 8.0, 0.003);

		dest.v[1] = MULTI_Y_VIEW;
		dest.v[2] = MULTI_Z_VIEW + TIC_HEIGHT * pos.tic;
		view.v[2] = dest.v[2];
		vector_transition(&view, &dest, dt * 8.0, 0.003);
	}
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
