/*
   Marfitude
   Copyright (C) 2004 Mike Shal

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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
