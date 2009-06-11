/*
 * EigenWrapper.cpp
 *
 *  Created on: May 26, 2009
 *      Author: max
 */

#include "CoordinatesManipulator.h"
#include <GL/glew.h>

#include <iostream>
#include <cstdlib>


CoordinatesManipulator::CoordinatesManipulator() {
	calcM = Matrix4f::Identity();
	modelViewMatrix = Matrix4f::Identity();
}

void CoordinatesManipulator::resetPosition() {
	modelViewMatrix.setIdentity();
}

void CoordinatesManipulator::setPosition() {
// ############
	float temp[16];
	glGetFloatv( GL_MODELVIEW_MATRIX, temp );
	loadMatrix( temp );
}

//void CoordinatesManipulator::setPosition( float x, float y, float z ) {
//	// ############
//	v[0] = x;
//	v[1] = y;
//	v[2] = z;
//}
//
//void CoordinatesManipulator::setPosition( float position[3] ) {
//	// ############
//	v[0] = position[0];
//	v[1] = position[1];
//	v[2] = position[2];
//}

void CoordinatesManipulator::loadMatrix( float matrix[16] ) {
	// FIXME
//	for( int i=0; i<4; i++ ) {
//		for( int j=0; j<4; j++ ) {
//			modelViewMatrix(i,j) = matrix[ i*4 + j ];
//		}
//	}
}

void CoordinatesManipulator::rotate( float angle, int x, int y, int z ) {
	calcM.setIdentity();
	// cooler effekt, wenn nicht in radiant umgerechnet wird
	angle = angle*M_PI/180;
	if( x == 1 && y == 0 && z == 0 ){
		calcM(1,1) =  cos(angle);
		calcM(1,2) = -sin(angle);
		calcM(2,1) =  sin(angle);
		calcM(2,2) =  cos(angle);
	} else if( x == 0 && y == 1 && z == 0 ){
		calcM(0,0) =  cos(angle);
		calcM(0,2) =  sin(angle);
		calcM(2,0) = -sin(angle);
		calcM(2,2) =  cos(angle);
	} else if( x == 0 && y == 0 && z == 1 ){
		calcM(0,0) =  cos(angle);
		calcM(0,1) = -sin(angle);
		calcM(1,0) =  sin(angle);
		calcM(1,1) =  cos(angle);
	}

	modelViewMatrix = modelViewMatrix*calcM;
}

void CoordinatesManipulator::translate( float x, float y, float z ) {
	calcM.setIdentity();

	calcM(0,3) = x;
	calcM(1,3) = y;
	calcM(2,3) = z;

	modelViewMatrix = modelViewMatrix*calcM;
}

void CoordinatesManipulator::getPosition( float* position ) {
	position[0] = modelViewMatrix(0,3);
	position[1] = modelViewMatrix(1,3);
	position[2] = modelViewMatrix(2,3);
}

// FIXME
// set matrix mode
void CoordinatesManipulator::setModelViewMatrix() {
	float temp[16];
	for( int i=0; i<4; i++ ){
		for( int j=0; j<4; j++ ) {
			temp[ i*4 + j ] = modelViewMatrix(i,j);
		}
	}
	glLoadMatrixf( temp );
}

/**
 * returns row-wise, to use with opengl
 */
void CoordinatesManipulator::getMatrix( float* matrix ) {
	for( int i=0; i<4; i++ ){
		for( int j=0; j<4; j++ ) {
			matrix[ j*4+i ] = modelViewMatrix(i,j);
		}
	}
}

void CoordinatesManipulator::debug_ShowCalcMatrix() {
	std::cout << std::endl;
	std::cout << "calcM:" << std::endl;
	std::cout << calcM << std::endl;
	std::cout << std::endl;

}



