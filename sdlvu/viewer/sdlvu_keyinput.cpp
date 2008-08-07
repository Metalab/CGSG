//------------------------------------------------------------------------------
// GLVU : Copyright 1997 - 2002 
//        The University of North Carolina at Chapel Hill
//------------------------------------------------------------------------------
// Permission to use, copy, modify, distribute and sell this software and its 
// documentation for any purpose is hereby granted without fee, provided that 
// the above copyright notice appear in all copies and that both that copyright 
// notice and this permission notice appear in supporting documentation. 
// Binaries may be compiled with this software without any royalties or 
// restrictions. 
//
// The University of North Carolina at Chapel Hill makes no representations 
// about the suitability of this software for any purpose. It is provided 
// "as is" without express or implied warranty.

//============================================================================
// glvu_keyinput.cpp : used to add keyboard key events to the viewer.
//============================================================================

// SDL port by Karin Kosina (vka kyrah) 20080716

#include "sdlvu.h"
#include "snapshot.h"

#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif
#include <SDL.h>

#include <iostream>
#include <stdio.h>
#include <assert.h>

using namespace std;

// callback functions (use if you don't want to subclass SDLVU)

void 
SDLVU::SetKeyboardFunc(SDLVU::KeyboardFunc f)
{
  if (NULL == f) {
    pfKeyboard = DefaultKeyboardFunc;
  }
  else {
    pfKeyboard = f;
  }
}

/// The default SDL keyboard function implementation
/**
 * This just calls GetSDLVU()->Keyboard(), thus allowing Object-Oriented
 * people to customize SDLVU's behavior by overriding the Keyboard() method 
 *
 * @see Keyboard, DefaultDisplayFunc, DefaultInertiaFunc, DefaultReshapeFunc,
 *     DefaultMouseFunc, DefaultMotionFunc 
 */
void SDLVU::DefaultKeyboardFunc(const SDL_KeyboardEvent & event)
{
  GetSDLVU()->Keyboard(event);
}

void SDLVU::displayHelp()
{
  cout << "--- AVAILABLE KEYBOARD SHORTCUTS: --- " << endl;
  cout << "?: display this help" << endl;
  cout << "z: navigation = trackball" << endl;
  cout << "h: navigation = hyperball; " << endl;
  cout << "x: navigation = drive; " << endl;
  cout << "c: navigation = translate; " << endl;
  cout << "v: navigation = look; " << endl;
  cout << "=: make snapshot" << endl;
  cout << "o: toggle in/out viewing mode" << endl;
  cout << "i: toggle inertia" << endl;
  cout << "d: dump current camera parameters" << endl;
  cout << "w: solid/lines/point" << endl;
  cout << "l: toggle lighting " << endl;
  cout << "b: toggle backface culling" << endl;
  cout << "n: switch front/back face" << endl;
  cout << "m: toggle materials" << endl;
  cout << "r: start recording camera path" << endl;
  cout << "s: stop recording camera path" << endl;
  cout << "p: play back camera path" << endl;
  cout << "0: reset cameras" << endl;
  cout << "1: select camera 1" << endl;
  cout << "2: select camera 2" << endl;
  cout << "3: select camera 3" << endl;
  cout << "4: select camera 4" << endl;
  cout << "!: toggle camera 1 display" << endl;
  cout << "@: toggle camera 2 display" << endl;
  cout << "#: toggle camera 3 display" << endl;
  cout << "$: toggle camera 4 display" << endl;
}

void SDLVU::toggleCameraDisplay(int value)
{
  SDLVU *g = GetSDLVU();
  if (g->CamDisplayOn[value]) g->CamDisplayOn[value]=0; 
  else g->CamDisplayOn[value]=1;
}

void SDLVU::changeGlOptions(int value)
{
  GLint State;
  GLint PolygonState[2];

  switch(value)
  {
    case 2: 
      glGetIntegerv(GL_POLYGON_MODE,PolygonState);
      if (PolygonState[0]==GL_POINT) glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); 
      else if (PolygonState[0]==GL_FILL) {
        glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE,&State);
        glPolygonMode(State?GL_FRONT_AND_BACK:GL_FRONT,GL_LINE);
      }
      else glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
      break;
    case 3:
      if (glIsEnabled(GL_CULL_FACE)) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
      break;
    case 6:
      glGetIntegerv(GL_SHADE_MODEL,&State);
      if (State==GL_SMOOTH) glShadeModel(GL_FLAT); else glShadeModel(GL_SMOOTH);
      break;
    case 8:
      if (glIsEnabled(GL_LIGHTING)) glDisable(GL_LIGHTING); else glEnable(GL_LIGHTING);
      break;
    case 7:
      glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE,&State);
      if (State == GL_TRUE) 
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
      else
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
      break;
    case 1:
      glGetIntegerv(GL_LIGHT_MODEL_LOCAL_VIEWER,&State);
      if (State == GL_TRUE) 
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
      else
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
      break;
    case 9:
      glGetIntegerv(GL_CULL_FACE_MODE,&State);
      if (State==GL_BACK) glCullFace(GL_FRONT); else glCullFace(GL_BACK);
      break;
    case 10:
      if (glIsEnabled(GL_COLOR_MATERIAL)) glDisable(GL_COLOR_MATERIAL); else glEnable(GL_COLOR_MATERIAL);
      break;
  }
}

