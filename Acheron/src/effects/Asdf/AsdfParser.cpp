/*
 * AsdfParser.cpp
 *
 *  Created on: May 08,2009
 *      Author: scriptythekid
 */

#include "AsdfParser.h"
#include "Asdf.h"
#include "iostream"

using namespace asdfns;

AnimateableObject* AsdfParser::ReadObject( int startTickAbsolute, int duration,
											std::stringstream &additionalParameters ) {

	std::string photoFilename;

	float start[3];
	float end[3];
  float buildingheight;
  int   fadeintime;
  
	additionalParameters >> start[0] >> start[1] >> start[2];
	additionalParameters >> end[0] >> end[1] >> end[2];
	additionalParameters >> photoFilename;
	additionalParameters >> buildingheight;
	additionalParameters >> fadeintime;
  printf("photo: %s\n",photoFilename.c_str());
	return new Asdf( start, end, photoFilename.c_str(), startTickAbsolute, duration);

}

