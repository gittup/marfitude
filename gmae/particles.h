#include "phys.h"

typedef enum {	P_Point = 0,
		P_Line,
		P_BlueNova,
		P_BlueStar,
		P_Fireball,
		P_StarBurst,
		P_SunBurst,
		P_LAST
		} ParticleTypes;

typedef enum {	PT_POINT = 0,
		PT_LINE,
		PT_TLINE,
		PT_TQUAD,
		PT_2TQUAD,
		PT_LAST
		} ParticleListTypes;

typedef struct {
	Obj *o;		/* used for physics stuff */
	float col[4];	/* RGBA colors */
	int type;	/* one of ParticleTypes above */
	float size;	/* dimension of particle */
	int active;	/* 1 = drawn, 0 not drawn */
	float life;	/* TTL in seconds */
	} Particle;

typedef struct {
	int type;	/* one of ParticleListTypes above */
	int billboard;	/* 1 = billboarded, 0 = not billboarded */
	GLuint tex1;	/* first texture (if necessary) */
	GLuint tex2;	/* second texture (if necessary) */
	} ParticleType;

typedef int (*PTestFunc)(Particle *);

int InitParticles(void);
void QuitParticles(void);
void DrawParticles(void);
void DrawParticlesTest(PTestFunc p);
void StartParticles(void);	/* set up OpenGL variables */
void StopParticles(void);	/* reset OpenGL variables */
void CreateParticle(Obj *o, float col[4], int type, float size);
void ClearParticles(void);

extern GLuint plist;
