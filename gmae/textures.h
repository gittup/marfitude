#include "SDL_opengl.h"

/** @file
 * Manages OpenGL textures. All .png's are loaded on initialization, and can
 * be accessed by using the TextureNum function. Alternatively, another
 * texture not in the main images directory can be loaded with LoadTexture.
 */

GLuint LoadTexture(const char *filename);
GLuint TextureNum(const char *name);
int InitTextures(void);
void QuitTextures(void);
