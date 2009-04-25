/*
 * TimelineParseException.h
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#ifndef TIMELINEPARSEEXCEPTION_H_
#define TIMELINEPARSEEXCEPTION_H_

#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <sstream>

class TimelineParseException : public std::exception {
	public:
		//TimelineParseException() : exception("Timeline parse error") { }
		TimelineParseException() : exception() {
		};

		virtual const char* what() {
			return "Timeline parse error";
		}
};

#endif /* TIMELINEPARSEEXCEPTION_H_ */
