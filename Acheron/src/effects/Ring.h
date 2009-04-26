/*
 * Ring.h
 *
 *  Created on: Apr 26, 2009
 *      Author: max
 */

#ifndef RING_H_
#define RING_H_

#include "Base3DObject.h"
#include "../HSBColors.h"

namespace multiKa {

	class Ring: public Base3DObject {
		public:
			Ring( float radius, float width, int segments,
					float rSpeed, float rDiff );

			virtual void draw( float spectrum[], int startSpectrum, int endSpectrum );

		protected:
			int segments;
			virtual void createVertices();

		private:
			float radius;
			float width;
			float rSpeed;
			float rDiff;
			float rPos;

			HSBColors *colors;

	};

}

#endif /* RING_H_ */
