/*
 * TimelineItemParser.h
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#ifndef TIMELINEITEMPARSER_H_
#define TIMELINEITEMPARSER_H_

#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <iostream>

#include "AnimateableObject.h"

class TimelineItemParser {
	public:
		virtual ~TimelineItemParser() {}

		virtual AnimateableObject* ReadObject( int startTickAbsolute, int duration, std::stringstream &additionalParameters) = 0;

};

#endif /* TIMELINEITEMPARSER_H_ */