void SDLVU::setNavigationMode(int value)
{
  // FIXME: typedef the NAV_MODEs and pass type instead of int
  switch(value) {
  case NAV_MODE_TRACKBALL:
  case NAV_MODE_HYPERBALL:
  case NAV_MODE_DRIVE:
  case NAV_MODE_TRANSLATE:
  case NAV_MODE_LOOK: {
    SDLVU *g = GetSDLVU();
    g->WorldNavMode=value;
    g->SetInertiaOn(0);
    }
    break;
  default:
    assert (0 && "invalid navigation mode!");
    break;
  };
}

/// Handler for keyboard events
/**
 * The default implementation handles all the SDLVU default key bindings.
 * Override this method to add your own key bindings, but if you don't handle 
 * the key, be sure to call the superclass (i.e. call SDLVU::Keyboard()) 
 *
 */
void SDLVU::Keyboard(const SDL_KeyboardEvent & event)
{
  SDLMod mod = SDL_GetModState();
  CtrlPressed = (mod & KMOD_CTRL);
  AltPressed = (mod & KMOD_ALT);
  ShiftPressed = (mod & KMOD_SHIFT);

  // FIXME: hard-coded keybindings below assume US keyboard layout...

  switch (event.type) {
  case SDL_KEYDOWN:
    // cout << "Key pressed: " << SDL_GetKeyName(event.keysym.sym) << endl;
    switch (event.keysym.sym) {
    case SDLK_ESCAPE:
      SDL_Event e;
      e.type = SDL_QUIT;
      SDL_PushEvent(&e);
      break;

    case 'z': setNavigationMode(0); break;
    case 'h': setNavigationMode(1); break;
    case 'x': setNavigationMode(2); break;
    case 'c': setNavigationMode(3); break;
    case 'v': setNavigationMode(4); break;

    case '/': if (ShiftPressed) displayHelp(); break; //?
    case '=': SnapShot(); break;
    case 'o': GetSDLVU()->ToggleInOutMode(); break;
    case '0': GetSDLVU()->AllCamsResetToOrig(); break;

    case 'i': {  // toggle inertia
      SDLVU *g = GetSDLVU();
      if (g->GetInertiaEnabled()) { g->SetInertiaEnabled(0); g->SetInertiaOn(0); }
      else g->SetInertiaEnabled(1);
      }
      break;

    case 'd': { // dump current camera parameters
      Camera *Cam;
      Vec3f Eye, ViewRefPt, ViewUp;
      float Yfov, Aspect, Ndist, Fdist;

      Cam = GetSDLVU()->GetCurrentCam();
      Cam->GetLookAtParams(&Eye,&ViewRefPt,&ViewUp);
      Cam->GetPerspectiveParams(&Yfov,&Aspect,&Ndist,&Fdist);
      printf("--- CURRENT CAM PARAMS ---\n");
      printf("       Eye: "); Eye.Print();
      printf("LookAtCntr: "); ViewRefPt.Print();
      printf("    ViewUp: "); ViewUp.Print();
      printf("     Y FOV: %f\n", Yfov);  
      printf("    Aspect: %f\n", Aspect);
      printf("      Near: %f\n", Ndist);
      printf("       Far: %f\n", Fdist);
      }
      break;

    case 'w': changeGlOptions(2); break;
    case 'b': changeGlOptions(3); break;
    case 'l': changeGlOptions(8); break;
    case 'n': changeGlOptions(9); break;
    case 'm': changeGlOptions(10); break;

    case 'r': GetSDLVU()->StartRecording("cam_record0.dat"); break;
    case 's': GetSDLVU()->StopRecordingPlayback(); break;
    case 'p': GetSDLVU()->StartPlayback("cam_record0.dat"); break;

    case '1': // camera 1
      if (ShiftPressed) toggleCameraDisplay(0);
      else GetSDLVU()->SelectCam(0);
      break;
    case '2': // camera 2
      if (ShiftPressed) toggleCameraDisplay(1);
      else GetSDLVU()->SelectCam(1);
      break;
    case '3': // camera 3
      if (ShiftPressed) toggleCameraDisplay(2);
      else GetSDLVU()->SelectCam(2);
      break;
    case '4': // camera 4
      if (ShiftPressed) toggleCameraDisplay(3);
      else GetSDLVU()->SelectCam(3);
      break;
    default:
      break;
    }
    break;
  case SDL_KEYUP:
    switch (event.keysym.sym) {
    default:
      break;
    }
    break;
  }
}

