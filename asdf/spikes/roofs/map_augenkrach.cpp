#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include "map_augenkrach.h"

//for the file/dir listing stuff:
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <dirent.h>
#include <string>

using namespace std;

ViennaMap::ViennaMap() {
  cvRessourcesGuard = SDL_CreateMutex();
  fragmentGuard = SDL_CreateMutex();
  cvMemStorage = cvCreateMemStorage(0);
  tempBinarizedImage = cvCreateImage(cvSize(fragmentImageWidth, fragmentImageHeight), IPL_DEPTH_8U, 1);
  
}

ViennaMap::ViennaMap(char *imageDirectory) {
  cvRessourcesGuard = SDL_CreateMutex();
  fragmentGuard = SDL_CreateMutex();
  cvMemStorage = cvCreateMemStorage(0);
  
  this->PNGimageDirectory = imageDirectory;
  this->NumberOfImageFiles = getNoOfPNGfiles((char*)imageDirectory);
  this->getPNGfiles((char*)imageDirectory);
  
 // tempBinarizedImage = cvCreateImage(cvSize(fragmentImageWidth, fragmentImageHeight), IPL_DEPTH_8U, 1);
  
}

void freePolygonList(PolygonList& list) {
  PolygonList::iterator begin = list.begin(), end=list.end();
  for(; begin != end; begin++)
    delete *begin;

  list.clear();
}

void freeBuildingList(BuildingList& list) {
  BuildingList::iterator begin = list.begin(), end=list.end();
  for(; begin != end; begin++)
    delete *begin;

  list.clear();
}

ViennaMap::~ViennaMap() {
  SDL_mutex *mut = cvRessourcesGuard;

  //free cv ressources and their guard
  if (SDL_mutexP(mut) == -1) 
    throw "could not acquire cvRessourcesGuard to destruct ViennaMap";

  cvRessourcesGuard = 0;
  cvReleaseMemStorage(&cvMemStorage);
  cvReleaseImage(&tempBinarizedImage);
  

  if (SDL_mutexV(mut) == -1) 
    throw "could not release cvRessourcesGuard after freeing cv ressources";
  SDL_DestroyMutex(mut);

  //free fragments and their guard
  mut = fragmentGuard;
  if (SDL_mutexP(mut) == -1)
    throw "could not acquire fragment guard to destruct ViennaMap";

  fragmentGuard = 0;
  std::vector<MapFragment*>::iterator it=fragments.begin(), end = fragments.end();
  for(;it != end; it++)
    if (*it != NULL) {
      freePolygonList((*it)->polygons);
      freePolygonList((*it)->incompletePolygons);
      freeBuildingList((*it)->buildings);
      freeBuildingList((*it)->incompleteBuildings);
      delete *it;
      *it = NULL;
    }

  fragments.clear();

  if (SDL_mutexV(mut) == -1)
    throw "could not release fragmentGuard after freeing fragments";
  
}

int ViennaMap::getNoOfPNGfiles(char* directory) {
  DIR *dirp;
  struct dirent *entry;
  int num = 0;
  if(dirp = opendir(directory))
  {
    while(entry = readdir(dirp)) {
      if(entry->d_type == 8) {
        string str = entry->d_name;
        string ext = str.substr(strlen(entry->d_name)-4);
        for (int Idx = 0; Idx < ext.length(); Idx++) {
          ext[Idx] = tolower(ext[Idx]);
        }
        if(!ext.compare(".png")) {
          //cout << entry->d_name << endl;
          num++;
        }
      }
    }
    closedir(dirp);
  } 
  return num;
}

void ViennaMap::getPNGfiles(char* directory) {
  DIR *dirp;
  struct dirent *entry;
  int num = 0;
  
  
  if(dirp = opendir(directory))
  {
    while(entry = readdir(dirp)) {
      if(entry->d_type == 8) {
        string str = entry->d_name;
        string ext = str.substr(strlen(entry->d_name)-4);
        for (int Idx = 0; Idx < ext.length(); Idx++) {
          ext[Idx] = tolower(ext[Idx]);
        }
        if(!ext.compare(".png")) {
          cout << entry->d_name << endl;
          string pngfile = entry->d_name;
          this->PNGfileList.push_back(pngfile);
        }
      }
    }
    closedir(dirp);
  } 

}

MapFragment* ViennaMap::getMapFragment(int x, int y) {
  //printf("in getmapfragment a:    imgNum: %d\n",x);
  
  if (SDL_mutexP(fragmentGuard) == -1)
    throw "could not acquire fragmentGuard";

  MapFragment *frag = NULL;

  std::vector<MapFragment*>::iterator it=fragments.begin(), end=fragments.end();
  for (; it != end; it++)
    if ((*it)->x == x && (*it)->y == y) {
      printf("fragment found\n");
      frag = *it;
      break;
    }

  if (SDL_mutexV(fragmentGuard) == -1)
    throw "could not release fragmentGuard";

  return frag;
}

bool ViennaMap::hasFragment(int x, int y) 
{
  return getMapFragment(x,y) != NULL;
}


BuildingList& ViennaMap::getBuildingsOfFragment(int x, int y) {
  MapFragment* frag = getMapFragment(x, y);

  if (frag != NULL) return frag->buildings;
  
  printf("FATAL:   FIXME: fragment not loaded while getting buildings\n");
  exit(1);
}

