/*

  adjust visualized fft window size and move it around with up,down,q,a, skip ahead with left, right


*/

#include <sdlvu.h> // will also include GL headers
#include "map_augenkrach.h"

#include <vector>

//#include "vorbis_fft.h"

#include <stdio.h>
#include <iostream>

#include <limits.h>
#include <float.h>

#include <fmod.hpp>
#include <fmod_errors.h>

#include "midi.h"
#include "portmidi.h"
//#include "porttime.h"
//#include "pmutil.h"

//for timing
#include <ctime>
clock_t start,finish,start2,finish2;
double mytime;


//image file directory:
char *imageDIR = "imgs_adjusted/";

//baergh:
//#include "/Developer/FMOD Programmers API/examples/common/wincompat.h"

using namespace std;

const int SPECTRUM_LENGTH = 4096;
float MySpectrum[SPECTRUM_LENGTH];
const int SPEC_DEPTH = 100;

//size of raster spectrum w spectrum_length gets mapped to
//all vars in comparison 2 4096 spectrum_length 
// 2x32 nice for bass only visualization  (non log spectrum)
//32x64 nice for overall visualization (bass is not that much visible)
const int DEF_RASTER_X = 8;//32;
const int DEF_RASTER_Y = 8;//64;
const int DEF_RASTER_POLY_SEARCH_DIST = 400;

FMOD::System *fmodsystem;
FMOD::Sound *sound;
FMOD::Channel *channel;
FMOD_RESULT result;
FMOD_CREATESOUNDEXINFO exinfo;
bool  iscoreaudio = false;

#define OUTPUTRATE          44100

void ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}


// SDLVU subclass definition
class MySDLVU : public SDLVU
{
  ViennaMap map;
  const PolygonList *polygons;
  BuildingList *buildings;
  public:
    MySDLVU();
    int MyMainLoop();
    int spectrumIndex;
    virtual ~MySDLVU() { }
    virtual void Display();

    //SpecBuffer specRingBuffer;

    virtual void Motion(const SDL_MouseMotionEvent & event);
    bool muteToggle;
    bool motionToggleModifier;

    bool drawBuildingsToggle;
    bool drawGridToggle;
    bool drawSpec2GridLinesToggle;
    bool drawRoofToggle;
    
    static void processMidiMsg(void *userdata, PmEvent *buffer);
    bool updateSpectrum;
  private:
    int raster_x;
    int raster_y;
    float raster_poly_search_distance;

		float colors[2048];
		int colorIndex;
		int tmpframecount;
		int polycount;
		float* zHistory;
		int *poly2spec;
		int *spec2raster;

    int spectrumDivisor;
    int specStartIndex;

		void postSoundSetup();

    void DrawPoly(const Polygon &poly, float height, float specVar);

		void findPlaneMaxima();
		void findPolyMaxima(const Polygon &poly);
		void DrawFloor();

		void DrawRoofs(const Polygon &poly);
//    void DrawRoofTesselated(Building &building);
    void DrawRoofTesselated(Building &building, float height, float specVar);
    void DrawCentroid(Building &building, float height);
    void DrawMeanCenter(Building &building, float height);
    void DrawBuildingByRasterSpectrum(Building &building, float height);

		GLfloat maxX, maxY, minX, minY;
		GLdouble **roofVertices;

		void generateRoofVertices();
		void TessellateBuilding(Building &building);
		void tessVcb(void *v);
		//void tessVcbd(GLdouble *v[3], void *user_data);
		int rvc;

    void genSpec2PolyMapping(int polyCount, size_t spectrumsize, int specStartIndex);
    void genPoly2RasterFactors(int rasterX, int rasterY, float maxDistance);
    void genSpec2RasterMapping(int rasterX, int rasertY, int specStartIndex);
    void genSpec2RasterMapping_midStart(int rasterX, int rasterY, int specStartIndex);
    void DrawGrid(int rasterX, int rasterY, int rasterZ);
    void DrawFFTGrid(int rasterX, int rasterY, int rasterZ);
    void DrawCentroid2GridLines(Building &building, float height, float gridHeight, int rasterX, int rasterY);

  int currentImage;
  void loadNextImage(int next);
};

void tessVcbd(void *v, void *user_data);

void MySDLVU::processMidiMsg(void *userdata, PmEvent *buffer) {
  printf("mysdlvu static function called.\n");
  MySDLVU *thisp = (MySDLVU*)userdata;
  //printf("midiMsg: %d\n", thisp->spectrumIndex);
  printf("MySDLVU-midi: %08x\n", buffer->message);
  if (Pm_MessageStatus(buffer->message) == 0x90 && Pm_MessageData2(buffer->message) != 0) {
     printf("Note: %d\n", Pm_MessageData1(buffer->message));
     
     //ugly test hack:
     //mess with the spectrum buffer:
     int c = (int)Pm_MessageData1(buffer->message);
     memset(&MySpectrum, 0, sizeof(float)*SPECTRUM_LENGTH);
     
     MySpectrum[c] = 0.5;
//     MySpectrum[c] = 0.5 * ((16000/SPECTRUM_LENGTH / c) )  / 200;
     thisp->updateSpectrum = false;
   } else {
     thisp->updateSpectrum = true;
   }
}
/*

startMidi(mycallback, this);


void mycallback(void *userdata)
{
   Myclass *thisp = (MyClass *)userdata;
}
*/


//ugly hack for instatiating map with imageDIR
MySDLVU::MySDLVU():map(imageDIR) {

  this->spectrumIndex =0;
  
  //void (*p)(void *userdata);
  midiCallbackFuncType *p;
  p = MySDLVU::processMidiMsg;
  printf("calling startMidi...\n");
  startMidi(p,this);
  
  updateSpectrum = true;

  currentImage = 0;
  polygons = &map.getPolygonsOfFragment(currentImage,0);
  if( map.hasFragment(currentImage,0) ) printf("fragment 0,0 loaded\n");
  buildings = &map.getBuildingsOfFragment(currentImage,0);
  printf("have %d polys\n", polygons->size());

  printf("have %d buildings\n", buildings->size());
	//zHistory = (float*) malloc(polygons->size()*sizeof(float));
	maxX=FLT_MIN;
	minX=FLT_MAX;
	maxY=FLT_MIN;
	minY=FLT_MAX;

  spectrumDivisor = 2;
  specStartIndex = 0;
  //setupSpectrumHistory();

  raster_x = DEF_RASTER_X;
  raster_y = DEF_RASTER_Y;
  raster_poly_search_distance = DEF_RASTER_POLY_SEARCH_DIST;

  muteToggle = false;
  motionToggleModifier=false;

  drawBuildingsToggle = true;
  drawRoofToggle = true;
  drawGridToggle = false;
  drawSpec2GridLinesToggle = false;
  
}

