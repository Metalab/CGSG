#include "zoom.h"

#include <iostream>
#include <sstream>



zoom::ZoomEffect::ZoomEffect(int startTick, int duration, int targetResolution, std::string imageDirectory, int startLevel, int endLevel)
: AnimateableObject(startTick, duration), targetResolution(targetResolution), imageDirectory(imageDirectory), startLevel(startLevel), endLevel(endLevel)
{

				//load images - should be done on an extra thread...
				for (int i=startLevel; i <= endLevel; i++) {
					std::ostringstream s;
					s << imageDirectory << i << ".jpg";

					textures.push_back(new Texture(s.str()));
					if (!textures.back()->load()) {
						std::cout << "Error loading texture " << s.str() << std::endl;
					}
				}
}

bool zoom::ZoomEffect::Draw(int ticks, Context* context) {
	if (startTick > ticks) return true;
	if (ticks > startTick+duration) return false;

	int relativeTick = ticks - startTick; 

	float width = (float)(2 << (endLevel - startLevel));
	float maxDistance = width/2;
	float minDistance = width / (2 << (endLevel - startLevel));

	float tFactor = relativeTick / (float)duration;

	//  distFactor = (tFactor-1)^4
	float distFactor = tFactor - 1.0f;
	distFactor *= distFactor; // ^2
	distFactor *= distFactor; // ^4

	float distance = (maxDistance-minDistance) * distFactor + minDistance;

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);



	//draw all applicable levels
	int levelsToDraw = 1;
	for (int i=0; i <= endLevel-startLevel && levelsToDraw > 0; i++) {
		if ((distance*2) > width) {
			glBindTexture(GL_TEXTURE_2D, textures[i]->id);
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glNormal3f(0, 0, 1.0f);

			float antiZFightAndCam = 5.0f;

			glBegin(GL_QUADS);
			glTexCoord2f(0, 1.0f);
			glVertex3f(-width, width, -distance + antiZFightAndCam);
			glTexCoord2f(0, 0);
			glVertex3f(-width, -width, -distance + antiZFightAndCam);
			glTexCoord2f(1.0f, 0);
			glVertex3f(width, -width, -distance + antiZFightAndCam);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(width, width, -distance + antiZFightAndCam);
			glEnd();

			levelsToDraw--;
		}

		width /= 2;
	}
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	return true;
}

zoom::ZoomEffect::~ZoomEffect() {
	for (std::vector<Texture*>::iterator it = textures.begin(); it != textures.end(); it++)
		delete *it;
}