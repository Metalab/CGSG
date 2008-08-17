//
// class for vorbis playback and fft

#ifndef VORBISFFT__H_
#define VORBISFFT__H_


#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

#include "portaudio.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "metafft.h"
extern  void compute();

typedef struct
{
		float *vorbisDataP;
    int allSndDataLength;
		int allSndDataIndex;
		bool nodataleft;
		ogg_int64_t totalSamples;
		
}   
SoundData;


typedef int PaStreamCallback( const void *input,
                                      void *output,
                                      unsigned long frameCount,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags statusFlags,
                                      void *userData );



													 
class vorbisFFT
{
	
  public:
		static const int SND_BUFFER_SIZE = 2048;
		static const int SAMPLE_RATE = 44100;

		//public for metafft for now
		SoundData soundData;
		
		SoundData* getSoundDataP(){ return &soundData; };
		
		
		static int patestCallback(const void *inputBuffer, void *outputBuffer,
														 unsigned long framesPerBuffer,
														 const PaStreamCallbackTimeInfo* timeInfo,
														 PaStreamCallbackFlags statusFlags,
														 void *userData );
		
		void set_fftBuffer(unsigned long framesPerBuffer);
		void metafft_compute();
		//void vorbisComputeFFT();
	//static void set_fftBuffer(unsigned long framesPerBuffer);
		
		vorbisFFT();
		//vorbisFFT(char *file);
		//vorbisFFT(char *data_begin, char *data_end);
		
		int initializeSound();
		int loadDataFromFile(char *file);
		int loadEmbeddedData(char *data_begin, char *data_end);
		int loadData();
		int start();
		int stop();
		int terminateSound();
		
		
		
	private:
	
		OggVorbis_File vorbisFile;

		PaStreamParameters outputParameters;
		SDL_mutex *FFTmutex;
		PaError err;
		PaStream *stream;
		
		int sampleRate;
		
		 

	
	
};


#endif