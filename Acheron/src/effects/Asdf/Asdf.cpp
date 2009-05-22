/*
* Asdf.cpp
*
*  Created on: May 08,2009
*      Author: scriptythekid
*/

#include <opencv/cv.h>
#include <opencv/highgui.h>
//#include <SDL/SDL.h>
//#include <SDL/SDL_thread.h>
#include "iostream"

#include "Asdf.h"
#include "../../misc.h"

#ifdef WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif

using namespace asdfns;
using namespace std;

Asdf::Asdf( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration )
: LinearInterpolatedAnimateable( startPos, endPos, startTick, duration )
{
	this->colors = new HSBColors(3);
	//FIXME
	//set raster and searchdist via timeline params
	raster_x = 32;
  raster_y = 32;
  raster_poly_search_distance = 400;
  
  
  
	//parse filename with opencv
	//store vector of polygons with a vector for each polygons
  
	cvRessourcesGuard = SDL_CreateMutex();
	cvMemStorage = cvCreateMemStorage(0);

	if (SDL_mutexP(cvRessourcesGuard) == -1)
		throw "could not acquire cvRessourcesGuard";

	IplImage *img = cvLoadImage(photoFilename, 3);
	tempBinarizedImage = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
	//get one color channel and set white to zero
	cvSplit(img, tempBinarizedImage, NULL, NULL, NULL);
	cvThreshold(tempBinarizedImage, tempBinarizedImage, 250, 255, CV_THRESH_TOZERO_INV);
	//find polygons
	CvSeq *contours, *polys;

	///*//trace "outlines" only
	cvFindContours(tempBinarizedImage, cvMemStorage, &contours, sizeof(CvContour),
		CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);//*/
	/*//trace all polys
	cvFindContours(tempBinarizedImage, cvMemStorage, &contours, sizeof(CvContour),
	CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);*/
	polys = cvApproxPoly(contours, sizeof(CvContour), cvMemStorage, CV_POLY_APPROX_DP, 1, 1);
	for (; polys; polys = polys->h_next) {
		if (polys->total < 3) continue;
		Polygon* polygon = new Polygon(polys->total);
		Building* building = new Building();
		(*building).poly = new Polygon(polys->total);
		Polygon* bpoly = (*building).poly;
		bool incomplete = false;
		CvPoint *point;

		(*building).orderedVertices = new VertexIndexList();
		(*building).rasterPoints2affect = new Raster2VertexList();

		for (int i=0; i < polys->total; i++) {
			point = (CvPoint*)cvGetSeqElem(polys, i);
			int x, y;
			//x = point->x + fragX*3200;
			//y = point->y + fragY*3200;
			x = point->x;
			y = point->y;
			(*polygon)[i].x = x;
			(*polygon)[i].y = y;
			(*bpoly)[i].x = x;
			(*bpoly)[i].y = y;
			(*bpoly)[i].z = 0;

			if (x == 1 || y == 1 || x == img->width-2 || y == img->height-2)
				incomplete = true;
		}
    //calculate center of poly (center of gravity)
    //http://local.wasp.uwa.edu.au/~pbourke/geometry/polyarea/

     //printf("new poly with %d polys\n",polys->total);

    //mean center ( not center of gravity)
    float tmpX=0;
    float tmpY=0;
    float tmpResult=0;
    for (int i=0; i < polys->total; i++) {
      tmpX +=  (*bpoly)[i].x;
      tmpY +=  (*bpoly)[i].y;
    }
    (*building).fmeanCenterX = tmpX/polys->total;
    (*building).fmeanCenterY = tmpY/polys->total;

    double A=0;
    int p=0;

//area of poly:
    int i,j;
    double area = 0;

    for (i=0;i<polys->total;i++) {
      j = (i + 1) % polys->total;
      area += (*bpoly)[i].x * (*bpoly)[j].y;
      area -= (*bpoly)[i].y * (*bpoly)[j].x;
//      area += ((*bpoly)[i].x * (*bpoly)[j].y) - ((*bpoly)[i].y * (*bpoly)[j].x);
    }

    area /= 2;
    A = (area < 0 ? area * -1 : area );



//centroid:
    double tmpResultx=0;
    double tmpResulty=0;
    p=0;
    for (int i=0; i < polys->total; i++) {
      p = (i + 1) % polys->total;
      tmpResultx += ( (double)(*bpoly)[i].x + (double)(*bpoly)[p].x ) * ( ((double)(*bpoly)[i].x * (double)(*bpoly)[p].y) - ((double)(*bpoly)[p].x * (double)(*bpoly)[i].y) );
      tmpResulty += ( (double)(*bpoly)[i].y + (double)(*bpoly)[p].y ) * ( ((double)(*bpoly)[i].x * (double)(*bpoly)[p].y) - ((double)(*bpoly)[p].x * (double)(*bpoly)[i].y) );
    }

    (*building).dCenterX = (double)( (double)1 / (double)(A * 6.)) * tmpResultx;
    (*building).dCenterY = (double)( (double)1 / (double)(A * 6.)) * tmpResulty;
    (*building).fCenterX = (double)( (double)1 / (double)(A * 6.)) * tmpResultx;
    (*building).fCenterY = (double)( (double)1 / (double)(A * 6.)) * tmpResulty;

//    printf("centerX:%f\n",(*building).fCenterX);

//FIXME
//i have no idea why, but to get centroids for polys inside other polys right, i have to mangle the centroid...
// otherwise it would have inverted x,y coords 
//carmack come save me
    if((*building).fCenterY > 0) {
      (*building).dCenterY *= -1.;
      (*building).fCenterY *= -1.;
    }

    if((*building).fCenterX > 0) {
      (*building).dCenterX *= -1.;
      (*building).fCenterX *= -1.;
    }
		if (!incomplete) {
			buildings.push_back(building);
		}
	}

	//clean up
	cvClearMemStorage(cvMemStorage);
	cvReleaseImage(&img);

	if (SDL_mutexV(cvRessourcesGuard) == -1)
		throw "could not release cvRessourcesGuard";

	tessMissedCounter = 0;
	maxX=FLT_MIN;
  minX=FLT_MAX;
  maxY=FLT_MIN;
  minY=FLT_MAX;
  
  findPlaneMaxima();
	CreateVertexArray();
	
	this->genPoly2RasterFactors(raster_x, raster_y, raster_poly_search_distance);
  this->genSpec2RasterMapping(raster_x, raster_y,0);
}

