#include "objs/greynotes.h"
#include "objs/lines.h"
#include "objs/scoreboard.h"
#include "objs/targets.h"
#include "objs/view.h"

static void space_init(void) __attribute__ ((constructor));
static void space_exit(void) __attribute__ ((destructor));

void space_init(void)
{
	greynotes_init();
	lines_init();
	scoreboard_init();
	targets_init();
	view_init();
}

void space_exit(void)
{
	view_exit();
	targets_exit();
	scoreboard_exit();
	lines_exit();
	greynotes_exit();
}
