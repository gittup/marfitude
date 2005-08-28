#include "SDL_opengl.h"

#include "marfitude.h"
#include "targets.h"

#include "gmae/event.h"
#include "gmae/input.h"
#include "gmae/textures.h"

static void draw_targets(const void *);

static int target_tex[MAX_PLAYERS];

void targets_init(void)
{
	target_tex[0] = texture_num("Target-1.png");
	target_tex[1] = texture_num("Target-2.png");
	target_tex[2] = texture_num("Target-3.png");
	target_tex[3] = texture_num("Target-4.png");
	register_event("draw transparent", draw_targets);
}

void targets_exit(void)
{
	deregister_event("draw transparent", draw_targets);
}

void draw_targets(const void *data)
{
	int x;
	int p;
	const struct marfitude_player *ps;
	struct marfitude_pos pos;

	if(data) {}
	marfitude_get_pos(&pos);

	for(p=0; p<marfitude_num_players(); p++) {
		ps = marfitude_get_player(p);
		glPushMatrix();
		glTranslated((double)ps->channel * -BLOCK_WIDTH, 0.0, TIC_HEIGHT * pos.tic);
		glBindTexture(GL_TEXTURE_2D, target_tex[p]);
		glTranslated(-NOTE_WIDTH, 0.0, 0.0);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glNormal3f(0.0, 1.0, 0.0);
		for(x=-1;x<=1;x++) {
			glBegin(GL_QUADS); {
				glTexCoord2f(0.0, 0.0);
				glVertex3f(-0.25, 0.01, -0.25);
				glTexCoord2f(1.0, 0.0);
				glVertex3f(0.25, 0.01, -0.25);
				glTexCoord2f(1.0, 1.0);
				glVertex3f(0.25, 0.01, 0.25);
				glTexCoord2f(0.0, 1.0);
				glVertex3f(-0.25, 0.01, 0.25);
			} glEnd();
			glTranslated(NOTE_WIDTH, 0.0, 0.0);
		}
		glPopMatrix();
	}
}
