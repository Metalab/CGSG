/*

  adjust visualized fft window size and move it around with up,down,q,a, skip ahead with left, right
  
  
*/

#include <sdlvu.h> // will also include GL headers
#include "map.h"

#include <vector>

//#include "vorbis_fft.h"

#include <stdio.h>
#include <iostream>

#include <limits.h>
#include <float.h>

#include <fmod.hpp>
#include <fmod_errors.h>


using namespace std;

const int SPECTRUM_LENGTH = 4096;
float MySpectrum[SPECTRUM_LENGTH];
const int SPEC_DEPTH = 100;


FMOD::System *fmodsystem;
FMOD::Sound *sound;
FMOD::Channel *channel;

class SpecBuffer {
	GLfloat buffer[SPEC_DEPTH * SPECTRUM_LENGTH];
	int readPos;
	int writePos;
	int readFrame;
	int writeFrame;
	GLfloat internalReadBuffer;
	
	public:
		SpecBuffer();
		void insert(GLfloat);
		GLfloat read();
		void resetWritePos();
		void resetReadPos();
		void resetReadFrame();
		void resetWriteFrame();
		void nextReadFrame();
		void previousReadFrame();
		void nextWriteFrame();
    
    void insertFullSpectrum(float *spectrumP);
	private:
};

SpecBuffer::SpecBuffer() {
	readFrame = 0;
	writeFrame = 0;
	readPos = 0;
	writePos = 0;
	for( int i = 0; i < (SPEC_DEPTH*SPECTRUM_LENGTH); i++) {
		buffer[i] = 0.0;
	}
}

void SpecBuffer::insert(GLfloat specValue) {
	buffer[writeFrame * SPECTRUM_LENGTH + writePos] = specValue;
	writePos = (++writePos)%SPECTRUM_LENGTH;
}

void SpecBuffer::insertFullSpectrum(float *spectrumP) {
  memcpy((void*)&buffer[writeFrame * SPECTRUM_LENGTH], (void*)spectrumP, SPECTRUM_LENGTH);
  this->nextWriteFrame();
	//buffer[writeFrame * SPECTRUM_LENGTH] = specValue;
	//writePos = (++writePos)%SPECTRUM_LENGTH;
}

GLfloat SpecBuffer::read() {
	internalReadBuffer = buffer[ readFrame * SPECTRUM_LENGTH + readPos] ;
	readPos = (++readPos)%(SPECTRUM_LENGTH);
	return internalReadBuffer;
}

void SpecBuffer::resetWritePos() {
	writePos = 0;
}

void SpecBuffer::resetReadPos() {
	readPos = 0;
}

void SpecBuffer::nextReadFrame() {
	readFrame = (++readFrame)%SPEC_DEPTH;
}

void SpecBuffer::previousReadFrame() {
	readFrame = (--readFrame)%SPEC_DEPTH;
}

void SpecBuffer::nextWriteFrame() {
	writeFrame = (++writeFrame)%SPEC_DEPTH;
}

void SpecBuffer::resetReadFrame() {
	readFrame = 0;
}

void SpecBuffer::resetWriteFrame() {
	writeFrame = 0;
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
    
    SpecBuffer specRingBuffer;
    
    virtual void Motion(const SDL_MouseMotionEvent & event);
    bool muteToggle;
    bool motionToggleModifier;
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
    
    void DrawGrid(int rasterX, int rasterY, int rasterZ);
    void DrawFFTGrid(int rasterX, int rasterY, int rasterZ);
    void DrawCentroid2GridLines(Building &building, float height, float gridHeight, int rasterX, int rasterY);
    /*
    float spectrumHistory[SPEC_HISTORY_SIZE][SPECTRUM_LENGTH];
    float *rbReadPos;
    float *rbWritePos;
    int historySpecIndex;
    void setupSpectrumHistory();
    void rememberSpectrum(void *Spectrum,int size);
    float* readHistorySpectrum(int offset);
    */
};

void tessVcbd(void *v, void *user_data);

MySDLVU::MySDLVU() {
  polygons = &map.getPolygonsOfFragment(0,0);
  if( map.hasFragment(0,0) ) printf("fragment 0,0 loaded\n");
  buildings = &map.getBuildingsOfFragment(0,0);
  printf("have %d polys\n", polygons->size());

  printf("have %d buildings\n", buildings->size());
	//zHistory = (float*) malloc(polygons->size()*sizeof(float));
	maxX=FLT_MIN;
	minX=FLT_MAX;
	maxY=FLT_MIN;
	minY=FLT_MAX;
  
  spectrumDivisor = 8;
  specStartIndex = 0;
  //setupSpectrumHistory();
  
  muteToggle = false;
  motionToggleModifier=false;
}


