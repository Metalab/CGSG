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
float linearSpectrum[SPECTRUM_LENGTH];
float *MySpectrum = linearSpectrum;
const int SPEC_DEPTH = 100;

const int DEF_LOG_SPECTRUM_BANDS = 8192;

int LOG_SPECTRUM_BANDS = DEF_LOG_SPECTRUM_BANDS;//512 in max' code
int *conversionBoundaries;// need it variable in size  int conversionBoundaries[ LOG_SPECTRUM_BANDS *2 ];
float *MyLogSpectrum;// need it variable in size  float MyLogSpectrum[SPECTRUM_LENGTH];

//size of raster spectrum w spectrum_length gets mapped to
//all vars in comparison 2 4096 spectrum_length 
// 2x32 nice for bass only visualization  (non log spectrum)
//32x64 nice for overall visualization (bass is not that much visible)
const int DEF_RASTER_X = 64;//32;
const int DEF_RASTER_Y = 128;//64; //128 for 8192 spec size
const int DEF_RASTER_POLY_SEARCH_DIST = 450;

bool AUTO_SWITCH_IMAGES = false;
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

#define DEF_LT_BRUSH_OFFSET_X 0;
#define DEF_LT_BRUSH_OFFSET_Y 6;
#define DEF_LT_BRUSH_SIZE 12;

bool TESS_COMBINE_needed_error_occurred = false;
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
    GLfloat *fftgridVertexArray;
    GLfloat *fftgridColorArray;
    
    int imgnum;
    
    int raster_x;
    int raster_y;
    float raster_poly_search_distance;
    int fftgridsize;
    
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
    float GetUnmangledSpecValByBuilding(Building &building);
		GLfloat maxX, maxY, minX, minY;

		void generateRoofVertices();
    void generateRoofVerticesForBuilding(Building *building);
		void TessellateBuilding(Building &building);
		void tessVcb(void *v);
		//void tessVcbd(GLdouble *v[3], void *user_data);
		int rvc;

    void genSpec2PolyMapping(int polyCount, size_t spectrumsize, int specStartIndex);
    void genPoly2RasterFactors(int rasterX, int rasterY, float maxDistance);
    void genSpec2RasterMapping(int rasterX, int rasertY, int specStartIndex);
    void GenerateFFTGridVertexArray(int rasterX, int rasertY);
    void DrawGrid(int rasterX, int rasterY, int rasterZ);
    void UpdateFFTGridVertexArray(int rasterX, int rasterY, int rasterZ, bool manglespectrum);
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
  
  void osc_setRasterPolySearchDistance(osc::ReceivedMessage& oscmsg);
  void osc_setRasterSize(osc::ReceivedMessage& oscmsg);
  
  void osc_setColorsTop(osc::ReceivedMessage& oscmsg);
  void osc_setColorsBottom(osc::ReceivedMessage& oscmsg);
  
  void update_spectrum_raster_mappings();
  
  /*
  * laser tag stuff
  */
  bool ltbattlemodetoggle;
  void osc_toggle_ltbattlemode(osc::ReceivedMessage& oscmsg);
  void set_ltbattlemode(bool toggle);
  void osc_ltbattle_recvCoords(osc::ReceivedMessage& oscmsg);
  void osc_ltbattle_cmd(osc::ReceivedMessage& oscmsg);
  void setupLTbattlemode();
  
  int lt_animate_until_varray_index;
  int lt_animate_until_roofsvarray_index;
  std::vector<int> strokebeginindices;
  
  float osc_minimum_specvar;
  void osc_set_osc_minimum_specvar(osc::ReceivedMessage& oscmsg);
  float osc_actstroke_specvar;
  void osc_set_osc_actstroke_specvar(osc::ReceivedMessage& oscmsg);
  
  std::vector<Building> lt_playback_shape;
  
  std::vector<int> lt_int_shape; // stores only 4 points that come in via osc.
  
  //std::vector<int> lt_shape; // stores the full shape with timestamps
  typedef struct {
    //fixme 
    //i push maxint into the array for a move command
    std::vector<int> *lt_shape_P;
    std::vector<Uint32> *lt_shape_timestamps_P;
  } t_lt_shape;
  std::vector<t_lt_shape> lt_shapes; // collection of all captured shapes/tags
  
  std::vector<int> act_lt_shape;
  std::vector<Uint32> act_lt_shape_timestamps;
  
  //flag if lt_battle sent "move" command new stroke begins
  bool lt_newstroke_starting;
  //lt_battle shape point count
  int ltshape_pointcount;
  
  
  void process_lt_coords();
  
  int lt_brush_offset_x;
  int lt_brush_offset_y;
  int lt_brush_size;
  
  GLfloat *last_normalv;
  GLfloat *last_lt_v;
  
  void osc_ltbattle_setbrushsize(osc::ReceivedMessage& oscmsg);
  void create_building_from_lasercoord();
  bool geometry_resources_freed;
  
  void addBuilding2VertexArray(Building *building);
  void calcCentroidForBuilding(Building*);
  
  void genPoly2RasterFactorForBuilding(int rasterX, int rasterY, float maxDistance, Building *thebuilding);
  
  void fake_init_vertexarrays();
  void fake_osc_input();
  
  //new audio stuff
  bool normalizeaudiospectrum;
  bool convertSpectrum2Decibel;
  bool convertSpectrum2logScale;
  bool adjustSpectrum_freqVSamp;
  bool normalizeaudiospectrumWithFixedValue;
  float fixednormalizeMaxValue;
  float logSpectrumBandwidth;
  int logSpectrumLowCutOff;
  void calculateConversionBoundaries();
  void osc_setLogSpectrumSize(osc::ReceivedMessage& oscmsg);
  void osc_toggleLogSpectrum(osc::ReceivedMessage& oscmsg);
  void osc_setLogSpectrumLowCutoff(osc::ReceivedMessage& oscmsg);
  void osc_toggleNormalizeSpectrum(osc::ReceivedMessage& oscmsg);
  void osc_toggleAdjustSpectrum_freqVSamp(osc::ReceivedMessage& oscmsg);
  void osc_toggleNormalizeWithFixedValue(osc::ReceivedMessage& oscmsg);
  void osc_setFixedNormalizeMaxValue(osc::ReceivedMessage& oscmsg);
  
  //specHeight & color new approach ... dont fail me
  float fftgrid_color_multiply_R;
  float fftgrid_color_multiply_G;
  float fftgrid_color_multiply_B;
  float fftgrid_vert_multiply;
  
  float building_color_multiply_R;
  float building_color_multiply_G;
  float building_color_multiply_B;
  float building_vert_multiply;
  float building_min_height;
  
  void osc_set_fftgrid_color_rgb(osc::ReceivedMessage& oscmsg);
  void osc_set_fftgrid_vert_multiply(osc::ReceivedMessage& oscmsg);
  void osc_set_building_color_rgb(osc::ReceivedMessage& oscmsg);
  void osc_set_building_vert_multiply(osc::ReceivedMessage& oscmsg);
  void osc_set_building_min_height(osc::ReceivedMessage& oscmsg);
  
  bool lasertagbattlemode;
  bool lt_even_polygons;
  void osc_set_evenpolygons(osc::ReceivedMessage& oscmsg);
};

/*
  void MySDLVU::dumpGeometryData(char *filename) {
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
  
  ///*
	//lol is this wrong on purpose?
	maxX=FLT_MIN; 
	minX=FLT_MAX;
	maxY=FLT_MIN;
	minY=FLT_MAX;
	//*/
  /*
  maxX=0; 
  minX=0;
  maxY=0;
  minY=0;
*/
  spectrumDivisor = 2;
  specStartIndex = 0;
  //setupSpectrumHistory();

  raster_x = DEF_RASTER_X;
  raster_y = DEF_RASTER_Y;
  fftgridsize = raster_x * raster_y;
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
  
  ltbattlemodetoggle = false;
  bool lt_newstroke_starting = true;
  int ltshape_pointcount = 0;
  lt_brush_offset_x = DEF_LT_BRUSH_OFFSET_X;
  lt_brush_offset_y = DEF_LT_BRUSH_OFFSET_Y;
  lt_brush_size = DEF_LT_BRUSH_SIZE;
  geometry_resources_freed = false;
  lasertagbattlemode = true;
  lt_even_polygons = true;
  
  this->spec2raster = new int[raster_x * raster_y];
  
  //int images_rx = 6;
  //int images_ry = 10;
  buildings = new BuildingList();
  polygons = new PolygonList();
  buildings->clear();
  
  //alloc them so we can delete[] them on demand
  myVertexArray = new GLfloat[1];
  myVertexColorArray = new GLfloat[1];
  myRoofsVertexArray = new GLfloat[1];
  myRoofsColorVertexArray = new GLfloat[1];
  myNormalsArray = new GLfloat[1];
  myRoofsNormalsArray = new GLfloat[1];
  //loadImage(0,0);
  
  last_normalv = new GLfloat[2];
  last_lt_v = new GLfloat[2];
  
  //create some buildings by faking recvd lasertag coords
  fake_init_vertexarrays();
  this->genSpec2RasterMapping(raster_x, raster_y,0);
  fftgridVertexArray = new GLfloat[1];
  fftgridColorArray = new GLfloat[1];
  this->GenerateFFTGridVertexArray(raster_x, raster_y);
  normalizeaudiospectrum = false;
  adjustSpectrum_freqVSamp = false;
  convertSpectrum2Decibel = false;
  convertSpectrum2logScale = false;
  normalizeaudiospectrumWithFixedValue = false;
  fixednormalizeMaxValue = 1.0;
  logSpectrumBandwidth = 0;
  logSpectrumLowCutOff = 0;
  
  fftgrid_color_multiply_R = 1.0;
  fftgrid_color_multiply_G = 1.0;
  fftgrid_color_multiply_B = 1.0;
  fftgrid_vert_multiply = 1.0;
  
  building_color_multiply_R = 1.0;
  building_color_multiply_G = 1.0;
  building_color_multiply_B = 1.0;
  building_vert_multiply = 1.0;
  building_min_height = 50.0;
  
  lt_animate_until_varray_index = 0;
  lt_animate_until_roofsvarray_index = 0;
  osc_minimum_specvar = 0.0;
  osc_actstroke_specvar = 0.0;
  //act_lt_shape = new std::vector<int>;
  //act_lt_shape_timestamps = new std::vector<Uint32>;
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
  
  
  this->polycount = buildings->size();
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
  this->GenerateFFTGridVertexArray(raster_x, raster_y);
  
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
  //printf("have %d polys\n", buildings->size());
  
  printf("have %d buildings\n", buildings->size());
}

