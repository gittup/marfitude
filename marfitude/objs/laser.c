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
	union vector p1; /**< One endpoint of the laser */
	union vector p2; /**< The other endpoint of the laser */
	float time;      /**< How long this laser has been 'round the block.
			  * Starts at 1.0 and decreases to 0.0 when it dies.
			  */
};

static double laser_adj(double a, double b, double dt);
static void draw_laser(struct laser *l);
static void make_laser(const void *);
static void draw_lasers(const void *);
static void create_laser(void *pixels, int pitch);
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
}

void make_laser(const void *data)
{
	const struct button_e *b = data;
	const int *offsets = marfitude_get_offsets();
	struct marfitude_pos p;
	const float *fireball = fireball_get_pos();

	marfitude_get_pos(&p);

	/* p1 is set to the light position */
	laser[num_lasers].p1.v[0] = fireball[0];
	laser[num_lasers].p1.v[1] = fireball[1];
	laser[num_lasers].p1.v[2] = fireball[2];

	/* p2 is set to where the note is */
	laser[num_lasers].p2.v[0] = -p.channel * BLOCK_WIDTH - NOTE_WIDTH * offsets[b->button];
	laser[num_lasers].p2.v[1] = 0.0;
	laser[num_lasers].p2.v[2] = TIC_HEIGHT * p.tic;
	laser[num_lasers].time = 1.0;
	num_lasers++;
	if(num_lasers >= NUM_LASERS) num_lasers = 0;
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
		glVertex3f(l->p1.v[0]-0.1, l->p1.v[1], l->p1.v[2]);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(l->p1.v[0]+0.1, l->p1.v[1], l->p1.v[2]);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(l->p2.v[0]+0.1, l->p2.v[1], l->p2.v[2]);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(l->p2.v[0]-0.1, l->p2.v[1], l->p2.v[2]);
	} glEnd();
	l->p1.v[0] = laser_adj(l->p1.v[0], l->p2.v[0], timeDiff * 4.0);
	l->p1.v[1] = laser_adj(l->p1.v[1], l->p2.v[1], timeDiff * 4.0);
	l->p1.v[2] = laser_adj(l->p1.v[2], l->p2.v[2], timeDiff * 4.0);
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

void create_laser(void *pixels, int pitch)
{
	int x;
	int y;
	unsigned char *p;

        for(x=0; x<width; x++) {
                int tmp;
                int t2 = abs(width/2 - x);
                tmp = 255 - t2 * 255 / 15;
                for(y=0; y<height; y++) {
                        p = pixels;
                        p += x * 4;
                        p += y * pitch;
                        p[RED] = 255;
                        p[GREEN] = 255;
                        p[BLUE] = 255;
                        if(tmp < 0)
                                p[ALPHA] = 0;
                        else
                                p[ALPHA] = tmp;
                }
        }
}
