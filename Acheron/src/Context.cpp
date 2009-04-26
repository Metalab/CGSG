/*
 * Context.cpp
 *
 *  Created on: Apr 25, 2009
 *      Author: max
 */

#include "Context.h"

Context::Context() {
	audioStuff =  new AudioStuff();
}

Context::~Context() {
	delete audioStuff;
}
