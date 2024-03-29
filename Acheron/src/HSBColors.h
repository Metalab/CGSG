/*
 * HSBColors.h
 *
 *  Created on: Apr 14, 2009
 *      Author: max
 */

#ifndef HSBCOLORS_H_
#define HSBCOLORS_H_

#include <GL/glew.h>
#define _USE_MATH_DEFINES
#include <cmath>


struct RGBColor {
	float r;
	float g;
	float b;
};
struct HSBColor {
	float h;
	float s;
	float b;
};

class HSBColors {
	public:
		HSBColors( int );
		virtual ~HSBColors();

		void update();
		RGBColor* getRGBColors();
		void setMaterial( GLenum, GLenum );

	private:
		int numberOfColors;
		int activeColor;
		RGBColor *rgbColors;
		HSBColor *hsbColors;

		void convertHSBColors();
};

#endif /* HSBCOLORS_H_ */
