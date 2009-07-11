/*

  adjust visualized fft window size and move it around with up,down,q,a, skip ahead with left, right


*/

#include <sdlvu.h> // will also include GL headers
#include "map_augenkrach.h"

#include <vector>
#include <algorithm>

//#include "vorbis_fft.h"

#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include <limits.h>
#include <float.h>

#include <fmod.hpp>
#include <fmod_errors.h>

//Open Sound Control osc stuff
#include "oscReceive.h"
#define OSC_PORT 2345
extern ExamplePacketListener listener;
#include <deque>
#include <string>

// our queue for osc messages
#include "oscmsgqueue.h"
OSCMsgQueue oscMsgQ;
SDL_mutex *oscMsgQmutex;


#include "asdf_eventhandler.h"

//using namespace std;

typedef deque<osc::ReceivedMessage>::const_iterator oscMsgQ_CI;

/* midi works, but i have no use for it yet...
#include "midi.h"
#include "portmidi.h"

#include "porttime.h"
#include "pmutil.h"
*/

//sdl_console yeah
#include "SDL_console/SDL_console.h"
//extern "C" const int CONSOLE_N;
extern "C" ConsoleInformation *Consoles[3];  /* Pointers to all the consoles */

int   krach_console_startup();
void  krach_console_shutdown();
int   krach_console_processEvents(SDL_Event	event);
void  krach_console_draw();


//for timing
#include <ctime>
clock_t start,finish,start2,finish2;
double mytime;

clock_t imgSwitchTime_start, imgSwitchTime_end;
double imgSwitchTime;

clock_t camSwitchTime_start, camSwitchTime_end;
double camSwitchTime;
int act_camera = 0;

//image file directory:
char *imageDIR = "imgs_adjusted/";

//baergh:
//#include "/Developer/FMOD Programmers API/examples/common/wincompat.h"

using namespace std;
using namespace osc;
int tessMissedCounter=0;

const int SPECTRUM_LENGTH = 8192;
float MySpectrum[SPECTRUM_LENGTH];
const int SPEC_DEPTH = 100;

//size of raster spectrum w spectrum_length gets mapped to
//all vars in comparison 2 4096 spectrum_length 
// 2x32 nice for bass only visualization  (non log spectrum)
//32x64 nice for overall visualization (bass is not that much visible)
const int DEF_RASTER_X = 64;//32;
const int DEF_RASTER_Y = 64;//64;
const int DEF_RASTER_POLY_SEARCH_DIST = 450;

int SECS_PER_IMAGE = 300;
int SECS_PER_CAMERA = 1;
float CAM_SWITCH_VOLUME = 0.009;

//color vars
//FIXME   if i only knew some math.... hoehoe
//OMG!!! colors are calculated like this: spectrumVar * 20 => / 16384
// and spectrumvar is calculated by GetSpecValByBuilding with super strange formula  :(
#define init_rgb_r_multiplier 1.0f;
#define init_rgb_r_divisor 1.0f
#define init_rgb_g_multiplier 20.0f
#define init_rgb_g_divisor 16384.0f;
#define init_rgb_b_multiplier 20.0f
#define init_rgb_b_divisor 65536.0f;
float rgb_R_multiplier = init_rgb_r_multiplier;
float rgb_R_divisor = init_rgb_r_divisor;
float rgb_G_multiplier = init_rgb_g_multiplier;
float rgb_G_divisor = init_rgb_g_divisor;
float rgb_B_multiplier = init_rgb_b_multiplier;
float rgb_B_divisor = init_rgb_b_divisor;

//s*rgb_G_multiplier/rgb_G_divisor

//amount of images: (raster)
int images_rx = 6;
int images_ry = 10;

typedef struct
{
  float pos[4];
  float diffuse[4];
  float specular[4];
  float ambient[4];
}light_t;
light_t light;

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
class MySDLVU : public SDLVU, public asdfEventHandler
{
  ViennaMap map;
  PolygonList *polygons;
  BuildingList *buildings;
  public:
    MySDLVU();
    int MyMainLoop();
    int spectrumIndex;
    virtual ~MySDLVU() { }
    virtual void Display();
    void DisplayOld();

    //SpecBuffer specRingBuffer;

    virtual void Motion(const SDL_MouseMotionEvent & event);
    bool muteToggle;
    bool motionToggleModifier;
    
    bool drawBuildingsToggle;
    bool drawGridToggle;
    bool drawSpec2GridLinesToggle;
    bool drawRoofToggle;
    bool fullscreenToggle;
    
//    static void processMidiMsg(void *userdata, PmEvent *buffer);
    bool updateSpectrum;
    
    bool colorShiftToggle;
    bool changeDivisor;
    bool changeMultiplier;
    
    bool cam2beatToggle;
    
    GLfloat colorShiftR;
    GLfloat colorShiftG;
    GLfloat colorShiftB;
    bool cameraSwitchToggle;
  private:
    
    std::vector<float> vertexArray;
    std::vector<float> colorVertexArray;
    std::vector<float> roofsVertexArray;
    std::vector<float> roofsColorVertexArray;
    std::vector<int> buildingOffsetsinRoofsArray;
    std::vector<float> normalsArray;
    std::vector<float> roofsNormalsArray;
    
    GLfloat *myVertexArray;
    GLfloat *myVertexColorArray;
    GLfloat *myRoofsVertexArray;
    GLfloat *myRoofsColorVertexArray;
    GLfloat *myNormalsArray;// = new GLubyte[buildings->size()*sizeof(GLfloat)*2];
    GLfloat *myRoofsNormalsArray;
    
    int imgnum;
    
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
    float GetSpecValByBuilding(Building &building, float height);

		GLfloat maxX, maxY, minX, minY;

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
    void loadImage(int x, int y);
    
    //void dumpGeometryData(char *filename);
    
  int currentImage;
  void loadNextImage(int next);
  
  void UpdateColorShiftValues(bool changeDivisor, bool changeMultiplier);
  
  
  //osc triggerable functions:
  void SelectCamera(int camNo);
  void osc_SelectCamera(osc::ReceivedMessage& oscmsg);
  void toggleDrawBuildings();
  void toggleDrawBuildings(bool drawToggle);
  void osc_toggleDrawBuildings(osc::ReceivedMessage& oscmsg);
  void osc_DrawBuildings(osc::ReceivedMessage& oscmsg);
  void toggleDrawSpec2GridLines();
  void toggleDrawGrid();
  void osc_toggleDrawSpec2GridLines(osc::ReceivedMessage& oscmsg);
  void osc_toggleDrawGrid(osc::ReceivedMessage& oscmsg);
  void toggleDrawRoofs();
  void toggleDrawRoofs(bool drawToggle);
  void osc_toggleDrawRoofs(osc::ReceivedMessage& oscmsg);
  void osc_DrawRoofs(osc::ReceivedMessage& oscmsg);
  void osc_loadNextImage(osc::ReceivedMessage& oscmsg);
  void osc_loadImage(osc::ReceivedMessage& oscmsg);
};