void Asdf::DrawAtPosition( float* position, float factor, int tick, Context* context) {
  
  this->context = context;
  
  //FIXME
  //add if for modify polys yes/no
  BuildingList::const_iterator build, buildend;
  build = buildings.begin();
	buildend = buildings.end();
  float s = 0;
  int n = 0;
  int p=6;//coords are inverted compared to augenkrach. floor coords are roof coords here... oh lol. happy inverting everything
  int r=0;
	///*
	for (;build != buildend; build++) {
    s = GetSpecValByBuilding(**build, 5.0f);
    //cout << "specforBuilding: " << s << endl;
    Polygon *blah = (*build)->poly;
    for (int i=(*blah).size()-1; i >= 0; i--) {
      n=i-1;
      i=i<0?(*blah).size()-1:i;
      //myVertexArray[p+2] = 5.0f + s*5 *-1;
      //myVertexArray[p+2+3] = 5.0f + s*5 *-1;
      myVertexArray[p+2] = 5.0f + s*15 *-1;
      myVertexArray[p+2+3] = 5.0f + s*15 *-1;
      p+=12;
    }
    
    VertexIndexList *bleh = (*build)->orderedVertices;  
    for (int i=0;i<(*bleh).size();i++) {
      //UARGH FIXME
      //r is running beyond array size?  (ooold note :) )
      myRoofsVertexArray[r+2] = 5.0f + s*15 *-1;
      r += 3;
    } 
  }
	///*
	glPushMatrix();
	glTranslatef( position[0], position[1], position[2] );
	glRotatef( 90.0f, 1,0,0 );


  //debug:
  //DrawFFTGrid(raster_x, raster_y, 0);
  /*
  //colors->update();
  build = buildings.begin();
	buildend = buildings.end();
  for (;build != buildend; build++) {
    DrawCentroid2GridLines(**build, 0, 0, raster_x, raster_y);
    DrawCentroid(**build, 100); //slow debug func
  }
  //*/
  
	//glDisable(GL_CULL_FACE);
	///*
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, myRoofsVertexArray);
	glNormalPointer(GL_FLOAT, 0, myRoofsNormalsArray);
	glDrawArrays(GL_TRIANGLES, 0, roofsVertexVector.size()/3);

	glVertexPointer(3, GL_FLOAT, 0, myVertexArray);
	glNormalPointer(GL_FLOAT, 0, myNormalsArray);
	glDrawArrays(GL_QUADS, 0, tmpVertexVector.size()/3);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	//*/
	//glEnable(GL_CULL_FACE);

	glPopMatrix();

}

