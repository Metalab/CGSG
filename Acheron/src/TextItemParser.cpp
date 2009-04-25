/*
 * TextItemParser.cpp
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#include "TextItemParser.h"
#include "effects/Text.h"

#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

//TextItemParser::TextItemParser() {
//	// TODO Auto-generated constructor stub
//
//}
//
//TextItemParser::~TextItemParser() {
//	// TODO Auto-generated destructor stub
//}

AnimateableObject* TextItemParser::ReadObject(int startTicksAbsolute, int duration, stringstream &params) {
	string text;

	float start[3];
	float end[3];

	params >> start[0] >> start[1] >> start[2];
	params >> end[0] >> end[1] >> end[2];

	getline(params, text);
	return new Text(*font, text.c_str(), start, end, startTicksAbsolute, duration);

}
