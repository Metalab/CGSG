/*
 * SpirographParser.cpp
 *
 *  Created on: May 21, 2009
 *      Author: max
 */

#define NOMINMAX

#include "SpirographParser.h"
#include "Spirograph.h"
#include "ObjectContainer.h"

using namespace multiKa;

AnimateableObject* SpirographParser::ReadObject( int startTickAbsolute, int duration,
												std::stringstream &additionalParameters ) {

	ObjectContainer* spirographContainer = new ObjectContainer( startTickAbsolute, duration );

	spirographContainer->addObject( new Spirograph( 1, 10, 1, 10, 1, 10, 500 ) );


	spirographContainer->prepareAudio();
	return spirographContainer;

}
