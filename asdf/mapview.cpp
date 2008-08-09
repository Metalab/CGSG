// Basic SDLVU usage example: demonstrates how to subclass SDLVU
// rather than just setting the callback funcion.  
// Karin Kosina (vka kyrah) 20080716

#include <sdlvu.h> // will also include GL headers
#include "map.h"

#include <vector>

// SDLVU subclass definition
class MySDLVU : public SDLVU
{
  ViennaMap map;
  const PolygonList *polygons;

  public:
     MySDLVU();
     virtual ~MySDLVU() { }
     virtual void Display();

  private:
    void DrawPoly(const Polygon &poly, float height);
};

MySDLVU::MySDLVU() {
  polygons = &map.getPolygonsOfFragment(0,0);
  printf("have %d polys\n", polygons->size());
}

// override the display method to do your custom drawing

void MySDLVU::Display()
{
  static GLUquadric * q = gluNewQuadric();
  BeginFrame();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();

  PolygonList::const_iterator poly, polyend;
  poly = polygons->begin();
  polyend = polygons->end();

  glScalef(0.01, 0.01, 0.01);
  glTranslatef(-500,500,0);
  glColor3f(0.95,0.95,0.95);
  for (;poly != polyend; poly++) {
    DrawPoly(**poly, 80);
  }
/*
  glColor3f(0.95,0.95,0.95);
  gluSphere(q, 0.5, 100, 100);
  glRotatef(70, 1, 0, 0);
  glColor3f(0.6,0.6,0.6);
  gluDisk(q, 0.7, 1, 100, 100);
*/

  glPopMatrix();
  EndFrame();
}

void MySDLVU::DrawPoly(const Polygon &poly, float height) {
  if (poly.size() < 3) return;

  glBegin(GL_QUADS);

  for (int i=poly.size()-1, n; i >= 0; i--) {
    n = i-1;
    if (n < 0) n = poly.size()-1;
    glColor3f(1., 0, 0);
    glVertex3f(poly[i].x, -poly[i].y, height);
    glVertex3f(poly[n].x, -poly[n].y, height);
    glColor3f(0, 0, 1.);
    glVertex3f(poly[n].x, -poly[n].y, 0.);
    
    glVertex3f(poly[i].x, -poly[i].y, 0.);
  }

  glEnd();
}

MySDLVU sdlvu;

int main(int argc, char *argv[])
{
  sdlvu.Init("SDLVU Basic Object Oriented Example",
             50, 50, 512, 512);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);

  // camera setup
  Vec3f modelmin(-1, -1, -1), modelmax(1, 1, 1); 
  Vec3f lookatcntr(modelmin + modelmax); lookatcntr *= 0.5;
  Vec3f mintoctr(lookatcntr - modelmin);
  Vec3f up(0, 1, 0);
  Vec3f eye(lookatcntr - 3 * (mintoctr - up * (mintoctr * up)));
  float yfov = 45;
  float aspect = 1;    
  float near = 0.1f; // near plane distance relative to model diagonal length
  float far = 10.0f; // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr, 
                   up, yfov, aspect, near, far);

  sdlvu.MainLoop();
  return 0;
}
