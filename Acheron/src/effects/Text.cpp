/*
 * Text.cpp
 *
 *  Created on: Apr 25, 2009
 *      Author: meta, cygen
 */

#include "Text.h"

#include <GL/glew.h>

#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif

Text::~Text() {
	// TODO Auto-generated destructor stub
}

Text::Text(Font &theFont, const char *text, float *startPos, float *endPos, int startTick, int duration)
	:LinearInterpolatedAnimateable(startPos, endPos, startTick, duration), points()
{
	int chars = strlen(text);
	int x=0;

	for (int i=0; i < chars; i++)
	{
		int maxx = x;
		vector<Point<int> > charPoints = theFont.GetChar(text[i]);
		for (vector<Point<int> >::iterator iter = charPoints.begin(); iter != charPoints.end(); iter++) {
			int px = x + iter->x;
			points.push_back(Point<float>(px*1.3f, iter->y*1.3f));
			if (maxx < px) maxx = px;
		}
		x = maxx+2;
	}
	this->maxx = x;

}

void Text::DrawAtPosition(float* position, float factor, int tick) {
	glPushMatrix();
	glTranslatef(position[0], position[1], position[2]);
	float wobbleFact = factor * 1000;

	for (vector<Point<float> >::iterator iter = points.begin(); iter != points.end(); iter++)
		//drawCube(0, sin(factor*60), 0);
		DrawCube(iter->x + 0.4f*sin(iter->y+wobbleFact+0.1f*position[1]),
			-iter->y + 0.6f*sin(position[0]+iter->x),
			0);
	glPopMatrix();
}

void Text::DrawCube(float x, float y, float z) {
	glPushMatrix();
	glTranslatef(x, y, z);

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
