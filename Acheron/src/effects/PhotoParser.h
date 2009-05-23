/*
 * PhotoParser.h
 *
 *  Created on: Apr 28, 2009
 *      Author: max
 */

#ifndef PHOTOPARSER_H_
#define PHOTOPARSER_H_

#include "../TimelineItemParser.h"
#include "../AnimateableObject.h"

namespace photo {

	class PhotoParser : public TimelineItemParser {

		public:
			virtual AnimateableObject* ReadObject( int startTickAbsolute, int duration, std::stringstream &additionalParameters );
	};

}

#endif /* PHOTOPARSER_H_ */
