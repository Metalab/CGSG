/*
 * ObjectContainer.h
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#ifndef OBJECTCONTAINER_H_
#define OBJECTCONTAINER_H_

#include <list>

#include "Base3DObject.h"
#include "math.h"

#include "../AnimateableObject.h"

using namespace std;

namespace multiKa {

	class ObjectContainer: public AnimateableObject {
	private:
		AudioStuff *audio;
		int objectCounter;
		int spectrumPerObject;
		int spectrumSize;
		float *spectrumPointer;

	protected:
		list<Base3DObject *> myObjects;

	public:
		ObjectContainer();
		virtual ~ObjectContainer();

		void setAudio(AudioStuff *audio);

		void addObject(Base3DObject *my3DObject) {
			myObjects.push_back( my3DObject );
			objectCounter++;
		}

		void prepareAudio();

		void draw();
		void draw(AudioStuff &audio);
		virtual void Draw( int ticks, Context context );
	};

}

#endif /* OBJECTCONTAINER_H_ */
