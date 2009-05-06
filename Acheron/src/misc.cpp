/*
 * misc.cpp
 *
 *  Created on: Apr 25, 2009
 *      Author: meta, cygen
 */

void calcNV( float v1[], float v2[], float v3[], float* normalVector ) {

	float aX, aY, aZ, bX, bY, bZ;

	// calculate vector a=v2-v1
	aX = v2[0] - v1[0];
	aY = v2[1] - v1[1];
	aZ = v2[2] - v1[2];

	// calculate vector b=v3-v1
	bX = v3[0] - v1[0];
	bY = v3[1] - v1[1];
	bZ = v3[2] - v1[2];

	// calculate cross product
	// a x b
	normalVector[0] = aY*bZ - aZ*bY;
	normalVector[1] = aZ*bX - aX*bZ;
	normalVector[2] = aX*bY - aY*bX;

}
