/*
 * spirograph.h
 *
 *  Created on: May 21, 2009
 *      Author: max
 */

#ifndef SPIROGRAPH_H_
#define SPIROGRAPH_H_

#include <Eigen/Core>
// import most common Eigen types
USING_PART_OF_NAMESPACE_EIGEN

#include "Base3DObject.h"
#include "RingBuffer.h"
#include "../../HSBColors.h"
#include "../../CoordinatesManipulator.h"

#include "../../misc.h"


namespace multiKa {

	class Spirograph : public Base3DObject {

	public:
		Spirograph(float av1, float d1, float av2, float d2, float av3, float d3, int length);
		virtual void draw( float spectrum[], int spectrumStart, int spectrumEnd );

	private:

		int length;
		float minDistance;

		float a1, a2, a3;
		float av1, av2, av3;
		float d1, d2, d3;
		float rot;

		CoordinatesManipulator* cManipulator;

		float vecA[3], vecB[3], vecC[3];
		float normA, normB, normC;

		RingBuffer *path;
		float pathReadBuffer1[16];
		float pathReadBuffer2[16];
		float pathWriteBuffer[16];

		float pathBuffer1[16], pathBuffer2[16], pathBuffer3[16];

		HSBColors *colors;

		int frameCounter;
		int pathLength;

		void drawArm();
		void drawPath();
		void drawPath( float spectrum[], int spectrumStart, int spectrumEnd );

		inline float distanceMod( float coordinate, float loopPosition ) {
			return coordinate + ( coordinate * ( (length - loopPosition) / length )*1.0 );
		}

		void calculateNewPosition();
		void precalculate();
		void insert();

	};

}

#endif /* SPIROGRAPH_H_ */