/*void MySDLVU::dumpGeometryData(char *filename) {
  std::vector<float> vertexArray;
  std::vector<float> colorVertexArray;
  std::vector<float> roofsVertexArray;
  std::vector<float> roofsColorVertexArray;
  std::vector<int> buildingOffsetsinRoofsArray;
  std::vector<float> normalsArray;
  std::vector<float> roofsNormalsArray;
  
}*/

void tessVcbd(void *v, void *user_data);

/*
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
*/
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
  /*
  midiCallbackFuncType *proccb;
  proccb = MySDLVU::processMidiMsg;
  printf("calling startMidi...\n");
  if(startMidi(proccb,this) == -1) {
    printf("startMidi failed!\n");
  }*/
  
  updateSpectrum = true;

  currentImage = 0;
  //polygons = &map.getPolygonsOfFragment(currentImage,0);
  //if( map.hasFragment(currentImage,0) ) printf("fragment 0,0 loaded\n");
  //buildings = &map.getBuildingsOfFragment(currentImage,0);
  
  
  //int images_rx = 6;
  //int images_ry = 10;
  loadImage(0,0);
  
  int p = 0;
  int n = 0;
  int w = 0;
  
  int specVar=1;
  int height=50;
  
	BuildingList::const_iterator build, buildend;
	//PolygonList::const_iterator poly, polyend;
	build = buildings->begin();
	buildend = buildings->end();
	for (;build != buildend; build++) {
	    Polygon *poly = (Polygon*) (*build)->poly;
	    for (int i=(*poly).size()-1, n; i >= 0; i--) {
        n = i-1;
        if (n < 0)
          n = (*poly).size()-1;

        p = i-2;
        if (p <0)
          p = (*poly).size()-2;
        
        w = i+1;
        if (w > (*poly).size()-1)
          w = 0;
        
        vertexArray.push_back((*poly)[i].x);
        vertexArray.push_back(-(*poly)[i].y);
        vertexArray.push_back(50*i +1);
        vertexArray.push_back((*poly)[n].x);
        vertexArray.push_back(-(*poly)[n].y);
        vertexArray.push_back(50*i +1);
        vertexArray.push_back((*poly)[n].x);
        vertexArray.push_back(-(*poly)[n].y);
        vertexArray.push_back(0);
        vertexArray.push_back((*poly)[i].x);
        vertexArray.push_back(-(*poly)[i].y);
        vertexArray.push_back(0);
        
        colorVertexArray.push_back(1.);
        colorVertexArray.push_back(rgb_G_multiplier/rgb_G_divisor);
        colorVertexArray.push_back(rgb_B_multiplier/rgb_B_divisor);
        
        colorVertexArray.push_back(1.);
        colorVertexArray.push_back(rgb_G_multiplier/rgb_G_divisor);
        colorVertexArray.push_back(rgb_B_multiplier/rgb_B_divisor);
        
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        
        //normals:
        GLfloat *v1 = new GLfloat[3];
        GLfloat *v2 = new GLfloat[3];
        GLfloat *v3 = new GLfloat[3];
        
        v1[0] = (*poly)[i].x - (*poly)[n].x;
        v1[1] = (-1*(*poly)[i].y) - (-1*(*poly)[n].y);
        v1[2] = 0;
        
        v2[0] = (*poly)[n].x - (*poly)[p].x;
        v2[1] = (-1*(*poly)[n].y) - (-1*(*poly)[p].y);
        v2[2] = 0;
        
        //normalvektor:
        //b,-a ("rechts" -daten sind counterclockwise (?) )
        v1[0] *= -1;
        
        //normalize:
        GLfloat length = sqrt(
          pow(v1[0],2)+
          pow(v1[1],2)+
          pow(v1[2],2)
        );
        v1[0] /= length;
        v1[1] /= length;
        v1[2] /= length;
//        v2[0] = (*poly)[i].x - (*poly)[i].x;
//        v2[1] = (-1*(*poly)[i].y) - (-1*(*poly)[i].y);
//        v2[2] = 0;
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        delete[] v1;
        delete[] v2;
        delete[] v3;
      }
      //p++;
	}
	myVertexArray = new GLfloat[vertexArray.size()];
  copy(vertexArray.begin(), vertexArray.end(), myVertexArray);
  
  myVertexColorArray = new GLfloat[colorVertexArray.size()];
  copy(colorVertexArray.begin(), colorVertexArray.end(), myVertexColorArray);
  
  myNormalsArray = new GLfloat[normalsArray.size()];
  copy(normalsArray.begin(), normalsArray.end(), myNormalsArray);
  
  cout << "" << endl;
  cout << "vertArray size: " << vertexArray.size() << endl;
  cout << "" << endl;
  

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
  fullscreenToggle = false;
  drawGridToggle = false;
  drawSpec2GridLinesToggle = false;
  
  colorShiftToggle = false;
  changeDivisor = false;
  changeMultiplier = true;
  colorShiftR = 0;
  colorShiftG = 0;
  colorShiftB = 0;
  cam2beatToggle = false;
  cameraSwitchToggle = false;
  
  
}


