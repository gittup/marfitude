#include "SDL_opengl.h"

/** @file
 * Manages OpenGL textures. All .png's are loaded on initialization, and can
 * be accessed by using the texture_num function. Alternatively, another
 * texture not in the main images directory can be loaded with load_texture.
 */

void create_texture(int *tex, int width, int height, void (*draw)(void *, int));
GLuint load_texture(const char *filename);
GLuint texture_num(const char *name);
int init_textures(void);
void quit_textures(void);
