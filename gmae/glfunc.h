#include "SDL.h"

#define FONT_HEIGHT 14
#define FONT_WIDTH 10

SDL_Surface *InitGL();
void QuitGL(void);
void SetOrthoProjection();
void ResetProjection();
void GLError(char *file, int line, char *func);
GLuint LoadTex(char *file);
void PrintGL(int x, int y, char *msg, ...);
void SetFontSize(float size);
void UpdateScreen(); // swap buffers and update fps

int DisplayWidth();
int DisplayHeight();

// functions between glBegin and glEnd can't call the error function

//#define GLAdoo(...) glAdoo(__VA_ARGS__); GLError(__FILE__, __LINE__, "Adoo");
#define GLClear(...) glClear(__VA_ARGS__); GLError(__FILE__, __LINE__, "Clear");
#define GLEnd(...) glEnd(__VA_ARGS__); GLError(__FILE__, __LINE__, "End");
#define GLGenTextures(...) glGenTextures(__VA_ARGS__); GLError(__FILE__, __LINE__, "GenTextures");
#define GLDeleteTextures(...) glDeleteTextures(__VA_ARGS__); GLError(__FILE__, __LINE__, "DeleteTextures");
#define GLBindTexture(...) glBindTexture(__VA_ARGS__); GLError(__FILE__, __LINE__, "BindTexture");
#define GLTexImage2D(...) glTexImage2D(__VA_ARGS__); GLError(__FILE__, __LINE__, "TexImage2D");
#define GLLoadIdentity(...) glLoadIdentity(__VA_ARGS__); GLError(__FILE__, __LINE__, "LoadIdentity");
#define GLMatrixMode(...) glMatrixMode(__VA_ARGS__); GLError(__FILE__, __LINE__, "MatrixMode");
#define GLPushMatrix(...) glPushMatrix(__VA_ARGS__); GLError(__FILE__, __LINE__, "PushMatrix");
#define GLPopMatrix(...) glPopMatrix(__VA_ARGS__); GLError(__FILE__, __LINE__, "PopMatrix");

#define GLTexParameteri(...) glTexParameteri(__VA_ARGS__); GLError(__FILE__, __LINE__, "TexParameteri");

#define GLTranslatef(...) glTranslatef(__VA_ARGS__); GLError(__FILE__, __LINE__, "Translatef");

#define GLRotatef(...) glRotatef(__VA_ARGS__); GLError(__FILE__, __LINE__, "Rotatef");

#define GLEnable(...) glEnable(__VA_ARGS__); GLError(__FILE__, __LINE__, "Enable");
#define GLDisable(...) glDisable(__VA_ARGS__); GLError(__FILE__, __LINE__, "Disable");

#define GLRasterPos2f(...) glRasterPos2f(__VA_ARGS__); GLError(__FILE__, __LINE__, "RasterPos2f");

#define GLAlphaFunc(...) glAlphaFunc(__VA_ARGS__); GLError(__FILE__, __LINE__, "AlphaFunc");
#define GLViewPort(...) glViewPort(__VA_ARGS__); GLError(__FILE__, __LINE__, "ViewPort");
#define GLClearColor(...) glClearColor(__VA_ARGS__); GLError(__FILE__, __LINE__, "ClearColor");
#define GLClearDepth(...) glClearDepth(__VA_ARGS__); GLError(__FILE__, __LINE__, "ClearDepth");
#define GLDepthFunc(...) glDepthFunc(__VA_ARGS__); GLError(__FILE__, __LINE__, "DepthFunc");
#define GLShadeModel(...) glShadeModel(__VA_ARGS__); GLError(__FILE__, __LINE__, "ShadeModel");
#define GLPixelZoom(...) glPixelZoom(__VA_ARGS__); GLError(__FILE__, __LINE__, "PixelZoom");
#define GLScalef(...) glScalef(__VA_ARGS__); GLError(__FILE__, __LINE__, "Scalef");
#define GLViewport(...) glViewport(__VA_ARGS__); GLError(__FILE__, __LINE__, "Viewport");
#define GLGenLists(...) glGenLists(__VA_ARGS__); GLError(__FILE__, __LINE__, "GenLists");
#define GLTranslated(...) glTranslated(__VA_ARGS__); GLError(__FILE__, __LINE__, "Translated");
#define GLPushAttrib(...) glPushAttrib(__VA_ARGS__); GLError(__FILE__, __LINE__, "PushAttrib");
#define GLPopAttrib(...) glPopAttrib(__VA_ARGS__); GLError(__FILE__, __LINE__, "PopAttrib");
#define GLCallLists(...) glCallLists(__VA_ARGS__); GLError(__FILE__, __LINE__, "CallLists");
#define GLCallList(...) glCallList(__VA_ARGS__); GLError(__FILE__, __LINE__, "CallList");
#define GLDeleteLists(...) glDeleteLists(__VA_ARGS__); GLError(__FILE__, __LINE__, "DeleteLists");
#define GLListBase(...) glListBase(__VA_ARGS__); GLError(__FILE__, __LINE__, "ListBase");
#define GLLightfv(...) glLightfv(__VA_ARGS__); GLError(__FILE__, __LINE__, "Lightfv");
