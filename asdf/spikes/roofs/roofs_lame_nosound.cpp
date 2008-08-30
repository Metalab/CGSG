/*
	derived from ../../mapview.cpp
	usage: fftmapview <oggfile>
	tries to do oggfile->fft spec animation
	
*/

// Basic SDLVU usage example: demonstrates how to subclass SDLVU
// rather than just setting the callback funcion.  
// Karin Kosina (vka kyrah) 20080716

#include <sdlvu.h> // will also include GL headers
#include "map.h"

#include <vector>

//#include "vorbis_fft.h"

#include <stdio.h>
#include <iostream>

#include <limits.h>
#include <float.h>

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
		int polycount;
		float* zHistory;
		int *poly2spec;
		
		void MySDLVU::postSoundSetup();
		void genSpec2PolyMapping(int polyCount, size_t spectrumsize);
    void DrawPoly(const Polygon &poly, float height, float specVar);
		
		void findPlaneMaxima();
		void findPolyMaxima(const Polygon &poly);
		void DrawFloor();
		
		void DrawRoofs(const Polygon &poly);
		void DrawRoofTesselated(const Polygon &poly);
    
		GLfloat maxX, maxY, minX, minY;
		GLdouble **roofVertices;
		
		void generateRoofVertices();
		void TessellatePoly(const Polygon &poly);
		void tessVcb(void *v);
		//void tessVcbd(GLdouble *v[3], void *user_data);
		int rvc;
};

void tessVcbd(GLdouble *v, void *user_data);

MySDLVU::MySDLVU() {
  polygons = &map.getPolygonsOfFragment(0,0);
  printf("have %d polys\n", polygons->size());
	//zHistory = (float*) malloc(polygons->size()*sizeof(float));
	maxX=FLT_MIN;
	minX=FLT_MAX;
	maxY=FLT_MIN;
	minY=FLT_MAX;
}

void MySDLVU::postSoundSetup() {
//	this->polycount = polygons->size()*4;
//	this->poly2spec = new int[this->polycount];
	spectrumIndex=0;
	genSpec2PolyMapping(this->polycount, MySpectrumsize);
	findPlaneMaxima();

	rvc=0;
	//generateRoofVertices();
}


void MySDLVU::genSpec2PolyMapping(int polyCount, size_t spectrumsize) {
	
	float polyIterator = (float)polyCount / ((float)spectrumsize*2);
	float tmpPolyIterator = polyIterator;
	int specIndex = 0;
	
	printf("polyCount: %d\n",polyCount);
	printf("polyIter: %f\n",polyIterator);
	
	for(int i=0;i<polyCount;i++) {
		poly2spec[i] = specIndex;
		printf("polyIter: %f\n",polyIterator);
		printf("specIDX: %d\n",specIndex);
		if( (float) i > polyIterator ) {
			if(specIndex > spectrumsize) {
				printf("specIndex out of bounds. setting 0\n");
				specIndex=0;
			}
			specIndex++;
			polyIterator += tmpPolyIterator;
		}
	}
	
	printf("genSpec2Poly end\n");
}

void MySDLVU::findPolyMaxima(const Polygon &poly) {
	if (poly.size() < 3) return;
	
	for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
    
		maxX = poly[i].x > maxX ? poly[i].x : maxX;
		maxY = poly[i].y > maxY ? poly[i].y : maxY;
		minX = poly[i].x < minX ? poly[i].x : minX;
		minY = poly[i].y < minY ? poly[i].y : minY;
  }

	
}


void MySDLVU::findPlaneMaxima() {
	PolygonList::const_iterator poly, polyend;
	poly = polygons->begin();
  polyend = polygons->end();
	for (;poly != polyend; poly++) {
		findPolyMaxima(**poly);
	}
	printf("minX: %f\n",minX);
	printf("maxX: %f\n",maxX);
	printf("minY: %f\n",minY);
	printf("maxY: %f\n",maxY);
}



void MySDLVU::DrawFloor() {
	glPushMatrix();
	glScalef(0.01, 0.01, 0.01);
  glTranslatef(-500,500,0);
	glTranslatef(0,-1024,0);	//this is stupid  FIXME
	glBegin(GL_QUADS);
		glBegin(GL_QUADS);

    glColor3f(0.7, 0.7, 0.7);
		
		glVertex3f(minX, minY, 0.);
		glVertex3f(minX, maxY, 0.);
		glVertex3f(maxX, maxY, 0.);
		glVertex3f(maxX, minY, 0.);
		
  glEnd();
	glPopMatrix();
}

