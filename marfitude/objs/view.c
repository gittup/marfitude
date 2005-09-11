#include "SDL_opengl.h"

#include "marfitude.h"
#include "view.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/glfunc.h"
#include "gmae/timer.h"

static void set_main_view(const void *);
static double view_focus = 0.0;

void view_init(void)
{
	register_event("set view", set_main_view);
	view_focus = 0.0;
}

void view_exit(void)
{
	deregister_event("set view", set_main_view);
}

void set_main_view(const void *data)
{
	float eye[3] = {0.0, 3.0, -8.0};
	float view[3] = {0.0, 0.8, 0.0};
	double tmp;
	const struct marfitude_player *ps;
	struct marfitude_pos pos;

	if(data) {}
	marfitude_get_pos(&pos);

	/* Get the first active player */
	marfitude_foreach_player(ps) {
		break;
	}

	/* Make sure the view focus update doesn't go past channel focus.
	 * It should be a smooth transition and stop when it gets there.
	 */
	tmp = view_focus + ((double)ps->channel - view_focus) * timeDiff * 8.0;
	if( (ps->channel < view_focus) != (ps->channel < tmp) ) {
		view_focus = (double)ps->channel;
	} else {
		view_focus = tmp;
	}
	view[2] = TIC_HEIGHT * pos.tic;
	eye[2] = view[2] - 8.0;

	glLoadIdentity();
	look_at(eye[0] - view_focus * BLOCK_WIDTH, eye[1], eye[2],
		view[0] - view_focus * BLOCK_WIDTH, view[1], view[2],
		0.0, 1.0, 0.0);

}

double get_view_focus(void)
{
	return view_focus;
}
