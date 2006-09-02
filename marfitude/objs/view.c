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

#define MULTI_Y_EYE 10.0
#define MULTI_Z_EYE -48.0
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
	union vector e, v, n;
	int eye_offset = *((const int *)data);

	glLoadIdentity();
	e.v[0] = eye.v[0] + eye_offset * 0.03;
	e.v[1] = eye.v[1];
	e.v[2] = eye.v[2];
	v.v[0] = view.v[0];
	v.v[1] = view.v[1];
	v.v[2] = view.v[2];
	marfitude_evalv(&e, 0, 0, &n, &e);
	marfitude_evalvec(&v);
	look_at(e.v[0], e.v[1], e.v[2],
		v.v[0], v.v[1], v.v[2],
		n.v[0], n.v[1], n.v[2]);
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
		dest.v[2] = MULTI_Z_EYE + pos.tic;
		eye.v[2] = dest.v[2];
		vector_transition(&eye, &dest, dt * 8.0, 0.003);

		dest.v[1] = MULTI_Y_VIEW;
		dest.v[2] = MULTI_Z_VIEW + pos.tic;
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
	return ((double)wam->num_cols / 2.0 - 0.5);
}
