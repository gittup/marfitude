#include "SDL.h"

#define FONT_HEIGHT 14
#define FONT_WIDTH 10

SDL_Surface *InitGL(void);
void QuitGL(void);
void SetOrthoProjection(void);
void ResetProjection(void);
void GLError(char *file, int line, char *func);
GLuint LoadTex(char *file);
void PrintGL(int x, int y, const char *msg, ...);
void SetFontSize(float size);
void UpdateScreen(void); /* swap buffers and update fps */

int DisplayWidth(void);
int DisplayHeight(void);
