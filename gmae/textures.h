#include "SDL_opengl.h"

GLuint LoadTexture(const char *filename);
GLuint TextureNum(const char *name);
int InitTextures(void);
void QuitTextures(void);

struct tex_entry {
	GLuint texture;
	char *name;
};
