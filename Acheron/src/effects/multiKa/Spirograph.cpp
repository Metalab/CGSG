/*
 * Spirograph.cpp
 *
 *  Created on: May 21, 2009
 *      Author: max
 */

#define NOMINMAX

#include <GL/glew.h>

#include "Spirograph.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "../../extramath.h"

// for debugging
#include <iostream>
#include <cstdlib>


using namespace multiKa;

Spirograph::Spirograph( float av1, float d1, float av2, float d2, float av3, float d3, int length )
: Base3DObject( 0.0, 0.0, 0.0 )
{

	this->a1	= 0;
	this->av1	= av1;
	this->d1	= d1;
	this->a2	= 0;
	this->av2	= av2;
	this->d2	= d2;
	this->a3	= 0;
	this->av3	= av3;
	this->d3	= d3;

	this->rot = 0.0;

	//fixme
	this->length = length;
//	this->path = new RingBuffer( length, 6 );
	this->path = new RingBuffer( length, 16 );
	this->minDistance = 0.5;

	this->frameCounter = 0;
	this->pathLength = 0;

	this->colors = new HSBColors(6);

	this->cManipulator = new CoordinatesManipulator();

	precalculate();
}

void Spirograph::draw( float spectrum[], int spectrumStart, int spectrumEnd ) {

	calculateNewPosition();
	colors->update();

	drawPath( spectrum, spectrumStart, spectrumEnd );

	drawArm();

}

void Spirograph::precalculate(){
	glPushMatrix();
	glLoadIdentity();
	float tempPos[16];

	//cManipulator->setPosition();

	while( pathLength < length ) {
		cManipulator->resetPosition();
		cManipulator->rotate( a1, 0, 1, 0 );
		cManipulator->translate( d1, 0.0, 0.0 );

		cManipulator->rotate( a2, 1, 0, 0 );
		cManipulator->translate( 0.0, d2, 0.0 );

		cManipulator->rotate( a3, 0, 0, 1 );
		cManipulator->translate( 0.0, 0.0, d3 );

		a1 = fmod( (a1+av1), 360 );
		a2 = fmod( (a2+av2), 360 );
		a3 = fmod( (a3+av3), 360 );

		//cManipulator->getMatrix( pathWriteBuffer );
		cManipulator->getPosition( tempPos );
		pathWriteBuffer[0] = tempPos[0];
		pathWriteBuffer[1] = tempPos[1];
		pathWriteBuffer[2] = tempPos[2];

		//path->write( pathWriteBuffer );

		//pathLength++;

		if( pathLength > 0 ){
				path->read(pathReadBuffer1, -1);
				if( calcDistance(pathBuffer3[12],pathBuffer3[13],pathBuffer3[14], pathReadBuffer1[0], pathReadBuffer1[1], pathReadBuffer1[2]) > minDistance ){
					//insert();
					path->write( pathWriteBuffer );
					pathLength++;
				}
				if( pathLength > 2 ) {
					path->read( pathReadBuffer2, -3 );

					vecA[0] = pathReadBuffer2[0] - pathReadBuffer1[0];
					vecA[1] = pathReadBuffer2[1] - pathReadBuffer1[1];
					vecA[2] = pathReadBuffer2[2] - pathReadBuffer1[2];

					normA = sqrt(vecA[0]*vecA[0]+vecA[1]*vecA[1]+vecA[2]*vecA[2]);

					vecA[0] = vecA[0]/normA;
					vecA[1] = vecA[1]/normA;
					vecA[2] = vecA[2]/normA;

					// calculate and normalize b
					vecB[0] = pathReadBuffer1[0] - pathBuffer3[0];
					vecB[1] = pathReadBuffer1[1] - pathBuffer3[1];
					vecB[2] = pathReadBuffer1[2] - pathBuffer3[2];

					normB = sqrt(vecB[0]*vecB[0]+vecB[1]*vecB[1]+vecB[2]*vecB[2]);

					vecB[0] = vecB[0]/normB;
					vecB[1] = vecB[1]/normB;
					vecB[2] = vecB[2]/normB;

					// calculate c
					vecC[0] = (vecA[0]+vecB[0])/2;
					vecC[1] = (vecA[1]+vecB[1])/2;
					vecC[2] = (vecA[2]+vecB[2])/2;

					pathWriteBuffer[0] = pathReadBuffer1[0];
					pathWriteBuffer[1] = pathReadBuffer1[1];
					pathWriteBuffer[2] = pathReadBuffer1[2];
					pathWriteBuffer[3] = vecC[0];
					pathWriteBuffer[4] = vecC[1];
					pathWriteBuffer[5] = vecC[2];

					path->write( pathWriteBuffer, -2 );

				}

			}else{
				//insert();
				path->write( pathWriteBuffer );
				pathLength++;
			}
	}

	glPopMatrix();

}