void MySDLVU::loadNextImage(int next) {
  
  //free/delete any prev resources - vert arrays etc!
  vertexArray.clear();
  vertexArray.resize(0);
  colorVertexArray.clear();
  colorVertexArray.resize(0);
  normalsArray.clear();
  normalsArray.resize(0);
  
  roofsVertexArray.clear();
  roofsVertexArray.resize(0);
  roofsColorVertexArray.clear();
  roofsColorVertexArray.resize(0);
  roofsNormalsArray.clear();
  roofsNormalsArray.resize(0);
  buildingOffsetsinRoofsArray.clear();
  buildingOffsetsinRoofsArray.resize(0);
  
  delete[] myVertexArray;
  delete[] myVertexColorArray;
  delete[] myRoofsVertexArray;
  delete[] myRoofsColorVertexArray;
  delete[] myNormalsArray;
  delete[] myRoofsNormalsArray;
  
  
  //pngFile = this->PNGfileList.begin();
  //pngFileEnd = this->PNGfileList.end();
  //int c=0;
  //for (;pngFile != pngFileEnd; pngFile++) {
  
  ImageFileList *filelist = &map.PNGfileList;
  
  
  if(currentImage < filelist->size()-1 && next > 0)
    currentImage++;
  else if(currentImage > 0 && next < 0)
    currentImage--;
  else
    currentImage = 0;
  
  cout << "FILELIST SIZE, CIMG: " << filelist->size() << " " << currentImage << endl;
  
  
  polygons = &map.getPolygonsOfFragment(currentImage,0);
  buildings = &map.getBuildingsOfFragment(currentImage,0);
  
  int p = 0;
  int n = 0;
  int w = 0;
  
  int specVar=1;
  int height=50;

	BuildingList::const_iterator build, buildend;
	//PolygonList::const_iterator poly, polyend;
	build = buildings->begin();
	buildend = buildings->end();
	for (;build != buildend; build++) {
	    Polygon *poly = (Polygon*) (*build)->poly;
	    for (int i=(*poly).size()-1, n; i >= 0; i--) {
        n = i-1;
        if (n < 0)
          n = (*poly).size()-1;

        p = i-2;
        if (p <0)
          p = (*poly).size()-2;
        
        w = i+1;
        if (w > (*poly).size()-1)
          w = 0;
        
        vertexArray.push_back((*poly)[i].x);
        vertexArray.push_back(-(*poly)[i].y);
        vertexArray.push_back(50*i +1);
        vertexArray.push_back((*poly)[n].x);
        vertexArray.push_back(-(*poly)[n].y);
        vertexArray.push_back(50*i +1);
        vertexArray.push_back((*poly)[n].x);
        vertexArray.push_back(-(*poly)[n].y);
        vertexArray.push_back(0);
        vertexArray.push_back((*poly)[i].x);
        vertexArray.push_back(-(*poly)[i].y);
        vertexArray.push_back(0);
        
        colorVertexArray.push_back(1.0);
        colorVertexArray.push_back(rgb_G_multiplier/rgb_G_divisor);
        colorVertexArray.push_back(rgb_B_multiplier/rgb_B_divisor);
        
        colorVertexArray.push_back(1.0);
        colorVertexArray.push_back(rgb_G_multiplier/rgb_G_divisor);
        colorVertexArray.push_back(rgb_B_multiplier/rgb_B_divisor);
        
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        colorVertexArray.push_back(0.);
        
        //normals:
        GLfloat *v1 = new GLfloat[3];
        GLfloat *v2 = new GLfloat[3];
        GLfloat *v3 = new GLfloat[3];
        
        v1[0] = (*poly)[i].x - (*poly)[n].x;
        v1[1] = (-1*(*poly)[i].y) - (-1*(*poly)[n].y);
        v1[2] = 0;
        
        v2[0] = (*poly)[n].x - (*poly)[p].x;
        v2[1] = (-1*(*poly)[n].y) - (-1*(*poly)[p].y);
        v2[2] = 0;
        
        //normalvektor:
        //b,-a ("rechts" -daten sind counterclockwise (?) )
        v1[0] *= -1;
        
        //normalize:
        GLfloat length = sqrt(
          pow(v1[0],2)+
          pow(v1[1],2)+
          pow(v1[2],2)
        );
        v1[0] /= length;
        v1[1] /= length;
        v1[2] /= length;
//        v2[0] = (*poly)[i].x - (*poly)[i].x;
//        v2[1] = (-1*(*poly)[i].y) - (-1*(*poly)[i].y);
//        v2[2] = 0;
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        normalsArray.push_back(v1[1]);
        normalsArray.push_back(v1[0]);
        normalsArray.push_back(0);
        delete[] v1;
        delete[] v2;
        delete[] v3;
      }
      //p++;
	}
	myVertexArray = new GLfloat[vertexArray.size()];
  copy(vertexArray.begin(), vertexArray.end(), myVertexArray);
  
  myVertexColorArray = new GLfloat[colorVertexArray.size()];
  copy(colorVertexArray.begin(), colorVertexArray.end(), myVertexColorArray);
  
  myNormalsArray = new GLfloat[normalsArray.size()];
  copy(normalsArray.begin(), normalsArray.end(), myNormalsArray);
  
  cout << "" << endl;
  cout << "vertArray size: " << vertexArray.size() << endl;
  cout << "" << endl;
  

  printf("have %d buildings\n", buildings->size());
  
  
  this->polycount = polygons->size();
  delete[] this->poly2spec;
	this->poly2spec = new int[this->polycount];
	spectrumIndex=0;
	//genSpec2PolyMapping(this->polycount, SPECTRUM_LENGTH/spectrumDivisor, specStartIndex);      //FIXME  here i constrain the effective spectrum we use in a shitty hardcoded way


  maxX=FLT_MIN;
  minX=FLT_MAX;
  maxY=FLT_MIN;
  minY=FLT_MAX;
  
	findPlaneMaxima();

	rvc=0;
	generateRoofVertices();

  this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
  this->genSpec2RasterMapping(raster_x, raster_y,0);
  
}

void MySDLVU::loadImage(int x, int y) {
  BuildingList *tmpBuildings;
  const PolygonList *tmpPolygons;
 
//  polygons = &map.getPolygonsOfFragment(x,y);
  if(x==0 && y==0) {
    polygons = &map.getPolygonsOfFragment(x,y);
    buildings = &map.getBuildingsOfFragment(x,y);
  } else {
    tmpPolygons = &map.getPolygonsOfFragment(x,y);
    for(int i=0; i<(*tmpPolygons).size(); i++) {
      (*polygons).push_back((*tmpPolygons)[i]);
    }
    
    tmpBuildings = &map.getBuildingsOfFragment(x,y);
    for(int i=0; i<(*tmpBuildings).size(); i++) {
      (*buildings).push_back((*tmpBuildings)[i]);
    }
    
  }
  //polygons = &map.getPolygonsOfFragment(x,y);
  //buildings = &map.getBuildingsOfFragment(x,y);  
  printf("have %d polys\n", polygons->size());
  
  printf("have %d buildings\n", buildings->size());
}