void MySDLVU::DrawBuildingByRasterSpectrum(Building &building, float height) {
  float specVar = 0;
  
  //Building *build = *building;
  Polygon *poly = (Polygon*) building.poly;
  if ((*poly).size() < 3) return;
  
  Raster2VertexList *rasterPointList = building.rasterPoints2affect;
  VertexIndexList *verticesOrder = building.orderedVertices;
	
  glBegin(GL_TRIANGLES);
  //glColor3f(0.,  1,  0);
  glColor3f(1.,  (specVar*2000)/16384,  (specVar*2000)/65384);

  float specDistHeightFactor = 0;
  for (int i=(*rasterPointList).size()-1, n; i >= 0; i--) {
    int rp = (*rasterPointList)[i].rasterpoint;
    float dist = (*rasterPointList)[i].distance;
    specDistHeightFactor +=1;
    //printf("rp: %d s2rRP: %d spec:%f\t", rp, this->spec2raster[rp], MySpectrum[this->spec2raster[rp]]);
    //specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * (1/dist);         //rvf.rasterpoint = (x*rasterX)+(y);
    specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * dist;         //rvf.rasterpoint = (x*rasterX)+(y);
  }
  specVar = (specDistHeightFactor/(*rasterPointList).size());
	
  //DrawPoly(*poly, 80, specVar*100);
  DrawRoofTesselated(building, height, specVar*100);
  ///*
//	glColor3f(1., 0, (GLfloat)MySpectrum[spectrumIndex]);
  glBegin(GL_QUADS);

  for (int i=(*poly).size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = (*poly).size()-1;
    glColor3f(1.,  (specVar*20)/16384,  (specVar*20)/65384);
		
    glVertex3f((*poly)[i].x, -(*poly)[i].y, height+(specVar*100));
    glVertex3f((*poly)[n].x, -(*poly)[n].y, height+(specVar*100));

		//farbe "unten":
    glColor3f(0,0,0);
		
    glVertex3f((*poly)[n].x, -(*poly)[n].y, 0.);    
    glVertex3f((*poly)[i].x, -(*poly)[i].y, 0.);
		
  }

  glEnd();
  //*/
}


void MySDLVU::genSpec2RasterMapping(int rasterX, int rasterY, int specStartIndex) {
  int rasterSize = rasterX*rasterY;
  float rasterIterator = (float)rasterSize / (float)SPECTRUM_LENGTH;
  
  //printf("rasterIterator: %d\n",rasterIterator);
  
  this->spec2raster = new int[rasterSize];
  
  if(specStartIndex > SPECTRUM_LENGTH || specStartIndex < 0) {
    specStartIndex = 0;
  }
  int specIndex = specStartIndex;
  
  //init
  for(int i=0;i < rasterSize; i++) {
      this->spec2raster[i] = specIndex;
      //specIndex++;
  }
  
  int c=0;
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
      c++;
    }
//    specIndex++;
    //if( i % rasterIterator == 0) {
    //  printf("i:%d\n",i);
    //  specIndex++;
    //}
  }
  
  for(int i=0;i < rasterSize; i++) {
      //printf("s2r%d:\t%d\t", i, this->spec2raster[i]);
    }
  
  
  //printf("Last specIndex:%d\n",specIndex);
  //printf("Last c:%d\n",c);
  //exit(1);
  
  //map spectrum to n rows of spectrum:
}


//spectrumsize is also used as "window" size 
void MySDLVU::genSpec2PolyMapping(int polyCount, size_t spectrumsize, int specStartIndex) {
	
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
	
	//printf("genSpec2Poly end\n");
}

/*
void MySDLVU::setupSpectrumHistory() {
  rbReadPos = spectrumHistory[0];
  rbWritePos = spectrumHistory[0];
  historySpecIndex=0;
}


void MySDLVU::rememberSpectrum(void *Spectrum,int size) {
  //add newest spectrum(MySpectrum)
  memcpy(rbWritePos,Spectrum,SPECTRUM_LENGTH);//tgt, data

  if(rbWritePos == (float*)&spectrumHistory+((SPEC_HISTORY_SIZE-1)*SPECTRUM_LENGTH)) {
    rbWritePos = (float*)&spectrumHistory[0];
    historySpecIndex=0;
  } else {
    rbWritePos = (float*)&spectrumHistory+(SPECTRUM_LENGTH*historySpecIndex);
    historySpecIndex++;
  }
}

float* MySDLVU::readHistorySpectrum(int offset) {
  
  return spectrumHistory[offset];
}
*/

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
    glColor3f(1/dist,  1/dist,  1);
    glVertex3f( -building.fCenterX, building.fCenterY, 0);
    glColor3f(0.0,  0.,  0.); 
    glVertex3f( spacingX*x, -spacingY*y, gridHeight);
  }
  glEnd();
  //glColor3f(1/dist,  1/dist,  1);
  //glVertex3f( -building->fCenterX, building->fCenterY, 0);
  //glColor3f(0.0,  0.,  0.); 
  //glVertex3f( spacingX*x, -spacingY*y, 0);
}