void Asdf::CreateVertexArray() {
	GLfloat v1[3];
	GLfloat v2[3];
	GLfloat v3[3];
	GLfloat v4[3];
	GLfloat normal[3];

	BuildingList::const_iterator build, buildend;
	build = buildings.begin();
	buildend = buildings.end();
	for (;build != buildend; build++) {
		Polygon *poly = (Polygon*) (*build)->poly;

		for (int i=(*poly).size()-1, n; i >= 0; i--) {
			n = i-1;
			if (n < 0)
				n = (*poly).size()-1;

			v1[0] = (float)(*poly)[i].x;
			v1[1] = (float)-(*poly)[i].y;
			v1[2] = 5.0f;
			v2[0] = (float)(*poly)[n].x;
			v2[1] = (float)-(*poly)[n].y;
			v2[2] = 5.0f;
			v3[0] = (float)(*poly)[n].x;
			v3[1] = (float)-(*poly)[n].y;
			v3[2] = 0.0f;
			v4[0] = (float)(*poly)[i].x;
			v4[1] = (float)-(*poly)[i].y;
			v4[2] = 0.0f;

			tmpVertexVector.push_back(v1[0]);
			tmpVertexVector.push_back(v1[1]);
			tmpVertexVector.push_back(v1[2]);
			tmpVertexVector.push_back(v2[0]);
			tmpVertexVector.push_back(v2[1]);
			tmpVertexVector.push_back(v2[2]);
			tmpVertexVector.push_back(v3[0]);
			tmpVertexVector.push_back(v3[1]);
			tmpVertexVector.push_back(v3[2]);
			tmpVertexVector.push_back(v4[0]);
			tmpVertexVector.push_back(v4[1]);
			tmpVertexVector.push_back(v4[2]);

			calcNV( v1, v2, v3, normal);
			//cout << "normal: " << normal[0] << ", " << normal[1] << ", " << normal[2] << endl;
			tmpNormalsVector.push_back(normal[0]);
			tmpNormalsVector.push_back(normal[1]);
			tmpNormalsVector.push_back(normal[2]);
			tmpNormalsVector.push_back(normal[0]);
			tmpNormalsVector.push_back(normal[1]);
			tmpNormalsVector.push_back(normal[2]);
			tmpNormalsVector.push_back(normal[0]);
			tmpNormalsVector.push_back(normal[1]);
			tmpNormalsVector.push_back(normal[2]);
			tmpNormalsVector.push_back(normal[0]);
			tmpNormalsVector.push_back(normal[1]);
			tmpNormalsVector.push_back(normal[2]);
		}
		TessellateBuilding(**build);
	}

	//cout << "tessMissedCounter: " << tessMissedCounter << endl;

	myVertexArray = new GLfloat[tmpVertexVector.size()];
	copy(tmpVertexVector.begin(), tmpVertexVector.end(), myVertexArray);
	myNormalsArray = new GLfloat[tmpNormalsVector.size()];
	copy(tmpNormalsVector.begin(), tmpNormalsVector.end(), myNormalsArray);

	myRoofsVertexArray = new GLfloat[roofsVertexVector.size()];
	copy(roofsVertexVector.begin(), roofsVertexVector.end(), myRoofsVertexArray);
	myRoofsNormalsArray = new GLfloat[roofsNormalsVector.size()];
	copy(roofsNormalsVector.begin(), roofsNormalsVector.end(), myRoofsNormalsArray);

	//cout << tmpVertexVector.size() << " Vertices, " << tmpNormalsVector.size() << " Normals" << endl;
	//cout << roofsVertexVector.size() << " roofVertices, " << roofsNormalsVector.size() << " roofNormals" << endl;
}


//tesselation:
typedef struct {
	const asdfns::Polygon *poly;
	Building *building;
	int vIndex;
	int polySize;
} tessUserData;

//    non class functions:
void mytessError(GLenum err, void *)
{
	gluErrorString(err);
}

void STDCALL mytessBegin(GLenum type, void *user_data) {
}

void STDCALL mytessEdgeFlagCB(GLboolean flag ) {
}