void MySDLVU::loadNextImage(int next) {
//  printf("PNGfileList size:%d\n\n",this->map.PNGfileList.size());
//  currentImage += currentImage + next < this->map.PNGfileList.size()-1 ? next : 0;
  
  if(currentImage + next < this->map.PNGfileList.size() && currentImage + next > -1) {
    currentImage += next;
  }
  printf("currImg: %d\n", currentImage);

  polygons = &map.getPolygonsOfFragment(currentImage,0);
  if( map.hasFragment(currentImage,0) ) printf("fragment %d,0 loaded\n",currentImage);
  buildings = &map.getBuildingsOfFragment(currentImage,0);
  printf("have %d polys\n", polygons->size());
  
  printf("have %d buildings\n", buildings->size());
  
  //this->raster_x *= 2;
  //this->raster_y *= 2;
}

void MySDLVU::DrawBuildingByRasterSpectrum(Building &building, float height) {
  float specVar = 0;

  //Building *build = *building;
  Polygon *poly = (Polygon*) building.poly;
  if ((*poly).size() < 3) return;

  Raster2VertexList *rasterPointList = building.rasterPoints2affect;
  VertexIndexList *verticesOrder = building.orderedVertices;

  //glBegin(GL_TRIANGLES);
  //glColor3f(0.,  1,  0);
  glColor3f(1.,  (specVar*2000)/16384,  (specVar*2000)/65384);

  float specDistHeightFactor = 0;
  for (int i=(*rasterPointList).size()-1, n; i >= 0; i--) {
    int rp = (*rasterPointList)[i].rasterpoint;
    float dist = (*rasterPointList)[i].distance;
    //specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * (dist);         //rvf.rasterpoint = (x*rasterX)+(y);


    //use average spectrum var of all rasterpoints for this building:
    //specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[rp]+1)) ) * dist / 200;         //rvf.rasterpoint = (x*rasterX)+(y);

    //use maximum spectrum var of all rasterpoints for this building
    float tmpspecv = MySpectrum[this->spec2raster[rp]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[rp]+1)) ) * dist / 200;
    specVar = tmpspecv > specVar ? tmpspecv : specVar;
  }
  //use average spectrum var of all rasterpoints for this building:
  //specVar = (specDistHeightFactor/(*rasterPointList).size());


  //DrawPoly(*poly, 80, specVar*100);
  if(drawRoofToggle)
    DrawRoofTesselated(building, height, specVar*100);  //*1/specVar     --> clifford je hoeher frequenz desto kleiner amplitude. energie = f*a??? FIXME  (also above! -> specVar =...)
  ///*

  //draw the building:
  glBegin(GL_QUADS);

  for (int i=(*poly).size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = (*poly).size()-1;
    glColor3f(1.,  (specVar*20)/16384,  (specVar*20)/65384);

    glVertex3f((*poly)[i].x, -(*poly)[i].y, height+(specVar*100));
    glVertex3f((*poly)[n].x, -(*poly)[n].y, height+(specVar*100));

		//farbe "unten":
    glColor3f(0,0,0);
    
    glVertex3f((*poly)[n].x, -(*poly)[n].y, (specVar*100));
    glVertex3f((*poly)[i].x, -(*poly)[i].y, (specVar*100));
//    glVertex3f((*poly)[n].x, -(*poly)[n].y, 0.);
//    glVertex3f((*poly)[i].x, -(*poly)[i].y, 0.);

  }

  glEnd();
  //*/
}

void MySDLVU::genSpec2RasterMapping_midStart(int rasterX, int rasterY, int specStartIndex) {
	int rasterSize = rasterX*rasterY;
	  float rasterIterator = (float)rasterSize / (float)SPECTRUM_LENGTH;

	  //printf("rasterIterator: %d\n",rasterIterator);

	  this->spec2raster = new int[rasterSize];

	  if(specStartIndex > SPECTRUM_LENGTH || specStartIndex < 0) {
	    specStartIndex = 0;
	  }
	  int specIndex = specStartIndex;

	  specIndex = (SPECTRUM_LENGTH/2)/2;

	  //init
	  for(int i=0;i < rasterSize; i++) {
	      this->spec2raster[i] = specIndex; //=0
	      //specIndex++;
	  }

	  int c=0;
	  int specAdd = -1;
	  //map spectrum to the whole raster:

	  //rvf.rasterpoint = x * rasterY + y;
	  for(int i=0;i < rasterX; i++) {

	    for(int q=0;q < rasterY; q++) {
	//      printf("s2r:%d\n",i*2+q);
	//      this->spec2raster[i+q] = specIndex;
	        int p = i * rasterY + q;
	        //printf("p: %d\t",p);
	        this->spec2raster[p] = specIndex;
	        specIndex += specAdd;
	        if(specIndex < 0) {
	        	specIndex = 0;
	        	specAdd = 1;
	        }

	      c++;
	    }

	  }

	  for(int i=0;i < rasterSize; i++) {
	      //printf("s2r%d:\t%d\t", i, this->spec2raster[i]);
	    }


	  //printf("Last specIndex:%d\n",specIndex);
	  //printf("Last c:%d\n",c);
	  //exit(1);

	  //map spectrum to n rows of spectrum:
}

void MySDLVU::genSpec2RasterMapping(int rasterX, int rasterY, int specStartIndex) {
  int rasterSize = rasterX*rasterY;
  float rasterIterator = (float)rasterSize / (float)SPECTRUM_LENGTH;

  printf("rasterIterator: %f\n",rasterIterator);

  this->spec2raster = new int[rasterSize];

  if(specStartIndex > SPECTRUM_LENGTH || specStartIndex < 0) {
    specStartIndex = 0;
  }
  int specIndex = specStartIndex;



  //init
  for(int i=0;i < rasterSize; i++) {
      this->spec2raster[i] = specIndex; //=0
      //specIndex++;
  }

  int c=0;
  int specAdd = -1;
  //map spectrum to the whole raster:

  //rvf.rasterpoint = x * rasterY + y;
  for(int i=0;i < rasterX; i++) {

    for(int q=0;q < rasterY; q++) {
//      printf("s2r:%d\n",i*2+q);
//      this->spec2raster[i+q] = specIndex;
        int p = i * rasterY + q;
        //printf("p: %d\t",p);
        this->spec2raster[p] = specIndex;
        specIndex++;
        /*specIndex += specAdd;
        if(specIndex < 0) {
        	specIndex = 0;
        	specAdd = 1;
        }*/
      c++;
    }

  }

  for(int i=0;i < rasterSize; i++) {
      //printf("s2r%d:\t%d\t", i, this->spec2raster[i]);
    }


  //printf("Last specIndex:%d\n",specIndex);
  //printf("Last c:%d\n",c);
  //exit(1);

  //map spectrum to n rows of spectrum:
}


