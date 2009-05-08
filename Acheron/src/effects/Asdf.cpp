/*
 * Asdf.cpp
 *
 *  Created on: May 08,2009
 *      Author: scriptythekid
 */

#include "Asdf.h"

using namespace beidlpracker;
 
Asdf::Asdf( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration )
  : LinearInterpolatedAnimateable( startPos, endPos, startTick, duration )
{
  
  
}

void Asdf::createVertices() {
  
}

void Asdf::DrawAtPosition( float* position, float factor, int tick ) {
  
}