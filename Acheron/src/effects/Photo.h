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

namespace multiKa {

	class Photo : public LinearInterpolatedAnimateable {

		private:
			const char* photoFilename;

		protected:
			virtual void DrawAtPosition( float* position, float factor, int tick );

		public:
			Photo( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration );


	};

}
#endif /* PHOTO_H_ */
