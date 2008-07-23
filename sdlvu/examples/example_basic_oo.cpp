// Basic SDLVU usage example: demonstrates how to subclass SDLVU
// rather than just setting the callback funcion.  
// Karin Kosina (vka kyrah) 20080716

#include <sdlvu.h> // will also include GL headers

// SDLVU subclass definition
class MySDLVU : public SDLVU
{
  public:
     MySDLVU() {};
     virtual ~MySDLVU() { }
     virtual void Display();
};

// override the display method to do your custom drawing

void MySDLVU::Display()
{
  static GLUquadric * q = gluNewQuadric();
  BeginFrame();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glColor3f(0.95,0.95,0.95);
  gluSphere(q, 0.5, 100, 100);
  glRotatef(70, 1, 0, 0);
  glColor3f(0.6,0.6,0.6);
  gluDisk(q, 0.7, 1, 100, 100);
  glPopMatrix();
  EndFrame();
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
