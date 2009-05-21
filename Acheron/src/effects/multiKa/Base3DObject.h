/*
 * Base3DObject.h
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#ifndef BASE3DOBJECT_H_
#define BASE3DOBJECT_H_

namespace multiKa {

	class Base3DObject {

		public:
			Base3DObject(float posX, float posY, float posZ) {
				this->posX = posX;
				this->posY = posY;
				this->posZ = posZ;

				child = 0;
			};

			//virtual Base3DObject* getChild();

			void assignChild(Base3DObject *myChild) { child = myChild; }
//			virtual void draw(AudioStuff &audio);
			virtual void draw( float spectrum[256], int spectrumStart, int spectrumEnd){}
//			virtual void draw();

		protected:
		//	int segments;
			float posX, posY, posZ;
			float *vertices;
			Base3DObject *child;
			int Base3DObjectNumber;

			virtual void createVertices(){}
	};
}

#endif /* BASE3DOBJECT_H_ */
