/*
 * SpirographParser.h
 *
 *  Created on: May 21, 2009
 *      Author: max
 */

#ifndef SPIROGRAPHPARSER_H_
#define SPIROGRAPHPARSER_H_

#include "../../TimelineItemParser.h"
#include "../../LinearInterpolatedAnimateable.h"

namespace multiKa {

	class SpirographParser : public TimelineItemParser {
	public:
		virtual AnimateableObject* ReadObject( int startTickAbsolute, int duration, std::stringstream &additionalParameters );
	};

}

#endif /* SPIROGRAPHPARSER_H_ */
