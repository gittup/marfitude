#include "SDL_opengl.h"

#include "marfitude.h"
#include "objs/bluenotes.h"
#include "objs/explode.h"
#include "objs/greynotes.h"
#include "objs/lines.h"
#include "objs/rows.h"
#include "objs/scoreboard.h"
#include "objs/targets.h"
#include "view.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/glfunc.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

void static_view_init(void) __attribute__ ((constructor));
void static_view_exit(void) __attribute__ ((destructor));
static void set_main_view(const void *);
static double view_focus = 0.0;

void static_view_init(void)
{
	bluenotes_init();
	explode_init();
	greynotes_init();
	lines_init();
	rows_init();
	scoreboard_init();
	targets_init();
	register_event("set view", set_main_view);
	view_focus = 0.0;
}

void static_view_exit(void)
{
	deregister_event("set view", set_main_view);
	targets_exit();
	scoreboard_exit();
	rows_exit();
	lines_exit();
	greynotes_exit();
	explode_exit();
	bluenotes_exit();
}

void set_main_view(const void *data)
{
	float eye[3] = {0.0, 10.0, -8.0};
	float view[3] = {0.0, 2.8, 3.0};
	const struct wam *wam;
	struct marfitude_pos pos;

	if(data) {}
	marfitude_get_pos(&pos);
	wam = marfitude_get_wam();

	view_focus = (double)wam->num_cols / 2.0 - 0.5;
	view[2] = TIC_HEIGHT * pos.tic;
	eye[2] = view[2] - 12.0;

	glLoadIdentity();
	look_at(eye[0] - view_focus * BLOCK_WIDTH, eye[1], eye[2],
		view[0] - view_focus * BLOCK_WIDTH, view[1], view[2],
		0.0, 1.0, 0.0);

}

int get_view_focus(void)
{
	return view_focus;
}
