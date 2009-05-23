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
												std::stringstream &additionalParameters )
{

	ObjectContainer* spirographContainer = new ObjectContainer( startTickAbsolute, duration );

	float av1, d1, av2, d2, av3, d3;
	int length;

	additionalParameters >> av1 >> d1 >> av2 >> d2 >> av3 >> d3 >> length;
	spirographContainer->addObject( new Spirograph( av1, d1, av2, d2, av3, d3, length ) );


	spirographContainer->prepareAudio();
	return spirographContainer;

}
