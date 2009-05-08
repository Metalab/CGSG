/*
 * RingParser.cpp
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#include "RingParser.h"
#include "ObjectContainer.h"
#include "Ring.h"
#include "iostream"

using namespace multiKa;

AnimateableObject* RingParser::ReadObject(int startTickAbsolute, int duration,
											std::stringstream &additionalParameters) {

	int ringCount;
	int ringSegments;
	float ringAngle;

	ObjectContainer *ringContainer = new ObjectContainer(startTickAbsolute, duration);

	additionalParameters >> ringCount >> ringSegments >> ringAngle ;

	for( int i=0; i<ringCount; i++ ) {
		ringContainer->addObject( new Ring(5-i*0.3, 0.5, ringSegments, 0.5, ringAngle) );
	}
	ringContainer->prepareAudio();

	return ringContainer;
}
