#define RED 0
#define GREEN 1
#define BLUE 2
#define ALPHA 3

typedef struct {
	double x;
	double y;
	double z;
	} Point, Vector;

typedef struct {
	Point pos;	// position of object
	Vector vel;	// velocity of object
	Vector acc;	// acceleration of object
	// need jerk for camera movement?
	Vector axis;	// axis of rotation
	double theta;	// amount of rotation
	double rotvel;	// rotation velocity
	double rotacc;	// rotation acceleration
	float mass;	// object's mass
	} Obj;

Obj *NewObj();
void DeleteObj(Obj *o);
void ClearObjs();
void UpdateObjs(int dt);
void CheckObjs();
