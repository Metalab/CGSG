/*
 * AudioStuff.cpp
 *
 *  Created on: Dec 1, 2008
 *      Author: max
 */

#include "AudioStuff.h"

AudioStuff::AudioStuff() {

	result = FMOD::System_Create(&system);
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}

	/*
	 * so i can have sound-output while amarok is playing on my system
	 */
#ifdef linux
	result = system->setOutput( FMOD_OUTPUTTYPE_ALSA );
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}
#endif

	result = system->init( 100, FMOD_INIT_NORMAL, 0 );
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}

	result = system->createSound("music.mp3", FMOD_CREATECOMPRESSEDSAMPLE, 0, &sound);
//	result = system->createSound("/home/max/Music/testSounds/kandel_maschine_sample1.ogg", FMOD_CREATECOMPRESSEDSAMPLE, 0, &sound);
//	result = system->createStream("/dev/dsp", FMOD_CREATESTREAM, 0, &sound);
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}
	result = sound->setMode(FMOD_LOOP_NORMAL);
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}


	result = system->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}

	logSpectrumBandwidth = log2((float)SPECTRUM_BANDS)/LOG_SPECTRUM_BANDS;
	tempSum=0;
	spectrumSize = sizeof(spectrumAvg)/sizeof(float);
	calculateConversionBoundaries();

}



void AudioStuff::calculateConversionBoundaries() {
	for( int j=0; j<LOG_SPECTRUM_BANDS; j++ ) {
		conversionBoundaries[j]		= (int)round(pow(2,  j   *logSpectrumBandwidth)-1);
		conversionBoundaries[j+1]	= (int)round(pow(2, (j+1)*logSpectrumBandwidth)-1);
	}
}

void AudioStuff::refreshSpectrum() {
	result = channel->getSpectrum(spectrumLeft, SPECTRUM_BANDS, 0, FMOD_DSP_FFT_WINDOW_BLACKMAN);
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}
	result = channel->getSpectrum(spectrumRight, SPECTRUM_BANDS, 1, FMOD_DSP_FFT_WINDOW_BLACKMAN);
	if( result != FMOD_OK ){
		printf( "FMOD error! (%d) %s\n", result, FMOD_ErrorString(result) );
		exit(-1);
	}

	tempMaxSpec = 0;
	for( int i=0; i<SPECTRUM_BANDS; i++) {
		spectrumAvg[i] = (spectrumLeft[i]+spectrumRight[i])/2.0f;
		tempMaxSpec = ( spectrumAvg[i] > tempMaxSpec ? spectrumAvg[i] : tempMaxSpec );
	}
	tempMaxSpec = 1/tempMaxSpec;
	for( int i=0; i<SPECTRUM_BANDS; i++) {
		spectrumAvg[i] = spectrumAvg[i]*tempMaxSpec;
	}

	for( int j=0; j<LOG_SPECTRUM_BANDS; j++ ) {
		tempSum = 0;
		for( int i=conversionBoundaries[j]; i<=conversionBoundaries[j+1]; i++){
			tempSum += spectrumAvg[i];
		}
		logSpectrumAvg[j] = tempSum/( conversionBoundaries[j+1]-conversionBoundaries[j]+1 );
	}

}

float* AudioStuff::getSpectrum(int *size){
	*size = spectrumSize;
	return spectrumAvg;
}

float* AudioStuff::getLogSpectrum( int *size ) {
	*size = spectrumSize;
	return logSpectrumAvg;
}
