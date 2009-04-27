/*
 * ObjectContainer.cpp
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#include "ObjectContainer.h"
#include "iostream.h"

using namespace multiKa;

bool ObjectContainer::Draw( int ticks, Context* context ){

	glPushMatrix();
glTranslatef(0,0,-20);
	spectrumPointer = context->getAudio()->getLogSpectrum( &spectrumSize );

	int startSpectrum = 0;
	int endSpectrum = startSpectrum + spectrumPerObject;

	list<Base3DObject*>::iterator i;
	for( i=myObjects.begin(); i!=myObjects.end(); i++ ){
		(*i)->draw(spectrumPointer, startSpectrum, endSpectrum);

		startSpectrum += spectrumPerObject;
		endSpectrum += spectrumPerObject;

	}

	glPopMatrix();

	return ticks < startTick+duration;
}