/*
Tessellation:

1. Allocate a new GLU tessellation object:
GLUtesselator *tess = gluNewTess();
2. Assign callbacks for use with this tessellation object:
	gluTessCallback (tess, GLU_TESS_BEGIN, tcbBegin);
	gluTessCallback (tess, GLU_TESS_VERTEX, tcbVertex);
	gluTessCallback (tess, GLU_TESS_END, tcbEnd);
2a. If your primitive is self-intersecting, you must also specify a callback to create new vertices:
	gluTessCallback (tess, GLU_TESS_COMBINE, tcbCombine);
3. Send the complex primitive data to GLU:
// Assumes: // GLdouble data[numVerts][3];
// ...and assumes the array has been filled with 3D vertex data.
	gluTessBeginPolygon (tess, NULL);
	gluTessBeginContour (tess);
	for (i=0; i<sizeof(data)/(sizeof(GLdouble)*3);i++)
		gluTessVertex (tess, data[i], data[i]);

	gluTessEndContour (tess);
	gluEndPolygon (tess);
	
4. In your callback routines, make the appropriate OpenGL calls:

void tcbBegin (GLenum prim); { glBegin (prim); }
void tcbVertex (void *data) { glVertex3dv ((GLdouble *)data); }
void tcbEnd (); { glEnd (); }
void tcbCombine (GLdouble c[3], void *d[4], GLfloat w[4], void **out) {
	GLdouble *nv = (GLdouble *) malloc(sizeof(GLdouble)*3); nv[0] = c[0]; nv[1] = c[1]; nv[2] = c[2]; *out = nv;  
}

*/

/*
	FIXME;
	trianglestrip coords for roofs should be generated once (per ViennaMapfragment)
	move it to map.cpp?
*/

void MySDLVU::DrawRoofs(const Polygon &poly) {
	if (poly.size() < 3)
		return;
	
	GLUtesselator *tess = gluNewTess();
	
	int blah = poly.size();
	
	GLdouble **tmppd = (GLdouble**) malloc(sizeof(GLdouble) * blah);
	for (int i=poly.size()-1; i >= 0; i--) {	
		tmppd[i] = (double*) malloc(sizeof( double) * 3);
		tmppd[i][0] =  poly[i].x;
		tmppd[i][1] = -poly[i].y;
		tmppd[i][2] =  50;
	}
	
	
	gluTessCallback(tess,  GLU_TESS_BEGIN, (GLvoid(*)())&glBegin);
	gluTessCallback(tess, GLU_TESS_VERTEX,	(GLvoid (*)())&glVertex3dv);
	gluTessCallback(tess, GLU_TESS_END,		(GLvoid(*)())&glEnd);
	
	gluTessBeginPolygon (tess, NULL);
	gluTessBeginContour (tess);
	glColor3f(0.,  1,  0);
	for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
		gluTessVertex (tess, (GLdouble*)tmppd[i], tmppd[i]);
		gluTessVertex (tess, (GLdouble*)tmppd[n], tmppd[n]);
	}
	gluTessEndContour (tess);
	gluEndPolygon (tess);
	gluDeleteTess(tess);
	for (int i=poly.size()-1; i >= 0; i--) {	
		free(tmppd[i]);
	}
	free(tmppd);
}

void MySDLVU::DrawRoofTesselated(const Polygon &poly) {
  if (poly.size() < 3) return;
	
//	glColor3f(1., 0, (GLfloat)MySpectrum[spectrumIndex]);
  glBegin(GL_TRIANGLES);

  for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
    glColor3f(0.,  1,  0);
		
    glVertex3d(poly[i].dx, -poly[i].dy, 50);
    glVertex3d(poly[n].dx, -poly[n].dy, 50);
		
  }

  glEnd();
}


void MySDLVU::generateRoofVertices() {
	
	PolygonList::const_iterator poly, polyend;
	poly = polygons->begin();
  polyend = polygons->end();
	for (;poly != polyend; poly++) {
			TessellatePoly(**poly);
	}
}


void MySDLVU::tessVcb(void *v) {

}


//    non class function:

void tessVcbd(GLdouble *v, void *user_data) { //USE WITH GLU_TESS_VERTEX_DATA
	
  return;
  

  
}

typedef struct {
  const Polygon *poly;
  int vIndex;
} tessUserData;

void MySDLVU::TessellatePoly(const Polygon &poly) {
	
	if (poly.size() < 3)
		return;
	
	GLUtesselator *tess = gluNewTess();
	
	GLdouble **tmppd = (GLdouble**) malloc(sizeof(GLdouble) * poly.size());
	this->roofVertices = (GLdouble**) malloc(sizeof(GLdouble) * poly.size());
	for (int i=poly.size()-1; i >= 0; i--) {	
		tmppd[i] = (double*) malloc(sizeof( double) * 3);
		tmppd[i][0] =  poly[i].x;
		tmppd[i][1] = -poly[i].y;
		tmppd[i][2] =  50;
	}
	
	gluTessCallback(tess,  GLU_TESS_BEGIN, (GLvoid(*)())&glBegin);
	//gluTessCallback(tess, GLU_TESS_VERTEX,	(GLvoid (*)())&tessVcb);				//FIXME
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA,	(GLvoid (*)())&tessVcbd);
	gluTessCallback(tess, GLU_TESS_END,		(GLvoid(*)())&glEnd);
	
	rvc = 0; // for tessVcb
  tessUserData polyData;
