/*
 * PhotoParser.cpp
 *
 *  Created on: Apr 28, 2009
 *      Author: max
 */

#include "PhotoParser.h"
#include "Photo.h"
#include "iostream"

using namespace photo;

AnimateableObject* PhotoParser::ReadObject( int startTickAbsolute, int duration,
											std::stringstream &additionalParameters ) {

	std::string photoFilename;

	float start[3];
	float end[3];
	float av;

	additionalParameters >> start[0] >> start[1] >> start[2];
	additionalParameters >> end[0] >> end[1] >> end[2];
	additionalParameters >> av;
	additionalParameters >> photoFilename;

	return new Photo( start, end, av, photoFilename.c_str(), startTickAbsolute, duration );

}

