/*
 * RingParser.h
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#ifndef RINGPARSER_H_
#define RINGPARSER_H_

#include "../../TimelineItemParser.h"
#include "../../AnimateableObject.h"

namespace multiKa {

	class RingParser : public TimelineItemParser {

		public:
			virtual AnimateableObject* ReadObject( int startTickAbsolute, int duration, std::stringstream &additionalParameters );
		private:


	};

}

#endif /* RINGPARSER_H_ */