//old dont use!!!
//spectrumsize is also used as "window" size
void MySDLVU::genSpec2PolyMapping(int polyCount, size_t spectrumsize, int specStartIndex) {
	printf("genSpec2PolyMapping is borken & deprecated    returning!\n");
	return;

/*
	float polyIterator = (float)polyCount / ((float)spectrumsize);
	float tmpPolyIterator = polyIterator;
  if(specStartIndex > SPECTRUM_LENGTH || specStartIndex < 0) {
    specStartIndex = 0;
  }
	int specIndex = specStartIndex;

	//printf("polyCount: %d\n",polyCount);
	//printf("polyIter: %f\n",polyIterator);
	printf("specStartIndex: %d\n",specStartIndex);

	for(int i=0;i<polyCount;i++) {
		poly2spec[i] = specIndex;
		//printf("polyIter: %f\n",polyIterator);
		//printf("i: %d specIDX: %d\n", i, specIndex);
		if( (float) i > polyIterator ) {
			if(specIndex > SPECTRUM_LENGTH) {
				printf("specIndex out of bounds. setting 0\n");
				specIndex=specStartIndex;
			}
			specIndex++;
			polyIterator += tmpPolyIterator;
		}
	}
*/
	//printf("genSpec2Poly end\n");
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
  glTranslatef(0,-3200,0);
	//glTranslatef(0,-1024,0);	//this is stupid  FIXME
	//(GL_QUADS);
		glBegin(GL_QUADS);

    //glColor3f(0.7, 0.7, 0.7);
//    glColor3f(0.8, 0.8, 0.8);
		glColor3f(0.1, 0.2, 0.3);
		glVertex3f(minX, minY, 0.);
		glVertex3f(minX, maxY, 0.);
		glVertex3f(maxX, maxY, 0.);
		glVertex3f(maxX, minY, 0.);

  glEnd();
	glPopMatrix();
}


void MySDLVU::DrawRoofTesselated(Building &building, float height, float specVar) {
  Polygon *poly = (Polygon*) building.poly;
  VertexIndexList *verticesOrder = building.orderedVertices;
  if ((*poly).size() < 3) return;

  glBegin(GL_TRIANGLES);
  //glColor3f(0.,  1,  0);
  glColor3f(1.,  (specVar/100*20)/16384,  (specVar/100*20)/65384);

  for (int i=(*verticesOrder).size()-1, n; i >= 0; i--) {
    glVertex3f((*poly)[(*verticesOrder)[i]].x, -(*poly)[(*verticesOrder)[i]].y, height+(specVar));
  }

  glEnd();

}


void MySDLVU::DrawCentroid2GridLines(Building &building, float height, float gridHeight, int rasterX, int rasterY) {
  int maxX = this->map.fragmentImageWidth;
  int maxY = this->map.fragmentImageHeight;

  int spacingX = maxX / rasterX;
  int spacingY = maxY / rasterY;

  Raster2VertexList* rasterList = building.rasterPoints2affect;
  Raster2VertexList::const_iterator rasterPoint, rasterPointEnd;

  rasterPoint = rasterList->begin();
  rasterPointEnd = rasterList->end();
  float dist = 0;

  glBegin(GL_LINES);
  for (int i=(*rasterList).size()-1, n; i >= 0; i--) {
    float y = (*rasterList)[i].rasterpoint % rasterY;
    float x = ( (*rasterList)[i].rasterpoint - y ) / rasterY;
    //rvf.rasterpoint = x * rasterY + y;
    dist = (*rasterList)[i].distance;
    glColor3f(1/dist*0.6,  1/dist*0.75,  0.3);
    glVertex3f( -building.fCenterX, building.fCenterY, 0+MySpectrum[this->spec2raster[(*rasterList)[i].rasterpoint]+1]* ((16000/SPECTRUM_LENGTH * (x*rasterY+y+1))) * 50);
    glColor3f(0.023,  0.3*(1/dist),  0.2);
    glVertex3f( spacingX*x, -spacingY*y, gridHeight);
  }
  glEnd();
}


void MySDLVU::DrawFFTGrid(int rasterX, int rasterY, int rasterZ=0) {
//  int maxX = this->map.fragmentImageWidth;
//  int maxY = this->map.fragmentImageHeight;

  int maxX = this->maxX;
  int maxY = this->maxY;

  int spacingX = maxX / rasterX;
  int spacingY = maxY / rasterY;

  glBegin(GL_LINES);

  for(int x = 0; x < rasterX; x++) {

      for(int y = 0; y < rasterY; y++) {
        //float specHeight = MySpectrum[this->spec2raster[x*rasterY+y]]*4000;
        //float specHeight = MySpectrum[this->spec2raster[x*rasterY+y]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[x*rasterY+y]))) * 50;
      	float specHeight = MySpectrum[this->spec2raster[x*rasterY+y]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[x*rasterY+y]))) * 50;
        //float specHeight = 0;
        glColor3f(0.0,  0.86*MySpectrum[this->spec2raster[x*rasterY+y]]*10,  0.8*MySpectrum[this->spec2raster[x*rasterY+y]]*10);
        glVertex3f( spacingX*x,           -y*spacingY,            rasterZ+specHeight);
        glVertex3f( spacingX*x,           -y*spacingY - spacingY, rasterZ+specHeight);
        glVertex3f( spacingX*x,           -y*spacingY - spacingY, rasterZ+specHeight);
        glVertex3f( spacingX*x+spacingX,  -y*spacingY - spacingY, rasterZ+specHeight);
        glVertex3f( spacingX*x+spacingX,  -y*spacingY - spacingY, rasterZ+specHeight);
        glVertex3f( spacingX*x+spacingX,  -y*spacingY,            rasterZ+specHeight);
        glVertex3f( spacingX*x+spacingX,  -y*spacingY,            rasterZ+specHeight);
        glVertex3f( spacingX*x,           -y*spacingY,            rasterZ+specHeight);
      }
  }

  glEnd();
}



