// The most basic SDLVU usage example.
// Karin Kosina (vka kyrah) 20080716

#include <sdlvu.h> // will also include GL headers

SDLVU sdlvu;

// user-supplied display function: your drawing code here

void userDisplayFunc0()
{
  static GLUquadric * q = gluNewQuadric();
  sdlvu.BeginFrame();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glColor3f(0.95,0.95,0.95);
  gluSphere(q, 0.5, 100, 100);
  glRotatef(70, 1, 0, 0);
  glColor3f(0.6,0.6,0.6);
  gluDisk(q, 0.7, 1, 100, 100);
  glPopMatrix();
  sdlvu.EndFrame();
}

int main(int argc, char ** argv)
{
  sdlvu.Init("SDLVU Basic Example", 50, 50, 512, 512);

  sdlvu.SetDisplayFunc(userDisplayFunc0);

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

