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
#include "../misc.h"

using namespace multiKa;

Photo::Photo( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration )
	: LinearInterpolatedAnimateable( startPos, endPos, startTick, duration )
{
	this->photoFilename = photoFilename;
	this->photoTexture = new  Texture( this->photoFilename );
	this->photoTexture->load();

	this->thickness = 0.1;

	this->rPos = 0.0;

	this->colors = new HSBColors(1);

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
	float normal[3];

	colors->update();

	glPushMatrix();

	glTranslatef( position[0], position[1], position[2] );

	glRotatef( rPos, 0,1,0 );
	rPos = rPos+0.2;
	rPos = rPos>360.0 ? rPos-360.0 : rPos;

	GLfloat mat_shininess[] = {50.0};
	glMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess);
	colors->setMaterial(GL_FRONT, GL_AMBIENT);
	colors->setMaterial(GL_FRONT, GL_DIFFUSE);
	colors->setMaterial(GL_FRONT, GL_SPECULAR);


	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D, photoTexture->id);

	// front
	calcNV( v1, v2, v3, normal );
	glNormal3fv( normal );

	glBegin(GL_QUADS);
	glTexCoord2f(0,0); glVertex3fv(v1);
	glTexCoord2f(1,0); glVertex3fv(v2);
	glTexCoord2f(1,1); glVertex3fv(v3);
	glTexCoord2f(0,1); glVertex3fv(v4);
	glEnd();

	//back
	calcNV( v5, v6, v7, normal );
	glBegin(GL_QUADS);
	glTexCoord2f(1,0); glVertex3fv(v5);
	glTexCoord2f(0,0); glVertex3fv(v6);
	glTexCoord2f(0,1); glVertex3fv(v7);
	glTexCoord2f(1,1); glVertex3fv(v8);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	//top
	calcNV( v4, v3, v8, normal );
	glNormal3fv( normal );
	glVertex3fv(v4);
	glVertex3fv(v3);
	glVertex3fv(v8);
	glVertex3fv(v7);

	//right
	calcNV( v2, v5, v8, normal );
	glNormal3fv( normal );
	glVertex3fv(v2);
	glVertex3fv(v5);
	glVertex3fv(v8);
	glVertex3fv(v3);

	//bottom
	calcNV( v1, v6, v5, normal );
	glNormal3fv( normal );
	glVertex3fv(v1);
	glVertex3fv(v6);
	glVertex3fv(v5);
	glVertex3fv(v2);

	//left
	calcNV( v6, v1, v4, normal );
	glNormal3fv( normal );
	glVertex3fv(v6);
	glVertex3fv(v1);
	glVertex3fv(v4);
	glVertex3fv(v7);

	glEnd();


	glPopMatrix();


}
