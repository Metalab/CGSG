#include "zoomparser.h"
#include "zoom.h"
#include <iostream>
#include <string>

AnimateableObject* zoom::ZoomParser::ReadObject(int startTickAbsolute, int duration, std::stringstream &additionalParameters) {
	int targetResolution;
	int startLevel;
	int endLevel;
	std::string directoryName;

	additionalParameters >> targetResolution;
	additionalParameters >> directoryName;
	additionalParameters >> startLevel >> endLevel;

	return new zoom::ZoomEffect(startTickAbsolute, duration, targetResolution, directoryName, startLevel, endLevel);
}