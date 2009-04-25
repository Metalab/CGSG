/*
 * TimelineReader.h
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#ifndef TIMELINEREADER_H_
#define TIMELINEREADER_H_

#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <sstream>

#include "AnimateableObject.h"
#include "TimelineItemParser.h"

class TimelineReader {
	private:
		typedef std::map<const std::string, TimelineItemParser *> ParserMap;
		typedef ParserMap::iterator ParserIter;

		ParserMap itemParsers;
		std::ifstream inputStream;
		AnimateableObject* nextObject;
		int nextObjectTicks;

		AnimateableObject* ParseObject();

	public:
		TimelineReader() { nextObject = 0; nextObjectTicks=0; }

		void SetParser(const std::string &elementName, TimelineItemParser *parser);
		void Open(const std::string &filename);
		AnimateableObject* GetNextObject(int curentTick, int lookaheadTicks);
		bool IsAtEnd();

		~TimelineReader();

};

#endif /* TIMELINEREADER_H_ */
