

#ifndef ZOOMPARSER_H 
#define ZOOMPARSER_H

#include "../../timelineItemParser.h"
#include "../../AnimateableObject.h"

namespace zoom {
	class ZoomParser : public TimelineItemParser {
	public:
		virtual AnimateableObject* ReadObject(int startTickAbsolute, int duration, std::stringstream &additionalParameters);
	};
}

#endif
