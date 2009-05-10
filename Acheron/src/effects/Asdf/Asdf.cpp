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


using namespace asdfns;
using namespace std;

Asdf::Asdf( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration )
  : LinearInterpolatedAnimateable( startPos, endPos, startTick, duration )
{
  //parse filename with opencv
  //store vector of polygons with a vector for each polygons
  //
  
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
  
  /*//trace "outlines" only
  cvFindContours(tempBinarizedImage, cvMemStorage, &contours, sizeof(CvContour),
                  CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);*/
  //trace all polys
  cvFindContours(tempBinarizedImage, cvMemStorage, &contours, sizeof(CvContour),
                  CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
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
    VertexIndexList* vil = (*building).orderedVertices;
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
    
    if (!incomplete) {
      buildings.push_back(building);
    }
  }
  
  //clean up
  cvClearMemStorage(cvMemStorage);
  cvReleaseImage(&img);

  if (SDL_mutexV(cvRessourcesGuard) == -1)
    throw "could not release cvRessourcesGuard";
    
  
}

void Asdf::DrawAtPosition( float* position, float factor, int tick ) {
  BuildingList::const_iterator build, buildend;
	//PolygonList::const_iterator poly, polyend;
	build = buildings.begin();
	buildend = buildings.end();
	for (;build != buildend; build++) {
	  Polygon *poly = (Polygon*) (*build)->poly;
    glBegin(GL_LINES);
    for (int i=(*poly).size()-1, n; i >= 0; i--) {
      n = i-1;
      if (n < 0)
        n = (*poly).size()-1;
      glVertex3f((*poly)[i].x, -(*poly)[i].y, 50.0f);
      glVertex3f((*poly)[n].x, -(*poly)[n].y, 50.0f);
      glVertex3f((*poly)[n].x, -(*poly)[n].y, 0.0f);
      glVertex3f((*poly)[i].x, -(*poly)[i].y, 0.0f);
    }
    glEnd();
	}
}