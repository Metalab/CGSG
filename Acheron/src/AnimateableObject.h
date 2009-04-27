/*
 * AnimateableObject.h
 *
 *  Created on: Apr 21, 2009
 *      Author: meta, cygen
 */

#ifndef ANIMATEABLEOBJECT_H_
#define ANIMATEABLEOBJECT_H_

#include "Context.h"

class AnimateableObject {
	public:
		virtual ~AnimateableObject() { }

		AnimateableObject(int startTick, int duration) {
			this->startTick = startTick; this->duration = duration;
		}
		virtual bool Draw(int ticks, Context* context) = 0;

		int GetStartTick() { return startTick; }
		int GetDuration() { return duration; }

	protected:
		int startTick;
		int duration;
};

#endif /* ANIMATEABLEOBJECT_H_ */
