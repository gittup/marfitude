#define RED 0
#define GREEN 1
#define BLUE 2
#define ALPHA 3

struct vector {
	double x;
	double y;
	double z;
};

struct obj {
	struct vector pos;	/* position of object */
	struct vector vel;	/* velocity of object */
	struct vector acc;	/* acceleration of object */
	/* need jerk for camera movement? */
	struct vector axis;	/* axis of rotation */
	double theta;	/* amount of rotation */
	double rotvel;	/* rotation velocity */
	double rotacc;	/* rotation acceleration */
	float mass;	/* object's mass */
};

struct obj *NewObj(void);
void DeleteObj(struct obj *o);
void ClearObjs(void);
void UpdateObjs(double dt);
void CheckObjs(void);
