#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/textures.h"

#include "util/plugin.h"

/* TEmproarry */
extern int channelFocus;
extern int curTic;
extern double partialTic;
/* End TEmproarry */

static int targets_init(void);
static void targets_exit(void);
static void draw_targets(const void *);

static int target_tex;

int targets_init(void)
{
	target_tex = TextureNum("Target.png");
	RegisterEvent("draw transparent", draw_targets, EVENTTYPE_MULTI);
	return 0;
}

void targets_exit(void)
{
	DeregisterEvent("draw transparent", draw_targets);
}

void draw_targets(const void *data)
{
	int x;
	if(data) {}

	glPushMatrix();
	glTranslated((double)channelFocus * -BLOCK_WIDTH, 0.0, TIC_HEIGHT * ((double)curTic + partialTic));
	glBindTexture(GL_TEXTURE_2D, target_tex);
	glTranslated(-NOTE_WIDTH, 0.0, 0.0);
	glColor4f(1.0, 1.0, 1.0, 0.7);
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

plugin_init(targets_init);
plugin_exit(targets_exit);