void MySDLVU::DrawGrid(int rasterX, int rasterY, int rasterZ=0) {
  int maxX = this->map.fragmentImageWidth;
  int maxY = this->map.fragmentImageHeight;

  int spacingX = maxX / rasterX;
  int spacingY = maxY / rasterY;

  glBegin(GL_LINES);
  glColor3f(0.0,  0.86,  0.8);

  for(int x = 0; x <= rasterX; x++) {
        glVertex3f( spacingX*x, 0, rasterZ);
        glVertex3f( spacingX*x, -maxY, rasterZ);
  }
  for(int y = 0; y <= rasterY; y++) {
      glVertex3f( 0, -spacingY*y, rasterZ);
      glVertex3f( maxX, -spacingY*y, rasterZ);
  }

  glEnd();
}


//this is way to slow!   0.5 sec for 64x64 grid, dist=100
// search in a bounding box, near the centroid of the building, instead of the whole grid!
//capt. obvious to the rescue!
//
//50,50 for 1000 polys?(3200px*3200px at 500 extentwidth)
void MySDLVU::genPoly2RasterFactors(int rasterX, int rasterY, float maxDistance) {

    //for raster draw lines

    int maxX = this->map.fragmentImageWidth;
    int maxY = this->map.fragmentImageHeight;
    int rasterZ = 0;

    int spacingX = maxX / rasterX;
    int spacingY = maxY / rasterY;

    //for buildings
    ///*
    int minRP = 0;
    int maxRP = 0;
    //printf(" minRP:%d maxRP:%d\n", minRP, maxRP);
    BuildingList::const_iterator build, buildend;
    PolygonList::const_iterator poly, polyend;
    build = buildings->begin();
    buildend = buildings->end();
    float dist = 0;
    for (;build != buildend; build++) {
      Building *building = *build;
//      Raster2VertexList *rasterList = building->rasterPoints2affect;

      (*building).rasterPoints2affect = new Raster2VertexList();
      Raster2VertexList* rasterList = (*building).rasterPoints2affect;
//x= building->fCenterX - maxDistance
        for(int x=0; x < rasterX; x++) {
          for(int y=0; y < rasterY; y++) {
            //printf(" x:%f y:%f\n", x, y);
            dist = sqrt(pow(spacingX*x - -1*building->fCenterX,2) + pow(-spacingY*y - building->fCenterY,2) );
            if(dist <= maxDistance) {
              //save the raster point for this the building
              Raster2VertexFactor rvf;
              rvf.rasterpoint = x * rasterY + y;
              maxRP = rvf.rasterpoint > maxRP ? rvf.rasterpoint : maxRP;
              minRP = rvf.rasterpoint < minRP && rvf.rasterpoint > 0 ? rvf.rasterpoint : minRP;
              //printf("rp: %d\t",rvf.rasterpoint);
              rvf.distance = dist;
              (*rasterList).push_back(rvf);
            }
          }
        }
    }
    printf("\nminRP: %d\t",minRP);
    printf("maxRP: %d\t\n",maxRP);
    //exit(1);
    //glEnd();
    //*/
}



void MySDLVU::DrawCentroid(Building &building, float height) {
  Polygon *poly = (Polygon*) building.poly;
  if ((*poly).size() < 3) return;

	glBegin(GL_LINES);
  glColor3f(0.,  1,  50/65384);
  /*
  FIXME   why do i have to invert the coords here? in drawpoly its +x, -y  here i have to do -x, +y  to make it look(!) correct. maybe computation in map.cpp is still wrong
  */
  if(-building.fCenterX < 0) {
    glColor3f(0.,  1,  0.8);
  }  
  glVertex3f( -building.fCenterX, building.fCenterY, 0);
  glVertex3f( -building.fCenterX, building.fCenterY, 200);

  glVertex3f( -building.fCenterX, building.fCenterY, 0);
  glVertex3f( -building.fCenterX+10, building.fCenterY, 0);

  glVertex3f( -building.fCenterX+10, building.fCenterY, 0);
  glVertex3f( -building.fCenterX+10, building.fCenterY+10, 0);

  glVertex3f( -building.fCenterX+10, building.fCenterY+10, 0);
  glVertex3f( -building.fCenterX, building.fCenterY+10, 0);

  glVertex3f( -building.fCenterX, building.fCenterY+10, 0);
  glVertex3f( -building.fCenterX, building.fCenterY, 0);

  glEnd();
}

void MySDLVU::DrawMeanCenter(Building &building, float height) {
  Polygon *poly = (Polygon*) building.poly;
  VertexIndexList *verticesOrder = building.orderedVertices;
  if ((*poly).size() < 3) return;
	glBegin(GL_LINES);
  glColor3f(0.3,  0.01,  1);
  glVertex3f( building.fmeanCenterX, -building.fmeanCenterY, 0);
  glVertex3f( building.fmeanCenterX, -building.fmeanCenterY, 200);
  glEnd();
}



void MySDLVU::generateRoofVertices() {
	BuildingList::const_iterator build, buildend;
	build = buildings->begin();
  buildend = buildings->end();
	for (;build != buildend; build++) {
			TessellateBuilding(**build);
	}
}


typedef struct {
  const Polygon *poly;
  Building *building;
  int vIndex;
  int polySize;

} tessUserData;

//    non class function:
void mytessError(GLenum err, void *)
{
  gluErrorString(err);
}

void mytessBegin(GLenum type, void *user_data) {
	//printf("begin with type: %d\n",type);
}

void mytessEdgeFlagCB(GLboolean flag ) {
	//printf("flagCB: %d\n",flag);
}


void tessVcbd(void *v, void *user_data) { //USE WITH GLU_TESS_VERTEX_DATA
  tessUserData *tud = (tessUserData*) user_data;
	Building *building = (Building*) tud->building;
  VertexIndexList* vil = (*building).orderedVertices;

  int vertexIndex = ((GLdouble*)v)[3];
  (*vil).push_back(vertexIndex);

  //printf("vertexIndex: %d\n",vertexIndex);
}



