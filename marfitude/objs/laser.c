#include <stdlib.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "laser.h"
#include "fireball.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/timer.h"

#include "util/math/vector.h"

#define NUM_LASERS 10
#define LASER_DECAY 3.0

/** Defines a laser */
struct laser {
	union vector p; /**< One endpoint of the laser */
	union vector l; /**< The length of the laser */
	float time;     /**< How long this laser has been 'round the block.
			 * Starts at 1.0 and decreases to 0.0 when it dies.
			 */
};

static void draw_laser(struct laser *l);
static void make_laser(const void *);
static void draw_lasers(const void *);
static void create_laser(unsigned char *p, int x, int y);
static const int width = 32;
static const int height = 128;

static int laser_tex;
static int num_lasers;
static struct laser laser[NUM_LASERS];

void laser_init(void)
{
	create_texture(&laser_tex, width, height, create_laser);
	num_lasers = 0;
	register_event("shoot", make_laser);
	register_event("draw transparent", draw_lasers);
}

void laser_exit(void)
{
	deregister_event("draw transparent", draw_lasers);
	deregister_event("shoot", make_laser);
	delete_texture(&laser_tex);
}

void make_laser(const void *data)
{
	const struct button_e *b = data;
	const double *offsets = marfitude_get_offsets();
	struct marfitude_pos p;
	const float *fireball = fireball_get_pos();

	if(marfitude_num_players() != 1)
		return;
	marfitude_get_pos(&p);

	/* p is set to where the note is */
	laser[num_lasers].p.v[0] = p.channel + offsets[b->button];
	laser[num_lasers].p.v[1] = 0.0;
	laser[num_lasers].p.v[2] = p.tic;
	marfitude_evalv(&laser[num_lasers].p);

	/* Set l so p-l is where the light is. */
	laser[num_lasers].l.v[0] = fireball[0] - laser[num_lasers].p.v[0];
	laser[num_lasers].l.v[1] = fireball[1] - laser[num_lasers].p.v[1];
	laser[num_lasers].l.v[2] = fireball[2] - laser[num_lasers].p.v[2];

	laser[num_lasers].time = 1.0;
	num_lasers++;
	if(num_lasers >= NUM_LASERS) num_lasers = 0;
}

void draw_laser(struct laser *l)
{
	double x = l->l.v[0] * l->time;
	double y = l->l.v[1] * l->time;
	double z = l->l.v[2] * l->time;

	glPushMatrix();
	glTranslated(l->p.v[0], l->p.v[1], l->p.v[2]);
	glColor4f(1.0, 0.0, 1.0, l->time);
	glBegin(GL_QUADS); {
		glTexCoord2f(1.0, 0.0); glVertex3f(0.1, 0.0, 0.0);
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.1, 0.0, 0.0);

		glTexCoord2f(0.0, 1.0); glVertex3f(x-0.1, y, z);
		glTexCoord2f(1.0, 1.0); glVertex3f(x+0.1, y, z);
	} glEnd();
	glPopMatrix();

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

void create_laser(unsigned char *p, int x, int y)
{
	int tmp;
	int t2 = abs(width/2 - x);

	if(y) {}

	tmp = 255 - t2 * 255 / 15;
	p[RED] = 255;
	p[GREEN] = 255;
	p[BLUE] = 255;
	if(tmp < 0)
		p[ALPHA] = 0;
	else
		p[ALPHA] = tmp;
}
