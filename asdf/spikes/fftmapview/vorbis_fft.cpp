#include "vorbis_fft.h"

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "portaudio.h"

#include "metafft.h"
extern float audio_buffer[];
extern SDL_mutex *mutex;
extern  void compute();

//#include <sndfile.h>


int vorbisFFT::patestCallback(const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )	{
	
	SoundData *data = (SoundData*)userData;
	float *out = (float*)outputBuffer;
	unsigned long i;

	(void) timeInfo; /* Prevent unused variable warnings. */
	(void) statusFlags;
	(void) inputBuffer;
	int readcount;
	
	//set_fftBuffer(framesPerBuffer);
	///*
	if((data->allSndDataIndex/2) + (framesPerBuffer) >= data->totalSamples) {
		//printf("Idx: %d fpb: %d tf: %d\n",allSndDataIndex,framesPerBuffer,totalframes);
		return 0;
		
		framesPerBuffer=data->totalSamples-(data->allSndDataIndex/2);
		//doexit = 1;
		data->nodataleft = true;
	}
//*/
	for( i=0; i<framesPerBuffer; i++ )
	{
			*out++ = data->vorbisDataP[data->allSndDataIndex+i*2];//*0.01; // *0.2 for less volume
			*out++ = data->vorbisDataP[data->allSndDataIndex+1+i*2];//*0.01;
	}
	data->allSndDataIndex += framesPerBuffer*2;
//	return paContinue;
	return data->nodataleft == true ? paContinue : 0;

	
}


void vorbisFFT::set_fftBuffer(unsigned long framesPerBuffer) {
	// lock
  SDL_mutexP(mutex);

  // convert stereo to mono
  for (int i = 0; i < framesPerBuffer; i++) {
		audio_buffer[i] = this->soundData.vorbisDataP[this->soundData.allSndDataIndex+i*2] + this->soundData.vorbisDataP[ this->soundData.allSndDataIndex+1+i*2];
		audio_buffer[i] /= 2.0f;
  }
	
  // unlock
  SDL_mutexV(mutex);
}



void vorbisFFT::metafft_compute() {
	this->set_fftBuffer((unsigned long)SND_BUFFER_SIZE);
	compute();
}

vorbisFFT::vorbisFFT() {
  sampleRate = 44100;
	//allSndDataLength = 0;
	//allSndDataIndex = 0;
	
	this->soundData.totalSamples=0;
	this->soundData.allSndDataLength=0;
	this->soundData.allSndDataIndex=0;
}

/*
vorbisFFT::vorbisFFT(char *file) {
  sampleRate = 44100;
  
}

vorbisFFT::vorbisFFT(char *data_begin, char *data_end) {
  sampleRate = 44100;
  
}
*/
int vorbisFFT::loadData() {
	
}

int vorbisFFT::loadDataFromFile(char *file) {
	int eof=0;
  int current_section;
	
	if(ov_fopen(file, &this->vorbisFile) != 0) {
		printf("error opening file\n");
		exit(1);
	}
	
	char **ptr=ov_comment(&this->vorbisFile,-1)->user_comments;
	vorbis_info *vi=ov_info(&this->vorbisFile,-1);
	while(*ptr){
		fprintf(stderr,"%s\n",*ptr);
		++ptr;
	}
	fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
	fprintf(stderr,"\nDecoded length: %ld samples\n", (long)ov_pcm_total(&this->vorbisFile,-1));
	fprintf(stderr,"Encoded by: %s\n\n",ov_comment(&this->vorbisFile,-1)->vendor);
	this->soundData.totalSamples=0;
	this->soundData.totalSamples = ov_pcm_total(&vorbisFile,-1);
	
	this->soundData.vorbisDataP = (float*) malloc(this->soundData.totalSamples*sizeof(float)*vi->channels); //fixme: stat wav file for size, compute then space needed for frames
	float *allSndPBegin = soundData.vorbisDataP;
	float **allVorbisSndP = (float**) malloc(SND_BUFFER_SIZE*sizeof(float)*vi->channels);
	
	//readloop
	while(!eof){
		long ret = ov_read_float(&this->vorbisFile,&allVorbisSndP, SND_BUFFER_SIZE, &current_section);
    if (ret == 0) {
      /* EOF */
      eof=1;
    } else if(ret == OV_HOLE) {
			fprintf(stderr,"there was an interruption in the data.\n");
			fprintf(stderr, "one of: garbage between pages, loss of sync followed by recapture, or a corrupt page\n");
		} else if (ret < 0) {
      /* error in the stream.  Not a problem, just reporting it in
					case we (the app) cares.  In this case, we don't. */
    } else {
      /* we don't bother dealing with sample rate changes, etc, but
					you'll have to*/
      this->soundData.allSndDataLength+=ret;
			int p=0;
			for(int x=0;x<ret;x++) {
				for(p=0;p<vi->channels;p++) {
					memcpy(this->soundData.vorbisDataP+x*2+p, &allVorbisSndP[p][x], sizeof(float));
				}
			}
			this->soundData.vorbisDataP += ret*p;
    }
  }

	ov_clear(&this->vorbisFile);
	//free(allVorbisSndP);
	
	this->soundData.vorbisDataP = allSndPBegin;
  fprintf(stderr,"Done reading file.\n");

	
	return 0;
}

/*
void vorbisFFT::set_fftBuffer(unsigned long framesPerBuffer) {
	// lock
  SDL_mutexP(FFTmutex);
	
  // convert stereo to mono
  for (int i = 0; i < framesPerBuffer; i++) {
		audio_buffer[i] = allSndP[allSndDataIndex+i*2] + allSndP[allSndDataIndex+1+i*2];
		audio_buffer[i] /= 2.0f;
  }
    
  // unlock
  SDL_mutexV(FFTmutex);
}
*/


//
int vorbisFFT::initializeSound() {

	err = Pa_Initialize();
	if( err != paNoError ) {
		printf("error initializing portaudio?\n");
		exit(1);
	}
	
	this->outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
  this->outputParameters.channelCount = 2;       /* stereo output */
  this->outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
  //outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	this->outputParameters.hostApiSpecificStreamInfo = NULL;
	err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &this->outputParameters,
              sampleRate,//88200, //44100
              SND_BUFFER_SIZE,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &soundData );
  if( err != paNoError ) {
		printf("error opening stream?\n");
		exit(1);
	}
}

int vorbisFFT::start() {
	err = Pa_StartStream( stream );
	if( err != paNoError ) {
			printf("error starting stream\n");
	}
}

int vorbisFFT::stop() {
	Pa_StopStream(stream);
	if( err != paNoError ) {
			printf("error stop stream\n");
	}
}

int vorbisFFT::terminateSound() {
	
	this->stop();
	
	err = Pa_CloseStream( stream );
	if( err != paNoError ) {
			printf("error closing stream\n");
	}
	Pa_Terminate();
	free(soundData.vorbisDataP);
}