void MySDLVU::TessellateBuilding(Building &building) {

  Polygon *poly = (Polygon*) building.poly;

	if ((*poly).size() < 3)
		return;

	GLUtesselator *tess = gluNewTess();

  // gotcha:
  //this is done in 2 loops with an extra array of all vertices, because glutessvertex doesnt work with temporary/local vars only!
  //see man page
  //

	GLdouble *tmppd = (GLdouble*) malloc(sizeof(GLdouble) * (*poly).size()*4);
  //printf("newPoly with %d vertices\n",(*poly).size());

	for (int i=(*poly).size()-1; i >= 0; i--) {
		tmppd[4*i] =  (*poly)[i].x;
		tmppd[4*i+1] = -(*poly)[i].y;
		tmppd[4*i+2] =  50;
		tmppd[4*i+3] =  i;
	}

	gluTessCallback(tess,  GLU_TESS_BEGIN_DATA, (GLvoid(*)())&mytessBegin);
  gluTessCallback(tess,  GLU_TESS_EDGE_FLAG, (GLvoid(*)())&mytessEdgeFlagCB); //forces generation of GL_TRIANGLES only  yeah!
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA,	(GLvoid (*)())&tessVcbd);
	//gluTessCallback(tess, GLU_TESS_END,		(GLvoid(*)())&glEnd);

	rvc = 0; // for tessVcb
  tessUserData polyData;
  polyData.poly = poly;
  polyData.building = &building;
  polyData.vIndex = 0;
  polyData.polySize = poly->size();
	//gluTessBeginPolygon (tess, (GLvoid*)&poly); //&poly = USER_DATA
  gluTessBeginPolygon (tess, (GLvoid*)&polyData); //&poly = USER_DATA
	gluTessBeginContour (tess);

	for (int i=(*poly).size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = (*poly).size()-1;
		gluTessVertex (tess, (GLdouble*)&tmppd[4*i], &tmppd[4*i]);
	}
	gluTessEndContour (tess);
	gluEndPolygon (tess);

	gluDeleteTess(tess);

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


  glPushMatrix();

  poly = polygons->begin();
  polyend = polygons->end();

  glScalef(0.01, 0.01, 0.01);
  glTranslatef(-500,500,0);
  glColor3f(0.95,0.95,0.95);

  ///*
  p=0;
	BuildingList::const_iterator build, buildend;
	//PolygonList::const_iterator poly, polyend;
	build = buildings->begin();
	buildend = buildings->end();

  //DrawGrid(raster_x, raster_y,-200);

  if (drawGridToggle)
    DrawFFTGrid(raster_x, raster_y,0);

	for (;build != buildend; build++) {
      //DrawCentroid(**build, 100); //slow debug func
      //DrawMeanCenter(**build, 100);

      if(drawSpec2GridLinesToggle)
        DrawCentroid2GridLines(**build, 0, 0, raster_x, raster_y);
      if(drawBuildingsToggle)
        DrawBuildingByRasterSpectrum(**build, 50);
      p++;
	}


  glPopMatrix();
//*/
	//DrawFloor();

  EndFrame();


}




void MySDLVU::DrawPoly(const Polygon &poly, float height, float specVar) {
  if (poly.size() < 3) return;

//	glColor3f(1., 0, (GLfloat)MySpectrum[spectrumIndex]);
  glBegin(GL_QUADS);

  for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
    glColor3f(1.,  (specVar*2000)/16384,  (specVar*2000)/65384);


    glVertex3f(poly[i].x, -poly[i].y, height+(specVar*3000));
    glVertex3f(poly[n].x, -poly[n].y, height+(specVar*3000));

		//farbe "unten":
		//glColor3f(0.7,0.7,0.7);
    glColor3f(0,0,0);

    glVertex3f(poly[n].x, -poly[n].y, 0.);

    glVertex3f(poly[i].x, -poly[i].y, 0.);

  }

  glEnd();
}


/// Handler for 'active' mouse drag events, i.e. dragging with a button pressed.
/**
 *  This method can be overridden to perform application specific
 *  funcionality (e.g. direct manipulation of scene objects).  The
 *  default implementation does some important work for handling
 *  camera manipulation (world navigation), so if you override this
 *  method you should always call SDLVU::Mouse() from your override if
 *  you wish to preserve the built-in navigation capabilities.
 *
 *  The exact effect the default implementation has on the current
 *  camera depends upon the current world navigation mode.
 *  See SetWorldNavMode().
 *
 *  Users not interested in creating an object-oriented app can simply
 *  call SDLVU's \c SetMotionFunc to set a callback directly. If you
 *  do so you can still call GetCurrent()->Motion() or
 *  SDLVU::DefaultMotionFunc(), in your handler to get the default
 *  behavior back.
 *
 *  @param event The most recent SDL_MouseMotionEvent.
 *
 *  @see Motion, SetWorldNavMode
 */
void MySDLVU::Motion(const SDL_MouseMotionEvent & event)
{
  // STORE PREVIOUS NEW MOUSE POSITION (OLD)
  OldX=NewX; OldY=NewY;

  //if (LeftButtonDown) //move without that nasty buttonpress needed, see mainloop & motionToggleModifier
  {
    // STORE THE NEW MOUSE POSITION
    NewX=event.x; NewY=event.y;

    SDL_Surface * surface = SDL_GetVideoSurface();
    int WW = surface->w;
    int WH = surface->h;

    switch(GetWorldNavMode())
    {
      // -------  WORLD NAVIGATION -------
      case NAV_MODE_TRACKBALL:
        if (CtrlPressed) { TrackBallX(OldX,NewX,WW); DriveY(OldY,NewY,WH); }
        else { TrackBallX(OldX,NewX,WW); TrackBallY(OldY,NewY,WH); }
        break;
      case NAV_MODE_HYPERBALL:
        if (CtrlPressed) {
          HyperBall(OldX,OldY,NewX,OldY,WW,WH); DriveY(OldY,NewY,WH); }
        else { HyperBall(OldX,OldY,NewX,NewY,WW,WH); }
        break;
      case NAV_MODE_DRIVE:
        if (CtrlPressed) { TranslateX(NewX,OldX,WW); TranslateY(NewY,OldY,WH); }
        else if (AltPressed) { LookX(OldX,NewX,WW); LookY(OldY,NewY,WH); }
        else {
          LookX(OldX,NewX,WW);
          DriveY(OldY,NewY,WH);
          //printf("OldY: \n",);
          //printf("\n",);
        }
        break;
      case NAV_MODE_TRANSLATE:
        TranslateX(OldX,NewX,WW);
        if (CtrlPressed) DriveY(OldY,NewY,WH); else TranslateY(OldY,NewY,WH);
        break;
      case NAV_MODE_LOOK:
        if (CtrlPressed) { TranslateX(OldX,NewX,WW); TranslateY(OldY,NewY,WH); }
        else if (AltPressed) { LookX(OldX,NewX,WW); DriveY(OldY,NewY,WH); }
        else { LookX(OldX,NewX,WW); LookY(OldY,NewY,WH); }
        break;
    };
  }
}




