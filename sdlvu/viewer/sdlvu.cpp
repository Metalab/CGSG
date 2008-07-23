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

// SDL port by Karin Kosina (vka kyrah) 20080716

//============================================================================
// sdlvu.cpp : OpenGL/SDL-based viewer
//============================================================================

#include "sdlvu.h"

#include <iostream>
#include <stdio.h>

using namespace std;

// FIXME: can only have one SDLVU, so remove later
SDLVU *SDLVU::SDLVUs[MAX_SDLVUS];
static int SDLVU_Initialized = 0;

/// The constructor
/**
 * Create a new SDLVU object with default settings.
 *
 * @note To actually do anything useful with it you will need to call
 * either Init() or InitWin(), and probably SetAllCams() too.
 *
 * @see Init, InitWin 
 */
SDLVU::SDLVU() : WindowID(0) 
{
  if (!SDLVU_Initialized) {
    for (int i=0; i<MAX_SDLVUS; i++)
      SDLVUs[i]=NULL;
    SDLVU_Initialized = 1;
  }

  WindowID = -1;

  WorldNavMode=0;
  InsideLookingOutMode=0;
  NumCams=4;
  Cams = new Camera[NumCams];
  CamDisplayOn = new int[NumCams];
  for (int i=0;i<NumCams;i++) CamDisplayOn[i]=0;
  Cam = &(Cams[0]);
  LeftButtonDown=0;
  RightButtonDown=0;
  MiddleButtonDown=0;
  OldX=0; OldY=0; NewX=0; NewY=0;
  moveSpeed = 1;
  CtrlPressed=0;
  AltPressed=0;
  ShiftPressed=0;
  InertiaOn=0, 
  InertiaEnabled=1;
  InertiaDelay=10;
  SetInertiaFunc(NULL);
  calcFPS = 0;
  lastFPS = 0.0;
  lastFPSCount = 0;
  RecordingOn = 0;
  PlaybackOn = 0;
  RecordPlaybackFP = 0;
}

/// The destructor
/**
 * Virtual since SDLVU has virtual methods and allocates some members on the heap. 
 */
SDLVU::~SDLVU()
{
  delete[] Cams;
  delete[] CamDisplayOn;
}

/// Initialize a viewer
/**
 * Create an SDLVU window and initialize it. This must be done \em
 * before calling \c MainLoop(). OpenGL calls can be made after this
 * to set state for lights, textures, materials, etc.  This version
 * creates its own SDl window. If you already have a window handy for
 * whatever reason, you can call InitWin() instead, to wrap an SDLVU
 * around that window.
 *
 * Also set up default keyboard, mouse, and reshape callback.
 *
 * @note \em Every SDLVU object should have this method called on it
 *       (this or InitWin()).
 *
 * @see InitWin 
 */

int SDLVU::Init(char *windowtitle, 
                int windowstartx, int windowstarty,
                int windowwidth, int windowheight, unsigned int userflags)
{
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) {
    cerr << "Unable to init SDL: " << SDL_GetError() << endl;
    return -1;
  }

  unsigned int visualmode = SDL_OPENGL;
  visualmode |= userflags;

  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  // FIXME: should probably store surface instead of windowID in SDLVU object
  SDL_Surface *surface;
  if (!(surface = SDL_SetVideoMode(windowwidth, windowheight, 0, visualmode))) {
    cerr << "Unable to set SDL video mode: " << SDL_GetError() << endl;
    SDL_Quit();
    return -1;
  }
  if ((surface->flags & visualmode) != visualmode) {
    cerr << "Warning: Flags couldn't be set: " << ((surface->flags&visualmode)^visualmode) << endl;
  }
  SDL_WM_SetCaption(windowtitle, NULL);
  WindowID = 0; // FIXME: Unneessary, remove later

  glEnable(GL_MULTISAMPLE);

  return InitWin(WindowID);
}

/// Initialize a viewer with an existing SDL window
/**
 * Set a SDLVU window to use the existing SDLVU window. This is
 * something you way wish to do if you are integrating with a legacy
 * SDL application, for instance, or if you need to use some special
 * SDL functionality to create a window with a particular display
 * mode.
 *
 * Also set up default keyboard, mouse, and reshape callbacks.
 * 
 * @param wID The window identifier (unused, GLUT legacy)
 *
 * @see Init
 */
// FIXME: instead of int wID, this should probably pass the surface* (?)
int SDLVU::InitWin(int wID)
{
  WindowID = wID; // FIXME: legacy, remove later

  SDLVUs[0] = this;

  // REGISTER DEFAULT CALLBACKS
  SetKeyboardFunc(DefaultKeyboardFunc);
  SetMouseFunc(DefaultMouseFunc);
  SetMotionFunc(DefaultMotionFunc);
  SetReshapeFunc(DefaultReshapeFunc);
  SetDisplayFunc(DefaultDisplayFunc);

  SetInertiaFunc(DefaultInertiaFunc);

  PrintVisualInfo();

  return WindowID;
}

// viewer main loop
// calls display and does event processing
int 
SDLVU::MainLoop()
{
  SDL_Surface * surface = SDL_GetVideoSurface();
  pfReshape(surface->w, surface->h);
  bool done = false;
  while (!done) {
    pfDisplay();
    usleep(5000);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      // FIXME: This does not work on my machine (SDL 1.2.13, Mac OS X)
      // No idea why...?!? 20080718 kyrah
      case SDL_VIDEORESIZE:
        pfReshape(event.resize.w, event.resize.h);
        cout << "Resizing" << endl;
        break;
      case SDL_QUIT:
        done = true;
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        pfKeyboard(event.key);
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        pfMouse(event.button);
        break;
      case SDL_MOUSEMOTION:
        if (event.motion.state & SDL_BUTTON(1)) { // left button down
          pfMotion(event.motion);
        }
        break;
      }
    }
    // NB! SDL_GL_SwapBuffers() is called in endFrame()
  }

  SDL_Quit();
  return 0;
}

