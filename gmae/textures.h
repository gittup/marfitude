#include "SDL_opengl.h"
GLuint LoadTexture(const char *filename);
GLuint TextureNum(const char *name);
int InitTextures(void);
void QuitTextures(void);

struct tex_entry {
	GLuint texture;
	char *name;
};

enum texoffset {
	T_BlueNova,
	T_BlueStar,
	T_Clovers,
	T_ElectricBlue,
	T_Fireball,
	T_FlatlandFiery,
	T_Laser,
	T_Lava,
	T_Parque,
	T_Parque3,
	T_Slate,
	T_StarBurst,
	T_StarCenter,
	T_SunBurst,
	T_SunCenter,
	T_Target,
	T_Title,
	T_Walnut};