float MySDLVU::GetUnmangledSpecValByBuilding(Building &building) {
  float specVar = 0;
  Raster2VertexList *rasterPointList = building.rasterPoints2affect;
  VertexIndexList *verticesOrder = building.orderedVertices;
  int maxRP = 0;
  float maxRPdist = 0;
  for (int i=(*rasterPointList).size()-1, n; i >= 0; i--) {
    int rp = (*rasterPointList)[i].rasterpoint;
    float dist = (*rasterPointList)[i].distance;
    //float tmpspecv = MySpectrum[this->spec2raster[rp]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[rp]+1)) ) * dist / 200;
//fixme crashed here
    float tmpspecv = MySpectrum[this->spec2raster[rp]];
    if(tmpspecv > specVar) {
      maxRP = rp;
      maxRPdist = dist;
      specVar = tmpspecv;
    }
  }
  return specVar;
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
  int maxRP = 0;
  float maxRPdist = 0;
  float specDistHeightFactor = 0;
  for (int i=(*rasterPointList).size()-1, n; i >= 0; i--) {
    int rp = (*rasterPointList)[i].rasterpoint;
    float dist = (*rasterPointList)[i].distance;
    //specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * (dist);         //rvf.rasterpoint = (x*rasterX)+(y);


    //use average spectrum var of all rasterpoints for this building:
    //specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[rp]+1)) ) * dist / 200;         //rvf.rasterpoint = (x*rasterX)+(y);

    //use maximum spectrum var of all rasterpoints for this building
    /*
    float tmpspecv = MySpectrum[this->spec2raster[rp]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[rp]+1)) ) * dist / 200;
    specVar = tmpspecv > specVar ? tmpspecv : specVar;
    */
    //float tmpspecv = MySpectrum[this->spec2raster[rp]];
    float tmpspecv = MySpectrum[this->spec2raster[rp]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[rp]+1)) ) * dist / 200;
    if(tmpspecv > specVar) {
      maxRP = rp;
      maxRPdist = dist;
      specVar = tmpspecv;
    }
  }
  //use average spectrum var of all rasterpoints for this building:
  //specVar = (specDistHeightFactor/(*rasterPointList).size());
  //return specVar * ((16000/SPECTRUM_LENGTH * (this->spec2raster[maxRP]+1)) ) * maxRPdist / 200;
  return specVar;
  
}