void MySDLVU::DrawFFTGrid(int rasterX, int rasterY, int rasterZ=0) {
  int maxX = this->map.fragmentImageWidth;
  int maxY = this->map.fragmentImageHeight;
  
  int spacingX = maxX / rasterX;
  int spacingY = maxY / rasterY;

  glBegin(GL_LINES);
  
  for(int x = 0; x < rasterX; x++) {
  
      for(int y = 0; y < rasterY; y++) {
        //float specHeight = MySpectrum[x*rasterY+y]*4000;
        float specHeight = 0;
        glColor3f(0.0,  0.86*MySpectrum[x*rasterY+y],  0.8*MySpectrum[x*rasterY+y]);
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

//50,50 for 1000 polys?(3200px*3200px at 500 extentwidth)
void MySDLVU::genPoly2RasterFactors(int rasterX, int rasterY, float maxDistance) {
    
    //for raster draw lines
    
    int maxX = this->map.fragmentImageWidth;
    int maxY = this->map.fragmentImageHeight;
    int rasterZ = 0;
    //glBegin(GL_LINES);
    //glColor3f(0.0,  0.86,  0.8); 
    int spacingX = maxX / rasterX;
    int spacingY = maxY / rasterY;
    
    /*
    for(int x = 0; x <= rasterX; x++) {
        glVertex3f( spacingX*x, 0, rasterZ);
        glVertex3f( spacingX*x, -maxY, rasterZ);
    }


    
    for(int y = 0; y <= rasterY; y++) {
        glVertex3f( 0, -spacingY*y, rasterZ);
        glVertex3f( maxX, -spacingY*y, rasterZ);
    }
    //*/
    
    
    
    //for buildings
    ///*
    int minRP, maxRP = 0;
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
      
        //glVertex3f( -building->fCenterX, building->fCenterY, 0);
        //glVertex3f( -building->fCenterX, building->fCenterY, rasterZ);
        for(int x=0; x < rasterX; x++) {
          for(int y=0; y < rasterY; y++) {
            dist = sqrt(pow(spacingX*x - -1*building->fCenterX,2) + pow(-spacingY*y - building->fCenterY,2) );
            if(dist <= maxDistance) {
              //glColor3f(1/dist,  1/dist,  1);
              //glVertex3f( -building->fCenterX, building->fCenterY, 0);
              //glColor3f(0.0,  0.,  0.); 
              //glVertex3f( spacingX*x, -spacingY*y, 0);
              //save the raster point for this the building
              Raster2VertexFactor rvf;
//              rvf.rasterpoint = x*y;//FIXME  shouldnt it be x*rasterX + y  -1

              rvf.rasterpoint = x * rasterY + y;
              maxRP = rvf.rasterpoint > maxRP ? rvf.rasterpoint : maxRP;
              minRP = rvf.rasterpoint < minRP ? rvf.rasterpoint : minRP;
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
	
  for (;poly != polyend; poly++) {
      
			//DrawPoly(**poly, 50, MySpectrum[this->poly2spec[p]]);
      
      //DrawPoly(**poly, 50, specRingBuffer.read());
      //if(p%30 < 1) {
      //  specRingBuffer.nextReadFrame();
      //}
		p++;
  }
///*
  p=0;
	BuildingList::const_iterator build, buildend;
	//PolygonList::const_iterator poly, polyend;
	build = buildings->begin();
	buildend = buildings->end();
  
  DrawGrid(20,20,-200);
  DrawFFTGrid(20,20,200);
	for (;build != buildend; build++) {
      DrawCentroid(**build, 100);
      //DrawMeanCenter(**build, 100);
      //DrawRoofTesselated(**build, 50, MySpectrum[this->poly2spec[p]]);
      DrawCentroid2GridLines(**build, 0, -200, 20, 20);
      DrawBuildingByRasterSpectrum(**build, 50);
      p++;
	}
  
//  genPoly2RasterFactors(20, 20, 200);
  
  glPopMatrix();
//*/	
	//DrawFloor();
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
    glColor3f(1.,  (specVar*2000)/16384,  (specVar*2000)/65384);
		
//		glVertex3f(poly[i].x, -poly[i].y, height+(specVar/2));
//    glVertex3f(poly[n].x, -poly[n].y, height+(specVar/2));
    glVertex3f(poly[i].x, -poly[i].y, height+(specVar*3000));
    glVertex3f(poly[n].x, -poly[n].y, height+(specVar*3000));
//    glVertex3f(poly[i].x, -poly[i].y, height);
//    glVertex3f(poly[n].x, -poly[n].y, height);

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
  
  this->genPoly2RasterFactors(20, 20, 200);
  this->genSpec2RasterMapping(20,20,0);
  
	while (!done) {
		channel->getSpectrum(MySpectrum, SPECTRUM_LENGTH, 0, FMOD_DSP_FFT_WINDOW_TRIANGLE);
    specRingBuffer.insertFullSpectrum(MySpectrum);
    
    //rememberSpectrum(MySpectrum,SPECTRUM_LENGTH);
		//channel->getSpectrum(MySpectrum, 4096, 0, FMOD_DSP_FFT_WINDOW_HAMMING);
    
    //channel->setPosition(hopToPosition, FMOD_TIMEUNIT_MS);
    channel->getPosition(&soundPosition,  FMOD_TIMEUNIT_MS);
    //printf("%ud\n",soundPosition);
    
    
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
				pfKeyboard(event.key);
        break;
      case SDL_KEYUP:
        
					switch( event.key.keysym.sym ){
						case SDLK_RIGHT:
              hopToPosition = soundPosition + 1000;
              channel->setPosition(hopToPosition, FMOD_TIMEUNIT_MS);
						break;
						case SDLK_LEFT:
              hopToPosition = soundPosition - 1000;
              channel->setPosition(hopToPosition, FMOD_TIMEUNIT_MS);
						break;
						
            case SDLK_q:
                specStartIndex += specStartIndex < SPECTRUM_LENGTH ? 1 : 0;
                printf("specStartIndex: %d\n",specStartIndex);
                genSpec2PolyMapping(this->polycount, SPECTRUM_LENGTH/spectrumDivisor, specStartIndex);
						break;
            case SDLK_a:
                specStartIndex -= specStartIndex > 1 ? 1 : 0;
                printf("specStartIndex: %d\n",specStartIndex);
                genSpec2PolyMapping(this->polycount, SPECTRUM_LENGTH/spectrumDivisor, specStartIndex);
						break;
            
            
						case SDLK_UP:
                spectrumDivisor -= spectrumDivisor > 1 ? 1 : 0;
                printf("specdivisor: %d\n",spectrumDivisor);
                genSpec2PolyMapping(this->polycount, SPECTRUM_LENGTH/spectrumDivisor, specStartIndex);
						break;
						case SDLK_DOWN:
                spectrumDivisor += 1;
                printf("specdivisor: %d\n",spectrumDivisor);
                genSpec2PolyMapping(this->polycount, SPECTRUM_LENGTH/spectrumDivisor, specStartIndex);
						break;
            
            case SDLK_u:
                muteToggle = !muteToggle;
                channel->setMute(muteToggle);
						break;
            
            case SDLK_e:
                motionToggleModifier = !motionToggleModifier;
                printf("motionToggleModifier: %d\n",motionToggleModifier);
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


MySDLVU sdlvu;



int main(int argc, char *argv[])
{
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

  //if we are on linux, set output type to alsa
  #ifdef __LINUX__
  frc = fmodsystem->setOutput(FMOD_OUTPUTTYPE_ALSA);
  if (frc != FMOD_OK) {
    printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
    exit(1);
  }

  #endif
  frc = fmodsystem->init(1, FMOD_INIT_NORMAL, 0);
  if (frc != FMOD_OK) {
    printf("FMOD error! (%d) %s \n", frc, FMOD_ErrorString(frc));
    exit(1);
  }
	
  sdlvu.Init("SDLVU Basic Object Oriented Example",
             50, 50, 800, 600);

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
  Vec3f up(0.979, 0.189, 0.078);
  Vec3f lookatcntr(3.812, -10.799, 53.023);
  Vec3f eye(3.730, -10.786, 54.019);
  float yfov = 45;
  float aspect = 1;    
  float near = 0.1f; // near plane distance relative to model diagonal length
  float far = 299.999725; // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr, 
                   up, yfov, aspect, near, far);


  sdlvu.MyMainLoop();
	sound->release();
  fmodsystem->close();
  fmodsystem->release();	
  return 0;
}
/* yeah 'd' dumps the camera data. 

      Eye: (3.730, -10.786, 54.019)
LookAtCntr: (3.812, -10.799, 53.023)
    ViewUp: (0.979, 0.189, 0.078)
     Y FOV: 44.999557
    Aspect: 1.333332
      Near: 0.300000
       Far: 299.999725

*/