//  polyData.poly = poly;
//  polyData.vIndex = 0;
	//gluTessBeginPolygon (tess, (GLvoid*)&poly); //&poly = USER_DATA
  gluTessBeginPolygon (tess, (GLvoid*)&polyData); //&poly = USER_DATA
	gluTessBeginContour (tess);
	glColor3f(0.,  1,  0);
	for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
		gluTessVertex (tess, (GLdouble*)tmppd[i], tmppd[i]);
		//gluTessVertex (tess, (GLdouble*)tmppd[n], tmppd[n]);
	}
	gluTessEndContour (tess);
	gluEndPolygon (tess);
	
	gluDeleteTess(tess);
	for (int i=poly.size()-1; i >= 0; i--) {	
		free(tmppd[i]);
	}
	free(tmppd);
}

// override the display method to do your custom drawing
void MySDLVU::Display()
{
	
  static GLUquadric * q = gluNewQuadric();
  BeginFrame();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	int p = 0;
	
	PolygonList::const_iterator poly, polyend;
	
	if(spectrumIndex>(MySpectrumsize) ) {
		printf("SPECINDEX TO HIGH!!!!!!!!\n\n\n\n\n\n");
		spectrumIndex=0;
	}
	  glPushMatrix();

  poly = polygons->begin();
  polyend = polygons->end();

  glScalef(0.01, 0.01, 0.01);
  glTranslatef(-500,500,0);
  glColor3f(0.95,0.95,0.95);
	
  for (;poly != polyend; poly++) {
			DrawPoly(**poly, 50, 0);
			DrawRoofs(**poly);
      //DrawRoofTesselated(**poly);
		p++;
  }

	

  glPopMatrix();
	
	DrawFloor();
	//spectrumIndex=0;
  EndFrame();
	

}




void MySDLVU::DrawPoly(const Polygon &poly, float height, float specVar) {
  if (poly.size() < 3) return;
	
//	glColor3f(1., 0, (GLfloat)MySpectrum[spectrumIndex]);
  glBegin(GL_QUADS);

  for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
    glColor3f(0.,  0,  0);
		
//		glVertex3f(poly[i].x, -poly[i].y, height+(specVar/2));
//    glVertex3f(poly[n].x, -poly[n].y, height+(specVar/2));
    glVertex3f(poly[i].x, -poly[i].y, height);
    glVertex3f(poly[n].x, -poly[n].y, height);

		//farbe "unten":
		glColor3f(0.7,0.7,0.7);
		
    glVertex3f(poly[n].x, -poly[n].y, 0.);
    
    glVertex3f(poly[i].x, -poly[i].y, 0.);
		
  }

  glEnd();
	
}





// viewer main loop
// calls display and does event processing
int MySDLVU::MyMainLoop()
{
  SDL_Surface * surface = SDL_GetVideoSurface();
  pfReshape(surface->w, surface->h);
  bool done = false;
	int fftWait=0;
	
	this->postSoundSetup();
  
	while (!done) {
		
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
					switch( event.key.keysym.sym ){
						case SDLK_RIGHT:
//							sound.skip((signed int)32768);
						break;
						case SDLK_LEFT:
//							sound.skip(-32768);
						break;
						
						case SDLK_UP:
//							sound.volumeChange((float)1.5);
						break;
						case SDLK_DOWN:
//							sound.volumeChange((float)-1.5);
						break;
					}					
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
  sdlvu.Init("SDLVU Basic Object Oriented Example",
             50, 50, 800, 600);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);

	float myLight[] = { 0.1, 0.6, 0.9, 0.9 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, myLight);
	glEnable(GL_NORMALIZE);

  // camera setup
  Vec3f modelmin(-3, -1, 5), modelmax(1, 1, 1); 
  Vec3f lookatcntr(modelmin + modelmax); lookatcntr *= 0.5;
  Vec3f mintoctr(lookatcntr - modelmin);
  Vec3f up(0, 0, 1);
  Vec3f eye(lookatcntr - 5 * (mintoctr - up * (mintoctr * up)));
  float yfov = 45;
  float aspect = 1;    
  float near = 0.1f; // near plane distance relative to model diagonal length
  float far = 100.0f; // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr, 
                   up, yfov, aspect, near, far);


  sdlvu.MyMainLoop();

  return 0;
}
