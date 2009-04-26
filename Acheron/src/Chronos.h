/*
 * Chronos.h
 *
 *  Created on: Apr 24, 2009
 *      Author: meta, cygen
 */

#ifndef CHRONOS_H_
#define CHRONOS_H_

#include <list>
#include "AnimateableObject.h"
#include "Context.h"

class Chronos {
	public:
		Chronos():objects(){};

		bool HasObjects() { return objects.size() > 0; }
		void AddObject(AnimateableObject *object);
		void Draw(int tick);

		void setContext( Context* context ) { this->context = context; };


	private:
		std::list<AnimateableObject*> objects;

		Context* context;
};

#endif /* CHRONOS_H_ */
