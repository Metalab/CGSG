/*
 * Photo.cpp
 *
 *  Created on: Apr 29, 2009
 *      Author: max
 */

#include "Photo.h"

#include <GL/glew.h>

#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif
#include <iostream>
using namespace multiKa;

Photo::Photo( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration )
	: LinearInterpolatedAnimateable( startPos, endPos, startTick, duration )
{
	this->photoFilename = photoFilename;
	std::cout << "PHOTO constr" << std::endl;

}

void Photo::DrawAtPosition( float* position, float factor, int tick ) {

	glPushMatrix();

	glTranslatef( position[0], position[1], position[2] );
glColor3f( 1.0, 0.0, 0.0 );
	glBegin(GL_QUADS);
		// front
		glNormal3f(0, 0, 1);
		glVertex3f(0, 0, 1);
		glVertex3f(1, 0, 1);
		glVertex3f(1, 1, 1);
		glVertex3f(0, 1, 1);

		// back
		glNormal3f(0, 0, 0);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 1, 0);
		glVertex3f( 1, 1, 0);
		glVertex3f( 1, 0, 0);

		// left
		glNormal3f(0, 0, 0);
		glVertex3f(0, 0,  1);
		glVertex3f(0, 1,  1);
		glVertex3f(0, 1, 0);
		glVertex3f(0, 0, 0);

		// right
		glNormal3f(1, 0, 0);
		glVertex3f(1, 0,  1);
		glVertex3f(1, 0, 0);
		glVertex3f(1, 1, 0);
		glVertex3f(1, 1,  1);

		// top
		glNormal3f(0, 1, 0);
		glVertex3f(0, 1,  1);
		glVertex3f(1, 1,  1);
		glVertex3f(1, 1, 0);
		glVertex3f(0, 1, 0);

		// bottom
		glNormal3f(0, 0, 0);
		glVertex3f(1, 0,  1);
		glVertex3f(0, 0,  1);
		glVertex3f(0, 0, 0);
		glVertex3f(1, 0, 0);

	glEnd();

	glPopMatrix();

}
