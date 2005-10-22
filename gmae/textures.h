#include "SDL_opengl.h"

/** @file
 * Manages OpenGL textures. All .png's are loaded on initialization, and can
 * be accessed by using the texture_num function. Alternatively, another
 * texture not in the main images directory can be loaded with load_texture.
 */

void create_texture(const char *name, int *tex, int width, int height, void (*draw)(unsigned char *, int, int));
void delete_texture(int *tex);
GLuint load_texture(const char *filename);
GLuint texture_num(const char *name);
int init_textures(void);
void quit_textures(void);

/** Masks for SDL_CreateRGBSurface */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define MASKS 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
#define MASKS 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
