#include "SDL_opengl.h"

#include "marfitude/press.h"

#include "gmae/event.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/timer.h"

#include "util/plugin.h"

#define NUM_LASERS 10
#define LASER_DECAY 3.0

/** Defines a laser
 */
struct laser {
	struct vector p1; /**< One endpoint of the laser */
	struct vector p2; /**< The other endpoint of the laser */
	float time;       /**< How long this laser has been 'round the block.
			   * Starts at 1.0 and decreases to 0.0 when it dies.
			   */
};

static double laser_adj(double a, double b, double dt);
static void draw_laser(struct laser *l);
static int laser_init(void);
static void laser_exit(void);
static void make_laser(const void *);
static void draw_lasers(const void *);

static int laser_tex;
static int numLasers;
static struct laser laser[NUM_LASERS];

int laser_init(void)
{
	laser_tex = TextureNum("Laser.png");
	numLasers = 0;
	RegisterEvent("press", make_laser, EVENTTYPE_MULTI);
	RegisterEvent("draw transparent", draw_lasers, EVENTTYPE_MULTI);
	return 0;
}

void laser_exit(void)
{
	DeregisterEvent("draw transparent", draw_lasers);
	DeregisterEvent("press", make_laser);
}

void make_laser(const void *data)
{
	const struct press_e *e = data;
	/* p1 is set to the light position */
	laser[numLasers].p1.x = e->v1->x;
	laser[numLasers].p1.y = e->v1->y;
	laser[numLasers].p1.z = e->v1->z;

	/* p2 is set to where the note is */
	laser[numLasers].p2.x = e->v2->x;
	laser[numLasers].p2.y = e->v2->y;
	laser[numLasers].p2.z = e->v2->z;
	laser[numLasers].time = 1.0;
	numLasers++;
	if(numLasers >= NUM_LASERS) numLasers = 0;
}

double laser_adj(double a, double b, double dt)
{
	return a * (1.0 - dt) + b * dt;
}

void draw_laser(struct laser *l)
{
	glColor4f(1.0, 0.0, 1.0, l->time);
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0);
		glVertex3f(l->p1.x-0.1, l->p1.y, l->p1.z);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(l->p1.x+0.1, l->p1.y, l->p1.z);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(l->p2.x+0.1, l->p2.y, l->p2.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(l->p2.x-0.1, l->p2.y, l->p2.z);
	} glEnd();
	l->p1.x = laser_adj(l->p1.x, l->p2.x, timeDiff * 4.0);
	l->p1.y = laser_adj(l->p1.y, l->p2.y, timeDiff * 4.0);
	l->p1.z = laser_adj(l->p1.z, l->p2.z, timeDiff * 4.0);
	l->time -= timeDiff * LASER_DECAY;
}

void draw_lasers(const void *data)
{
	int x;
	if(data) {}

	glBindTexture(GL_TEXTURE_2D, laser_tex);
	for(x=0;x<NUM_LASERS;x++) {
		draw_laser(&laser[x]);
	}
}

plugin_init(laser_init);
plugin_exit(laser_exit);