void Spirograph::calculateNewPosition() {
	glPushMatrix();
	glLoadIdentity();

	glRotatef(a1,0,1,0);
	glTranslatef(d1,0,0);
	glGetFloatv(GL_MODELVIEW_MATRIX, pathBuffer1);

	glPushMatrix();

	glRotatef(a2,1,0,0);
	glTranslatef(0,d2,0);
	glGetFloatv(GL_MODELVIEW_MATRIX, pathBuffer2);

	glPushMatrix();

	glRotatef(a3,0,0,1);
	glTranslatef(0,0,d3);
	glGetFloatv(GL_MODELVIEW_MATRIX, pathBuffer3);

	glPopMatrix();
	glPopMatrix();
	glPopMatrix();

	a1 = fmod( (a1+av1), 360 );
	a2 = fmod( (a2+av2), 360 );
	a3 = fmod( (a3+av3), 360 );

	if( pathLength > 0 ){
		path->read(pathReadBuffer1, -1);
		if( calcDistance(pathBuffer3[12],pathBuffer3[13],pathBuffer3[14], pathReadBuffer1[0], pathReadBuffer1[1], pathReadBuffer1[2]) > minDistance ){
			insert();
//			path->write( pathWriteBuffer );
//			pathLength++;
		}
		if( pathLength > 2 ) {
			path->read( pathReadBuffer2, -3 );

			vecA[0] = pathReadBuffer2[0] - pathReadBuffer1[0];
			vecA[1] = pathReadBuffer2[1] - pathReadBuffer1[1];
			vecA[2] = pathReadBuffer2[2] - pathReadBuffer1[2];

			normA = sqrt(vecA[0]*vecA[0]+vecA[1]*vecA[1]+vecA[2]*vecA[2]);

			vecA[0] = vecA[0]/normA;
			vecA[1] = vecA[1]/normA;
			vecA[2] = vecA[2]/normA;

			// calculate and normalize b
			vecB[0] = pathReadBuffer1[0] - pathBuffer3[0];
			vecB[1] = pathReadBuffer1[1] - pathBuffer3[1];
			vecB[2] = pathReadBuffer1[2] - pathBuffer3[2];

			normB = sqrt(vecB[0]*vecB[0]+vecB[1]*vecB[1]+vecB[2]*vecB[2]);

			vecB[0] = vecB[0]/normB;
			vecB[1] = vecB[1]/normB;
			vecB[2] = vecB[2]/normB;

			// calculate c
			vecC[0] = (vecA[0]+vecB[0])/2;
			vecC[1] = (vecA[1]+vecB[1])/2;
			vecC[2] = (vecA[2]+vecB[2])/2;

			pathWriteBuffer[0] = pathReadBuffer1[0];
			pathWriteBuffer[1] = pathReadBuffer1[1];
			pathWriteBuffer[2] = pathReadBuffer1[2];
			pathWriteBuffer[3] = vecC[0];
			pathWriteBuffer[4] = vecC[1];
			pathWriteBuffer[5] = vecC[2];

			path->write( pathWriteBuffer, -2 );
		}

	}else{
		insert();
//		path->write( pathWriteBuffer );
//		pathLength++;
	}

}

void Spirograph::insert() {
	pathWriteBuffer[0] = pathBuffer3[12];
	pathWriteBuffer[1] = pathBuffer3[13];
	pathWriteBuffer[2] = pathBuffer3[14];

	path->write( pathWriteBuffer );
	pathLength++;
}

void Spirograph::drawArm() {
	glPushMatrix();

	glColor4f(0.5, 0.6, 0.7, 0.9);
	glBegin(GL_LINE_STRIP);

	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(
		pathBuffer1[12],
		pathBuffer1[13],
		pathBuffer1[14]
	);
	glVertex3f(
		pathBuffer2[12],
		pathBuffer2[13],
		pathBuffer2[14]
	);
	glVertex3f(
		pathBuffer3[12],
		pathBuffer3[13],
		pathBuffer3[14]
	);

	glEnd();
	glPopMatrix();


}

