/*
 * Text.h
 *
 *  Created on: Apr 25, 2009
 *      Author: meta, cygen
 */

#ifndef TEXT_H_
#define TEXT_H_

#include "../LinearInterpolatedAnimateable.h"
#include "../font.h"

#include <cmath>
#include <vector>

class Text: public LinearInterpolatedAnimateable {
	private:
		std::vector<Point<float> > points;
		int maxx;


	protected:
		virtual void DrawAtPosition(float* position, float factor, int tick);
		void DrawCube(float x, float y, float z);

	public:
		//virtual ~Text();

		Text(Font &theFont, const char *text, float *startPos, float *endPos, int startTick, int duration);

};

#endif /* TEXT_H_ */
