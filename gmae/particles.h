#include "phys.h"

enum particleTypes {
	P_Point = 0,
	P_Line,
	P_BlueNova,
	P_BlueStar,
	P_Fireball,
	P_StarBurst,
	P_SunBurst,
	P_LAST
};

enum particleListTypes {
	PT_POINT = 0,
	PT_LINE,
	PT_TLINE,
	PT_TQUAD,
	PT_2TQUAD,
	PT_LAST
};

struct particle {
	struct obj *o;	/* used for physics stuff */
	float col[4];	/* RGBA colors */
	int type;	/* one of ParticleTypes above */
	float size;	/* dimension of particle */
	int active;	/* 1 = drawn, 0 not drawn */
	float life;	/* TTL in seconds */
};

struct particleType {
	int type;	/* one of ParticleListTypes above */
	int billboard;	/* 1 = billboarded, 0 = not billboarded */
	GLuint tex1;	/* first texture (if necessary) */
	GLuint tex2;	/* second texture (if necessary) */
};

typedef int (*PTestFunc)(struct particle *);

int InitParticles(void);
void QuitParticles(void);
void DrawParticles(void);
void DrawParticlesTest(PTestFunc p);
void StartParticles(void);	/* set up OpenGL variables */
void StopParticles(void);	/* reset OpenGL variables */
void CreateParticle(struct obj *o, float col[4], int type, float size);
void ClearParticles(void);

extern GLuint plist;
