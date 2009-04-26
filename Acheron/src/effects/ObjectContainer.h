/*
 * ObjectContainer.h
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#ifndef OBJECTCONTAINER_H_
#define OBJECTCONTAINER_H_

#include <GL/glew.h>

#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif

#include <list>

#include "Base3DObject.h"
#include "math.h"

#include "../AnimateableObject.h"
#include "../Context.h"

using namespace std;

namespace multiKa {

	class ObjectContainer: public AnimateableObject {
	private:
		int objectCounter;
		int spectrumPerObject;
		int spectrumSize;
		float *spectrumPointer;

	protected:
		list<Base3DObject *> myObjects;

	public:
		ObjectContainer() {}
		virtual ~ObjectContainer(){}

//		void setAudio(AudioStuff *audio);

		void addObject(Base3DObject *my3DObject) {
			myObjects.push_back( my3DObject );
			objectCounter++;
		}

		void prepareAudio() {
			spectrumPerObject = floor((float)LOG_SPECTRUM_BANDS/objectCounter);
		};

		void draw();
		void draw(AudioStuff &audio);
		virtual bool Draw( int ticks, Context* context );
	};

}

#endif /* OBJECTCONTAINER_H_ */