void MySDLVU::genSpec2RasterMapping(int rasterX, int rasterY, int specStartIndex) {
  cout << "genSpec2RasterMapping BEGIN" << endl;
  cout << "raster: " << rasterX << " / " << rasterY << endl;
  if(rasterX < 1) {
    rasterX = 1;
  }
  if(rasterY < 1) {
    rasterY = 1;
  }
  int rasterSize = rasterX*rasterY;
  float rasterIterator = (float)rasterSize / (float)SPECTRUM_LENGTH;
  
  //printf("rasterIterator: %f\n",rasterIterator);
  
//  if(this->spec2raster != NULL )
    delete[] this->spec2raster;
  this->spec2raster = new int[rasterSize];
  
  if(specStartIndex > SPECTRUM_LENGTH || specStartIndex < 0) {
    specStartIndex = 0;
  }
  int specIndex = specStartIndex;



  //init
  for(int i=0;i < rasterSize; i++) {
      this->spec2raster[i] = specIndex; //=0
  }


  //map spectrum to the whole raster:

  //rvf.rasterpoint = x * rasterY + y;
  for(int i=0;i < rasterX; i++) {
    for(int q=0;q < rasterY; q++) {
      int p = i * rasterY + q;
      this->spec2raster[p] = specIndex;
      specIndex++;
    }
  }
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
	BuildingList::const_iterator build, buildend;
	build = buildings->begin();
  buildend = buildings->end();
	for (;build != buildend; build++) {
    Polygon *poly = (*build)->poly;
		findPolyMaxima(*poly);
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


void MySDLVU::UpdateFFTGridVertexArray(int rasterX, int rasterY, int rasterZ=0, bool manglespectrum=true) {
  
  float vertSpec = 0;
  float colorSpec = 0;
  float RcolorSpec = 0;
  float GcolorSpec = 0;
  float BcolorSpec = 0;
  int p = 0;
  for(int x = 0; x < rasterX; x++) {
      for(int y = 0; y < rasterY; y++) {
        
        vertSpec = MySpectrum[this->spec2raster[x*rasterY+y]];
        colorSpec = vertSpec;
        
        
        RcolorSpec = colorSpec * fftgrid_color_multiply_R;
        GcolorSpec = colorSpec * fftgrid_color_multiply_G;
        BcolorSpec = colorSpec * fftgrid_color_multiply_B;
        vertSpec *= fftgrid_vert_multiply;
        
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        fftgridVertexArray[p+2] = vertSpec;
        fftgridColorArray[p] = RcolorSpec;
        fftgridColorArray[p+1] = GcolorSpec;
        fftgridColorArray[p+2] = BcolorSpec;
        p += 3;
        
      }
  }
}


void MySDLVU::GenerateFFTGridVertexArray(int rasterX, int rasterY) {
  this->fftgridsize = rasterX * rasterY;
  int maxX = this->maxX;
  int maxY = this->maxY;
  float spacingX = (float)maxX / (float)rasterX;
  float spacingY = (float)maxY / (float)rasterY;
  float specHeight = 0;
  float s = 0;
  float spacingXoverall=0;
  float spacingYoverall=0;
  
  delete[] fftgridVertexArray;
  delete[] fftgridColorArray;
  fftgridVertexArray = new GLfloat[rasterX*rasterY*8*3];
  fftgridColorArray = new GLfloat[rasterX*rasterY*8*3];
  //memset(fftgridColorArray,0,sizeof(float)*rasterX*rasterY*8*3);
  int p=0;
  for(int x = 0; x < rasterX; x++) {
      for(int y = 0; y < rasterY; y++) {
        fftgridVertexArray[p] = spacingXoverall;
        fftgridVertexArray[p+1] = -spacingYoverall;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        fftgridVertexArray[p] = spacingXoverall;
        fftgridVertexArray[p+1] = -spacingYoverall - spacingY;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        fftgridVertexArray[p] = spacingXoverall;
        fftgridVertexArray[p+1] = -spacingYoverall - spacingY;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        fftgridVertexArray[p] = spacingXoverall + spacingX;
        fftgridVertexArray[p+1] = -spacingYoverall - spacingY;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        fftgridVertexArray[p] = spacingXoverall + spacingX;
        fftgridVertexArray[p+1] = -spacingYoverall - spacingY;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        fftgridVertexArray[p] = spacingXoverall + spacingX;
        fftgridVertexArray[p+1] = -spacingYoverall;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        fftgridVertexArray[p] = spacingXoverall + spacingX;
        fftgridVertexArray[p+1] = -spacingYoverall;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        fftgridVertexArray[p] = spacingXoverall;
        fftgridVertexArray[p+1] = -spacingYoverall;
        fftgridVertexArray[p+2] = 0.0;
        fftgridColorArray[p] = 0.0;
        p += 3;
        spacingYoverall += spacingY;
      }
      spacingYoverall=0;
      spacingXoverall += spacingX;
  }
}


void MySDLVU::DrawGrid(int rasterX, int rasterY, int rasterZ=0) {
  float maxX = this->map.fragmentImageWidth;
  float maxY = this->map.fragmentImageHeight;

  float spacingX = maxX / rasterX;
  float spacingY = maxY / rasterY;

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


void MySDLVU::genPoly2RasterFactorForBuilding(int rasterX, int rasterY, float maxDistance, Building *thebuilding) {
    if(rasterX < 1) {
      rasterX = 1;
    }
    if(rasterY < 1) {
      rasterY = 1;
    }
    float maxX = this->maxX;//this->map.fragmentImageWidth;
    float maxY = this->maxY;//this->map.fragmentImageHeight;
    int rasterZ = 0;

    float spacingX = maxX / rasterX;
    float spacingY = maxY / rasterY;

    //for buildings
    ///*
    int minRP = 0;
    int maxRP = 0;

    float dist = 0;
    int pointCount = 0;
    //for (;build != buildend; build++) {
      Building *building = thebuilding;
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
      
    //}
}

/* search in a bounding box, near the centroid of the building, instead of the whole grid:
  lightyears faster!
*/
void MySDLVU::genPoly2RasterFactors(int rasterX, int rasterY, float maxDistance) {
    if(rasterX < 1) {
      rasterX = 1;
    }
    if(rasterY < 1) {
      rasterY = 1;
    }
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
    //printf("\nminRP: %d\t",minRP);
    //printf("maxRP: %d\t\n",maxRP);
    //printf("found %d points\n",pointCount);
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

//
void MySDLVU::generateRoofVerticesForBuilding(Building *building) {
  TessellateBuilding(*building);
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
  
  //cout << endl;
  //cout << "RoofsVertexArraySize: " << roofsVertexArray.size() << endl;
  //cout << "tessMissedCounter: " << tessMissedCounter << endl;
  //cout << "buildingOffsetsinRoofsArray Size: " << buildingOffsetsinRoofsArray.size() << endl;
  
  //cout << "last buildingOffsetsinRoofsArray: " << buildingOffsetsinRoofsArray.at(buildingOffsetsinRoofsArray.size()-1) << endl;
}


typedef struct {
  const Polygon *poly;
  Building *building;
  int vIndex;
  int polySize;

} tessUserData;

//    non class function:
void mytessError(GLenum err)
{
  if(err == GLU_TESS_NEED_COMBINE_CALLBACK) {
    TESS_COMBINE_needed_error_occurred = true;
    cout << "tess error" << endl;
    cout << gluErrorString(err) << endl;
  }
  
}

void mytessErrorData(GLenum err, void *error_data)
{
  gluErrorString(err);
}

void mytessBegin(GLenum type, void *user_data) {
	//printf("begin with type: %d\n",type);
}

void mytessEdgeFlagCB(GLboolean flag ) {
	//printf("flagCB: %d\n",flag);
}

void mytessEnd(GLenum type) {
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
	gluTessCallback(tess, GLU_TESS_END,		(GLvoid(*)())&mytessEnd);
	gluTessCallback(tess, GLU_TESS_ERROR,	(GLvoid (*)())&mytessError);

	rvc = 0; // for tessVcb
  tessUserData polyData;
  polyData.poly = poly;
  polyData.building = &building;
  polyData.vIndex = 0;
  polyData.polySize = poly->size();
	//gluTessBeginPolygon (tess, (GLvoid*)&poly); //&poly = USER_DATA
  //cout << "gluTessBeginPolygon" << endl;
  gluTessBeginPolygon (tess, (GLvoid*)&polyData); //&poly = USER_DATA
  //cout << "gluTessBeginContour" << endl;
	gluTessBeginContour (tess);

	for (int i=(*poly).size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = (*poly).size()-1;
		gluTessVertex (tess, (GLdouble*)&tmppd[4*i], &tmppd[4*i]);
	}
	//cout << "gluTessEndContour" << endl;
	gluTessEndContour (tess);
	//cout << "gluEndPolygon" << endl;
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
  int q=0;
  int buildC = 0;
	BuildingList::iterator build, buildend;
	//PolygonList::const_iterator poly, polyend;
	build = buildings->begin();
	buildend = buildings->end();
  float s = 0;
  float colorSpec = 0;
  float vertSpec = 0;
  float RcolorSpec = 0;
  float GcolorSpec = 0;
  float BcolorSpec = 0;
  float act_RcolorSpec = 0;
  float act_GcolorSpec = 0;
  float act_BcolorSpec = 0;
  float act_vertSpec = 0;
  float last_RcolorSpec = 0;
  float last_GcolorSpec = 0;
  float last_BcolorSpec = 0;
  float last_vertSpec = 0;
  
  int n=0;
	///*
  float lastSpecVar = 0;
  float act_specvar;
  float act_specvar2;
  bool stroketoright=true;
  Building *prevBuild;
  if(buildings->size() > 0) {
    
    lastSpecVar = GetUnmangledSpecValByBuilding(**build);
    
    prevBuild = *build;
  }
  //cout << "s: ";
  
	for (;build != buildend; build++) {
      
      //check for not animating the stroke currently being drawn
      if(lasertagbattlemode && p >= lt_animate_until_varray_index){
        s = osc_actstroke_specvar;
        lastSpecVar = osc_actstroke_specvar;
        //s = osc_minimum_specvar;
        //lastSpecVar = osc_minimum_specvar;
      } else {
        s = GetUnmangledSpecValByBuilding(**build);
      }
      
      //minimum specvar for everything... so stuff is visible even without audio. adjustable via osc
      if(lasertagbattlemode){
        if(s < osc_minimum_specvar) {
          s = osc_minimum_specvar;
        }
      }
      //lt_animate_until_roofsvarray_index = roofsVertexArray.size()-1;
      
      //dont do median of building's specVar in different strokes (beginning/end of strokes):
      if(lasertagbattlemode && strokebeginindices.size() > 0) {
        for(int k=0;k<strokebeginindices.size()-1;k++) {
          if(p == strokebeginindices.at(k)+1) {
            lastSpecVar = s;
          }
        }
      }
      
      RcolorSpec = s * building_color_multiply_R;
      GcolorSpec = s * building_color_multiply_G;
      BcolorSpec = s * building_color_multiply_B;
      vertSpec = s * building_vert_multiply;
    
      act_RcolorSpec = s * building_color_multiply_R;
      act_GcolorSpec = s * building_color_multiply_G;
      act_BcolorSpec = s * building_color_multiply_B;
//fixme crashed here
      act_vertSpec = s * building_vert_multiply;
      last_RcolorSpec = lastSpecVar * building_color_multiply_R;
      last_GcolorSpec = lastSpecVar * building_color_multiply_G;
      last_BcolorSpec = lastSpecVar * building_color_multiply_B;
      last_vertSpec = lastSpecVar * building_vert_multiply;
      
        
      if(drawBuildingsToggle) {
        
        if(lasertagbattlemode) {

            Polygon *blah = (*build)->poly;
            for (int i=(*blah).size()-1; i >= 0; i--) {
              if(i == 2 ) {
                //first/last quad
                //myVertexArray[p+2] = building_min_height*30;
                //myVertexArray[p+2+3] = building_min_height*30;
                myVertexArray[p+2] = building_min_height + act_vertSpec;
                myVertexArray[p+2+3] = building_min_height + act_vertSpec;
                myVertexColorArray[p] = act_RcolorSpec;
                myVertexColorArray[p+1] = act_GcolorSpec;
                myVertexColorArray[p+2] = act_BcolorSpec;
                myVertexColorArray[p+0+3] = act_RcolorSpec;
                myVertexColorArray[p+1+3] = act_GcolorSpec;
                myVertexColorArray[p+2+3] = act_BcolorSpec;
                
              } else if(i == 3) {
                //sidequad
                myVertexArray[p+2] = building_min_height + last_vertSpec;
                myVertexArray[p+2+3] = building_min_height + act_vertSpec;
                //myVertexArray[p+2] = building_min_height*20;
                //myVertexArray[p+2+3] = building_min_height*20;
                myVertexColorArray[p] = last_RcolorSpec;
                myVertexColorArray[p+1] = last_GcolorSpec;
                myVertexColorArray[p+2] = last_BcolorSpec;
                myVertexColorArray[p+0+3] = act_RcolorSpec;
                myVertexColorArray[p+1+3] = act_GcolorSpec;
                myVertexColorArray[p+2+3] = act_BcolorSpec;
              } else if( i == 0) {
                //first/last quad
                myVertexArray[p+2] = building_min_height + last_vertSpec;
                myVertexArray[p+2+3] = building_min_height + last_vertSpec;
                //myVertexArray[p+2] = building_min_height*40;
                //myVertexArray[p+2+3] = building_min_height*40;
                myVertexColorArray[p] = last_RcolorSpec;
                myVertexColorArray[p+1] = last_GcolorSpec;
                myVertexColorArray[p+2] = last_BcolorSpec;
                myVertexColorArray[p+0+3] = last_RcolorSpec;
                myVertexColorArray[p+1+3] = last_GcolorSpec;
                myVertexColorArray[p+2+3] = last_BcolorSpec;
              } else if (i == 1){
                //sidequad
                myVertexArray[p+2] = building_min_height + act_vertSpec;
                myVertexArray[p+2+3] = building_min_height + last_vertSpec;
                //myVertexArray[p+2] = building_min_height*40;
                //myVertexArray[p+2+3] = building_min_height*40;
                myVertexColorArray[p] = act_RcolorSpec;
                myVertexColorArray[p+1] = act_GcolorSpec;
                myVertexColorArray[p+2] = act_BcolorSpec;
                myVertexColorArray[p+0+3] = last_RcolorSpec;
                myVertexColorArray[p+1+3] = last_GcolorSpec;
                myVertexColorArray[p+2+3] = last_BcolorSpec;
              }
              p+=12;//
            }
          
          
        } else {
          Polygon *blah = (*build)->poly;
          for (int i=(*blah).size()-1; i >= 0; i--) {
            myVertexArray[p+2] = building_min_height + vertSpec;
            myVertexArray[p+2+3] = building_min_height + vertSpec;
            myVertexColorArray[p] = RcolorSpec;
            myVertexColorArray[p+1] = GcolorSpec;
            myVertexColorArray[p+2] = BcolorSpec;
            myVertexColorArray[p+0+3] = RcolorSpec;
            myVertexColorArray[p+1+3] = GcolorSpec;
            myVertexColorArray[p+2+3] = BcolorSpec;
            p+=12;//
          }
        }
        
      }
      
      
      ///*
      if(drawRoofToggle) {
        VertexIndexList *bleh = (*build)->orderedVertices;  
        if(lasertagbattlemode) {
          
          Polygon *blah = (*build)->poly;
          
          for (int i=(*blah).size()-1; i >= 0; i--) {
            //myRoofsVertexArray[q+2] = building_min_height + act_vertSpec;
            ///*
            if(i == 0 ) {
              myRoofsVertexArray[q+2] = building_min_height + last_vertSpec;
              myRoofsColorVertexArray[q] = last_RcolorSpec;
              myRoofsColorVertexArray[q+1] = last_GcolorSpec;
              myRoofsColorVertexArray[q+2] = last_BcolorSpec;
            } else if(i == 1) {
              myRoofsVertexArray[q+2] = building_min_height + act_vertSpec;
              myRoofsColorVertexArray[q] = act_RcolorSpec;
              myRoofsColorVertexArray[q+1] = act_GcolorSpec;
              myRoofsColorVertexArray[q+2] = act_BcolorSpec;
            } else if( i == 2) {  
              myRoofsVertexArray[q+2] = building_min_height + act_vertSpec;
              myRoofsColorVertexArray[q] = act_RcolorSpec;
              myRoofsColorVertexArray[q+1] = act_GcolorSpec;
              myRoofsColorVertexArray[q+2] = act_BcolorSpec;
            } else if (i == 3){
              myRoofsVertexArray[q+2] = building_min_height + last_vertSpec;
              myRoofsColorVertexArray[q] = last_RcolorSpec;
              myRoofsColorVertexArray[q+1] = last_GcolorSpec;
              myRoofsColorVertexArray[q+2] = last_BcolorSpec;
            }
            //*/
            q += 3;//
          }
          
          /*
          //stroke direction check needed?
          float prevX1 = prevBuild->poly->at(1).x;
          float prevX2 = prevBuild->poly->at(2).x;
          float actX1 = (*build)->poly->at(1).x;
          float actX2 = (*build)->poly->at(2).x;
          prevBuild = *build;
          
          
          for (int i=0;i<6;i++) {
            
            
            //if(i==0 || i == 1 || i == 5) {
            //super hardcoded cause lasertag has only quads...--> 2 triangles on top
            if(i==1 || i==2 || i == 3 ) {
              myRoofsVertexArray[r+2] = building_min_height + act_vertSpec;
              myRoofsColorVertexArray[r+0] = act_RcolorSpec;
              myRoofsColorVertexArray[r+1] = act_GcolorSpec;
              myRoofsColorVertexArray[r+2] = act_BcolorSpec;
            } else { 
              myRoofsVertexArray[r+2] = building_min_height + last_vertSpec;
              myRoofsColorVertexArray[r+0] = last_RcolorSpec;
              myRoofsColorVertexArray[r+1] = last_GcolorSpec;
              myRoofsColorVertexArray[r+2] = last_BcolorSpec;
            }
            
            r += 3;
          }
          //*/
        } else {
          for (int i=0;i<(*bleh).size();i++) {
              myRoofsVertexArray[r+2] = building_min_height + vertSpec;
              myRoofsColorVertexArray[r+0] = RcolorSpec;
              myRoofsColorVertexArray[r+1] = GcolorSpec;
              myRoofsColorVertexArray[r+2] = BcolorSpec;
              r += 3;
          }
        }
        
      }
      lastSpecVar = s;
      buildC++;
  }
  if (drawGridToggle) {
    UpdateFFTGridVertexArray(raster_x, raster_y,0,false);
  }
  BeginFrame();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  //glScalef(0.01, 0.01, 0.01);
  //ltbattlescale
  glScalef(0.04, 0.04, 0.04);
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
      if(lasertagbattlemode && lt_even_polygons) {
        glDrawArrays(GL_QUADS, 0, roofsVertexArray.size()/3);
      } else {
        glDrawArrays(GL_TRIANGLES, 0, roofsVertexArray.size()/3);
      }
    }
    
    if(drawBuildingsToggle) {
      glVertexPointer(3, GL_FLOAT, 0, myVertexArray);
      glColorPointer(3, GL_FLOAT, 0, myVertexColorArray);
      glNormalPointer(GL_FLOAT, 0, myNormalsArray);
      glDrawArrays(GL_QUADS, 0, vertexArray.size()/3);
    }
    
    
    
    if (drawGridToggle) {
      //lines array has no normals, disable normal pointer or funny crashes happen!
      glDisableClientState(GL_NORMAL_ARRAY);
      glVertexPointer(3, GL_FLOAT, 0, fftgridVertexArray);
      glColorPointer(3, GL_FLOAT, 0, fftgridColorArray);
      glDrawArrays(GL_LINES, 0, (fftgridsize*8));
    }
      
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    //FIXME old immediatemode code
    if(drawSpec2GridLinesToggle) {
      build = buildings->begin();
    	buildend = buildings->end();
  	  for (;build != buildend; build++) {
          DrawCentroid2GridLines(**build, 0, 0, raster_x, raster_y);
    	}
  	}
  
  glPopMatrix();
  
  //krach_console_draw();
  
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

void MySDLVU::osc_set_evenpolygons(osc::ReceivedMessage& oscmsg) {
  osc::int32 evenpolytoggle = 0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> evenpolytoggle >> osc::EndMessage;
    lt_even_polygons = evenpolytoggle == 0?false:true;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
    //fallback just toggle if wrong argument/toomany/none is provided...
    lt_even_polygons = !lt_even_polygons;
  }
  cout << "lt_even_polygons: " << lt_even_polygons << endl;
}

void MySDLVU::osc_set_fftgrid_color_rgb(osc::ReceivedMessage& oscmsg) {
  float r=0;
  float g=0;
  float b=0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> r >> g >> b >> osc::EndMessage;
    fftgrid_color_multiply_R = r;
    fftgrid_color_multiply_G = g;
    fftgrid_color_multiply_B = b;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "fftgrid_color_multiply_RGB: " << fftgrid_color_multiply_R << " " << fftgrid_color_multiply_G << " " << fftgrid_color_multiply_B << " " << endl;
}

void MySDLVU::osc_set_fftgrid_vert_multiply(osc::ReceivedMessage& oscmsg) {
  float m = 0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> m >> osc::EndMessage;
    fftgrid_vert_multiply = m;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "fftgrid_vert_multiply: " << fftgrid_vert_multiply << endl;
}

void MySDLVU::osc_set_building_color_rgb(osc::ReceivedMessage& oscmsg) {
  float r=0;
  float g=0;
  float b=0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> r >> g >> b >> osc::EndMessage;
    building_color_multiply_R = r;
    building_color_multiply_G = g;
    building_color_multiply_B = b;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "building_color_multiply_RGB: " << r << " " << g << " " << b << " " << endl;
}

void MySDLVU::osc_set_building_vert_multiply(osc::ReceivedMessage& oscmsg) {
  float m = 0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> m >> osc::EndMessage;
    building_vert_multiply = m;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "building_vert_multiply: " << building_vert_multiply << endl;
}

void MySDLVU::osc_set_building_min_height(osc::ReceivedMessage& oscmsg) {
  float h = 0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> h >> osc::EndMessage;
    building_min_height = h;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "building_min_height: " << building_min_height << endl;
}

void MySDLVU::osc_toggleNormalizeWithFixedValue(osc::ReceivedMessage& oscmsg) {
  osc::int32 fixednormalizetoggle=0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> fixednormalizetoggle >> osc::EndMessage;
    normalizeaudiospectrumWithFixedValue = fixednormalizetoggle == 0?false:true;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
    //fallback just toggle if wrong argument/toomany/none is provided...
    normalizeaudiospectrumWithFixedValue = !normalizeaudiospectrumWithFixedValue;
  }
  cout << "normalizeaudiospectrumWithFixedValue: " << normalizeaudiospectrumWithFixedValue << endl;
}

void MySDLVU::osc_setFixedNormalizeMaxValue(osc::ReceivedMessage& oscmsg) {
  float newfixednormalizeMaxValue = 0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> newfixednormalizeMaxValue >> osc::EndMessage;
    fixednormalizeMaxValue = newfixednormalizeMaxValue;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "fixednormalizeMaxValue: " << fixednormalizeMaxValue << endl;
}

void MySDLVU::osc_toggleAdjustSpectrum_freqVSamp(osc::ReceivedMessage& oscmsg) {
  osc::int32 adjusttoggle=0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> adjusttoggle >> osc::EndMessage;
    adjustSpectrum_freqVSamp = adjusttoggle == 0?false:true;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
    //fallback just toggle if wrong argument/toomany/none is provided...
    adjustSpectrum_freqVSamp = !adjustSpectrum_freqVSamp;
  }
  cout << "adjustSpectrum_freqVSamp: " << adjustSpectrum_freqVSamp << endl;
}

void MySDLVU::osc_toggleNormalizeSpectrum(osc::ReceivedMessage& oscmsg) {
  osc::int32 normtoggle=0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> normtoggle >> osc::EndMessage;
    normalizeaudiospectrum = normtoggle == 0?false:true;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
    //fallback just toggle if wrong argument/toomany/none is provided...
    normalizeaudiospectrum = !normalizeaudiospectrum;
  }
  cout << "normalizeaudiospectrum: " << normalizeaudiospectrum << endl;
}

