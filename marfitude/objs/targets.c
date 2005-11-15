#include "SDL_opengl.h"

#include "marfitude.h"
#include "targets.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/textures.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#include "util/slist.h"

static void draw_target(void);
static void draw_targets(const void *);
static void update_shot(const void *data);

static int target_tex[MAX_PLAYERS];
static int light_tex;
static float flare[MAX_PLAYERS][MAX_NOTE+1];

void targets_init(void)
{
	int p, i;

	for(p=0; p<MAX_PLAYERS; p++)
		for(i=0; i<MAX_NOTE+1; i++)
			flare[p][i] = 0.0;
	target_tex[0] = texture_num("Target-1.png");
	target_tex[1] = texture_num("Target-2.png");
	target_tex[2] = texture_num("Target-3.png");
	target_tex[3] = texture_num("Target-4.png");
	light_tex = texture_num("Target-light.png");
	register_event("draw transparent", draw_targets);
	register_event("shoot", update_shot);
}

void targets_exit(void)
{
	deregister_event("shoot", update_shot);
	deregister_event("draw transparent", draw_targets);
}

void update_shot(const void *data)
{
	const struct button_e *b = data;

	flare[b->player][b->button] += 0.5;
	if(flare[b->player][b->button] > 1.0)
		flare[b->player][b->button] = 1.0;
}

void draw_target(void)
{
	glBegin(GL_QUADS); {
		glTexCoord2f(0.0, 0.0); glVertex3f(-0.25, 0.0, -0.25);
		glTexCoord2f(0.0, 1.0); glVertex3f(-0.25, 0.0, 0.25);
		glTexCoord2f(1.0, 1.0); glVertex3f(0.25, 0.0, 0.25);
		glTexCoord2f(1.0, 0.0); glVertex3f(0.25, 0.0, -0.25);
	} glEnd();
}

void draw_targets(const void *data)
{
	int x;
	int z;
	const struct marfitude_player *ps;
	const struct marfitude_attack_col *ac = marfitude_get_ac();
	struct marfitude_pos pos;

	if(data) {}
	marfitude_get_pos(&pos);

	marfitude_foreach_player(ps) {
		struct slist *t;

		z = 0;
		slist_foreach(t, ac[ps->channel].ps) {
			if(t->data == ps)
				break;
			z++;
		}
		glPushMatrix();
		glTranslated((double)ps->channel * -BLOCK_WIDTH, 0.0, TIC_HEIGHT * pos.tic);
		glTranslated(-NOTE_WIDTH, 0.0, -(double)z);
		glNormal3f(0.0, 1.0, 0.0);
		for(x=-1;x<=1;x++) {
			int n = 4;

			if(x == 0)
				n = 2;
			if(x == 1)
				n = 1;
			glBindTexture(GL_TEXTURE_2D, target_tex[ps->num]);
			glColor4f(1.0, 1.0, 1.0, 1.0);
			draw_target();

			if(flare[ps->num][n] > 0.0) {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glBindTexture(GL_TEXTURE_2D, light_tex);
				glDisable(GL_DEPTH_TEST);
				glColor4f(1.0, 1.0, 1.0, flare[ps->num][n]);
				draw_target();
				draw_target();
				glEnable(GL_DEPTH_TEST);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}

			glTranslated(NOTE_WIDTH, 0.0, 0.0);
		}
		glPopMatrix();
	}

	marfitude_foreach_player(ps) {
		for(x=0; x<MAX_NOTE+1; x++) {
			flare[ps->num][x] -= timeDiff * 3.3;
			if(flare[ps->num][x] < 0.0)
				flare[ps->num][x] = 0.0;
		}
	}
}
