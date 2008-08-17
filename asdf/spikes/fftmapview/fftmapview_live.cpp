/*
	derived from ../../mapview.cpp
	usage: fftmapview_live
	tries to do live fft spec animation from default system audio input
	
*/
// Basic SDLVU usage example: demonstrates how to subclass SDLVU
// rather than just setting the callback funcion.  
// Karin Kosina (vka kyrah) 20080716

#include <sdlvu.h> // will also include GL headers
#include "map.h"

#include <vector>

#include "vorbis_fft.h"

#include <stdio.h>
#include <iostream>

#include <limits.h>

using namespace std;

float *MySpectrum;
size_t MySpectrumsize;

// SDLVU subclass definition
class MySDLVU : public SDLVU
{
  ViennaMap map;
  const PolygonList *polygons;

  public:
     MySDLVU();
		 int MyMainLoop();
		 int spectrumIndex;
     virtual ~MySDLVU() { }
     virtual void Display();

  private:
		GLfloat Rshift;
		GLfloat Gshift;
		GLfloat Bshift;
		GLfloat R;
		GLfloat G;
		GLfloat B;
		GLfloat blah;
		float colors[2048];
		int colorIndex;
		int tmpframecount;
    void DrawPoly(const Polygon &poly, float height);
};

MySDLVU::MySDLVU() {
  polygons = &map.getPolygonsOfFragment(0,0);
  printf("have %d polys\n", polygons->size());
	tmpframecount=0;
	spectrumIndex=0;
}

// override the display method to do your custom drawing

void MySDLVU::Display()
{
	
  static GLUquadric * q = gluNewQuadric();
  BeginFrame();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();

  PolygonList::const_iterator poly, polyend;
  poly = polygons->begin();
  polyend = polygons->end();

  glScalef(0.01, 0.01, 0.01);
  glTranslatef(-500,500,0);
  glColor3f(0.95,0.95,0.95);
  for (;poly != polyend; poly++) {
    DrawPoly(**poly, 50);
  }
	if(spectrumIndex>(MySpectrumsize/4) ) {// || spectrumIndex > polygons->size()) {
		spectrumIndex=0;
	}
/*
  glColor3f(0.95,0.95,0.95);
  gluSphere(q, 0.5, 100, 100);
  glRotatef(70, 1, 0, 0);
  glColor3f(0.6,0.6,0.6);
  gluDisk(q, 0.7, 1, 100, 100);
*/

  glPopMatrix();
  EndFrame();
}

void MySDLVU::DrawPoly(const Polygon &poly, float height) {
  if (poly.size() < 3) return;
	
	tmpframecount+=1;
	
	if(tmpframecount>5) {
		spectrumIndex+=1;
		tmpframecount=0;
	}
	
	
//	glColor3f(1., 0, (GLfloat)MySpectrum[spectrumIndex]);
  glBegin(GL_QUADS);

  for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
    glColor3f(1.,  MySpectrum[spectrumIndex]/16384,  MySpectrum[spectrumIndex]/65384);
		
    glVertex3f(poly[i].x+(int)MySpectrum[spectrumIndex]%5, -poly[i].y+(int)MySpectrum[spectrumIndex]%5, height+(MySpectrum[spectrumIndex]));
    glVertex3f(poly[n].x+(int)MySpectrum[spectrumIndex]%5, -poly[n].y+(int)MySpectrum[spectrumIndex]%5, height+(MySpectrum[spectrumIndex]));
    
		glColor3f(0,0,0);
		
    glVertex3f(poly[n].x, -poly[n].y, 0.);
    
    glVertex3f(poly[i].x, -poly[i].y, 0.);
		
  }

  glEnd();
	
}


vorbisFFT sound;

// viewer main loop
// calls display and does event processing
int MySDLVU::MyMainLoop()
{
  SDL_Surface * surface = SDL_GetVideoSurface();
  pfReshape(surface->w, surface->h);
  bool done = false;
	int fftWait=0;
  while (!done) {
		
		//sound.metafft_compute();

		while(!metafft_ready()) {
			usleep(1000);
			fftWait++;
			if(fftWait>5) break;
		}
		if(fftWait<6) {
			spectrumIndex=0;
			metafft_get_spectrum(MySpectrum, MySpectrumsize);
			spectrumIndex=0;
		}
		
    pfDisplay();
    usleep(5000);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      // FIXME: This does not work on my machine (SDL 1.2.13, Mac OS X)
      // No idea why...?!? 20080718 kyrah
      case SDL_VIDEORESIZE:
        pfReshape(event.resize.w, event.resize.h);
        cout << "Resizing" << endl;
        break;
      case SDL_QUIT:
        done = true;
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        pfKeyboard(event.key);
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        pfMouse(event.button);
        break;
      case SDL_MOUSEMOTION:
        if (event.motion.state & SDL_BUTTON(1)) { // left button down
          pfMotion(event.motion);
        }
        break;
      }
    }
    // NB! SDL_GL_SwapBuffers() is called in endFrame()
  }

  SDL_Quit();
  return 0;
}


MySDLVU sdlvu;



int main(int argc, char *argv[])
{

	
	if (!metafft_init()) {
    fprintf(stderr, "Unable to init metafft\n");
    return 1;
  }
	
	//sound.test();
	//sound.initializeSound();
	
	/*
	if(argv[1] && argc > 0) {
		printf("huhu\n");
		sound.loadDataFromFile(argv[1]);
		sound.start();
	} else {

	}
	*/
	
  sdlvu.Init("SDLVU Basic Object Oriented Example",
             50, 50, 800, 600);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);

  // camera setup
  Vec3f modelmin(-3, -1, 5), modelmax(1, 1, 1); 
  Vec3f lookatcntr(modelmin + modelmax); lookatcntr *= 0.5;
  Vec3f mintoctr(lookatcntr - modelmin);
  Vec3f up(0, 0, 1);
  Vec3f eye(lookatcntr - 3 * (mintoctr - up * (mintoctr * up)));
  float yfov = 45;
  float aspect = 1;    
  float near = 0.1f; // near plane distance relative to model diagonal length
  float far = 40.0f; // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr, 
                   up, yfov, aspect, near, far);

  sdlvu.MyMainLoop();
	//sound.stop();
	//sound.terminateSound();
  return 0;
}
