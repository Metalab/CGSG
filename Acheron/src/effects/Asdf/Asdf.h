/*
 * Asdf.h
 *
 *  Created on: May 08,2009
 *      Author: scriptythekid
 */

#ifndef ASDF_H_
#define ASDF_H_

#include <string>
#include <sstream>

#include "../../LinearInterpolatedAnimateable.h"
#include "../../HSBColors.h"

#include <vector>
#ifdef __APPLE__
	#include <SDL/SDL_thread.h>
#else
	#include <SDL_thread.h>
#endif


// external types that we need to reference but whose size we don't need
struct CvMemStorage;
struct _IplImage;
class SDL_mutex;
  
  
namespace asdfns {
    
  struct Point3D {
    int x,y,z;
    Point3D(int x, int y, int z) { this->x = x; this->y = y; this->z = z;}
    Point3D() { }
  };

  struct Raster2VertexFactor {
    int rasterpoint;
    float distance;
  };

  typedef std::vector<Point3D> Polygon;
  typedef std::vector<Polygon*> PolygonList;
  typedef std::vector<int> VertexIndexList;
  typedef std::vector<Raster2VertexFactor> Raster2VertexList;
  
  class Building {
    public:
      Polygon *poly;
      VertexIndexList *orderedVertices;
      Raster2VertexList *rasterPoints2affect;
      GLdouble dCenterX, dCenterY;
      GLfloat fCenterX, fCenterY;
      GLfloat fmeanCenterX, fmeanCenterY;
  };
  
  typedef std::vector<Building*> BuildingList;
  
  
  
	class Asdf : public LinearInterpolatedAnimateable {

		private:
			const char* photoFilename;
			

			float v1[3], v2[3], v3[3], v4[3], v5[3], v6[3], v7[3], v8[3];
			float vn1[3], vn2[3], vn3[3];
			float thickness;

			float rPos;
			HSBColors* colors;

      SDL_mutex *cvRessourcesGuard;
      CvMemStorage *cvMemStorage;
      _IplImage *tempBinarizedImage;
      BuildingList buildings;
      
      std::vector<GLfloat> tmpVertexVector;
      std::vector<GLfloat> tmpNormalsVector;
      std::vector<GLfloat> roofsVertexVector;
      std::vector<GLfloat> roofsNormalsVector;
      GLfloat *myVertexArray;
      GLfloat *myNormalsArray;
      GLfloat *myRoofsVertexArray;
      GLfloat *myRoofsNormalsArray;
      int tessMissedCounter;
      void Asdf::CreateVertexArray();
      void Asdf::TessellateBuilding(Building &building);
		protected:
			virtual void DrawAtPosition( float* position, float factor, int tick );

		public:
			Asdf( float* startPos, float* endPos, const char* photoFilename, int startTick, int duration );


	};

}
#endif /* ASDF_H_ */