float MySDLVU::GetSpecValByBuilding(Building &building, float height) {
  float specVar = 0;

  //Building *build = *building;
  Polygon *poly = (Polygon*) building.poly;
  if ((*poly).size() < 3) return 0;

  Raster2VertexList *rasterPointList = building.rasterPoints2affect;
  VertexIndexList *verticesOrder = building.orderedVertices;

  //glBegin(GL_TRIANGLES);
  //glColor3f(0.,  1,  0);
  //glColor3f(1.,  (specVar*2000)/16384,  (specVar*2000)/65384);

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
  return specVar;

  //DrawPoly(*poly, 80, specVar*100);
  //if(drawRoofToggle)
  //  DrawRoofTesselated(building, height, specVar*100);  //*1/specVar     --> clifford je hoeher frequenz desto kleiner amplitude. energie = f*a??? FIXME  (also above! -> specVar =...)
  ///*
/*
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
    
    //glVertex3f((*poly)[n].x, -(*poly)[n].y, (specVar*100));
    //glVertex3f((*poly)[i].x, -(*poly)[i].y, (specVar*100));
    glVertex3f((*poly)[n].x, -(*poly)[n].y, 0.);
    glVertex3f((*poly)[i].x, -(*poly)[i].y, 0.);

  }

  glEnd();*/
  
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
  
  //if(this->spec2raster != NULL )
  delete[] this->spec2raster;
  
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
  int maxX = this->maxX;//this->map.fragmentImageWidth;
  int maxY = this->maxY;//this->map.fragmentImageHeight;

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
        float s = MySpectrum[this->spec2raster[x*rasterY+y]];
        if(colorShiftToggle) {
          glColor3f(0.86*s*rgb_R_multiplier/(rgb_R_divisor/1000),  0.86*s*rgb_G_multiplier/(rgb_G_divisor/1000),  0.86*s*rgb_B_multiplier/(rgb_B_divisor/1000));
        } else {
          glColor3f(0.0,  0.86*MySpectrum[this->spec2raster[x*rasterY+y]]*10,  0.8*MySpectrum[this->spec2raster[x*rasterY+y]]*10);
        }
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
  //cout << "fft grid(Rmd Gmd Bmd): " << rgb_R_multiplier << " "<< rgb_R_divisor << " " << rgb_G_multiplier << " " << rgb_G_divisor << " " << rgb_B_multiplier << " " << rgb_B_divisor << " " << endl;
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


/* search in a bounding box, near the centroid of the building, instead of the whole grid:
  lightyears faster!
*/
void MySDLVU::genPoly2RasterFactors(int rasterX, int rasterY, float maxDistance) {

    int maxX = this->maxX;//this->map.fragmentImageWidth;
    int maxY = this->maxY;//this->map.fragmentImageHeight;
    int rasterZ = 0;

    int spacingX = maxX / rasterX;
    int spacingY = maxY / rasterY;

    //for buildings
    ///*
    int minRP = 0;
    int maxRP = 0;
    BuildingList::const_iterator build, buildend;
    PolygonList::const_iterator poly, polyend;
    build = buildings->begin();
    buildend = buildings->end();
    float dist = 0;
    int pointCount = 0;
    for (;build != buildend; build++) {
      Building *building = *build;
//      if( (*building).rasterPoints2affect) {
        (*building).rasterPoints2affect->clear();
        (*building).rasterPoints2affect->resize(0);
//        cout << "CLEARING building rasterlist" << endl;
//      }
//      (*building).rasterPoints2affect = new Raster2VertexList();
      Raster2VertexList* rasterList = (*building).rasterPoints2affect;
      
      int startX =  (int) floor( (-1*building->fCenterX - (maxDistance)) / spacingX);
      int endX =  (int) ceil( (-1*building->fCenterX + (maxDistance)) / spacingX);
      if(endX > rasterX || endX < 0)
        endX = rasterX;
      if(startX < 0 || startX > rasterX)
        startX = 0;

      int startY =  (int) floor( (-1*building->fCenterY - (maxDistance)) / spacingY);
      int endY =  (int) ceil( (-1*building->fCenterY + (maxDistance)) / spacingY);
      if(endY > rasterY || endY < 0)
        endY = rasterY;
      if(startY < 0 || startY > rasterY)
        startY = 0;
      
      for(int x = startX; x < endX; x++) {
        for(int y = startY; y < endY; y++) {
          
          //FIXME here centroid(x) is inverted, same problem with centroid drawing and calculation!
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
            pointCount++;
          }
        }
      }
    }
    printf("\nminRP: %d\t",minRP);
    printf("maxRP: %d\t\n",maxRP);
    printf("found %d points\n",pointCount);
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
	myRoofsVertexArray = new GLfloat[roofsVertexArray.size()];
  copy(roofsVertexArray.begin(), roofsVertexArray.end(), myRoofsVertexArray);

  myRoofsColorVertexArray = new GLfloat[roofsVertexArray.size()];
  copy(roofsColorVertexArray.begin(), roofsColorVertexArray.end(), myRoofsColorVertexArray);

  myRoofsNormalsArray = new GLfloat[roofsNormalsArray.size()];
  copy(roofsNormalsArray.begin(), roofsNormalsArray.end(), myRoofsNormalsArray);
  
  cout << endl;
  cout << "RoofsVertexArraySize: " << roofsVertexArray.size() << endl;
  cout << "tessMissedCounter: " << tessMissedCounter << endl;
  cout << "buildingOffsetsinRoofsArray Size: " << buildingOffsetsinRoofsArray.size() << endl;
  //cout << "last buildingOffsetsinRoofsArray: " << buildingOffsetsinRoofsArray.at(buildingOffsetsinRoofsArray.size()-1) << endl;
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
  
//  printf("vertexIndex: %d\n",vertexIndex);
}



void MySDLVU::TessellateBuilding(Building &building) {
//  static int roofVertexCounter=0;
  Polygon *poly = (Polygon*) building.poly;
  
	if ((*poly).size() < 3) {
    tessMissedCounter++;
		return;
	}
  building.orderedVertices->clear();
  building.orderedVertices->resize(0);
  
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
	
	//Polygon *poly = (Polygon*) building.poly;
  VertexIndexList *verticesOrder = building.orderedVertices;
  //if ((*poly).size() < 3) return;

  //glBegin(GL_TRIANGLES);
  //glColor3f(0.,  1,  0);
  //glColor3f(1.,  (specVar/100*20)/16384,  (specVar/100*20)/65384);
  
  for (int i=(*verticesOrder).size()-1, n; i >= 0; i--) {
  //for (int i=0; i < (*verticesOrder).size(); i++) {
    //glVertex3f((*poly)[(*verticesOrder)[i]].x, -(*poly)[(*verticesOrder)[i]].y, height+(specVar));
    
    roofsVertexArray.push_back((*poly)[(*verticesOrder)[i]].x);
    roofsVertexArray.push_back(-(*poly)[(*verticesOrder)[i]].y);
    roofsVertexArray.push_back(50);

    //roofVertexCounter++;
    
    roofsColorVertexArray.push_back(1.0);
    roofsColorVertexArray.push_back(rgb_G_multiplier/rgb_G_divisor);
    roofsColorVertexArray.push_back(rgb_B_multiplier/rgb_B_divisor);
    
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(1);
  }
  //buildingOffsetsinRoofsArray.push_back(roofVertexCounter);
//  cout << "buildingoffsetpushback: " << roofVertexCounter << endl;
//  glEnd();
}

