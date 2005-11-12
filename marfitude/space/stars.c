#include "SDL_opengl.h"

#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/timer.h"

#include "util/math/vector.h"
#include "util/myrand.h"

static void stars_init(void) __attribute__((constructor));
static void stars_exit(void) __attribute__((destructor));
static void draw_stars(const void *);

#define NUM_STARS 150

static union vector stars[NUM_STARS];

void stars_init(void)
{
	int x;

	register_event("draw ortho", draw_stars);
	for(x=0; x<NUM_STARS; x++) {
		stars[x].v[0] = rand_int(640);
		stars[x].v[1] = rand_int(480);
		stars[x].v[2] = rand_float();
		stars[x].v[3] = rand_float();
	}
}

void stars_exit(void)
{
	deregister_event("draw ortho", draw_stars);
}

void draw_stars(const void *data)
{
	int x;

	if(data) {}

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_POINTS); {
		for(x=0; x<NUM_STARS; x++) {
			glColor4f(1.0, 1.0, 1.0, stars[x].v[3]);
			glVertex3f(orthox(stars[x].v[0]), orthoy(stars[x].v[1]), 0.0);
			stars[x].v[3] += stars[x].v[2] * timeDiff;
			if(stars[x].v[3] > 1.0) {
				stars[x].v[3] = 1.0;
				stars[x].v[2] = -stars[x].v[2];
			} else if(stars[x].v[3] < 0.0) {
				stars[x].v[3] = 0.0;
				stars[x].v[2] = -stars[x].v[2];
			}
		}
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
}
