/*
 * camera.cpp
 *
 *  Created on: Apr 21, 2009
 *      Author: meta, cygen
 */

#include "camera.h"
#include <cmath>
#include <stdio.h>

float radius;
float startvec[3];
int center[2] = {0, 0};

/*!
  x,y: mouse positon relative to window
  w,h: window pixel dimensions
*/
void trackballInit(int x, int y, int w, int h)
{
	radius = 1.0f*(w>h?h:w);
	center[0] = (int)(0.5f * w);
	center[1] = (int)(0.5f * h);

	// surface->center vector
	startvec[0] = (float)(x - center[0]);
	startvec[1] = (float)(y - center[1]);
	float xxyy =  startvec[0] * startvec[0] + startvec[1] * startvec[1];
	if (xxyy > radius * radius) { // outside the sphere
		startvec[2] = 0.0f;
	}
	else {
		startvec[2] = sqrt(radius * radius - xxyy); // pythagoras
	}
}

/*!
  Rotate using trackball mode.

  x,y: Mouse position relative to window
  rot: Output rotation
*/
void trackballRotate(int x, int y, float rotvec[4])
{
	float endvec[3];

	endvec[0] = (float)(x - center[0]);
	endvec[1] = (float)(y - center[1]);
	// surface->center vector
	float xxyy = endvec[0] * endvec[0] + endvec[1] * endvec[1];
	if (xxyy > radius * radius) { // outside the sphere
		endvec[2] = 0.0f;
	} else {
		endvec[2] = sqrt(radius * radius - xxyy); // pythagoras
	}

	// rotvec = startvec x endvec
	rotvec[1] = startvec[1] * endvec[2] - startvec[2] * endvec[1];
	rotvec[2] = startvec[2] * endvec[0] - startvec[0] * endvec[2];
	rotvec[3] = startvec[0] * endvec[1] - startvec[1] * endvec[0];

	// Angle = 0.5*|endvec-startvec|
	float tmpvec[3] = {
		endvec[0]-startvec[0],
		endvec[1]-startvec[1],
		endvec[2]-startvec[2]
	};

	rotvec[0] = 0.5f*sqrt(tmpvec[0]*tmpvec[0] + tmpvec[1]*tmpvec[1] + tmpvec[2]*tmpvec[2]);
}
