/*
 * Context.h
 *
 *  Created on: Apr 25, 2009
 *      Author: max
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include "AudioStuff.h"

class Context {
	public:
		Context();
		~Context();

		AudioStuff* getAudio() { return audioStuff; }

	private:
		AudioStuff* audioStuff;

};

#endif /* CONTEXT_H_ */