void STDCALL tessVcbd(void *v, void *user_data) { //USE WITH GLU_TESS_VERTEX_DATA
	tessUserData *tud = (tessUserData*) user_data;
	Building *building = (Building*) tud->building;
	VertexIndexList* vil = (*building).orderedVertices;
	int vertexIndex = (int)((GLdouble*)v)[3];
	(*vil).push_back(vertexIndex);
}


void Asdf::TessellateBuilding(Building &building) {
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
	GLdouble *tmppd = (GLdouble*) malloc(sizeof(GLdouble) * (*poly).size()*4);

	for (int i=(*poly).size()-1; i >= 0; i--) {
		tmppd[4*i] =  (*poly)[i].x;
		tmppd[4*i+1] = -(*poly)[i].y;
		tmppd[4*i+2] =  0.0f;
		tmppd[4*i+3] =  i;
	}

	gluTessCallback(tess,  GLU_TESS_BEGIN_DATA, (void(STDCALL *)())&mytessBegin);
	gluTessCallback(tess,  GLU_TESS_EDGE_FLAG, (void(STDCALL *)())&mytessEdgeFlagCB); //forces generation of GL_TRIANGLES only  yeah!
	gluTessCallback(tess, GLU_TESS_VERTEX_DATA,	(void (STDCALL *)())&tessVcbd);

	tessUserData polyData;
	polyData.poly = poly;
	polyData.building = &building;
	polyData.vIndex = 0;
	polyData.polySize = poly->size();

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

	VertexIndexList *verticesOrder = building.orderedVertices;

	//for (int i=(*verticesOrder).size()-1; i >= 0; i--) {
	//for some strange reason i have to reverse the vertex order
	//maybe because my coords r fucked up. roofs are at z/height 0, "floors" are at z/height 5
	for (unsigned int i=0;  i < (*verticesOrder).size(); i++) {
		roofsVertexVector.push_back((float)(*poly)[(*verticesOrder)[i]].x);
		roofsVertexVector.push_back((float)-(*poly)[(*verticesOrder)[i]].y);
		roofsVertexVector.push_back(0.0f);

		roofsNormalsVector.push_back(0);
		roofsNormalsVector.push_back(0);
		roofsNormalsVector.push_back(-1);
		//cout << "normal: " << normal[0] << ", " << normal[1] << ", " << normal[2] << endl;
	}
}


