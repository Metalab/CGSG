// Basic SDLVU example using user-provided keyboard/mouse input handlers
// Karin Kosina (vka kyrah) 20080722

#include <sdlvu.h>
#include <stdio.h>

SDLVU sdlvu;
int callingdefaultmousefuncson=1;

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

// user-supplied mouse click handling function 

void userMouseFunc0(const SDL_MouseButtonEvent & event)
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
  if (callingdefaultmousefuncson) sdlvu.Mouse(event);
}

// user-supplied mouse motion handling function 

void userMotionFunc0(const SDL_MouseMotionEvent & event)
{
  printf("%d %d\n", event.x, event.y);

  // optional: call the default sdlvu motion handler
  if (callingdefaultmousefuncson) sdlvu.Motion(event);
}

// user-supplied keyboard event handling function 

void userKeyboardFunc0(const SDL_KeyboardEvent & event)
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
  sdlvu.Keyboard(event);
}

int main(int argc, char *argv[])
{
  sdlvu.Init("SDLVU Example: using mouse+keyboard", 50, 50, 512, 512);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_DEPTH_TEST);

  sdlvu.SetDisplayFunc(userDisplayFunc0);
  sdlvu.SetMouseFunc(userMouseFunc0);
  sdlvu.SetMotionFunc(userMotionFunc0);
  sdlvu.SetKeyboardFunc(userKeyboardFunc0);

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