void MySDLVU::UpdateColorShiftValues(bool changeDivisor, bool changeMultiplier) {
  //colorShiftToggle
  static int rshifter = 0;
  static int rshifter_step = 1;
  static int gshifter = 50;
  static int gshifter_step = 1;
  static int bshifter = 100;
  static int bshifter_step = 1;
  rshifter += rshifter_step;
  gshifter += gshifter_step;
  bshifter += bshifter_step;
  if (rshifter < 1) rshifter_step *= -1;
  if (gshifter < 1) gshifter_step *= -1;
  if (bshifter < 1) bshifter_step *= -1;
  if (rshifter > 100) rshifter_step *= -1;
  if (gshifter > 100) gshifter_step *= -1;
  if (bshifter > 100) bshifter_step *= -1;
  //float lightshift[4] = {rshifter/100.0, gshifter/100.0, bshifter/100.0, 1};
  
  if(changeMultiplier) {
    rgb_R_multiplier = rshifter;
    rgb_G_multiplier = gshifter;
    rgb_B_multiplier = bshifter;
  }
  if(changeDivisor) {
    rgb_R_divisor = rshifter*gshifter*10;
    rgb_G_divisor = gshifter*bshifter*10;
    rgb_B_divisor = bshifter*rshifter*10;
  }
  //cout << "updating colorshiftvalues " << endl;
  //cout << "rgb_R_multiplier: " << rgb_R_multiplier << endl;
  //cout << "rgb_G_multiplier: " << rgb_G_multiplier << endl;
  //cout << "rgb_B_multiplier: " << rgb_B_multiplier << endl;
  /*
  float rgb_R_multiplier = init_rgb_r_multiplier;
  float rgb_R_divisor = init_rgb_r_divisor;
  float rgb_G_multiplier = init_rgb_g_multiplier;
  float rgb_G_divisor = init_rgb_g_divisor;
  float rgb_B_multiplier = init_rgb_b_multiplier;
  float rgb_B_divisor = init_rgb_b_divisor;
  */
}
// override the display method to do your custom drawing
void MySDLVU::Display()
{ 
  

  int p=0;
  int r=0;
  int buildC = 0;
	BuildingList::const_iterator build, buildend;
	//PolygonList::const_iterator poly, polyend;
	build = buildings->begin();
	buildend = buildings->end();
  float s = 0;
  int n=0;
	///*
	for (;build != buildend; build++) {
      //DrawCentroid(**build, 100); //slow debug func
      //DrawMeanCenter(**build, 100);
      s = GetSpecValByBuilding(**build, 50);
      //s=0;
      if(drawBuildingsToggle) {
        Polygon *blah = (*build)->poly;
        for (int i=(*blah).size()-1; i >= 0; i--) {
          n=i-1;
          i=i<0?(*blah).size()-1:i;
          //myVertexArray[p] = ((*blah)[i].x*0.1) + ( s*100);
          //myVertexArray[p] = (vertexArray[p]) + ( s);
          //myVertexArray[p+3] = (vertexArray[p+3]) + ( s);
          //myVertexArray[p+1] = (vertexArray[p+1]) + s;
          //myVertexArray[p+1+3] = (vertexArray[p+1+3]) + s;
          myVertexArray[p+2] = 50 + s*100;
          myVertexArray[p+2+3] = 50 + s*100;
          //myVertexArray[p+2+6] = s*100;
          //myVertexArray[p+2+9] = s*100;
          
          myVertexColorArray[p] = s*rgb_R_multiplier/rgb_R_divisor;
          myVertexColorArray[p+2] = s*rgb_G_multiplier/rgb_G_divisor;
          myVertexColorArray[p+2+3] = s*rgb_B_multiplier/rgb_B_divisor;
          //myVertexColorArray[p+2+6] = s*rgb_G_multiplier/rgb_G_divisor;
          //myVertexColorArray[p+2+9] = s*rgb_B_multiplier/rgb_B_divisor;
          //myVertexArray[p+2] = 50 + s*100;
          //myVertexArray[p+2+3] = 50 + s*100;

          myVertexColorArray[p] = s*rgb_R_multiplier/rgb_R_divisor;
          myVertexColorArray[p+1] = s*rgb_G_multiplier/rgb_G_divisor;
          myVertexColorArray[p+2] = s*rgb_B_multiplier/rgb_B_divisor;
          
          myVertexColorArray[p+0+3] = s*rgb_R_multiplier/rgb_R_divisor;
          myVertexColorArray[p+1+3] = s*rgb_G_multiplier/rgb_G_divisor;
          myVertexColorArray[p+2+3] = s*rgb_B_multiplier/rgb_B_divisor;
          p+=12;
          
          myVertexArray[p+2] = 50 + s*100;
          myVertexArray[p+2+3] = 50 + s*100;
          //myVertexArray[p+2] = 50 + s*100;
          //myVertexArray[p+2+3] = 50 + s*100;

          ///*
          
          
          myVertexColorArray[p+0] = s*rgb_R_multiplier/rgb_R_divisor;
          myVertexColorArray[p+1] = s*rgb_G_multiplier/rgb_G_divisor;
          myVertexColorArray[p+2] = s*rgb_B_multiplier/rgb_B_divisor;
          myVertexColorArray[p+0+3] = s*rgb_R_multiplier/rgb_R_divisor;
          myVertexColorArray[p+1+3] = s*rgb_G_multiplier/rgb_G_divisor;
          myVertexColorArray[p+2+3] = s*rgb_B_multiplier/rgb_B_divisor;
          //*/
        }
      }
      
      
      ///*
      if(drawRoofToggle) {
        VertexIndexList *bleh = (*build)->orderedVertices;  
        for (int i=0;i<(*bleh).size();i++) {
          //UARGH FIXME
          //r is running beyond array size!
          //if(r < roofsVertexArray.size()) {
          //if(r < 150000) {
            myRoofsVertexArray[r+2] = 50 + s*100;
            myRoofsColorVertexArray[r] = 1.0;
            
            //BIG FIXME
            myRoofsColorVertexArray[r+0] = s*rgb_R_multiplier/rgb_R_divisor; //FIXME wasnt changed at runtime originally :(
            
            myRoofsColorVertexArray[r+1] = s*rgb_G_multiplier/rgb_G_divisor;
            myRoofsColorVertexArray[r+2] = s*rgb_B_multiplier/rgb_B_divisor;
            r += 3;
          //} 
        }
        //*/
        /*
        if(buildC < buildingOffsetsinRoofsArray.size()-1 ) {
          for(int i=buildingOffsetsinRoofsArray.at(buildC); i < buildingOffsetsinRoofsArray.at(buildC+1); i++) {
            myRoofsVertexArray[i*3+2] = 50 + s*100;
          }
        }*/
      }
      
      buildC++;
	}
//	const int size = (sizeof(myRoofsVertexArray)/sizeof(myRoofsVertexArray[0]));
//  cout << "roofsVertexArray size:" << roofsVertexArray.size() << endl;
//  cout << "myRoofsVertexArray size:" << size << endl;
//	cout << "rvcount: " << roofVcount << endl;
	//*/
	
	
//  for(int i=0;i<roofsVertexArray.size()/3;i++) {
//    myRoofsVertexArray[i*3+2] = s*500;
//	}

  BeginFrame();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glScalef(0.01, 0.01, 0.01);
  //glScalef(0.1,0.1,0.1);
  //glTranslatef(-500,500,0);
//  glColor3f(0.95,0.95,0.95);
  //glLightfv(GL_LIGHT0,GL_POSITION,light.pos);			//updates the light's position
	//glLightfv(GL_LIGHT0,GL_DIFFUSE,light.diffuse);		//updates the light's diffuse colour
	//glLightfv(GL_LIGHT0,GL_SPECULAR,light.specular);	//updates the light's specular colour
	//glLightfv(GL_LIGHT0,GL_AMBIENT,light.ambient);		//updates the light's ambient colour
  ///*
  
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    if(drawRoofToggle) {
      glVertexPointer(3, GL_FLOAT, 0, myRoofsVertexArray);
      glColorPointer(3, GL_FLOAT, 0, myRoofsColorVertexArray);
      glNormalPointer(GL_FLOAT, 0, myRoofsNormalsArray);
      glDrawArrays(GL_TRIANGLES, 0, roofsVertexArray.size()/3);
    }
    
    if(drawBuildingsToggle) {
      glVertexPointer(3, GL_FLOAT, 0, myVertexArray);
      glColorPointer(3, GL_FLOAT, 0, myVertexColorArray);
      glNormalPointer(GL_FLOAT, 0, myNormalsArray);
      glDrawArrays(GL_QUADS, 0, vertexArray.size()/3);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    //FIXME old immediatemode code
    build = buildings->begin();
  	buildend = buildings->end();
    if (drawGridToggle)
      DrawFFTGrid(raster_x, raster_y,0);

    if(drawSpec2GridLinesToggle) {
      //cout << "spec2gridlines" << endl;
  	  for (;build != buildend; build++) {
          //DrawCentroid(**build, 100); //slow debug func
          //DrawMeanCenter(**build, 100);

        
            DrawCentroid2GridLines(**build, 0, 0, raster_x, raster_y);
          //if(drawBuildingsToggle)
          //  DrawBuildingByRasterSpectrum(**build, 50);
          //p++;
    	}
  	}
  //*/
  glPopMatrix();
//*/
	//DrawFloor();
  
  krach_console_draw();
  
  EndFrame();

  //delete[] myVertexArray;
  //delete[] myVertexColorArray;
}

// override the display method to do your custom drawing
void MySDLVU::DisplayOld()
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
//        DrawBuildingByRasterSpectrum(**build, 50);
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

void MySDLVU::osc_loadNextImage(osc::ReceivedMessage& oscmsg) {
  int oscimgNo = 0;
  try {
    osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
    if(oscmsg.ArgumentCount() > 0) {
      if(arg->IsInt32()) {
        oscimgNo = arg->AsInt32();
      }
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  //checks arg so we dont have to here
  loadNextImage(oscimgNo);
}

void MySDLVU::osc_loadImage(osc::ReceivedMessage& oscmsg) {
  //2do
  //no function implemented yet that could do this nicely
  //loadNextImage() has to be used, which uses loadImage()
}

void MySDLVU::toggleDrawSpec2GridLines() {
  drawSpec2GridLinesToggle = !drawSpec2GridLinesToggle;
  printf("drawSpec2GridLinesToggle: %d\n",drawSpec2GridLinesToggle);
}

void MySDLVU::toggleDrawGrid() {
  drawGridToggle = !drawGridToggle;
  printf("drawGridToggle: %d\n",drawGridToggle);
}

void MySDLVU::osc_toggleDrawSpec2GridLines(osc::ReceivedMessage& oscmsg) {
  toggleDrawSpec2GridLines();
}

void MySDLVU::osc_toggleDrawGrid(osc::ReceivedMessage& oscmsg) {
  toggleDrawGrid();
}


void MySDLVU::toggleDrawRoofs() {
  drawRoofToggle = !drawRoofToggle;
  printf("drawRoofToggle: %d\n",drawRoofToggle);
}

void MySDLVU::toggleDrawRoofs(bool drawToggle) {
  drawRoofToggle = drawToggle;
  printf("drawRoofToggle: %d\n",drawRoofToggle);
}

void MySDLVU::osc_toggleDrawRoofs(osc::ReceivedMessage& oscmsg) {
  toggleDrawRoofs();
}

void MySDLVU::osc_DrawRoofs(osc::ReceivedMessage& oscmsg) {
  bool drawToggle;
  int osctoggle;
  try {
    osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
    if(oscmsg.ArgumentCount() > 0) {
      if(arg->IsInt32()) {
        osctoggle = arg->AsInt32();
      }
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  drawToggle = osctoggle ? true : false;
  toggleDrawRoofs(drawToggle);
}

void MySDLVU::toggleDrawBuildings() {
  drawBuildingsToggle = !drawBuildingsToggle;
  printf("drawBuildingsToggle: %d\n",drawBuildingsToggle);
}

void MySDLVU::toggleDrawBuildings(bool drawToggle) {
  drawBuildingsToggle = drawToggle;
  printf("drawBuildingsToggle: %d\n",drawBuildingsToggle);
}

void MySDLVU::osc_toggleDrawBuildings(osc::ReceivedMessage& oscmsg) {
  toggleDrawBuildings();
}

void MySDLVU::osc_DrawBuildings(osc::ReceivedMessage& oscmsg) {
  bool drawToggle;
  int osctoggle;
  try {
    osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
    if(oscmsg.ArgumentCount() > 0) {
      if(arg->IsInt32()) {
        osctoggle = arg->AsInt32();
      }
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  drawToggle = osctoggle ? true : false;
  toggleDrawBuildings(drawToggle);
}


void MySDLVU::SelectCamera(int camNo) {
  //GetSDLVU()->NumCams-1
  if(camNo > GetSDLVU()->NumCams-1) {
    camNo = GetSDLVU()->NumCams-1;
  } else if (camNo < 0) {
    camNo = 0;
  }
  act_camera = camNo;
  GetSDLVU()->SelectCam(act_camera);
}

void MySDLVU::osc_SelectCamera(osc::ReceivedMessage& oscmsg) {
  int osccamno = 0;
  try {
    osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
    if(oscmsg.ArgumentCount() > 0) {
      if(arg->IsInt32()) {
        osccamno = arg->AsInt32();
      }
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  SelectCamera(osccamno);
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
  
  imgSwitchTime_start = clock();
  
  int myticks = SDL_GetTicks();
  int tempColorShiftTicks = 0;
  int colorShiftWaitTicks = 30;
  int camTicks = SDL_GetTicks();
  int camHopWaitTicks = 60;
  int tempCamHopTicks = 0;
  
  
  registerEvent_memberfunc("/asdf/camera/select",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_SelectCamera);
  registerEvent_memberfunc("/asdf/grid/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleDrawGrid);
  registerEvent_memberfunc("/asdf/grid/spec2gridlines/draw",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleDrawSpec2GridLines);
  registerEvent_memberfunc("/asdf/buildings/roofs/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleDrawRoofs);
  registerEvent_memberfunc("/asdf/buildings/roofs/draw",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_DrawRoofs);
  registerEvent_memberfunc("/asdf/buildings/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleDrawBuildings);
  registerEvent_memberfunc("/asdf/buildings/draw",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_DrawBuildings);
  registerEvent_memberfunc("/asdf/image/prevnext",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_loadNextImage);
	while (!done) {
    //fmodsystem->update();
    
    //process OSC messages:
    //process OSC messages:
    SDL_mutexP(oscMsgQmutex);
    if(oscMsgQ.msgqueue.size() > 0) {
      //oscMsgQ_CI msgIter = oscMsgQ.msgqueue.begin();
      deque<osc::ReceivedMessage*>::iterator msgIter = oscMsgQ.msgqueue.begin();
      for(; msgIter != oscMsgQ.msgqueue.end(); msgIter++) {
        cout << "executing func for AddressPattern: " << (*msgIter)->AddressPattern() << endl;
        //try execute a registered func for msg
        execute(*msgIter);
        (*msgIter)->freeCopiedMessageBuffer();
        delete *msgIter;
      }
      //clear all msgs
      oscMsgQ.msgqueue.clear();
    }
    SDL_mutexV(oscMsgQmutex);
    // OSC msg processing
    
    
    if(updateSpectrum) {
      result = channel->getSpectrum(MySpectrum, SPECTRUM_LENGTH, 0, FMOD_DSP_FFT_WINDOW_TRIANGLE);
      ERRCHECK(result);
    }
    //ERRCHECK(result);
//    printf("FPS: %f\n",GetFPS());
    channel->getPosition(&soundPosition,  FMOD_TIMEUNIT_MS);
//    printf("\rsoundPos:%d",soundPosition);
        
    //colorshifting:
    if(colorShiftToggle) {
      //UpdateColorShiftValues
      tempColorShiftTicks = SDL_GetTicks();
      if(tempColorShiftTicks > myticks + colorShiftWaitTicks) {
        UpdateColorShiftValues(changeDivisor, changeMultiplier);
        myticks = SDL_GetTicks();
      }
      //int myticks = SDL_GetTicks();
    }
    
    //broken yet...
    if(cam2beatToggle) {
        tempCamHopTicks = SDL_GetTicks();
        if(tempCamHopTicks > camTicks + camHopWaitTicks) {
          GetSDLVU()->StartPlayback("cam_record0.dat");
          GetSDLVU()->StopRecordingPlayback();
          camTicks = SDL_GetTicks();
        }
      
        int WW = surface->w;
        int WH = surface->h;
      //MySpectrum[150]
        Camera *Cam;
        Cam = GetSDLVU()->GetCurrentCam();
        float RelY = (Cam->Orig.y-1)/(float)WH;
        Vec3f EyeToWorldCntr = WorldCenter - Cam->Orig;
        float DistToCntr = EyeToWorldCntr.Length();
        if (DistToCntr<WorldRadius) DistToCntr=WorldRadius;
        
        //cout << ""
        float M[16];
        Vec3f Trans = Cam->Z * (DistToCntr*RelY*2.0f) * (MySpectrum[150]*300);
        Translate16fv(M,Trans.x,Trans.y,Trans.z);
        Cam->Xform(M);
      /*
			void SDLVU::DriveY(int OldY, int NewY, int WH)
      {
        float RelY = (NewY-OldY)/(float)WH;

        Vec3f EyeToWorldCntr = WorldCenter - Cam->Orig;
        float DistToCntr = EyeToWorldCntr.Length();
        if (DistToCntr<WorldRadius) DistToCntr=WorldRadius;

        float M[16];
        Vec3f Trans = Cam->Z * (DistToCntr*RelY*2.0f) * moveSpeed;
        Translate16fv(M,Trans.x,Trans.y,Trans.z);
        Cam->Xform(M);
      }*/
    }
    //imageswitching
    imgSwitchTime_end = clock();
    imgSwitchTime = (double(imgSwitchTime_end)-double(imgSwitchTime_start))/CLOCKS_PER_SEC;
    if(imgSwitchTime > SECS_PER_IMAGE ) {
      loadNextImage(1);
      imgSwitchTime_start = clock();
    }
    //cameraSwitching
    camSwitchTime_end = clock();
    camSwitchTime = (double(camSwitchTime_end)-double(camSwitchTime_start))/CLOCKS_PER_SEC;
    if(camSwitchTime > SECS_PER_CAMERA && cameraSwitchToggle && MySpectrum[150] > CAM_SWITCH_VOLUME) {
      //loadNextImage(1);
      cout << "cam: " << act_camera << endl;
      act_camera += act_camera == (GetSDLVU()->NumCams-1) ? (GetSDLVU()->NumCams-1)*-1 : 1;
      GetSDLVU()->SelectCam(act_camera);
      camSwitchTime_start = clock();
    }
    
    pfDisplay();
    //usleep(5000);	//wtf?
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      
      if(krach_console_processEvents(event) == 1)
        continue;
      
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
        //break;
        //case SDL_KEYUP:
          GLint State;
          glGetIntegerv(GL_CULL_FACE_MODE,&State);
//          if (State==GL_BACK) glCullFace(GL_FRONT); else glCullFace(GL_BACK);
          printf("--> FPS: %f <-- cull: %d mode:%d\n",GetFPS(), glIsEnabled(GL_CULL_FACE), State);
          
					switch( event.key.keysym.sym ){
						case SDLK_RIGHT:
              //hopToPosition = soundPosition + 10000;
              //result = channel->setPosition(hopToPosition, FMOD_TIMEUNIT_MS);
              //ERRCHECK(result);
              loadNextImage(1);
						break;
						case SDLK_LEFT:
              //hopToPosition = soundPosition - 10000;
              //result = channel->setPosition(hopToPosition, FMOD_TIMEUNIT_MS);
              //ERRCHECK(result);
              loadNextImage(-1);
						break;



						case SDLK_UP:
						  if(mod & KMOD_SHIFT && mod & KMOD_CTRL) {
                CAM_SWITCH_VOLUME += 0.01;
                cout << "CAM_SWITCH_VOLUME: " << CAM_SWITCH_VOLUME << endl;
						  } else if(mod & KMOD_SHIFT) {
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
							printf("specdivisor: %d, x:%d y:%d\n-----------------\n",spectrumDivisor, raster_x, raster_y);
							
						break;
						case SDLK_DOWN:
						    if(mod & KMOD_SHIFT && mod & KMOD_CTRL) {
                  CAM_SWITCH_VOLUME -= 0.01;
                  cout << "CAM_SWITCH_VOLUME: " << CAM_SWITCH_VOLUME << endl;
  						  } else if(mod & KMOD_SHIFT) {
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
                updateSpectrum = !updateSpectrum;
                CON_Out(Consoles[0], "muteToggle: %d\n",muteToggle);
						break;
            
            case SDLK_y:
                cameraSwitchToggle = !cameraSwitchToggle;
                printf("cameraSwitchToggle: %d\n",cameraSwitchToggle);
						break;
            
            case SDLK_e:
                motionToggleModifier = !motionToggleModifier;
                printf("motionToggleModifier: %d\n",motionToggleModifier);
						break;

            case SDLK_q:
                toggleDrawBuildings();
						break;
            case SDLK_a:
                if(mod & KMOD_SHIFT) {
                  toggleDrawSpec2GridLines();
                } else {
                  toggleDrawGrid();
                }
						break;


            case SDLK_s:
                toggleDrawRoofs();
						break;
						
						case SDLK_f:
						    //shift f to reset colors and turn off colorshifttoggling
						    if(mod & KMOD_SHIFT) {
  						    rgb_R_multiplier = init_rgb_r_multiplier;
                  rgb_R_divisor = init_rgb_r_divisor;
                  rgb_G_multiplier = init_rgb_g_multiplier;
                  rgb_G_divisor = init_rgb_g_divisor;
                  rgb_B_multiplier = init_rgb_b_multiplier;
                  rgb_B_divisor = init_rgb_b_divisor;
                  colorShiftToggle = false;  
                } else {
                  colorShiftToggle = !colorShiftToggle;
                  printf("colorShiftToggle: %d\n",colorShiftToggle);
                }
						break;
						
						case SDLK_g:
						  if(mod & KMOD_SHIFT) {
						    changeDivisor = !changeDivisor;
						  } else {
						    changeMultiplier = !changeMultiplier;
						  }
						  printf("changeMultiplier: %d\n",changeMultiplier);
						  printf("changeDivisor: %d\n",changeDivisor);
            break;
						
						case SDLK_j:
              cam2beatToggle = !cam2beatToggle;
              printf("cam2beatToggle: %d\n",cam2beatToggle);
            break;
						
            
						/* // os x not supported currently?
						case SDLK_f:
						    SDL_Surface * theSurface = SDL_GetVideoSurface();
                int ftret = 0;
						    if(fullscreenToggle) {
						      ftret = SDL_WM_ToggleFullScreen(theSurface);
						    } else {
						      ftret = SDL_WM_ToggleFullScreen(theSurface);
						    }
                fullscreenToggle = !fullscreenToggle;
                printf("fullscreenToggle?: %d\n",ftret);
						break;
						*/

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

  //FIXME this is ugly!
  //midi finalize:
//  finalize();
  
  SDL_Quit();
  return 0;
}



int audioplayback_mode = false;

int main(int argc, char *argv[])
{
  //omg how ugly:
  printf("---------------------------------------------------------\n");
  printf("Usage:\n");
  printf("%s soundfile\n\
  \tplayback audiofile and animate\n\
  %s\n\
  \t capture audio and animate\n\
  see notes.txt for help\n", argv[0], argv[0]);
  printf("---------------------------------------------------------\n");

  int key = 0;
  char keychar;
  int driver = 0;
  int numdrivers = 0;
  int count = 0;
  bool develmode_fast_startup = false;
  int windowX, windowY;
  Uint32 myuserflags = SDL_OPENGL;
  //sdl_console:
  myuserflags |= SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_OPENGLBLIT;
  //no frame
  //myuserflags |= SDL_NOFRAME;
  
  int DEVELMODE_driver_rec = 0;
  int DEVELMODE_driver_pb = 0;
  int DEVELMODE_WIN_X = 800;
  int DEVELMODE_WIN_Y = 600;
  FMOD_OUTPUTTYPE DEVELMODE_outputType = FMOD_OUTPUTTYPE_COREAUDIO;
  if(argv[1] && argc > 1) {
    if(!strcmp(argv[1], "-devel")) {
      printf("devel mode\n");
      develmode_fast_startup = true;
    } 
  }
  
  //otherwise globally def with 6,10
  if (argv[2] && argv[3] && argc > 3) {
    images_rx = (int)strtoul(argv[2],0,0);
  	images_ry = (int)strtoul(argv[3],0,0);
    cout << "images_rx/ry: " << images_rx << "/" << images_ry << endl;
      cout << endl;
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
    //myuserflags |= SDL_RESIZABLE;
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
      //channel->setMute(sdlvu.muteToggle);
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

  MySDLVU sdlvu;
  
  if(audioplayback_mode) {
    channel->setMute(sdlvu.muteToggle);
  }
  sdlvu.Init("asdf",
             0, 0, windowX, windowY, myuserflags);
  sdlvu.StartFPSClock();
  
  //turn off inertia!
  sdlvu.SetInertiaEnabled(0);
  sdlvu.SetInertiaOn(0);
  //oh yeah we hide the cursor!
//  SDL_ShowCursor(SDL_DISABLE);
  
  light_t light={
        {1,1,0,1},  //position (the final 1 means the light is positional)
        {1,1,1,1},    //diffuse
        {1,1,1,1},    //specular
        {0,0,0,1}     //ambient
  };
  light_t mymaterial={
    {0.,0.,0.,1},  //shininess
    {0,0,0,1},    //diffuse
    {1,1,1,1},    //specular
    {0.3,0.,0.,1}     //ambient
  };
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);  
  //glColorMaterial(GL_FRONT,GL_DIFFUSE);
  
  GLfloat black[4]={0,0,0,0};
  
  //glMaterialfv(GL_FRONT,GL_SHININESS, mymaterial.pos); //shininess //yeah variablenames fuckup
  //glMaterialfv(GL_FRONT,GL_DIFFUSE, mymaterial.diffuse);
  //glMaterialfv(GL_FRONT,GL_SPECULAR,  mymaterial.specular);
  //glMaterialfv(GL_FRONT,GL_AMBIENT, mymaterial.ambient);
  
  GLfloat myEmission[4] = {0,0,0,1};
  glMaterialfv(GL_FRONT,GL_EMISSION, myEmission);
  
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glShadeModel(GL_SMOOTH);
  //glShadeModel(GL_FLAT);
  glEnable(GL_CULL_FACE);
  
	//float myLight[] = { 0.1, 0.6, 0.9, 0.9 };
	//glLightfv(GL_LIGHT0, GL_AMBIENT, myLight);
	glLightfv(GL_LIGHT0,GL_POSITION,light.pos);			//updates the light's position
	glLightfv(GL_LIGHT0,GL_DIFFUSE,light.diffuse);		//updates the light's diffuse colour
	glLightfv(GL_LIGHT0,GL_SPECULAR,light.specular);	//updates the light's specular colour
	glLightfv(GL_LIGHT0,GL_AMBIENT,light.ambient);		//updates the light's ambient colour
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
  Vec3f lookatcntr(14.505, -17.658, 47.381);
  Vec3f eye(14.496, -17.697, 48.380);
  float yfov = 45;
  float aspect = 1;
  float near = 1.0f; // near plane distance relative to model diagonal length
  float far = 100.999390; // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr,
                   up, yfov, aspect, near, far);

  
  if(krach_console_startup() != 0) {
    exit(1);
  }
  
  ///*
  int oscport = OSC_PORT;
	std::cout << "listening for osc input \n";
  listen_for_osc_packets(oscport);
  //*/
  
  sdlvu.MyMainLoop();
  krach_console_shutdown();
	sound->release();
  fmodsystem->close();
  fmodsystem->release();
  return 0;
}
/* yeah 'd' dumps the camera data.
--- CURRENT CAM PARAMS ---
       Eye: (14.496, -17.697, 48.380)
LookAtCntr: (14.505, -17.658, 47.381)
    ViewUp: (0.002, 0.999, 0.038)
     Y FOV: 44.999516
    Aspect: 1.333334
      Near: 0.003000
       Far: 2402.996338
       

 --- CURRENT CAM PARAMS ---
 Eye: (8.496, -8.565, 53.673)
 LookAtCntr: (8.554, -8.527, 52.675)
 ViewUp: (0.002, 0.999, 0.038)
 Y FOV: 44.999588
 Aspect: 1.333331
 Near: 0.300000
 Far: 899.999390
 
       Far: 299.999725
       --- CURRENT CAM PARAMS ---
              Eye: (-20.591, -6.779, 79.796)
       LookAtCntr: (-20.212, -7.076, 78.919)
           ViewUp: (0.465, -0.758, 0.458)
            Y FOV: 44.999119
           Aspect: 1.333346
             Near: 0.003000
              Far: 2402.979248
*/
/////
