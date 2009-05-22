#ifndef ZOOM_H
#define ZOOM_H

#include <string>

#include "../../AnimateableObject.h"

namespace zoom {

	class ZoomEffect : public AnimateableObject {
		std::string imageDirectory;
		int targetResolution;
		int startLevel;
		int endLevel;

	public:
		ZoomEffect(int startTick, int duration, int targetResolution, std::string imageDirectory, int startLevel, int endLevel)
			: AnimateableObject(startTick, duration), targetResolution(targetResolution), imageDirectory(imageDirectory), startLevel(startLevel), endLevel(endLevel) {

				//load images

		}

		virtual ~ZoomEffect() { }

		virtual bool Draw(int ticks, Context* context);
	};
}

#endif