// viewer main loop
// calls display and does event processing
int MySDLVU::MyMainLoop()
{
  SDL_Surface * surface = SDL_GetVideoSurface();
  pfReshape(surface->w, surface->h);
  bool done = false;
	int fftWait=0;

  unsigned int soundPosition = 0;
  unsigned int hopToPosition = 0;
  FMOD_TIMEUNIT myTimeUnit = 1;



  this->polycount = polygons->size();
	this->poly2spec = new int[this->polycount];
	spectrumIndex=0;
	genSpec2PolyMapping(this->polycount, SPECTRUM_LENGTH/spectrumDivisor, specStartIndex);      //FIXME  here i constrain the effective spectrum we use in a shitty hardcoded way



	findPlaneMaxima();

	rvc=0;
	generateRoofVertices();

  this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
  this->genSpec2RasterMapping(raster_x, raster_y,0);

	while (!done) {
    //fmodsystem->update();
    
    if(updateSpectrum) {
      result = channel->getSpectrum(MySpectrum, SPECTRUM_LENGTH, 0, FMOD_DSP_FFT_WINDOW_TRIANGLE);
    }
    //ERRCHECK(result);

    channel->getPosition(&soundPosition,  FMOD_TIMEUNIT_MS);
//    printf("\rsoundPos:%d",soundPosition);


    pfDisplay();
    //usleep(5000);	//wtf?
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
    	SDLMod mod = SDL_GetModState();
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
				pfKeyboard(event.key);
        break;
      case SDL_KEYUP:

					switch( event.key.keysym.sym ){
						case SDLK_RIGHT:
              //hopToPosition = soundPosition + 10000;
              //result = channel->setPosition(hopToPosition, FMOD_TIMEUNIT_MS);
              //ERRCHECK(result);
              loadNextImage(1);
              maxX=FLT_MIN;
              minX=FLT_MAX;
              maxY=FLT_MIN;
              minY=FLT_MAX;
              findPlaneMaxima();

              rvc=0;
              generateRoofVertices();
              this->genSpec2RasterMapping(raster_x, raster_y, 0);
              this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
						break;
						case SDLK_LEFT:
              //hopToPosition = soundPosition - 10000;
              //result = channel->setPosition(hopToPosition, FMOD_TIMEUNIT_MS);
              //ERRCHECK(result);
              loadNextImage(-1);
              maxX=FLT_MIN;
              minX=FLT_MAX;
              maxY=FLT_MIN;
              minY=FLT_MAX;
              findPlaneMaxima();

              rvc=0;
              generateRoofVertices();
              this->genSpec2RasterMapping(raster_x, raster_y, 0);
              this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
						break;



						case SDLK_UP:
							if(mod & KMOD_SHIFT) {
								raster_poly_search_distance +=10;
							} else if(mod & KMOD_CTRL) {
								raster_x *= 2;
							} else {
								if(raster_x > 1) {
									spectrumDivisor /= spectrumDivisor > 1 ? 2 : 1;
									raster_x /=  2;
									raster_y *= 2;
								}
							}
							
							//update all spectrum / raster mappings:
							start2 = clock();
							start = clock();
							
							this->genSpec2RasterMapping(raster_x, raster_y, 0);
							
							finish = clock();
							mytime = (double(finish)-double(start))/CLOCKS_PER_SEC;
							printf("spec2Raster took: %f\n",mytime);
							
							start = clock();
							
							this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
							
							finish = clock();
							finish2 = clock();
							mytime = (double(finish)-double(start))/CLOCKS_PER_SEC;
							printf("poly2RasterFactors took: %f\n",mytime);
							
							mytime = (double(finish2)-double(start2))/CLOCKS_PER_SEC;
							printf("Overall took: %f\n",mytime);
							
							printf("polysearchdistance: %f\n",raster_poly_search_distance);
							printf("specdivisor: %d, x:%d y:%d\n",spectrumDivisor, raster_x, raster_y);
							
						break;
						case SDLK_DOWN:
								if(mod & KMOD_SHIFT) {
									raster_poly_search_distance -= raster_poly_search_distance > 10 ? 10 : 0;
								} else if(mod & KMOD_CTRL) {
									raster_x /= raster_x > 2 ? 2 : 1;
								}else {
									if(raster_y > 1) {
										spectrumDivisor *= 2;
										raster_x *=  2;
										raster_y /= 2;
									}
								}
								
								//update all spectrum / raster mappings:
								start2 = clock();
								start = clock();
								
								this->genSpec2RasterMapping(raster_x, raster_y, 0);
								
								finish = clock();
								mytime = (double(finish)-double(start))/CLOCKS_PER_SEC;
								printf("spec2Raster took: %f\n",mytime);
								
								start = clock();
								
								this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
								
								finish = clock();
								finish2 = clock();
								mytime = (double(finish)-double(start))/CLOCKS_PER_SEC;
								printf("poly2RasterFactors took: %f\n",mytime);
								
								mytime = (double(finish2)-double(start2))/CLOCKS_PER_SEC;
								printf("Overall took: %f\n",mytime);
								
								printf("polysearchdistance: %f\n",raster_poly_search_distance);
								printf("specdivisor: %d, x:%d y:%d\n",spectrumDivisor, raster_x, raster_y);
						break;

            case SDLK_u:
                muteToggle = !muteToggle;
                channel->setMute(muteToggle);
                printf("muteToggle: %d\n",muteToggle);
						break;

            case SDLK_e:
                motionToggleModifier = !motionToggleModifier;
                printf("motionToggleModifier: %d\n",motionToggleModifier);
						break;



            case SDLK_q:
                drawBuildingsToggle = !drawBuildingsToggle;
                printf("drawBuildingsToggle: %d\n",drawBuildingsToggle);
						break;
            case SDLK_a:
                drawGridToggle = !drawGridToggle;
                printf("drawGridToggle: %d\n",drawGridToggle);
						break;
            case SDLK_s:
                drawSpec2GridLinesToggle = !drawSpec2GridLinesToggle;
                printf("drawSpec2GridLinesToggle: %d\n",drawSpec2GridLinesToggle);
						break;

            case SDLK_5:
                drawRoofToggle = !drawRoofToggle;
                printf("drawRoofToggle: %d\n",drawRoofToggle);
						break;

					}
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        pfMouse(event.button);
        break;
      case SDL_MOUSEMOTION:
        if(motionToggleModifier) {
          pfMotion(event.motion);
        } else {
          if (event.motion.state & SDL_BUTTON(1)) { // left button down
            pfMotion(event.motion);
          }
        }

        break;
      }
    }
    // NB! SDL_GL_SwapBuffers() is called in endFrame()
  }

  SDL_Quit();
  return 0;
}




