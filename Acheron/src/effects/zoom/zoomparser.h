

#ifndef ZOOMPARSER_H
#define ZOOMPARSER_H

#include "../../TimelineItemParser.h"
#include "../../AnimateableObject.h"

namespace zoom {
	class ZoomParser : public TimelineItemParser {
	public:
		virtual AnimateableObject* ReadObject(int startTickAbsolute, int duration, std::stringstream &additionalParameters);
	};
}

#endif
