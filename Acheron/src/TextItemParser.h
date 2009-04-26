/*
 * TextItemParser.h
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#ifndef TEXTITEMPARSER_H_
#define TEXTITEMPARSER_H_

#include <string>
#include "font.h"


#include "TimelineItemParser.h"
#include "AnimateableObject.h"

class TextItemParser: public TimelineItemParser {
	public:
		TextItemParser(Font *font) { this->font = font; }
		virtual AnimateableObject* ReadObject(int startTicksAbsolute, int duration, std::stringstream &params);
		virtual ~TextItemParser() { delete font; }

	private:
		Font *font;
};

#endif /* TEXTITEMPARSER_H_ */
