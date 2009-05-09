/*
 * Ring.cpp
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#include <GL/glew.h>

#include "Ring.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "../extramath.h"
#include "iostream"

using namespace multiKa;

Ring::Ring( float radius, float width, int segments, float rSpeed, float rDiff )
: Base3DObject(  0.0,  0.0,  0.0 )
//: Base3DObject(  posX,  posY,  posZ )
{
	this->radius = radius;
	this->width = width;
	this->segments = segments;

	this->rSpeed = rSpeed;
	this->rPos = 0;
	this->rDiff = rDiff;

	this->colors = new HSBColors(6);

	this->createVertices();

}

void Ring::createVertices(){
	// calculate vertices by converting polar coordinates to carthesian
	float phi = (float)((2*M_PI)/segments);

	vertices = new float[segments * 2];

	for(int i = 0; i<segments; i++){
		vertices[2*i] = radius*sin(phi*i); // x
		vertices[2*i+1] = radius*cos(phi*i); // y
	}
}
//void Ring::draw( float spectrum, int spectrumStart, int spectrumEnd ) {}

void Ring::draw( float spectrum[], int spectrumStart, int spectrumEnd) {
	colors->update();

	float specPerSegment = (spectrumEnd-spectrumStart)/(float)segments;
	float *spectrumPart = new float[segments];

	float tempSum;

	for( int i=0; i<segments; i++ ){
		tempSum = 0;
		int start = spectrumStart + (int)round(i*specPerSegment);
		int end =  spectrumStart + (int)round((i+1)*specPerSegment);

		for( int j=start; j<end; j++ ){
			tempSum += spectrum[j];
		}

		spectrumPart[i] = tempSum/(end-start);

	}

	float avgSpec= 0;
	for( int i=spectrumStart; i<spectrumEnd; i++ ){
		avgSpec += spectrum[i];
	}

	avgSpec = avgSpec/(spectrumEnd-spectrumStart);

	glRotatef(rPos,0,1,0);
	rPos = rPos + rSpeed*avgSpec*5;
	if(rPos > 360.0f) {
		rPos = rPos-360.0f;
	}

	// define material
	GLfloat mat_shininess[] = {50.0};

	glMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess);
	colors->setMaterial(GL_FRONT, GL_AMBIENT);
	colors->setMaterial(GL_FRONT, GL_DIFFUSE);
	colors->setMaterial(GL_FRONT, GL_SPECULAR);

	glEnable(GL_NORMALIZE);

	glBegin(GL_TRIANGLE_STRIP);

	for(int i = 0; i<(segments); i++){
		glNormal3f(vertices[2*i],vertices[2*i+1],-(width+spectrumPart[i]));
		glVertex3f(vertices[2*i],vertices[2*i+1],-(width+spectrumPart[i]));
		glNormal3f(vertices[2*i],vertices[2*i+1], (width+spectrumPart[i]));
		glVertex3f(vertices[2*i],vertices[2*i+1], (width+spectrumPart[i]));
	}
	glNormal3f(vertices[0],vertices[1],-(width+spectrumPart[0]));
	glVertex3f(vertices[0],vertices[1],-(width+spectrumPart[0]));
	glNormal3f(vertices[0],vertices[1], (width+spectrumPart[0]));
	glVertex3f(vertices[0],vertices[1], (width+spectrumPart[0]));

	glEnd();

	// draw back side with different material

	glMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess);
	colors->setMaterial(GL_FRONT, GL_AMBIENT);
	colors->setMaterial(GL_FRONT, GL_DIFFUSE);
	colors->setMaterial(GL_FRONT, GL_SPECULAR);

	glBegin(GL_TRIANGLE_STRIP);

	for(int i = 0; i<(segments); i++){
		glNormal3f(-vertices[2*i],-vertices[2*i+1],-(width+spectrumPart[i]));
		glVertex3f( vertices[2*i], vertices[2*i+1], (width+spectrumPart[i]));
		glNormal3f(-vertices[2*i],-vertices[2*i+1], (width+spectrumPart[i]));
		glVertex3f( vertices[2*i], vertices[2*i+1],-(width+spectrumPart[i]));
	}
	glNormal3f(-vertices[0],-vertices[1],-(width+spectrumPart[0]));
	glVertex3f( vertices[0], vertices[1], (width+spectrumPart[0]));
	glNormal3f(-vertices[0],-vertices[1], (width+spectrumPart[0]));
	glVertex3f( vertices[0], vertices[1],-(width+spectrumPart[0]));

	glEnd();
	glRotatef(rDiff,0,0,1);

	delete [] spectrumPart;
}
