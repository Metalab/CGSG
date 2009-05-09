/*
 * HSBColors.cpp
 *
 *  Created on: Apr 14, 2009
 *      Author: max
 */

#include "HSBColors.h"

#include <stdlib.h>
#include <cstdio>

HSBColors::HSBColors( int num ) {
	numberOfColors = num;
	activeColor = 0;

	rgbColors = new RGBColor[numberOfColors];
	hsbColors = new HSBColor[numberOfColors];

	float start = ( rand()%36000 ) / 100.0f;
	float difference = 360.0f/numberOfColors;

	for( int i=0; i<numberOfColors; i++ ){
		hsbColors[ i ].h = fmod((start + i*difference), 360);
		hsbColors[ i ].s = 1;
		hsbColors[ i ].b = 1;
	}


}

HSBColors::~HSBColors() {
	// TODO Auto-generated destructor stub
}

void HSBColors::update() {

	for( int i=0; i<numberOfColors; i++ ){
		hsbColors[i].h = fmod( ( hsbColors[i].h + 0.5f ), 360);
	}
	convertHSBColors();
	activeColor = 0;
}

void HSBColors::convertHSBColors() {

	for( int i=0; i<numberOfColors; i++ ){

		if( hsbColors[i].b == 0 ) { // black
			rgbColors[i].r = 0;
			rgbColors[i].g = 0;
			rgbColors[i].b = 0;
		} else if ( hsbColors[i].s == 0 ) { // grey
			rgbColors[i].r = hsbColors[i].b;
			rgbColors[i].g = hsbColors[i].b;
			rgbColors[i].b = hsbColors[i].b;
		} else {

			const float hf = hsbColors[i].h / 60.0f;
			const int    hi  = (int) floor( hf );
			const float f  = hf - hi;
			const float pv  = hsbColors[i].b * ( 1 - hsbColors[i].s );
			const float qv  = hsbColors[i].b * ( 1 - hsbColors[i].s * f );
			const float tv  = hsbColors[i].b * ( 1 - hsbColors[i].s * ( 1 - f ) );
			switch( hi )
			{
				// Red is the dominant color
				case 0:
					rgbColors[i].r = hsbColors[i].b;
					rgbColors[i].g = tv;
					rgbColors[i].b = pv;
				break;

				// Green is the dominant color
				case 1:
					rgbColors[i].r = qv;
					rgbColors[i].g = hsbColors[i].b;
					rgbColors[i].b = pv;
				break;
				case 2:
					rgbColors[i].r = pv;
					rgbColors[i].g = hsbColors[i].b;
					rgbColors[i].b = tv;
				break;

				// Blue is the dominant color
				case 3:
					rgbColors[i].r = pv;
					rgbColors[i].g = qv;
					rgbColors[i].b = hsbColors[i].b;
				break;
				case 4:
					rgbColors[i].r = tv;
					rgbColors[i].g = pv;
					rgbColors[i].b = hsbColors[i].b;
				break;

				//Red is the dominant color
				case 5:
					rgbColors[i].r = hsbColors[i].b;
					rgbColors[i].g = pv;
					rgbColors[i].b = qv;
				break;

				// Just in case we overshoot on our math by a little, we put these here. Since its a switch it won't slow us down at all to put these here.

				case 6:
					rgbColors[i].r = hsbColors[i].b;
					rgbColors[i].g = tv;
					rgbColors[i].b = pv;
				break;
				case -1:
					rgbColors[i].r = hsbColors[i].b;
					rgbColors[i].g = pv;
					rgbColors[i].b = qv;
				break;

				//The color is not defined, we should throw an error.

				default:
					//LFATAL("i Value error in Pixel conversion, Value is %d",i);
					printf("error while converting to rgb - %f\n", hsbColors[i].h);
				break;
			}

		}

	}

}

RGBColor* HSBColors::getRGBColors() {
	convertHSBColors();
	return rgbColors;
}

void HSBColors::setMaterial(GLenum face, GLenum pname) {
	glMaterialfv (face, pname, (float*)&(rgbColors[ activeColor ]) );
	activeColor = (activeColor++)%numberOfColors;
}
