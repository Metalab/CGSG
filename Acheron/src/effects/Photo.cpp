/*
 * Photo.cpp
 *
 *  Created on: Apr 29, 2009
 *      Author: max
 */

#include "Photo.h"

//#include <GL/glew.h>

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
	this->photoTexture = new  Texture( this->photoFilename );
	this->photoTexture->load();

	this->thickness = 0.5;

	createVertices();

}

void Photo::createVertices() {
		float x = 1024/100; // IMG_W
		float y = 768/100; // IMG_H
		float z = thickness;

		//
		v1[0] = -x ;
		v1[1] = -y ;
		v1[2] =  z ;

		//
		v2[0] =  x ;
		v2[1] = -y ;
		v2[2] =  z ;

		//
		v3[0] =  x ;
		v3[1] =  y ;
		v3[2] =  z ;

		//
		v4[0] = -x ;
		v4[1] =  y ;
		v4[2] =  z ;

		//
		v5[0] =  x ;
		v5[1] = -y ;
		v5[2] = -z ;

		//
		v6[0] = -x ;
		v6[1] = -y ;
		v6[2] = -z ;

		//
		v7[0] = -x ;
		v7[1] =  y ;
		v7[2] = -z ;

		//
		v8[0] =  x ;
		v8[1] =  y ;
		v8[2] = -z ;

}

void Photo::DrawAtPosition( float* position, float factor, int tick ) {

	glPushMatrix();

	glTranslatef( position[0], position[1], position[2] );

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D, photoTexture->id);



glColor3f( 1.0, 0.0, 0.0 );
	glBegin(GL_QUADS);

	glTexCoord2f(0,0); glVertex3fv(v1);
	glTexCoord2f(1,0); glVertex3fv(v2);
	glTexCoord2f(1,1); glVertex3fv(v3);
	glTexCoord2f(0,1); glVertex3fv(v4);

	glEnd();

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();


}