/// The default display function implementation
/**
 * This just calls GetSDLVU()->Display(), thus allowing Object-Oriented
 * people to customize SDLVU's behavior by overriding the Display()
 * method instead of setting the callback method.
 *
 * @see Display, DefaultReshapeFunc, DefaultInertiaFunc, DefaultKeyboardFunc,
 *     DefaultMouseFunc, DefaultMotionFunc         
 */
void SDLVU::DefaultDisplayFunc()
{
  GetSDLVU()->Display();
}

// callback functions (use if you don't want to subclass SDLVU)

void 
SDLVU::SetDisplayFunc(SDLVU::DisplayFunc f)
{
  if (NULL == f) {
    pfDisplay = DefaultDisplayFunc;
  }
  else {
    pfDisplay = f;
  }
}

/// Handler for redrawing application OpenGL content.
/**
 * This method can be overridden to perform the application's actual
 * OpenGL drawing. Typically one begins by calling BeginFrame() as
 * the first call, and ends with EndFrame() as the last call.  Those
 * two methods handle camera setup (path playback and recording),
 * buffer swapping for double buffered windows, and frame rate timing
 * calculations.
 *
 * The default implementation does nothing.
 *
 */
void SDLVU::Display()
{
}

/// The default SDL reshape function implementation
/**
 * This just calls GetSDLVU()->Reshape(), thus allowing Object-Oriented
 * people to customize GLVU's behavior by overriding the Reshape() method 
 *
 * @see Reshape, DefaultDisplayFunc, DefaultInertiaFunc, DefaultKeyboardFunc,
 *     DefaultMouseFunc, DefaultMotionFunc 
*/
void SDLVU::DefaultReshapeFunc(int WW, int WH)
{
  GetSDLVU()->Reshape(WW, WH);
}

// callback functions (use if you don't want to subclass SDLVU)
void 
SDLVU::SetReshapeFunc(SDLVU::ReshapeFunc f)
{
  if (NULL == f) {
    pfReshape = DefaultReshapeFunc;
  }
  else {
    pfReshape = f;
  }
}

/// Handler for changed window size
/**
 * Typically this method is overridden to handle setting up the
 * perspective matrix and viewport matrix.  
 *
 * The default implementation is quite serviceable.  It sets the
 * viewport to be the whole visible window area, and adjusts all the
 * Cameras so that the aspect ratio of the perspective transformation
 * is not all whacked out.  One reason you might have to override this
 * is if you are implementing stereo using multiple viewports, or if
 * you are using multiple viewports for any other reason.
 *
 */
void SDLVU::Reshape(int WW, int WH)
{
  glViewport(0, 0, WW, WH);  
  AllCamsPerspectiveAspectChange((GLfloat)WW/(GLfloat)WH);
}

/// Dump info about the selected visuals to standard out
/*
 * Currently, this is called automatically when a window is
 * initialized with Init() or InitWin().  Can't you tell that we're
 * researchers?  We use stdout, \em and we spit a bunch of geek
 * information out to it even if you don't explicitly ask.
 */
void SDLVU::PrintVisualInfo()
{
  GLint i;
  GLboolean j;
  printf("GRAPHICS VISUAL INFO (# bits of each):\n");
  glGetIntegerv(GL_RED_BITS, &i);    printf("RGBA: %d ", (int)i);
  glGetIntegerv(GL_GREEN_BITS, &i);  printf("%d ", (int)i);
  glGetIntegerv(GL_BLUE_BITS, &i);   printf("%d ", (int)i);
  glGetIntegerv(GL_ALPHA_BITS, &i);  printf("%d\n", (int)i);
  glGetIntegerv(GL_ACCUM_RED_BITS, &i);    printf("Accum RGBA: %d ", (int)i);
  glGetIntegerv(GL_ACCUM_GREEN_BITS, &i);  printf("%d ", (int)i);
  glGetIntegerv(GL_ACCUM_BLUE_BITS, &i);   printf("%d ", (int)i);
  glGetIntegerv(GL_ACCUM_ALPHA_BITS, &i);  printf("%d\n", (int)i);
  glGetIntegerv(GL_INDEX_BITS, &i);  printf("Color Index: %d\n", (int)i);
  glGetIntegerv(GL_DEPTH_BITS, &i);  printf("Depth: %d\n", (int)i);
  glGetIntegerv(GL_STENCIL_BITS, &i);  printf("Stencil: %d\n", (int)i);
  glGetBooleanv(GL_DOUBLEBUFFER, &j); printf("Double Buffer?  %s\n", j ? "yes" : "no");
  glGetBooleanv(GL_STEREO, &j); printf("Stereo Buffer?  %s\n", j ? "yes" : "no");
  glGetIntegerv(GL_AUX_BUFFERS, &i);  printf("Aux Buffers: %d\n", (int)i);
}

/// Print information about current OpenGL errors. (Debug only)
void SDLVU::CheckForGLError( char *msg )
{
#if defined(DEBUG) | defined(_DEBUG)
 GLenum errCode;
 const GLubyte *errStr;
 if ((errCode = glGetError()) != GL_NO_ERROR) 
 {
    errStr = gluErrorString(errCode);
    fprintf(stderr,"OpenGL ERROR: %s: %s\n", errStr, msg);
 }
#endif
}