const PolygonList& ViennaMap::getPolygonsOfFragment(int x, int y) {
  printf("imgNum: %d\n",x);
  MapFragment* frag = getMapFragment(x, y);
  
  if (frag != NULL) return frag->polygons;
  printf("in getPolygonsOfFrag:    imgNum: %d\n",x);
  return loadFragment(x, y);
}

const PolygonList& ViennaMap::loadFragment(int fragX, int fragY) {
  //TODO: keep track of the images that are being loaded so we don't issue
  //two load requests for the same picture. this will be important if 
  //we are used in a multi-threaded environment
  printf("in loadFragment:    imgNum: %d\n",fragX);
  
  //IplImage *img = getImage(fragX, fragY);
  IplImage *img = getPNGImage(fragX, fragY);
  
  this->fragmentImageWidth = img->width;
  this->fragmentImageHeight = img->height;
  
  if (SDL_mutexP(cvRessourcesGuard) == -1)
    throw "could not acquire cvRessourcesGuard";
  
  tempBinarizedImage = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1);
  
  //get one color channel and set white to zero
  cvSplit(img, tempBinarizedImage, NULL, NULL, NULL);
  cvThreshold(tempBinarizedImage, tempBinarizedImage, 250, 255, CV_THRESH_TOZERO_INV);
  
  //find polygons
  CvSeq *contours, *polys;
  //cvFindContours(tempBinarizedImage, cvMemStorage, &contours, sizeof(CvContour),
  //                CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
  cvFindContours(tempBinarizedImage, cvMemStorage, &contours, sizeof(CvContour),
                  CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
  polys = cvApproxPoly(contours, sizeof(CvContour), cvMemStorage, CV_POLY_APPROX_DP, 1, 1);

  //create MapFragment
  MapFragment *frag = new MapFragment();
  (*frag).x = fragX;
  (*frag).y = fragY;
  
  //read polygons
  for (; polys; polys = polys->h_next) {
    if (polys->total < 3) continue;
    Polygon* polygon = new Polygon(polys->total);
    Building* building = new Building();
    (*building).poly = new Polygon(polys->total);
    Polygon* bpoly = (*building).poly;
    
    bool incomplete = false;

    CvPoint *point;

    //(*building).poly[0].x=0;
    (*building).orderedVertices = new VertexIndexList();
    VertexIndexList* vil = (*building).orderedVertices;
    


    
    for (int i=0; i < polys->total; i++) {
      point = (CvPoint*)cvGetSeqElem(polys, i);
      int x, y;
//      x = point->x + fragX*fragmentImageWidth;
//      y = point->y + fragY*fragmentImageHeight;
//for image switching:
      x = point->x;
      y = point->y;
      (*polygon)[i].x = x;
      (*polygon)[i].y = y;

      
      (*bpoly)[i].x = x;
      (*bpoly)[i].y = y;
      (*bpoly)[i].z = 0;
      (*bpoly)[i].dx = (GLdouble)x;
      (*bpoly)[i].dy = (GLdouble)y;
      (*bpoly)[i].dz = (GLdouble)0;
      //tmpX += x;
      //tmpY += y;
      
      if (x == 1 || y == 1 || x == fragmentImageWidth-2 || y == fragmentImageHeight-2)
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

    (*building).dCenterX = ( 1 / (A * 6)) * tmpResultx;
    (*building).dCenterY = ( 1 / (A * 6)) * tmpResulty;
    (*building).fCenterX = ( 1 / (A * 6)) * tmpResultx;
    (*building).fCenterY = ( 1 / (A * 6)) * tmpResulty;
    
    if (!incomplete) {
      frag->polygons.push_back(polygon);
      frag->buildings.push_back(building);
    } else {
      frag->incompletePolygons.push_back(polygon);
      frag->incompleteBuildings.push_back(building);
    }
  }

  //clean up
  cvClearMemStorage(cvMemStorage);
  cvReleaseImage(&img);

  if (SDL_mutexV(cvRessourcesGuard) == -1)
    throw "could not release cvRessourcesGuard";

  if (SDL_mutexP(fragmentGuard) == -1)
    throw "could not acquire fragmentGuard";

  //TODO: tryCompletePolygons
  //throw "not implemented";
  
  //add map fragment to list
  fragments.push_back(frag);

  if (SDL_mutexV(fragmentGuard) == -1)
    throw "could not release fragmentGuard";

  return frag->polygons;
}

IplImage* ViennaMap::getImage(int x, int y) {
    //TODO: get from web!!
    if (x != 0 || y != 0) return NULL;

    return cvLoadImage("../../illbeBACH.png", 3);
}

IplImage* ViennaMap::getPNGImage(int x, int y) {
  ImageFileList::const_iterator pngFile, pngFileEnd;
  pngFile = this->PNGfileList.begin();
  pngFileEnd = this->PNGfileList.end();
  int c=0;
  for (;pngFile != pngFileEnd; pngFile++) {
    if(c==x) {
      string dir = this->PNGimageDirectory;
      string file = *pngFile;
      string s3 = dir + file;
      
      //s3 = "imgs_adjusted/illbeBACH.png";
      char *cstr;
      cstr = new char [s3.size()+1];
      strcpy (cstr, s3.c_str());
      
      printf("loading %s\n",cstr);
      return cvLoadImage(cstr, 3);
      //return cvLoadImage("imgs_adjusted/illbeBACH.png", 3);
    }
    c++;
  }
  
}

void tryCompletePolygons(int x, int y) {
  throw "not implemented";
}