void MySDLVU::osc_toggleLogSpectrum(osc::ReceivedMessage& oscmsg) {
  osc::int32 logtoggle=0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> logtoggle >> osc::EndMessage;
    convertSpectrum2logScale = logtoggle == 0?false:true;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
    //fallback just toggle if wrong argument/toomany/none is provided...
    convertSpectrum2logScale = !convertSpectrum2logScale;
  }
  cout << "convertSpectrum2logScale: " << convertSpectrum2logScale << endl;
}

void MySDLVU::osc_setLogSpectrumLowCutoff(osc::ReceivedMessage& oscmsg) {
  osc::int32 lcutoff = 0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> lcutoff >> osc::EndMessage;
    //fixme gotta check for an upper limit for lcutoff?   < LOG_SPECTRUM_BANDS ?
    if(lcutoff > -1 ) {
      logSpectrumLowCutOff = lcutoff;
    }
    calculateConversionBoundaries();
    cout << "logSpectrumLowCutOff: " << logSpectrumLowCutOff << endl;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
}

void MySDLVU::osc_setLogSpectrumSize(osc::ReceivedMessage& oscmsg) {
  osc::int32 logsize = DEF_LOG_SPECTRUM_BANDS;
  int new_raster_x=0;
  int new_raster_y=0;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> logsize >> osc::EndMessage;
    
    //fixme : logarithmic spectrum can be of any size... but switching the MySpectrum pointer to linear with to big raster crashes us of course.... separate this!  (crash happens in update_spectrum_raster_mappings() iirc)
    if(logsize != LOG_SPECTRUM_BANDS && logsize > 2 && logsize < SPECTRUM_LENGTH+1 && logsize >= raster_x*raster_y) {
      LOG_SPECTRUM_BANDS = logsize;
      free(conversionBoundaries);
      free(MyLogSpectrum);
      conversionBoundaries = (int*) malloc(sizeof(int)*LOG_SPECTRUM_BANDS*2);
      MyLogSpectrum = (float*) malloc(sizeof(float)*LOG_SPECTRUM_BANDS);
    
      
      calculateConversionBoundaries();
      update_spectrum_raster_mappings();
    }
    cout << "LOG_SPECTRUM_BANDS: " << LOG_SPECTRUM_BANDS << endl;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
}

void MySDLVU::osc_setColorsTop(osc::ReceivedMessage& oscmsg) {
  
  rgb_R_divisor = 1;
  rgb_G_divisor = 1;
  rgb_B_divisor = 1;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> rgb_R_multiplier >> rgb_G_multiplier >> rgb_B_multiplier >> osc::EndMessage;
    //raster_poly_search_distance = newsearchdist;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
};

void MySDLVU::osc_setColorsBottom(osc::ReceivedMessage& oscmsg) {
  
};

void MySDLVU::update_spectrum_raster_mappings() {
  this->genSpec2RasterMapping(raster_x, raster_y, 0);
  this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
  this->GenerateFFTGridVertexArray(raster_x, raster_y);
}

void MySDLVU::osc_setRasterPolySearchDistance(osc::ReceivedMessage& oscmsg) {
  osc::int32 newsearchdist = raster_poly_search_distance;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> newsearchdist >> osc::EndMessage;
    raster_poly_search_distance = newsearchdist;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  update_spectrum_raster_mappings();
}

