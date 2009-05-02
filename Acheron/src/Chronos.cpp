/*
 * Chronos.cpp
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#include "Chronos.h"
#include <iostream>

//Chronos::Chronos() : objects() {
//}

void Chronos::AddObject (AnimateableObject *object)
{
	objects.push_back(object);
}

void Chronos::Draw(int tick) {
	std::list<AnimateableObject*>::iterator iter = objects.begin();
	std::list<AnimateableObject*>::iterator end = objects.end();
	while (iter != end) {
		AnimateableObject *current = (*iter);
		bool keep = current->Draw( tick, context );
		if (!keep) {
			std::list<AnimateableObject*>::iterator old = iter;
			iter++;
			objects.erase(old);
			delete current;
		}
		else {
			iter++;
		}
	}
}
