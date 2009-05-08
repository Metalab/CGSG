/*
 * Asdf.h
 *
 *  Created on: May 08,2009
 *      Author: scriptythekid
 */

#ifndef ASDF_H_
#define ASDF_H_

#include <string>
#include <sstream>

#include "../LinearInterpolatedAnimateable.h"
#include "../HSBColors.h"

namespace beidlpracker {

	class Asdf : public LinearInterpolatedAnimateable {

		private:
			const char* photoFilename;
			

			float v1[3], v2[3], v3[3], v4[3], v5[3], v6[3], v7[3], v8[3];
			float vn1[3], vn2[3], vn3[3];
			float thickness;

			float rPos;
			HSBColors* colors;


			void createVertices();

		protected:
			virtual void DrawAtPosition( float* position, float factor, int tick );

		public:
			Asdf( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration );


	};

}
#endif /* ASDF_H_ */