void Spirograph::drawPath( float spectrum[], int spectrumStart, int spectrumEnd ) {

	float specPerSegment = (spectrumEnd-spectrumStart)/(float)length;
	float *spectrumPart = new float[ length ];

	float tempSum;

	for( int i=0; i<length; i++ ){
		tempSum = 0;
		int start = spectrumStart + round(i*specPerSegment);
		int end =  spectrumStart + round((i+1)*specPerSegment);

		for( int j=start; j<end; j++ ){
			tempSum += spectrum[j];
		}

		spectrumPart[i] = tempSum/(end-start);

	}

	float avgSpec= 0.0;
	for( int i=spectrumStart; i<spectrumEnd; i++ ){
		avgSpec += spectrum[i];
	}

	avgSpec = avgSpec/(spectrumEnd-spectrumStart);

	GLfloat mat_shininess[] = {100.0};

//	GLfloat ambientColor[3] ={0.5,0.5,0.5};
//	GLfloat diffuseColor[3] ={0.2,1.0-avgSpec,avgSpec};
//	GLfloat specularColor[3]={1.0-avgSpec,0.1,avgSpec};
//	glMaterialfv (GL_FRONT, GL_AMBIENT, ambientColor);
//	glMaterialfv (GL_FRONT, GL_DIFFUSE, diffuseColor);
//	glMaterialfv (GL_FRONT, GL_SPECULAR, specularColor);
	glMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess);
	colors->setMaterial(GL_FRONT, GL_AMBIENT);
	colors->setMaterial(GL_FRONT, GL_DIFFUSE);
	colors->setMaterial(GL_FRONT, GL_SPECULAR);


	//glColor4f(0.2, avgSpec, 1.0-avgSpec, 0.8);

	glPushMatrix();
	glDisable(GL_CULL_FACE);

//	glRotatef(rot,0,1,0);
//	rot = rot+0.1;
//	rot = rot>360.0f?rot-360.0f:rot;

	glBegin(GL_QUAD_STRIP);
	GLfloat vertA[3], vertB[3], vertC[3];
	//GLfloat normalX, normalY, normalZ;
	float normal[3];

	// drawing first part of path outside for-loop, because of different calculation of normal vector
	path->read( pathReadBuffer1 );

	vertA[0] = distanceMod( pathReadBuffer1[0] - spectrumPart[ length ], 0);
	vertA[1] = 				pathReadBuffer1[1] - spectrumPart[ length ];
  	vertA[2] = distanceMod( pathReadBuffer1[2] - spectrumPart[ length ], 0);

	vertB[0] = distanceMod( 0.5+ pathReadBuffer1[0] + spectrumPart[ length ], 0);
	vertB[1] = 				0.5+ pathReadBuffer1[1] + spectrumPart[ length ];
  	vertB[2] = distanceMod( 0.5+ pathReadBuffer1[2] + spectrumPart[ length ], 0);

  	// read current position, but dont increase read-pointer
  	path->read( pathReadBuffer1, 0 );

//	vertC[0] = distanceMod( 0.5+ pathReadBuffer1[0] + spectrumPart[ length-1 ], 0);
//	vertC[1] = 				0.5+ pathReadBuffer1[1] + spectrumPart[ length-1 ];
//  vertC[2] = distanceMod( 0.5+ pathReadBuffer1[2] + spectrumPart[ length-1 ], 0);

  	calcNV( vertA, vertB, vertC, normal );
  	glNormal3fv( normal );

  	// FIXME
//	glVertex3f( vertB[0], vertB[1], vertB[2] );
//	glVertex3f( vertA[0], vertA[1], vertA[2] );

	vertC[0] = vertA[0];
	vertC[1] = vertA[1];
	vertC[2] = vertA[2];

	for( int i=1; i<length; i++ ) {
		int fixedSpectrumPartIndex = length - i;
		path->read( pathReadBuffer1 );

		vertA[0] = distanceMod( 0.5 + pathReadBuffer1[0] + spectrumPart[ fixedSpectrumPartIndex ], i);
		vertA[1] = 				0.5 + pathReadBuffer1[1] + spectrumPart[ fixedSpectrumPartIndex ];
	  	vertA[2] = distanceMod( 0.5 + pathReadBuffer1[2] + spectrumPart[ fixedSpectrumPartIndex ], i);

	  	vertB[0] = distanceMod( pathReadBuffer1[0] - spectrumPart[ fixedSpectrumPartIndex ], i);
		vertB[1] = 				pathReadBuffer1[1] - spectrumPart[ fixedSpectrumPartIndex ];
	  	vertB[2] = distanceMod( pathReadBuffer1[2] - spectrumPart[ fixedSpectrumPartIndex ], i);

	  	calcNV( vertA, vertB, vertC, normal );
	  	glNormal3fv( normal);

		glVertex3f( vertA[0], vertA[1], vertA[2] );
		glVertex3f( vertB[0], vertB[1], vertB[2] );

		vertC[0] = vertB[0];
		vertC[1] = vertB[1];
		vertC[2] = vertB[2];

	}
	glEnd();

	glEnable(GL_CULL_FACE);

	glPopMatrix();

	delete[] spectrumPart;

}
