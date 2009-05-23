/*
 * AudioStuff.h
 *
 *  Created on: Dec 1, 2008
 *      Author: max
 */

#ifndef AUDIOSTUFF_H_
#define AUDIOSTUFF_H_

#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include "extramath.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "fmod.hpp"
#include "fmod_errors.h"

//#include "config.h"

const int SPECTRUM_BANDS = 8192;
const int LOG_SPECTRUM_BANDS = 1024;


class AudioStuff {
	public:
		AudioStuff();
		float* getSpectrum( int *size );
		float* getLogSpectrum( int *size );
		void refreshSpectrum();

		FMOD_RESULT result;
		FMOD::System *system;
		FMOD::Sound *sound;
		FMOD::Channel *channel;

	protected:

	private:
		int spectrumSize;

		float logSpectrumBandwidth;

		float logSpectrumLeft[LOG_SPECTRUM_BANDS];
		float logSpectrumRight[LOG_SPECTRUM_BANDS];
		float logSpectrumAvg[LOG_SPECTRUM_BANDS];

		float spectrumLeft[SPECTRUM_BANDS];
		float spectrumRight[SPECTRUM_BANDS];
		float spectrumAvg[SPECTRUM_BANDS];


		float tempSum;
		float tempMaxSpec;

		int conversionBoundaries[ LOG_SPECTRUM_BANDS *2 ];

		void calculateConversionBoundaries();

};

#endif /* AUDIOSTUFF_H_ */
