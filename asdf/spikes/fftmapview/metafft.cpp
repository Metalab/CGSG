#include <math.h>
#include <float.h>
#include <stdio.h>

#ifdef __APPLE__
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

#include <RtAudio.h>
#include <fftw3.h>
#include <vector>

#define SND_BUFFER_SIZE 2048
#define SAMPLE_RATE 44100

float stereo_buffer[SND_BUFFER_SIZE*2];
float audio_buffer[SND_BUFFER_SIZE];
float fft_buffer[SND_BUFFER_SIZE*2];
float tmp_bufferA[SND_BUFFER_SIZE];
float spectrum[SND_BUFFER_SIZE];
size_t spectrum_size = SND_BUFFER_SIZE/2+1;

RtAudio *audio = NULL;
SDL_mutex *mutex;
bool spectrum_ready = false;



void compute()
{
  SDL_mutexP(mutex);
  memset(fft_buffer, 0, SND_BUFFER_SIZE*2 * sizeof(float));
  // copy current audio_buffer into fft_buffer
  memcpy(fft_buffer, audio_buffer, SND_BUFFER_SIZE * sizeof(float));
  // some flag (hand off to audio cb thread)
  SDL_mutexV(mutex);

  fftw_plan p;
  double *in = (double *)fftw_malloc(sizeof(double) * SND_BUFFER_SIZE);
  fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * SND_BUFFER_SIZE);
  p = fftw_plan_dft_r2c_1d(SND_BUFFER_SIZE, in, out, FFTW_ESTIMATE);

  double inmin = FLT_MAX, inmax = -FLT_MAX;
  for (int i=0;i<SND_BUFFER_SIZE;i++) {
    double ind = fft_buffer[i];
    if (ind < inmin) inmin = ind;
    if (ind > inmax) inmax = ind;
    in[i] = fft_buffer[i];
  }
  //  printf("in: %.2lf %.2lf\n", inmin, inmax);

  fftw_execute(p);

//   double remin = FLT_MAX, remax = -FLT_MAX;
//   complex * cbuf = (complex *)fft_buffer;
//   for (int i=0;i<SND_BUFFER_SIZE;i++) {
//     double re =  out[i][0];
//     if (re < remin) remin = re;
//     if (re > remax) remax = re;
                      
//     cbuf[i].re = out[i][0];
//     cbuf[i].im = out[i][1];
//   }
//   printf("out: %.2lf %.2lf\n", remin, remax);

  double min = FLT_MAX;
  double max = -FLT_MAX;
  for (int i=0;i<SND_BUFFER_SIZE/2+1;i++) {
//     tmp_bufferA[i] = out[i][0];
//     tmp_bufferA[i] = sqrt(out[i][0]*out[i][0] - out[i][1]*out[i][1]) / sqrt(SND_BUFFER_SIZE);
//     tmp_bufferA[i] = sqrt(out[i][0]*out[i][0] - out[i][1]*out[i][1]) / sqrt(SND_BUFFER_SIZE);
    tmp_bufferA[i] = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
    if (tmp_bufferA[i] < min) min = tmp_bufferA[i];
    if (tmp_bufferA[i] > max) max = tmp_bufferA[i];
  }
  //  printf("Min: %f, max: %f\n", min, max);

  fftw_destroy_plan(p);
  fftw_free(in); fftw_free(out);

  // copy current magnitude spectrum into waterfall memory
  min = FLT_MAX;
  max = -FLT_MAX;
  for (int i = 0; i < SND_BUFFER_SIZE/2+1; i++) {
    //    spectrum[i] = tmp_bufferA[i];
//     spectrum[i] = sqrt(100 * tmp_bufferA[i]) + 50;
    spectrum[i] = 10*tmp_bufferA[i]+50;
    if (spectrum[i] < min) min = spectrum[i];
    if (spectrum[i] > max) max = spectrum[i];
  }
  //  printf("Min: %f, max: %f\n", min, max);

  spectrum_ready = true;
}

void metafft_get_spectrum(float *&spec, size_t &specsize)
{
  spec = spectrum;
  specsize = spectrum_size;
  spectrum_ready = false;
}

/*!
  /a nFrames sample of incoming stereo data in /a buffer

  Copy to stereo_buffer && 
  convert to mono and copy to audio_buffer
 */
static int stream_cb(void *outbuf, void *buffer, unsigned int nFrames, 
                     double streamTime, RtAudioStreamStatus status, void *userData)
{
  // lock
  SDL_mutexP(mutex);
  // copy
  memcpy(stereo_buffer, buffer, nFrames * 2 * sizeof(float));
  // convert stereo to mono
  for (int i = 0; i < nFrames; i++) {
    audio_buffer[i] = stereo_buffer[i*2] + stereo_buffer[i*2+1];
    audio_buffer[i] /= 2.0f;
  }
    
  // unlock
  SDL_mutexV(mutex);

  compute();


  return 0;
}

static bool initAudio()
{
  mutex = SDL_CreateMutex(); // Mutex for audio input sync
  try {
    unsigned int bufsize = SND_BUFFER_SIZE;
    audio = new RtAudio;
    if (audio->getDeviceCount() == 0) {
      fprintf(stderr, "No audio devices found.\n");
      return false;
    }
    printf("Devices:\n");
    for (int i=0;i<audio->getDeviceCount();i++) {
      const RtAudio::DeviceInfo &info = audio->getDeviceInfo(i);
      printf("  %s (%s)", info.name.c_str(), (info.isDefaultInput || info.isDefaultOutput)?"default":"");
      printf(" -");
      std::vector<unsigned int>::const_iterator iter = info.sampleRates.begin();
      while (iter != info.sampleRates.end()) {
        printf(" %d", *iter);
        iter++;
      }
      printf("\n");
    }

    RtAudio::StreamParameters *params = new RtAudio::StreamParameters;
    params->deviceId = audio->getDefaultInputDevice();
    params->nChannels = 2;

    RtAudio::StreamOptions *options = new RtAudio::StreamOptions;
    options->numberOfBuffers = 8;

    audio->openStream(NULL, params, RTAUDIO_FLOAT32, SAMPLE_RATE, &bufsize, 
                      stream_cb, NULL, options);
    if (bufsize != SND_BUFFER_SIZE) {
      fprintf(stderr, "warning: using different buffer sizes: %i : %i\n", 
              bufsize, SND_BUFFER_SIZE);
    }
  }
  catch (RtError & e) {
    fprintf(stderr, "RtError: %s\n", e.getMessage().c_str());
    return false;
  }

  audio->startStream();
    
  return true;
}

bool metafft_init()
{
  if (!initAudio()) {
    fprintf(stderr, "metafft_init(): Unable to init RtAudio\n");
    return false;
  }
  return true;
}

bool metafft_ready()
{
  return spectrum_ready;
}