/* search in a bounding box, near the centroid of the building, instead of the whole grid:
  lightyears faster!
*/
void Asdf::genPoly2RasterFactors(int rasterX, int rasterY, float maxDistance) {

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
    build = buildings.begin();
    buildend = buildings.end();
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


void Asdf::genSpec2RasterMapping(int rasterX, int rasterY, int specStartIndex) {
  int rasterSize = rasterX*rasterY;
  float rasterIterator = (float)rasterSize / (float)SPECTRUM_BANDS;

  printf("rasterIterator: %f\n",rasterIterator);
  
  //if(this->spec2raster != NULL )
    delete[] this->spec2raster;
  
  this->spec2raster = new int[rasterSize];

  if(specStartIndex > SPECTRUM_BANDS || specStartIndex < 0) {
    specStartIndex = 0;
  }
  int specIndex = specStartIndex;


  //init
  for(int i=0;i < rasterSize; i++) {
      this->spec2raster[i] = specIndex; //=0
  }

  int c=0;
  int specAdd = -1;
  //map spectrum to the whole raster:
  for(int i=0;i < rasterX; i++) {
    for(int q=0;q < rasterY; q++) {
        int p = i * rasterY + q;
        this->spec2raster[p] = specIndex;
        specIndex++;
      c++;
    }
  }

}

float Asdf::GetSpecValByBuilding(Building &building, float height) {
  //return 0.7f;
  ///*
  float specVar = 0;
  //tmp:
  spectrumPointer = context->getAudio()->getSpectrum( &spectrumSize );
  //cout << "spectrum 150: " << spectrumPointer[150] << endl;
  //Building *build = *building;
  Polygon *poly = (Polygon*) building.poly;
  if ((*poly).size() < 3) return 0;

  Raster2VertexList *rasterPointList = building.rasterPoints2affect;
  VertexIndexList *verticesOrder = building.orderedVertices;
  //cout << "rasterpointlist: " << (*rasterPointList).size() << endl;
  float specDistHeightFactor = 0;
  for (int i=(*rasterPointList).size()-1, n; i >= 0; i--) {
    int rp = (*rasterPointList)[i].rasterpoint;
    float dist = (*rasterPointList)[i].distance;
    //specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * (dist);         //rvf.rasterpoint = (x*rasterX)+(y);

    //use average spectrum var of all rasterpoints for this building:
    //specDistHeightFactor += MySpectrum[this->spec2raster[rp]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[rp]+1)) ) * dist / 200;         //rvf.rasterpoint = (x*rasterX)+(y);

    //NO IDEA how to really calc the right specvar .... 
    
    //use maximum spectrum var of all rasterpoints for this building
    //float tmpspecv = spectrumPointer[this->spec2raster[rp]] * ((16000/SPECTRUM_BANDS * (this->spec2raster[rp]+1)) ) * dist / 200;
    float tmpspecv = spectrumPointer[this->spec2raster[rp]]  * dist / 200;
    //float tmpspecv = spectrumPointer[this->spec2raster[rp]];
    //cout << "specAtRP: " << spectrumPointer[this->spec2raster[rp]] << " tmpspecv: " << tmpspecv << endl;
    specVar = tmpspecv > specVar ? tmpspecv : specVar;
  }
  //use average spectrum var of all rasterpoints for this building:
  //specVar = (specDistHeightFactor/(*rasterPointList).size());
  
  return specVar;//*/
}

void Asdf::findPolyMaxima(const Polygon &poly) {
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

void Asdf::findPlaneMaxima() {
  BuildingList::const_iterator build, buildend;
  
	PolygonList::const_iterator poly, polyend;
	build = buildings.begin();
  buildend = buildings.end();
	for (;build != buildend; build++) {
		Polygon *blah = (*build)->poly;
		findPolyMaxima(*blah);
	}
	printf("minX: %f\n",minX);
	printf("maxX: %f\n",maxX);
	printf("minY: %f\n",minY);
	printf("maxY: %f\n",maxY);
}

void Asdf::DrawFFTGrid(int rasterX, int rasterY, int rasterZ=0) {
//  int maxX = this->map.fragmentImageWidth;
//  int maxY = this->map.fragmentImageHeight;
  spectrumPointer = context->getAudio()->getSpectrum( &spectrumSize );
  
  int maxX = this->maxX;
  int maxY = this->maxY;

  int spacingX = maxX / rasterX;
  int spacingY = maxY / rasterY;

  glBegin(GL_LINES);

  for(int x = 0; x < rasterX; x++) {

      for(int y = 0; y < rasterY; y++) {
        //float specHeight = MySpectrum[this->spec2raster[x*rasterY+y]]*4000;
        //float specHeight = MySpectrum[this->spec2raster[x*rasterY+y]] * ((16000/SPECTRUM_LENGTH * (this->spec2raster[x*rasterY+y]))) * 50;
      	float specHeight = spectrumPointer[this->spec2raster[x*rasterY+y]] * ((16000/SPECTRUM_BANDS * (this->spec2raster[x*rasterY+y]))) * 50;
        //float specHeight = 0;
        glColor3f(0.0,  0.86*spectrumPointer[this->spec2raster[x*rasterY+y]]*10,  0.8*spectrumPointer[this->spec2raster[x*rasterY+y]]*10);
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

void Asdf::DrawCentroid(Building &building, float height) {
  Polygon *poly = (Polygon*) building.poly;
  if ((*poly).size() < 3) return;

	glBegin(GL_LINES);
  //glColor3f(0.,  1,  50/65384);
  /*
  FIXME   why do i have to invert the coords here? in drawpoly its +x, -y  here i have to do -x, +y  to make it look(!) correct. maybe computation in map.cpp is still wrong
  */
  if(-building.fCenterX < 0) {
    //glColor3f(0.,  1,  0.8);
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

void Asdf::DrawCentroid2GridLines(Building &building, float height, float gridHeight, int rasterX, int rasterY) {
  spectrumPointer = context->getAudio()->getSpectrum( &spectrumSize );

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
    glVertex3f( -building.fCenterX, building.fCenterY, 0+spectrumPointer[this->spec2raster[(*rasterList)[i].rasterpoint]+1]* ((16000/SPECTRUM_BANDS * (x*rasterY+y+1))) * 50);
    glColor3f(0.023,  0.3*(1/dist),  0.2);
    glVertex3f( spacingX*x, -spacingY*y, gridHeight);
  }
  glEnd();
}



