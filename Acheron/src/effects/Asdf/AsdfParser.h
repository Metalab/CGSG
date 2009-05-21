/*
 * AsdfParser.h
 *
 *  Created on: May 08,2009
 *      Author: scriptythekid
 */
#ifndef ASDFPARSER_H_
#define ASDFPARSER_H_

#include "../../TimelineItemParser.h"
#include "../../AnimateableObject.h"

namespace asdfns {

	class AsdfParser : public TimelineItemParser {

		public:
			virtual AnimateableObject* ReadObject( int startTickAbsolute, int duration, std::stringstream &additionalParameters );
	};

}

#endif /* ASDFPARSER_H_ */