int audioplayback_mode = false;

int main(int argc, char *argv[])
{
  
	MySDLVU sdlvu;
  
  
  
  //omg how ugly:
  printf("---------------------------------------------------------\n");
  printf("Usage:\n");
  printf("%s soundfile\n\
  \tplayback audiofile and animate\n\
  %s\n\
  \t capture audio and animate\n\
  \n\
  \n\
  KEYS:\n\
  \t q  toggle building\n\
  \t a  toggle fft grid\n\
  \t s  toggle grid to poly centroid mapping lines (slow)\n\
  \t 5  toggle poly roofs\n\
  \tup  resize grid, spec2grid mapping, grid2polymapping UP\n\
  \tdown  resize grid, spec2grid mapping, grid2polymapping DOWN\n\
  \tright  skip audio fwd\n\
  \tleft skip audio bwd\n\
  \n\
  !!!   FOR SDLVU keys (camera movement etc) press ? when its running\n", argv[0], argv[0]);
  printf("---------------------------------------------------------\n");

  int key = 0;
  char keychar;
  int driver = 0;
  int numdrivers = 0;
  int count = 0;
  bool develmode_fast_startup = false;
  int windowX, windowY;
  Uint32 myuserflags = SDL_OPENGL;
  
  int DEVELMODE_driver_rec = 0;
  int DEVELMODE_driver_pb = 0;
  int DEVELMODE_WIN_X = 1024;
  int DEVELMODE_WIN_Y = 768;
  FMOD_OUTPUTTYPE DEVELMODE_outputType = FMOD_OUTPUTTYPE_COREAUDIO;
  if(argv[1] && argc > 1) {
    if(!strcmp(argv[1], "-devel")) {
      printf("devel mode\n");
      develmode_fast_startup = true;
    } 
  }

  
  
  
  printf("---------------------------------------------------------\n");
  printf("Select videomode \n");
  printf("---------------------------------------------------------\n");
  printf("1 :  800x600 fullscreen\n");
  printf("2 :  800x600 window\n");
  printf("3 :  1280x800 fullscreen\n");
  printf("4 :  1280x800  window\n");
  printf("5 :  1024x768 fullscreen\n");
  printf("6 :  1024x768  window\n");
  printf("---------------------------------------------------------\n");
  printf("Press a corresponding number or ESC to quit\n");

  if(develmode_fast_startup) {
    windowX = DEVELMODE_WIN_X;
    windowY = DEVELMODE_WIN_Y;
    //myuserflags |= SDL_FULLSCREEN;
  } else {
    do
    {
        //key = getch();  //fmod sample code using wincompat.h
        cin >> keychar;

    } while (keychar != 27 && keychar < '1' && keychar > '6');

    switch (keychar)
    {
        case '1' :  windowX = 800; windowY = 600;
                    myuserflags |= SDL_FULLSCREEN;
                    break;
        case '2' :  windowX = 800; windowY = 600;
                    break;
        case '3' :  windowX = 1280; windowY = 800;
                    myuserflags |= SDL_FULLSCREEN;
                    break;
        case '4' :  windowX = 1280; windowY = 800;
                    break;
        case '5' :  windowX = 1024; windowY = 768;
                    myuserflags |= SDL_FULLSCREEN;
                    break;
        case '6' :  windowX = 1024; windowY = 768;
                    break;

        default :   exit(1);
                    windowX = 640; windowY = 480;
                    //myuserflags |= SDL_FULLSCREEN;
    }
  }
  
  
  

  if(argv[1] && argc > 0 && strcmp(argv[1], "-devel") != 0) {
    audioplayback_mode = true;
  }




  //Sound
	FMOD_RESULT frc;
  frc = FMOD::System_Create(&fmodsystem);
  if (frc != FMOD_OK) {
    printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
    exit(1);
  }

  unsigned int fmodVersion;
  frc = fmodsystem->getVersion(&fmodVersion);
  if (frc != FMOD_OK) {
    printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
    exit(1);
  }

  if (fmodVersion < FMOD_VERSION) {
    printf("Error! You are using an old version of FMOD %08x. This program requires %08x\n", fmodVersion, FMOD_VERSION);
    exit(1);
  }


  //hey... maybe FMOD_OUTPUTTYPE_AUTODETECT is good for ->setOutput()  !?

  if(audioplayback_mode) {
    //if we are on linux, set output type to alsa
    #ifdef __LINUX__

    frc = fmodsystem->setOutput(FMOD_OUTPUTTYPE_ALSA);
    if (frc != FMOD_OK) {
      printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
      exit(1);
    }
    printf("using ALSA\n");

    #endif


  } else {
    //Recording:

    /*
        System initialization
    */
    printf("---------------------------------------------------------\n");
    printf("Select OUTPUT type\n");
    printf("---------------------------------------------------------\n");
    printf("1 :  Core Audio (OS X)\n");
    printf("2 :  ALSA (linux)\n");
    printf("3 :  OSS (linux)\n");
    printf("4 :  let fmod try the 'best output mode for the platform'\n");
    printf("---------------------------------------------------------\n");
    printf("Press a corresponding number or ESC to quit\n");
//
    
    if(develmode_fast_startup) {
      result = fmodsystem->setOutput(DEVELMODE_outputType);
      iscoreaudio = true;
    } else {
      do
      {
          //key = getch();  //fmod sample code using wincompat.h
          //key = getchar();    //will this work on wind00ze? FIXME
          cin >> keychar;
      } while (keychar != 27 && keychar < '1' && keychar > '5');

      switch (keychar)
      {
          case '1' :  result = fmodsystem->setOutput(FMOD_OUTPUTTYPE_COREAUDIO);
                      iscoreaudio = true;
                      //printf("coreaudio\n");
                      break;
          case '2' :  result = fmodsystem->setOutput(FMOD_OUTPUTTYPE_ALSA);
                      break;
          case '3' :  result = fmodsystem->setOutput(FMOD_OUTPUTTYPE_OSS);
                      break;
          case '4' :  result = fmodsystem->setOutput(FMOD_OUTPUTTYPE_AUTODETECT);
                      break;
          default  :  printf("my creator was puzzled... exiting\n");
                      exit(1);

      }
    }
    ERRCHECK(result);

    /*
        Enumerate playback devices
    */

    result = fmodsystem->getNumDrivers(&numdrivers);
    ERRCHECK(result);

    printf("---------------------------------------------------------\n");
    printf("Choose a PLAYBACK driver\n");
    printf("---------------------------------------------------------\n");
    for (count=0; count < numdrivers; count++)
    {
        char name[256];

        result = fmodsystem->getDriverInfo(count, name, 256, 0);
        ERRCHECK(result);

        printf("%d : %s\n", count + 1, name);
    }
    printf("---------------------------------------------------------\n");
    printf("Press a corresponding number or ESC to quit\n");

    if(!develmode_fast_startup) {
      do
      {
          //key = getchar();
          cin >> keychar;
          if (keychar == 27)
          {
              return 0;
          }
          driver = keychar - '1';
      } while (driver < 0 || driver >= numdrivers);
    }
    
    if(develmode_fast_startup) {
      result = fmodsystem->setDriver(DEVELMODE_driver_pb);
    } else {
      result = fmodsystem->setDriver(driver);
    }
    ERRCHECK(result);


    /*
        Enumerate record devices
    */

    result = fmodsystem->getRecordNumDrivers(&numdrivers);
    ERRCHECK(result);

    printf("---------------------------------------------------------\n");
    printf("Choose a RECORD driver\n");
    printf("---------------------------------------------------------\n");
    for (count=0; count < numdrivers; count++)
    {
        char name[256];

        result = fmodsystem->getRecordDriverInfo(count, name, 256, 0);
        ERRCHECK(result);

        printf("%d : %s\n", count + 1, name);
    }
    printf("---------------------------------------------------------\n");
    printf("Press a corresponding number or ESC to quit\n");

    if(!develmode_fast_startup) {
      do
      {
          //key = getchar();
          cin >> keychar;
          if (keychar == 27)
          {
              return 0;
          }
          driver = keychar - '1';
      } while (driver < 0 || driver >= numdrivers);
    }
    printf("\n");
    
    if(develmode_fast_startup) {
      result = fmodsystem->setRecordDriver(DEVELMODE_driver_rec);
    } else {
      result = fmodsystem->setRecordDriver(driver);
    }
    ERRCHECK(result);


    result = fmodsystem->setSoftwareFormat(OUTPUTRATE, FMOD_SOUND_FORMAT_PCM16, 2, 0, FMOD_DSP_RESAMPLER_LINEAR);
    ERRCHECK(result);

  }//recording setup & driver selection end


  //recording example: result = system->init(32, FMOD_INIT_NORMAL, 0);
  frc = fmodsystem->init(1, FMOD_INIT_NORMAL, 0);
  if (frc != FMOD_OK) {
    printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
    exit(1);
  }



  //playback of a file or capture live audio?
  if(audioplayback_mode) {
    ///*
      if(argv[1] && argc > 0) {
        frc = fmodsystem->createSound(argv[1], FMOD_SOFTWARE | FMOD_2D | FMOD_CREATESTREAM, 0, &sound);
        if (frc != FMOD_OK) {
          printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
          exit(1);
        }
      } else {
          printf("no data\n");
          exit(1);
      }

      frc = fmodsystem->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
      if (frc != FMOD_OK) {
        printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
        exit(1);
      }
      channel->setMute(sdlvu.muteToggle);
    //*/
  } else {
    //setup audio capturing with fmod:

    /*
          Create a sound to record to.
    */
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));

    exinfo.cbsize           = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.numchannels      = 1;
    exinfo.defaultfrequency = OUTPUTRATE;
    if (iscoreaudio)
    {
        exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
        //exinfo.length = exinfo.defaultfrequency * sizeof(float) * exinfo.numchannels * 5;
        exinfo.length = SPECTRUM_LENGTH;
    }
    else
    {
        exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
        exinfo.length = exinfo.defaultfrequency * sizeof(short) * exinfo.numchannels * 5;
    }

  //  result = fmodsystem->createSound(0, FMOD_2D | FMOD_SOFTWARE | FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &sound);
    result = fmodsystem->createSound(0, FMOD_2D | FMOD_SOFTWARE | FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &sound);
    ERRCHECK(result);

    result = fmodsystem->recordStart(sound, true);
    ERRCHECK(result);

    result = fmodsystem->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
    ERRCHECK(result);

    /* Dont hear what is being recorded otherwise it will feedback.  Spectrum analysis is done before volume scaling in the DSP chain */
    result = channel->setVolume(0);
    ERRCHECK(result);

    //RECORD END

  }
  //sound play||record end

  sdlvu.Init("SDLVU Basic Object Oriented Example",
             50, 50, windowX, windowY, myuserflags);

  //oh yeah we hide the cursor!