void MySDLVU::osc_setRasterSize(osc::ReceivedMessage& oscmsg) {
  osc::int32 new_raster_x = DEF_RASTER_X;
  osc::int32 new_raster_y = DEF_RASTER_Y;
  osc::int32 new_spectrumDivisor = 2;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    osc::int32 new_raster_x, new_raster_y, new_spectrumDivisor;
    args >> new_raster_x >> new_raster_y >> new_spectrumDivisor >> osc::EndMessage;
    if(new_raster_x * new_raster_y < SPECTRUM_LENGTH+1 && new_raster_x * new_raster_y < LOG_SPECTRUM_BANDS+1) {
      raster_x = new_raster_x;
      raster_y = new_raster_y;
      spectrumDivisor = new_spectrumDivisor;
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  update_spectrum_raster_mappings();
}

void MySDLVU::osc_loadNextImage(osc::ReceivedMessage& oscmsg) {
  if(lasertagbattlemode) {
    return;
  }
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



void MySDLVU::osc_set_osc_actstroke_specvar(osc::ReceivedMessage& oscmsg) {
  float n_min;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> n_min >> osc::EndMessage;
    osc_actstroke_specvar = n_min;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "osc_actstroke_specvar: " << osc_actstroke_specvar << endl;
}

void MySDLVU::osc_set_osc_minimum_specvar(osc::ReceivedMessage& oscmsg) {
  float n_min;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> n_min >> osc::EndMessage;
    osc_minimum_specvar = n_min;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  cout << "osc_minimum_specvar: " << osc_minimum_specvar << endl;
}

//set brushsize via osc 
//behaviour if this is done in the middle of drawing a lt shape maybe funny!?
void MySDLVU::osc_ltbattle_setbrushsize(osc::ReceivedMessage& oscmsg) {
  osc::int32 n_brushsize;
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> n_brushsize >> osc::EndMessage;
    lt_brush_size = n_brushsize;
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  
  cout << "lt_brush_offset_x: " << lt_brush_offset_x << endl;
  cout << "lt_brush_offset_y: " << lt_brush_offset_y << endl;
  cout << "lt_brush_size: " << lt_brush_size << endl;
}

void MySDLVU::setupLTbattlemode() {
  
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
  
  //if(geometry_resources_freed == false) {
    delete[] myVertexArray;
    delete[] myVertexColorArray;
    delete[] myRoofsVertexArray;
    delete[] myRoofsColorVertexArray;
    delete[] myNormalsArray;
    delete[] myRoofsNormalsArray;
    //geometry_resources_freed = true;
  //}
  
  /**/
  freePolygonList(*polygons);
  freeBuildingList(*buildings);
  //geometry_resources_freed = true;
  
  maxX=1024;
  minX=0;
  maxY=768;
  minY=0;
  
	rvc=0;
	bool lt_newstroke_starting = true;
  int ltshape_pointcount = 0;
}

void MySDLVU::set_ltbattlemode(bool toggle) {
  ltbattlemodetoggle = toggle;
  if(ltbattlemodetoggle == true) {
    
    //freePolygonList(*polygons);
    //freeBuildingList(*buildings);
    //setupLTbattlemode();
    lasertagbattlemode = true;
  } else {
    lasertagbattlemode = false;
    
  }
  cout << "LaserTagBattle on/off: " << ltbattlemodetoggle << endl;
}

void MySDLVU::osc_toggle_ltbattlemode(osc::ReceivedMessage& oscmsg) {
  int ltbattlemode = 0;
  try {
    osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
    if(oscmsg.ArgumentCount() > 0) {
      if(arg->IsInt32()) {
        ltbattlemode = arg->AsInt32();
      }
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  bool tmptoggle = ltbattlemode == 0 ? false : true;
  set_ltbattlemode(tmptoggle);
}

void MySDLVU::osc_ltbattle_recvCoords(osc::ReceivedMessage& oscmsg) {
  if(!lasertagbattlemode) {
    return;
  }
  osc::int32 px;
  osc::int32 py;
  osc::int32 wpx;
  osc::int32 wpy;
  //fixme 
  //will work with warped points for now
  //maybe do warping ourself in future?
  try {
    osc::ReceivedMessageArgumentStream args = oscmsg.ArgumentStream();
    args >> px >> py >> wpx >> wpy >> osc::EndMessage;
    
    //test
    //if distance between last point and new incoming point < brushsize   we discard the new point
    float distance=0;
    int new_p_x = wpx;
    int new_p_y = wpy;
    //fixme hardcoded check if lasertag battle is sending us non warped coords aka non traced coords, but coords created by mouseinput
    if(wpx == -500 && wpy == -500) {
      new_p_x = px;
      new_p_y = py;
    }
    if(lt_int_shape.size() > 1 ) {
      int lastx = lt_int_shape.at(lt_int_shape.size()-2);
      int lasty = lt_int_shape.at(lt_int_shape.size()-1);
      float xcomp = new_p_x - lastx;
      float ycomp = new_p_y - lasty;
      distance = sqrt(pow(xcomp,2) + pow(ycomp,2)); //2d distance only
      //cout << "last wpx/y: " << lastx << " / " << lasty << endl;
      //cout << "     wpx/y: " << new_p_x << " / " << new_p_y << endl;
      //cout << "distance: " << distance << endl;
    } else {
      distance = lt_brush_size +1;
    }
    if(distance > lt_brush_size) {
      lt_int_shape.push_back(new_p_x);
      lt_int_shape.push_back(new_p_y);

      act_lt_shape.push_back(wpx);
      act_lt_shape.push_back(wpy);
      act_lt_shape_timestamps.push_back(SDL_GetTicks());
    
      ltshape_pointcount++;
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  //cout << "received coords via osc:" << endl;
  //cout << wpx << " " << wpy << endl;
  process_lt_coords();
}

void MySDLVU::process_lt_coords() {
  //fixme make brush width osc defineable
  //bool lt_newstroke_starting;
  //int ltshape_pointcount;
  
  
  //if timestamp == 0... = Move command
  if(lt_int_shape.size() > 2 && lt_int_shape.size() < 5) {
    //fresh start:
    //take last two points and make a bulding out of them
    create_building_from_lasercoord();
    //delete first point from lt_shape vector, we keep last one for next building/poly
    // & reset newstroke flag
    vector<int>::iterator sb = lt_int_shape.begin();
    lt_int_shape.erase(sb);
    sb = lt_int_shape.begin();
    lt_int_shape.erase(sb);
    lt_newstroke_starting = false;
    //cout << "process lt coords end" << endl;
  }
  
}

void MySDLVU::create_building_from_lasercoord() {
  //takes last two points from lt_shape vector and makes a "building" out of them
  Polygon *polygon = new Polygon(4);
  Building *building = new Building();
  (*building).poly = new Polygon(4);
  Polygon *bpoly = (*building).poly;
  
  (*building).orderedVertices = new VertexIndexList();
  VertexIndexList* vil = (*building).orderedVertices;
  (*building).rasterPoints2affect = new Raster2VertexList();
  
  float x,y,x2,y2,x3,y3,x4,y4;
  x=0;y=0;x2=0;y2=0;x3=0;y3=0;x4=0;y4=0;
  x = lt_int_shape.at(0);
  y = lt_int_shape.at(1);
  x2 = lt_int_shape.at(2);
  y2 = lt_int_shape.at(3);
  
  
  GLfloat *lt_v = new GLfloat[2];
  lt_v[0] = x2 - x;
  lt_v[1] = y2 - y;
  
  //normalize
  GLfloat length = sqrt(
    pow(lt_v[0],2)+
    pow(lt_v[1],2)
  );
  lt_v[0] /= length;
  lt_v[1] /= length;
  
  //normal:
  //to the right
  GLfloat *lt_nv = new GLfloat[2];
  lt_nv[0] = lt_v[1];
  lt_nv[1] = lt_v[0] * -1;
  //normal:
  //to the left
  /*GLfloat *lt_nvl = new GLfloat[2];
  lt_nvl[0] = lt_v[1] * -1;
  lt_nvl[1] = lt_v[0];*/
  
  x = lt_int_shape.at(0) - (lt_brush_size/2) * lt_nv[0];
  y = lt_int_shape.at(1) - (lt_brush_size/2) * lt_nv[1];
  x2 = lt_int_shape.at(2) - (lt_brush_size/2) * lt_nv[0];
  y2 = lt_int_shape.at(3) - (lt_brush_size/2) * lt_nv[1];
  x3 = lt_int_shape.at(2) + (lt_brush_size/2) * lt_nv[0];
  y3 = lt_int_shape.at(3) + (lt_brush_size/2) * lt_nv[1];
  x4 = lt_int_shape.at(0) + (lt_brush_size/2) * lt_nv[0];
  y4 = lt_int_shape.at(1) + (lt_brush_size/2) * lt_nv[1];
  
  //fixme
  //checking newstroke_startin should be enough.
  //then we should at least have buildings->size() > 0
  if(lt_newstroke_starting == false && buildings->size() > 0) {
    //"merge" this new polygon with previous one, so they dont overlap
    //x/y of last 2 points of last polygon
    GLfloat *mrgd_nv = new GLfloat[2];
    mrgd_nv[0] = lt_nv[0] + last_normalv[0];
    mrgd_nv[1] = lt_nv[1] + last_normalv[1];
    //normalize
    GLfloat mrgd_length = sqrt(
      pow(mrgd_nv[0],2)+
      pow(mrgd_nv[1],2)
    );
    mrgd_nv[0] /= mrgd_length;
    mrgd_nv[1] /= mrgd_length;
    
    //cout << "buildings size: " << buildings->size() << endl;
    //cout << "buildings size: " << buildings->size() << endl;
    
    Building *prevBuild = buildings->at(buildings->size()-1);
    Polygon *prevPoly = prevBuild->poly;
    ///*
    (*prevPoly)[1].x = lt_int_shape.at(0) - (lt_brush_size/2) * mrgd_nv[0];
    (*prevPoly)[1].y = lt_int_shape.at(1) - (lt_brush_size/2) * mrgd_nv[1];
    (*prevPoly)[2].x = lt_int_shape.at(0) + (lt_brush_size/2) * mrgd_nv[0];
    (*prevPoly)[2].y = lt_int_shape.at(1) + (lt_brush_size/2) * mrgd_nv[1];
    x = (*prevPoly)[1].x;
    y = (*prevPoly)[1].y;
    x4 = (*prevPoly)[2].x;
    y4 = (*prevPoly)[2].y;
    //*/
    
    /*
    //median works at last!!!!
    (*prevPoly)[1].x = ((*prevPoly)[1].x + x) / 2;
    x = (*prevPoly)[1].x;
    (*prevPoly)[1].y = ((*prevPoly)[1].y + y) / 2;
    y = (*prevPoly)[1].y;
    (*prevPoly)[2].x = ((*prevPoly)[2].x + x4) / 2;
    x4 = (*prevPoly)[2].x;
    (*prevPoly)[2].y = ((*prevPoly)[2].y + y4) / 2;
    y4 = (*prevPoly)[2].y;
    */
    
    
    
    
    //recalc area/centroid etc for prev poly and remap it onto fft grid
    calcCentroidForBuilding(prevBuild);
    //erase prevBuild from vertexarray - 4 quads 12vertices each 
    //fixme keep loop count variable!!
    for(int vaI=0; vaI < prevPoly->size()*12; vaI++) {
    //for(int vaI=0;vaI<48;vaI++) {
      vertexArray.pop_back();
      normalsArray.pop_back();
      colorVertexArray.pop_back();
    }
    addBuilding2VertexArray(prevBuild);
    //
    //for (int i=0; i < prevBuild->orderedVertices->size()*3; i++) {
    //forloop for QUAD roofs, instead of triangle roofs
    for (int i=0; i < prevPoly->size()*3; i++) {
      roofsVertexArray.pop_back();
      roofsColorVertexArray.pop_back();
      roofsNormalsArray.pop_back();
    }
    //TessellateBuilding(*prevBuild);
    //test for only using quads instead of triangles for "roofs"
    roofsVertexArray.push_back(prevPoly->at(0).x);
    roofsVertexArray.push_back(-prevPoly->at(0).y);
    roofsVertexArray.push_back(50);
    roofsVertexArray.push_back(prevPoly->at(1).x);
    roofsVertexArray.push_back(-prevPoly->at(1).y);
    roofsVertexArray.push_back(50);
    roofsVertexArray.push_back(prevPoly->at(2).x);
    roofsVertexArray.push_back(-prevPoly->at(2).y);
    roofsVertexArray.push_back(50);
    roofsVertexArray.push_back(prevPoly->at(3).x);
    roofsVertexArray.push_back(-prevPoly->at(3).y);
    roofsVertexArray.push_back(50);
    for(int i=0;i<4;i++) {
      roofsColorVertexArray.push_back(1.0);
      roofsColorVertexArray.push_back(1.0);
      roofsColorVertexArray.push_back(1.0);
    
      roofsNormalsArray.push_back(0);
      roofsNormalsArray.push_back(0);
      roofsNormalsArray.push_back(1);
    }
    //since i only use quads for lasertag visualization i can let the tessellator be a tessellator..... on its own...
    //loaded into mem but not called from here.
    /*
    roofsVertexArray.push_back(prevPoly->at(0).x);
    roofsVertexArray.push_back(-prevPoly->at(0).y);
    roofsVertexArray.push_back(50);
    roofsVertexArray.push_back(prevPoly->at(1).x);
    roofsVertexArray.push_back(-prevPoly->at(1).y);
    roofsVertexArray.push_back(50);
    roofsVertexArray.push_back(prevPoly->at(2).x);
    roofsVertexArray.push_back(-prevPoly->at(2).y);
    roofsVertexArray.push_back(50);
    
    roofsVertexArray.push_back(prevPoly->at(2).x);
    roofsVertexArray.push_back(-prevPoly->at(2).y);
    roofsVertexArray.push_back(50);
    roofsVertexArray.push_back(prevPoly->at(3).x);
    roofsVertexArray.push_back(-prevPoly->at(3).y);
    roofsVertexArray.push_back(50);
    roofsVertexArray.push_back(prevPoly->at(0).x);
    roofsVertexArray.push_back(-prevPoly->at(0).y);
    roofsVertexArray.push_back(50);
    
    for(int i=0;i<6;i++) {
    roofsColorVertexArray.push_back(1.0);
    roofsColorVertexArray.push_back(1.0);
    roofsColorVertexArray.push_back(1.0);
    
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(1);
    }
    //*/
    //
    
    //"need combine callback" / GLU_TESS_NEED_COMBINE_CALLBACK error may happen here
    //hmmmmm? means something is intersecting... 
    /* to recreate send coords: 480 480, 490 490, 493 490, 500 480    or 480 500, 490 490, 493 490, 500 500
      brushsize will matter if error occurs or not ofcourse...
    */
    
    
    
    
    
    //fixme will this then be double mapped to fft grid do i have to delete something first?
    //this->genPoly2RasterFactorForBuilding(raster_x, raster_y, raster_poly_search_distance, prevBuild);
    
   
    
    delete[] mrgd_nv;
    //merge end
  }
  //save last normalvector
  last_normalv[0] = lt_nv[0];
  last_normalv[1] = lt_nv[1];
  last_lt_v[0] = lt_v[0];
  last_lt_v[1] = lt_v[1];
  delete[] lt_v;
  delete[] lt_nv;
  
  
  double z;
  int count = 0;
  z  = (x3 - x) * (y3 - y3);
  z -= (y3 - y) * (x3 - x3);
  if (z < 0)
     count--;
  else if (z > 0)
     count++;
  z  = (x2 - x2) * (y - y2);
  z -= (y2 - y2) * (x - x2);
  if (z < 0)
     count--;
  else if (z > 0)
     count++;
  z  = (x - x3) * (y2 - y);
  z -= (y - y3) * (x2 - x);
  if (z < 0)
     count--;
  else if (z > 0)
     count++;
  
  if (count > 0) {
    //return(COUNTERCLOCKWISE);
    //cout << "COUNTERCLOCKWISE" << endl;
    (*polygon)[0].x = x4;
    (*polygon)[0].y = y4;
    (*bpoly)[0].x = x4;
    (*bpoly)[0].y = y4;
    (*polygon)[1].x = x3;
    (*polygon)[1].y = y3;
    (*bpoly)[1].x = x3;
    (*bpoly)[1].y = y3;
    (*polygon)[2].x = x2;
    (*polygon)[2].y = y2;
    (*bpoly)[2].x = x2;
    (*bpoly)[2].y = y2;
    (*polygon)[3].x = x;
    (*polygon)[3].y = y;
    (*bpoly)[3].x = x;
    (*bpoly)[3].y = y;
    
  } else if (count < 0) {
    //cout << "CLOCKWISE" << endl;
    //return(CLOCKWISE);
    (*polygon)[0].x = x;
    (*polygon)[0].y = y;
    (*bpoly)[0].x = x;
    (*bpoly)[0].y = y;
    (*polygon)[1].x = x2;
    (*polygon)[1].y = y2;
    (*bpoly)[1].x = x2;
    (*bpoly)[1].y = y2;
    (*polygon)[2].x = x3;
    (*polygon)[2].y = y3;
    (*bpoly)[2].x = x3;
    (*bpoly)[2].y = y3;
    (*polygon)[3].x = x4;
    (*polygon)[3].y = y4;
    (*bpoly)[3].x = x4;
    (*bpoly)[3].y = y4;
  } else {
     //return(0);
  }
  
  calcCentroidForBuilding(building);
  addBuilding2VertexArray(building);
  buildings->push_back(building);
  
	
  delete[] myVertexArray;
  delete[] myVertexColorArray;
  delete[] myRoofsVertexArray;
  delete[] myRoofsColorVertexArray;
  delete[] myNormalsArray;
  delete[] myRoofsNormalsArray;
//fixme crashed here!?	
	myVertexArray = new GLfloat[vertexArray.size()];
  copy(vertexArray.begin(), vertexArray.end(), myVertexArray);
  
  myVertexColorArray = new GLfloat[colorVertexArray.size()];
  copy(colorVertexArray.begin(), colorVertexArray.end(), myVertexColorArray);
  
  myNormalsArray = new GLfloat[normalsArray.size()];
  copy(normalsArray.begin(), normalsArray.end(), myNormalsArray);
  
  geometry_resources_freed = false;
  
  /*cout << "" << endl;
  cout << "vertArray size: " << vertexArray.size() << endl;
  cout << "" << endl;
  
  printf("have %d buildings\n", buildings->size());*/
  
  this->polycount = buildings->size();
	spectrumIndex=0;


  /*maxX=FLT_MIN;
  minX=FLT_MAX;
  maxY=FLT_MIN;
  minY=FLT_MAX;
	findPlaneMaxima();*/
	
	maxX=1024;
  minX=0;
  maxY=768;
  minY=0;
	
	rvc=0;
	
	//fixme temp fix
  //drawRoofToggle = false;
  
  //TessellateBuilding(*building);
  //only quads used for lasertag... no tessellation needed yet :D horray!  (see above   where i mangle prevbuilding)
  //test for using quads as roofs not triangles... no tessellation
  roofsVertexArray.push_back(building->poly->at(0).x);
  roofsVertexArray.push_back(-building->poly->at(0).y);
  roofsVertexArray.push_back(50);
  roofsVertexArray.push_back(building->poly->at(1).x);
  roofsVertexArray.push_back(-building->poly->at(1).y);
  roofsVertexArray.push_back(50);
  roofsVertexArray.push_back(building->poly->at(2).x);
  roofsVertexArray.push_back(-building->poly->at(2).y);
  roofsVertexArray.push_back(50);
  roofsVertexArray.push_back(building->poly->at(3).x);
  roofsVertexArray.push_back(-building->poly->at(3).y);
  roofsVertexArray.push_back(50);
  
  for(int i=0;i<4;i++) {
    roofsColorVertexArray.push_back(1.0);
    roofsColorVertexArray.push_back(1.0);
    roofsColorVertexArray.push_back(1.0);
  
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(1);
  }
  /*
  
  roofsVertexArray.push_back(building->poly->at(0).x);
  roofsVertexArray.push_back(-building->poly->at(0).y);
  roofsVertexArray.push_back(50);
  roofsVertexArray.push_back(building->poly->at(1).x);
  roofsVertexArray.push_back(-building->poly->at(1).y);
  roofsVertexArray.push_back(50);
  roofsVertexArray.push_back(building->poly->at(2).x);
  roofsVertexArray.push_back(-building->poly->at(2).y);
  roofsVertexArray.push_back(50);
  
  roofsVertexArray.push_back(building->poly->at(2).x);
  roofsVertexArray.push_back(-building->poly->at(2).y);
  roofsVertexArray.push_back(50);
  roofsVertexArray.push_back(building->poly->at(3).x);
  roofsVertexArray.push_back(-building->poly->at(3).y);
  roofsVertexArray.push_back(50);
  roofsVertexArray.push_back(building->poly->at(0).x);
  roofsVertexArray.push_back(-building->poly->at(0).y);
  roofsVertexArray.push_back(50);
  
  for(int i=0;i<6;i++) {
    roofsColorVertexArray.push_back(1.0);
    roofsColorVertexArray.push_back(1.0);
    roofsColorVertexArray.push_back(1.0);
  
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(0);
    roofsNormalsArray.push_back(1);
  }
  //*/
  myRoofsVertexArray = new GLfloat[roofsVertexArray.size()];
  copy(roofsVertexArray.begin(), roofsVertexArray.end(), myRoofsVertexArray);

  myRoofsColorVertexArray = new GLfloat[roofsVertexArray.size()];
  copy(roofsColorVertexArray.begin(), roofsColorVertexArray.end(), myRoofsColorVertexArray);

  myRoofsNormalsArray = new GLfloat[roofsNormalsArray.size()];
  copy(roofsNormalsArray.begin(), roofsNormalsArray.end(), myRoofsNormalsArray);
  //this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
  this->genPoly2RasterFactorForBuilding(raster_x, raster_y, raster_poly_search_distance, building);
  //this->genSpec2RasterMapping(raster_x, raster_y,0);
  
  //cout << "create building from laser coords end" << endl;
}

void MySDLVU::calcCentroidForBuilding(Building *building) {
  Polygon *bpoly = (*building).poly;
  //area of poly:
  double A=0;
  int p=0;
  int i,j;
  double area = 0;

  for (i=0;i<4;i++) {
    j = (i + 1) % 4;
    area += (*bpoly)[i].x * (*bpoly)[j].y;
    area -= (*bpoly)[i].y * (*bpoly)[j].x;
    //cout << "tmp area: " << area << endl;
  }
  area /= 2;
  A = (area < 0 ? area * -1 : area );
  //cout << "AREA: " << A << endl;
  
  //centroid:
  double tmpResultx=0;
  double tmpResulty=0;
  p=0;
  for (int i=0; i < 4; i++) {
    p = (i + 1) % 4;
    tmpResultx += ( (double)(*bpoly)[i].x + (double)(*bpoly)[p].x ) * ( ((double)(*bpoly)[i].x * (double)(*bpoly)[p].y) - ((double)(*bpoly)[p].x * (double)(*bpoly)[i].y) );
    tmpResulty += ( (double)(*bpoly)[i].y + (double)(*bpoly)[p].y ) * ( ((double)(*bpoly)[i].x * (double)(*bpoly)[p].y) - ((double)(*bpoly)[p].x * (double)(*bpoly)[i].y) );
  }

  (*building).dCenterX = (double)( (double)1 / (double)(A * 6.)) * tmpResultx;
  (*building).dCenterY = (double)( (double)1 / (double)(A * 6.)) * tmpResulty;
  (*building).fCenterX = (double)( (double)1 / (double)(A * 6.)) * tmpResultx;
  (*building).fCenterY = (double)( (double)1 / (double)(A * 6.)) * tmpResulty;
  if((*building).fCenterY > 0) {
    (*building).dCenterY *= -1.;
    (*building).fCenterY *= -1.;
  }
  
  if((*building).fCenterX > 0) {
    (*building).dCenterX *= -1.;
    (*building).fCenterX *= -1.;
  }
}


void MySDLVU::addBuilding2VertexArray(Building *building) {
  int p = 0;
  int n = 0;
  int w = 0;
  
  int specVar=1;
  int height=50;

  Polygon *poly = (Polygon*) (*building).poly;
  /*cout << "building debug:" << endl;
  cout << (*poly)[0].x << " " << (*poly)[0].y << endl;
  cout << (*poly)[1].x << " " << (*poly)[1].y << endl;
  cout << (*poly)[2].x << " " << (*poly)[2].y << endl;
  cout << (*poly)[3].x << " " << (*poly)[3].y << endl;*/
  
  //cout << "Poly SIZE: " << (*poly).size() << endl;
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
    //fixme (possible) 
    v1[0] *= -1;
    
    //fixme 
    //this is 2d only!
    
    //normalize:
    GLfloat length = sqrt(
      pow(v1[0],2)+
      pow(v1[1],2)
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
}

void MySDLVU::osc_ltbattle_cmd(osc::ReceivedMessage& oscmsg) {
  if(!lasertagbattlemode) {
    return;
  }
  
  string oscltcmd;
  try {
    osc::ReceivedMessage::const_iterator arg = oscmsg.ArgumentsBegin();
    if(oscmsg.ArgumentCount() > 0) {
      if(arg->IsString()) {
        oscltcmd = arg->AsString();
      }
    }
  } catch( Exception& e ) {
    cout << "exception: " << e.what() << endl;
  }
  
  if(oscltcmd.compare("move") == 0) {
    lt_newstroke_starting = true;
    lt_int_shape.clear();
    act_lt_shape.push_back(INT_MAX);
    act_lt_shape.push_back(INT_MAX);
    act_lt_shape_timestamps.push_back(SDL_GetTicks());
    lt_animate_until_varray_index = vertexArray.size()-1;
    lt_animate_until_roofsvarray_index = roofsVertexArray.size()-1;
    strokebeginindices.push_back(vertexArray.size()-1);
  } else if(oscltcmd.compare("clear") == 0) {
    //fixme 
    //possible memory leak? because of how t_lt_shape is defined... pointer hell once more for drug infested brainz
    t_lt_shape myshape;
    myshape.lt_shape_P = &act_lt_shape;
    myshape.lt_shape_timestamps_P = &act_lt_shape_timestamps;
    lt_shapes.push_back(myshape);
    act_lt_shape.clear();
    act_lt_shape_timestamps.clear();
    
    lt_int_shape.clear();
    freePolygonList(*polygons);
    freeBuildingList(*buildings);
    buildings->clear();
    
    vertexArray.clear();
    colorVertexArray.clear();
    normalsArray.clear();
    
    roofsVertexArray.clear();
    roofsColorVertexArray.clear();
    roofsNormalsArray.clear();
    buildingOffsetsinRoofsArray.clear();
    
    lt_animate_until_varray_index = 0;
    lt_animate_until_roofsvarray_index = 0;
    strokebeginindices.clear();
  } else {
    //
    cout << "unknown osc lasertag cmd received" <<  oscltcmd << endl;
  }
  /*
  std::vector< MapFragment > ltshapes;
  std::vector<Building> ltshape;
  std::vector<int> lt_int_shape;
  */
}

void MySDLVU::calculateConversionBoundaries() {
  logSpectrumBandwidth = log2(SPECTRUM_LENGTH)/LOG_SPECTRUM_BANDS;
	for( int j=0; j<LOG_SPECTRUM_BANDS; j++ ) {
		conversionBoundaries[j]		= round(pow(2,  j   *logSpectrumBandwidth)-1);
		conversionBoundaries[j+1]	= round(pow(2, (j+1)*logSpectrumBandwidth)-1);
	}
  if(logSpectrumLowCutOff > 0) {
    int j = 0;
    int cutoff=0;
    while(j < LOG_SPECTRUM_BANDS) {
      if(conversionBoundaries[j] <= logSpectrumLowCutOff) {
        cutoff++;
      }
      j++;
    }
    logSpectrumBandwidth = log2(SPECTRUM_LENGTH)/(LOG_SPECTRUM_BANDS+cutoff);
    for( int j=cutoff; j<LOG_SPECTRUM_BANDS+cutoff; j++ ) {
      conversionBoundaries[j-cutoff]   = round(pow(2,  j   *logSpectrumBandwidth)-1);
      conversionBoundaries[j+1-cutoff] = round(pow(2, (j+1)*logSpectrumBandwidth)-1);
    }
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

  this->polycount = buildings->size();
  
  
  conversionBoundaries = (int*) malloc(sizeof(int)*LOG_SPECTRUM_BANDS*2);
  MyLogSpectrum = (float*) malloc(sizeof(float)*LOG_SPECTRUM_BANDS);
  
  
  calculateConversionBoundaries();
  
  
  imgSwitchTime_start = clock();
  
  Uint32 myticks = SDL_GetTicks();
  int tempColorShiftTicks = 0;
  int colorShiftWaitTicks = 30;
  Uint32 camTicks = SDL_GetTicks();
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
  registerEvent_memberfunc("/asdf/grid/polysearchdistance",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_setRasterPolySearchDistance);
  registerEvent_memberfunc("/asdf/grid/size",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_setRasterSize);
  registerEvent_memberfunc("/asdf/colors/top",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_setColorsTop);
  registerEvent_memberfunc("/asdf/colors/bottom",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_setColorsBottom);
  
  registerEvent_memberfunc("/asdf/ltbattle/coordinates",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_ltbattle_recvCoords);
  registerEvent_memberfunc("/asdf/ltbattle/cmd",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_ltbattle_cmd);
  registerEvent_memberfunc("/asdf/ltbattle/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggle_ltbattlemode);
  registerEvent_memberfunc("/asdf/ltbattle/brushsize",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_ltbattle_setbrushsize);
  registerEvent_memberfunc("/asdf/ltbattle/evenpolygons/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_evenpolygons);
  registerEvent_memberfunc("/asdf/ltbattle/minimumspecvar/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_osc_minimum_specvar);
  registerEvent_memberfunc("/asdf/ltbattle/activestroke/minimumspecvar/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_osc_actstroke_specvar);
  
  registerEvent_memberfunc("/asdf/audio/logspectrum/size",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_setLogSpectrumSize);
  registerEvent_memberfunc("/asdf/audio/logspectrum/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleLogSpectrum);
  registerEvent_memberfunc("/asdf/audio/logspectrum/lowcutoff/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_setLogSpectrumLowCutoff);
  registerEvent_memberfunc("/asdf/audio/spectrum/normalize/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleNormalizeSpectrum);
  registerEvent_memberfunc("/asdf/audio/spectrum/adjustspectrumfreqsnamps/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleAdjustSpectrum_freqVSamp);
  registerEvent_memberfunc("/asdf/audio/spectrum/normalizewithfixedmaxvalue/toggle",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_toggleNormalizeWithFixedValue);
  registerEvent_memberfunc("/asdf/audio/spectrum/normalizewithfixedmaxvalue/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_setFixedNormalizeMaxValue);
  
  
  registerEvent_memberfunc("/asdf/fftgrid/colors/setrgb",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_fftgrid_color_rgb);
  registerEvent_memberfunc("/asdf/fftgrid/spectrummultiplier/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_fftgrid_vert_multiply);
  registerEvent_memberfunc("/asdf/buildings/colors/setrgb",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_building_color_rgb);
  registerEvent_memberfunc("/asdf/buildings/spectrummultiplier/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_building_vert_multiply);
  registerEvent_memberfunc("/asdf/buildings/minimumheight/set",
    (void(asdfEventHandler::*)(osc::ReceivedMessage&))&MySDLVU::osc_set_building_min_height);
  
  float *stdSpecPtr = linearSpectrum;

	while (!done) {
    //fmodsystem->update();
    
    //process OSC messages:
    //process OSC messages:
    SDL_mutexP(oscMsgQmutex);
    if(oscMsgQ.msgqueue.size() > 0) {
      //oscMsgQ_CI msgIter = oscMsgQ.msgqueue.begin();
      deque<osc::ReceivedMessage*>::iterator msgIter = oscMsgQ.msgqueue.begin();
      for(; msgIter != oscMsgQ.msgqueue.end(); msgIter++) {
        //cout << "executing func for AddressPattern: " << (*msgIter)->AddressPattern() << endl;
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
      MySpectrum = stdSpecPtr;
      
      result = channel->getSpectrum(MySpectrum, SPECTRUM_LENGTH, 0, FMOD_DSP_FFT_WINDOW_TRIANGLE);
      ERRCHECK(result);
      
      
      //fixme 
      //maybe we can flatten the boost of higher frequencies a little out?
      if(adjustSpectrum_freqVSamp) {
        //specHeight = MySpectrum[this->spec2raster[x*rasterY+y]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[x*rasterY+y]))) * 50;
        int nyquistrate = OUTPUTRATE/2; //44100/2 = 22050
        for( int i=0; i<SPECTRUM_LENGTH; i++) {
          MySpectrum[i] = MySpectrum[i] * (nyquistrate/SPECTRUM_LENGTH) * (i+1);
        }
      }
      
      if(normalizeaudiospectrumWithFixedValue) {
      	for( int i=0; i<SPECTRUM_LENGTH; i++) {
      		MySpectrum[i] = MySpectrum[i]*fixednormalizeMaxValue;
      	}
    	}
    	
      //normalize spectrum (code stolen shamelessly from max)
      if(normalizeaudiospectrum) { 
        float tempMaxSpec = 0;
      	for( int i=0; i<SPECTRUM_LENGTH; i++) {
      	  //spectrumAvg[i] = (spectrumLeft[i]+spectrumRight[i])/2.0;
      		tempMaxSpec = ( MySpectrum[i] > tempMaxSpec ? MySpectrum[i] : tempMaxSpec );
      	}
      	tempMaxSpec = 1/tempMaxSpec;
      	for( int i=0; i<SPECTRUM_LENGTH; i++) {
      		MySpectrum[i] = MySpectrum[i]*tempMaxSpec;
      	}
    	}
    	
    	if(convertSpectrum2logScale) {
        float tempSum=0;
        for( int j=0; j<LOG_SPECTRUM_BANDS; j++ ) {
      		tempSum = 0;
      		for( int i=conversionBoundaries[j]; i<=conversionBoundaries[j+1]; i++){
      			tempSum += MySpectrum[i];
      		}
      		MyLogSpectrum[j] = tempSum/( conversionBoundaries[j+1]-conversionBoundaries[j]+1 );
      	}
      	//fixme workaround for now:
      	//evil pointing with pointers ...
        MySpectrum = MyLogSpectrum;
      }
    	
    }
    
    
    /* holy shmoly documentation sez:
    
      The larger the numvalues, the more CPU the FFT will take. Choose the right value to trade off between accuracy / speed.
      The larger the numvalues, the more 'lag' the spectrum will seem to inherit. This is because the FFT window size stretches the analysis back in time to what was already played. For example if the numvalues size happened to be 44100 and the output rate was 44100 it would be analyzing the past second of data, and giving you the average spectrum over that time period.
      If you are not displaying the result in dB, then the data may seem smaller than it should be. To display it you may want to normalize the data - that is, find the maximum value in the resulting spectrum, and scale all values in the array by 1 / max. (ie if the max was 0.5f, then it would become 1).
      To get the spectrum for both channels of a stereo signal, call this function twice, once with channeloffset = 0, and again with channeloffset = 1. Then add the spectrums together and divide by 2 to get the average spectrum for both channels.
      
      //
      FMOD_DSP_FFT_WINDOW_RECT, 
      FMOD_DSP_FFT_WINDOW_TRIANGLE, 
      FMOD_DSP_FFT_WINDOW_HAMMING, 
      FMOD_DSP_FFT_WINDOW_HANNING, 
      FMOD_DSP_FFT_WINDOW_BLACKMAN, 
      FMOD_DSP_FFT_WINDOW_BLACKMANHARRIS,
    */
    
    
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
    if(AUTO_SWITCH_IMAGES == true && imgSwitchTime > SECS_PER_IMAGE ) {
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
    
    //if(buildings->size() > 0 && polygons->size() > 0) {
      pfDisplay();
    //}
    //usleep(5000);	//wtf?
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      
      //if(krach_console_processEvents(event) == 1)
      //  continue;
      
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
					  
					  case SDLK_y:
					    if(mod & KMOD_SHIFT) {
                adjustSpectrum_freqVSamp = !adjustSpectrum_freqVSamp;
                cout << "adjustSpectrum_freqVSamp: " << adjustSpectrum_freqVSamp << endl;
					    } else {
					      cameraSwitchToggle = !cameraSwitchToggle;
                printf("cameraSwitchToggle: %d\n",cameraSwitchToggle);
					    }
            break;
            
            case SDLK_t:
              if(mod & KMOD_CTRL && mod & KMOD_SHIFT) {
                convertSpectrum2logScale = !convertSpectrum2logScale;
                cout << "convertSpectrum2logScale: " << convertSpectrum2logScale << endl;
              } else if(mod & KMOD_CTRL) {
                normalizeaudiospectrum = !normalizeaudiospectrum;
                cout << "normalizeaudiospectrum: " << normalizeaudiospectrum << endl;
              } else if(mod & KMOD_SHIFT) {
                lt_int_shape.clear();
                freePolygonList(*polygons);
                freeBuildingList(*buildings);
                buildings->clear();
              } else {
                fake_osc_input();
              }
              
            break;
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
							this->GenerateFFTGridVertexArray(raster_x, raster_y);
							
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
								this->GenerateFFTGridVertexArray(raster_x, raster_y);
								
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
                //CON_Out(Consoles[0], "muteToggle: %d\n",muteToggle);
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
  exit(0); //for gprof
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
  //myuserflags |= SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_OPENGLBLIT;
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

    /*
    fixme  check out better sampling interpolation modes
      FMOD_DSP_RESAMPLER_NOINTERP
      No interpolation. High frequency aliasing hiss will be audible depending on the sample rate of the sound.

      FMOD_DSP_RESAMPLER_LINEAR
      Linear interpolation (default method). Fast and good quality, causes very slight lowpass effect on low frequency sounds.

      FMOD_DSP_RESAMPLER_CUBIC
      Cubic interoplation. Slower than linear interpolation but better quality.

      FMOD_DSP_RESAMPLER_SPLINE
      5 point spline interoplation. Slowest resampling method but best quality.
    
    */
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
  
  //glEnable(GL_CULL_FACE);
  
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
  Vec3f up(0.0, 1.0, 0.0);
  Vec3f lookatcntr(20.48, -16.00, 0.00);
  Vec3f eye(20.48, -16.00, 37.00);
  float yfov = 45;
  float aspect = 1;
  float near = 1.0f; // near plane distance relative to model diagonal length
  float far = 100.999390; // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr,
                   up, yfov, aspect, near, far);

  
  //if(krach_console_startup() != 0) {
  //  exit(1);
  //}
  
  ///*
  int oscport = OSC_PORT;
	std::cout << "listening for osc input \n";
  
  //if(!develmode_fast_startup) {
    listen_for_osc_packets(oscport);
  //}
  //*/
  
  sdlvu.MyMainLoop();
  //krach_console_shutdown();
	sound->release();
  fmodsystem->close();
  fmodsystem->release();
  return 0;
}
/* yeah 'd' dumps the camera data.
HOLY GRL grail:
Vec3f up(0.0, 1.0, 0.0);
Vec3f lookatcntr(20.48, -16.00, 0.00);
Vec3f eye(20.48, -16.00, 37.00);


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
void MySDLVU::fake_osc_input() {
  srand(time(NULL));
  int x=rand() % (1024 - 0 + 1);
  int y=rand() % (768 - 0 + 1);
  cout << "fake coords: " << x << " " << y << endl;
  lt_int_shape.push_back(x);
  lt_int_shape.push_back(y);
  process_lt_coords();
}

void MySDLVU::fake_init_vertexarrays() {
  lt_int_shape.clear();
  lt_int_shape.push_back(0);
  lt_int_shape.push_back(0);
  int x=0;
  int y=0;
  for(int i=1;i<100;i++) {
    x=i;
    y=768-i;
    lt_int_shape.push_back(x);
    lt_int_shape.push_back(y);
    process_lt_coords();
    lt_int_shape.push_back(0);
    lt_int_shape.push_back(0);
    process_lt_coords();
  }
}

