/*
 * TimelineReader.cpp
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#include "TimelineReader.h"
#include "TimelineItemParser.h"
#include "TimelineParseException.h"
#include "misc.h"

#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <sstream>


bool TimelineReader::IsAtEnd() {
	return !nextObject;
}


void TimelineReader::SetParser(const std::string &elementName, TimelineItemParser *parser) {
	//delete any previous parser that was associated with this string
	ParserIter prev = itemParsers.find(elementName);
	if (prev != itemParsers.end()) {
		delete prev->second;
	}

	//add the parser to our parser list
	this->itemParsers[elementName] = parser;
}

void TimelineReader::Open(const std::string &filename) {
	//open the stream for writing
	inputStream.open(filename.c_str());
	if (inputStream.fail())
		throw TimelineParseException();

	//read the first object
	nextObject = ParseObject();
}

AnimateableObject* TimelineReader::ParseObject() {
	std::string line;

	//parse the line and create the appropriate AnimatableObject
	std::stringstream s;
	std::string name;

	do {
		if (inputStream.eof()) return 0;

		getline(inputStream, line);

		if (inputStream.fail())
			throw TimelineParseException();
		s.str(line);
		s.seekg(0, std::ios::beg);
		s >> name;

	} while(  name.length() == 0 || name[0] == '#' );
	int start, duration;
	s >> start;
	s >> duration;

	//convert start from relative to absolute and increment nextObjectTicks
	nextObjectTicks += start;
	start = nextObjectTicks;

	//see if we have an appropriate parser
	ParserIter parserIter = itemParsers.find(name);
	if (parserIter == itemParsers.end())
		//throw an appropriate exception
		throw TimelineParseException();

	return parserIter->second->ReadObject(start, duration, s);
}

AnimateableObject *TimelineReader::GetNextObject(int currentTick, int lookaheadTicks) {
	if (!nextObject) return 0;

	if (nextObject->GetStartTick() <= currentTick + lookaheadTicks) {
		AnimateableObject *obj = nextObject;
		nextObject = ParseObject();
		return obj;
	}

	return 0;
}

TimelineReader::~TimelineReader() {
	//delete all our parsers
	for_each(itemParsers.begin(), itemParsers.end(), deleteSecond<const std::string, TimelineItemParser>());

	//delete the next item, if we have one
	if (nextObject != 0) delete nextObject;

	//close the input stream
	inputStream.close();
}
