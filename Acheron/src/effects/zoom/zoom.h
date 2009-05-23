#ifndef ZOOM_H
#define ZOOM_H

#include <string>
#include <vector>

#include "../../AnimateableObject.h"
#include "../../Texture.h"

namespace zoom {

	class ZoomEffect : public AnimateableObject {
		std::string imageDirectory;
		int targetResolution;
		int startLevel;
		int endLevel;
		std::vector<Texture*> textures;

	public:
		ZoomEffect(int startTick, int duration, int targetResolution, std::string imageDirectory, int startLevel, int endLevel);

		virtual ~ZoomEffect();

		virtual bool Draw(int ticks, Context* context);
	};
}

#endif