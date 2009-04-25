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

class Chronos {
	public:
		Chronos();
		virtual ~Chronos();

		bool HasObjects() { return objects.size() > 0; }
		void AddObject(AnimateableObject *object);
		void Draw(int tick);


	private:
		std::list<AnimateableObject*> objects;
};

#endif /* CHRONOS_H_ */
