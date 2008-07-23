// Basic SDLVU example using user-provided keyboard/mouse input handlers,
// done by subclassing SDLVU rather than just setting the callback funcion. 
// Karin Kosina (vka kyrah) 20080722

#include <sdlvu.h>

// SDLVU subclass definition
class MySDLVU : public SDLVU
{
protected:
  int callingdefaultmousefuncson;
  
public:
  MySDLVU() : callingdefaultmousefuncson(1) {};
  
  virtual void Display();
  virtual void Mouse(const SDL_MouseButtonEvent & event);
  virtual void Motion(const SDL_MouseMotionEvent & event);
  virtual void Keyboard(const SDL_KeyboardEvent & event);
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

// override the mouse click handling method 

void MySDLVU::Mouse(const SDL_MouseButtonEvent & event)
{
  SDLMod mod = SDL_GetModState();
  if (mod & KMOD_CTRL) printf("CTRL-");
  if (mod & KMOD_ALT) printf("ALT-");
  if (mod & KMOD_SHIFT) printf("SHIFT-");

  if (event.button == SDL_BUTTON_LEFT)
    if (event.state == SDL_PRESSED) printf("LEFT-DOWN\n"); 
    else printf("LEFT-UP\n");
  if (event.button == SDL_BUTTON_RIGHT)
    if (event.state==SDL_PRESSED) printf("RIGHT-DOWN\n"); 
    else printf("RIGHT-UP\n");

  // optional: call the default sdlvu mouse handler
  if (callingdefaultmousefuncson) SDLVU::Mouse(event);
}

// override the mouse motion handling method 

void MySDLVU::Motion(const SDL_MouseMotionEvent & event)
{
  printf("%d %d\n", event.x, event.y);

  // optional: call the default superclass sdlvu motion handler
  if (callingdefaultmousefuncson) SDLVU::Motion(event);
}

// override the keyboard handling method 

void MySDLVU::Keyboard(const SDL_KeyboardEvent & event)
{
  switch(event.keysym.sym)
  {
    case ' ':
      if (event.type == SDL_KEYDOWN) {
        callingdefaultmousefuncson = !callingdefaultmousefuncson;
        if (callingdefaultmousefuncson) printf("KEY: callingdefaultmousefuncson = 1\n");
        else printf("KEY: callingdefaultmousefuncson = 0\n");
      }
      break;
  default:
    break;
  };

  // optional: call the default sdlvu keyboard handler
  SDLVU::Keyboard(event);
}


MySDLVU sdlvu;

int main(int argc, char *argv[])
{
  sdlvu.Init("SDLVU Basic Example", 50, 50, 512, 512);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);

  // camera setup
  Vec3f modelmin(-1,-1,-1), modelmax(1, 1, 1), 
        eye(1, 2, 3), lookatcntr(0, 0, 0), up(0, 1, 0);
  float yfov = 45;
  float aspect = 1; 
  float near = 0.1f;  // near plane distance relative to model diagonal length
  float far = 10.0f;  // far plane distance (also relative)
  sdlvu.SetAllCams(modelmin, modelmax, eye, lookatcntr, 
                   up, yfov, aspect, near, far);

  sdlvu.MainLoop();

  return 0;
}