//  SDL_ShowCursor(SDL_DISABLE);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);

	float myLight[] = { 0.1, 0.6, 0.9, 0.9 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, myLight);
//  glEnable(GL_NORMALIZE);

  // camera setup
  Vec3f modelmin(-3, -1, 5), modelmax(1, 1, 1);
  //Vec3f lookatcntr(modelmin + modelmax); lookatcntr *= 0.5;
  //Vec3f mintoctr(lookatcntr - modelmin);
  //Vec3f up(0, 0, 1);
  /*Vec3f eye(lookatcntr - 5 * (mintoctr - up * (mintoctr * up)));
  float yfov = 45;
  float aspect = 1;
  float near = 0.1f; // near plane distance relative to model diagonal length
  float far = 100.0f; // far plane distance (also relative)*/
  Vec3f up(0.002, 0.999, 0.038);
  Vec3f lookatcntr(8.554, -8.527, 52.675);
  Vec3f eye(8.496, -8.565, 53.673);
  float yfov = 45;
  float aspect = 1;
  float near = 0.1f; // near plane distance relative to model diagonal length
  float far = 899.999390; // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr,
                   up, yfov, aspect, near, far);


  sdlvu.MyMainLoop();
	sound->release();
  fmodsystem->close();
  fmodsystem->release();
  return 0;
}
/* yeah 'd' dumps the camera data.
 --- CURRENT CAM PARAMS ---
 Eye: (8.496, -8.565, 53.673)
 LookAtCntr: (8.554, -8.527, 52.675)
 ViewUp: (0.002, 0.999, 0.038)
 Y FOV: 44.999588
 Aspect: 1.333331
 Near: 0.300000
 Far: 899.999390
 
       Far: 299.999725

*/

/////
