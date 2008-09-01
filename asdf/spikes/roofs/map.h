#ifndef VIENNAMAP__H_
#define VIENNAMAP__H_

//TODO: figure out size parameters of the maps we can retrieve
//      and set all the constants accordingly


//TODO: instead of remembering MapFragments use a spatial data structure
//to hold all polygons and allow access via something like
//
//getPolysInRectangle(Point2D topLeft, Point2D size)
//
//and just remember which regions have already been loaded

//TODO: use vector type from sdlvu

#include <vector>
#include <sdlvu.h> // will also include GL headers

// external types that we need to reference but whose size we don't need
struct CvMemStorage;
struct _IplImage;
class SDL_mutex;

struct Point2D {
  int x,y;

  Point2D(int x, int y) { this->x = x; this->y = y; }
  Point2D() { }
};

struct Point3D {
  int x,y,z;
  GLdouble dx,dy,dz;
  GLdouble v[3];
  int *tessOrder;
  
  Point3D(int x, int y, int z) { this->x = x; this->y = y; this->z = z;}
	//Point3D(int x, int y) { this->x = x; this->y = y; }
  Point3D() { }
};

typedef std::vector<Point3D> Polygon;
typedef std::vector<Polygon*> PolygonList;

typedef std::vector<int> VertexIndexList;

class Building {
  public:
    Polygon *poly;
    VertexIndexList *orderedVertices;
};

typedef std::vector<Building*> BuildingList;

class MapFragment {
  public:
    int x,y;
    PolygonList polygons;
    PolygonList incompletePolygons;
    BuildingList buildings;
    BuildingList incompleteBuildings;
};


class ViennaMap 
{

  public:
    //number of fragments
    static const int nrFragmentColumns = 100;
    static const int nrFragmentRows = 100;

    static const int fragmentImageWidth = 3200;
    static const int fragmentImageHeight = 3200;

    static const float fragmentWidth = 500.;
    static const float fragmentHeight = 500.;

    ViennaMap();
    ~ViennaMap();

    //retrieves the polygons of the gien map fragment, retrieving and
    //decoding them when necessary
    const PolygonList& getPolygonsOfFragment(int x, int y);
    BuildingList& getBuildingsOfFragment(int x, int y);
    
    //checks whether the specified map fragment has been loaded
    bool hasFragment(int x, int y);
  
  private:
    //helper stuff to decode a map image to polygons, these are guarded
    //by a mutex for multithreaded use of the map object
    SDL_mutex *cvRessourcesGuard;
    CvMemStorage *cvMemStorage;
    _IplImage *tempBinarizedImage;

    //holds all loaded map fragments
    SDL_mutex *fragmentGuard;
    std::vector<MapFragment*> fragments;

    MapFragment* getMapFragment(int x, int y);
    const PolygonList& loadFragment(int x, int y);
    _IplImage* getImage(int x, int y);
    void tryCompletePolygons(int x, int y);
};

#endif
