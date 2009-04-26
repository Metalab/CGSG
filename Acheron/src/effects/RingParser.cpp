/*
 * RingParser.cpp
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#include "RingParser.h"
#include "ObjectContainer.h"
#include "Ring.h"
#include "iostream.h"

using namespace multiKa;

AnimateableObject* RingParser::ReadObject(int startTickAbsolute, int duration, std::stringstream &additionalParameters) {

	int ringCount;
	int ringSegments;
	float ringAngle;

	ObjectContainer ringContainer;

	additionalParameters >> ringCount >> ringSegments >> ringAngle ;

	for( int i=0; i<ringCount; i++ ) {
		ringContainer.addObject( new Ring(5, 4, ringSegments, 5, ringAngle) );
cout << "bar";
	}
	ringContainer.prepareAudio();

	return 0;
}
