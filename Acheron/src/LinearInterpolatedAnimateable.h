/*
 * LinearInterpolatedAnimateable.h
 *
 *  Created on: Apr 23, 2009
 *      Author: meta, cygen
 */

#ifndef LINEARINTERPOLATEDANIMATEABLE_H_
#define LINEARINTERPOLATEDANIMATEABLE_H_

#include "AnimateableObject.h"

class LinearInterpolatedAnimateable : public AnimateableObject {
	public:
		//LinearInterpolatedAnimateable();
		//virtual ~LinearInterpolatedAnimateable();
		virtual bool Draw(int tick) {
			if (tick >= startTick+duration) return false;
			if (tick < startTick) return true;

			float factor = (tick - startTick) / (float)duration;
			float invFactor = 1 - factor;

			float pos[3];

			for (int i = 0; i < 3; i++)
				pos[i] = startPos[i] * invFactor + endPos[i]*factor;

			DrawAtPosition(pos, factor, tick);
			return true;
		}

	protected:
		LinearInterpolatedAnimateable(float *startPos, float *endPos, int startTick, int duration)
			:AnimateableObject(startTick, duration)
		{
			this->startPos[0] = startPos[0];
			this->startPos[1] = startPos[1];
			this->startPos[2] = startPos[2];

			this->endPos[0] = endPos[0];
			this->endPos[1] = endPos[1];
			this->endPos[2] = endPos[2];
		}

		virtual void DrawAtPosition(float *position, float factor, int tick) = 0;

	private:
		float startPos[3];
		float endPos[3];

};

#endif /* LINEARINTERPOLATEDANIMATEABLE_H_ */
