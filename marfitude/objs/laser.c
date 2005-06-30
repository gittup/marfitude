#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/timer.h"

#include "util/dl.h"
#include "util/plugin.h"

#define NUM_LASERS 10
#define LASER_DECAY 3.0

/** Defines a laser */
struct laser {
	struct vector p1; /**< One endpoint of the laser */
	struct vector p2; /**< The other endpoint of the laser */
	float time;       /**< How long this laser has been 'round the block.
			   * Starts at 1.0 and decreases to 0.0 when it dies.
			   */
};

void __attribute__ ((constructor)) laser_init(void);
void __attribute__ ((destructor)) laser_exit(void);
static double laser_adj(double a, double b, double dt);
static void draw_laser(struct laser *l);
static void make_laser(const void *);
static void draw_lasers(const void *);

static int laser_tex;
static int numLasers;
static struct laser laser[NUM_LASERS];
static const float *fireball;
static void *fireball_handle = NULL;
static float firetest[4] = {0.0, 0.0, 0.0, 0.0};

void laser_init(void)
{
	laser_tex = texture_num("Laser.png");
	numLasers = 0;
	register_event("shoot", make_laser, EVENTTYPE_MULTI);
	register_event("draw transparent", draw_lasers, EVENTTYPE_MULTI);
	fireball_handle = load_plugin("fireball");
	if(fireball_handle) {
		const float *(*fb)(void);
		fb = (const float *(*)(void))dlsym(fireball_handle, "fireball_get_pos");
		fireball = fb();
	}
	else
		fireball = firetest;
}

void laser_exit(void)
{
	free_plugin(fireball_handle);
	deregister_event("draw transparent", draw_lasers);
	deregister_event("shoot", make_laser);
}

void make_laser(const void *data)
{
	const int *s = data;
	const int *offsets = marfitude_get_offsets();
	struct marfitude_pos p;

	marfitude_get_pos(&p);

	/* p1 is set to the light position */
	laser[numLasers].p1.x = fireball[0];
	laser[numLasers].p1.y = fireball[1];
	laser[numLasers].p1.z = fireball[2];

	/* p2 is set to where the note is */
	laser[numLasers].p2.x = -p.channel * BLOCK_WIDTH - NOTE_WIDTH * offsets[*s];
	laser[numLasers].p2.y = 0.0;
	laser[numLasers].p2.z = TIC_HEIGHT * p.tic;
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
