#include <stdlib.h>

#include "SDL_opengl.h"
#include "planks.h"
#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/wam.h"

static void draw_planks(const void *data);
static void add_plank(const void *data);
static void setup_plank(int x, int tic);

struct plank {
	/* c ----- d
	 * | plank |
	 * a ----- b
	 *
	 * n points up
	 */
	union vector a, b, c, d;
	union vector n;
	int tic;
};

static struct plank *planks;
static int num_planks;
static int cur_plank;

void planks_init(void)
{
	int x;
	struct marfitude_pos pos;

	marfitude_get_pos(&pos);

	num_planks = NUM_TICKS / 4;
	planks = malloc(sizeof(*planks) * num_planks);
	for(x=0; x<num_planks; x++) {
		int tic = ((int)pos.tic) & ~3;
		setup_plank(x, tic - NEGATIVE_TICKS + x * 4);
	}
	cur_plank = 0;
	register_event("draw opaque", draw_planks);
	register_event("tic", add_plank);
}

void planks_exit(void)
{
	deregister_event("tic", add_plank);
	deregister_event("draw opaque", draw_planks);
	free(planks);
}

void setup_plank(int x, int tic)
{
	const double plank_width = 2.0;
	const struct wam *wam;
	union vector n1, n2;
	wam = marfitude_get_wam();

	planks[x].tic = tic;
	planks[x].a.v[0] = -0.5;
	planks[x].a.v[1] = 0.0;
	planks[x].a.v[2] = tic;
	planks[x].b.v[0] = wam->num_cols - 0.5;
	planks[x].b.v[1] = 0.0;
	planks[x].b.v[2] = tic;
	planks[x].c.v[0] = -0.5;
	planks[x].c.v[1] = 0.0;
	planks[x].c.v[2] = tic + plank_width;
	planks[x].d.v[0] = wam->num_cols - 0.5;
	planks[x].d.v[1] = 0.0;
	planks[x].d.v[2] = tic + plank_width;
	n1.v[0] = 0.0;
	n1.v[1] = 0.0;
	n1.v[2] = tic;
	n2.v[0] = 0.0;
	n2.v[1] = 1.0;
	n2.v[2] = tic;
	marfitude_evalv(&n1);
	marfitude_evalv(&n2);
	planks[x].n.v[0] = n2.v[0] - n1.v[0];
	planks[x].n.v[1] = n2.v[1] - n1.v[1];
	planks[x].n.v[2] = n2.v[2] - n1.v[2];
	vector_normalize(&planks[x].n);
	marfitude_evalv(&planks[x].a);
	marfitude_evalv(&planks[x].b);
	marfitude_evalv(&planks[x].c);
	marfitude_evalv(&planks[x].d);
}

void add_plank(const void *data)
{
	int tic = *((const int*)data);
	int newtic;
	int oldtic;
	const struct wam *wam;

	wam = marfitude_get_wam();

	tic &= ~3;
	newtic = tic + POSITIVE_TICKS;
	oldtic = tic - NEGATIVE_TICKS;

	if(planks[cur_plank].tic < oldtic && newtic < wam->num_tics) {
		setup_plank(cur_plank, newtic - 4);
		cur_plank++;
		if(cur_plank >= num_planks) {
			cur_plank = 0;
		}
	}
}

void draw_planks(const void *data)
{
	int x;

	if(data) {}

	glColor4f(0.8, 0.5, 0.2, 0.7);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_QUADS); {
		for(x=0; x<num_planks; x++) {
			glNormal3dv(planks[x].n.v);
			glVertex3dv(planks[x].a.v);
			glVertex3dv(planks[x].b.v);
			glVertex3dv(planks[x].d.v);
			glVertex3dv(planks[x].c.v);
		}
	} glEnd();
	glEnable(GL_TEXTURE_2D);
}
