/*
 * Photo.h
 *
 *  Created on: Apr 28, 2009
 *      Author: max
 */

#ifndef PHOTO_H_
#define PHOTO_H_

#include <string>
#include <sstream>

#include "../LinearInterpolatedAnimateable.h"
#include "../Texture.h"

namespace multiKa {

	class Photo : public LinearInterpolatedAnimateable {

		private:
			const char* photoFilename;
			Texture* photoTexture;

			float v1[3], v2[3], v3[3], v4[3], v5[3], v6[3], v7[3], v8[3];
			float vn1[3], vn2[3], vn3[3];
			float thickness;

			void createVertices();

		protected:
			virtual void DrawAtPosition( float* position, float factor, int tick );

		public:
			Photo( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration );


	};

}
#endif /* PHOTO_H_ */
