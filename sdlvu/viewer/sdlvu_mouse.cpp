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
// glvu_mouse.cpp : this routine provides the user-interface for world navigation
//   and the inertial system that allows for repeated motion upon application
//   of sufficient force. The user can bypass the normal mouse input handlers
//   by calling SetNewMouseHandlers(); the normal viewer mouse handling can
//   then be restored by calling RestoreMouseHandlers();
//============================================================================

// SDL port by Karin Kosina (vka kyrah) 20080716

#include "sdlvu.h"

#include <iostream>
#include <stdio.h>

using namespace std;

// SUPPORT FOR INERTIAL SYSTEM

#ifndef ABS
template <class T>
static bool ABS(T x)
{
  return (((x)<0)?(-(x)):x);  
}
#endif
#define MINMOVE 2  // AMOUNT (in pixels) TO MOVE TO INVOKE INERTIAL SYSTEM

/// The default SDL mouse button callback implementation
/**
 * This just calls GetSDLVU()->Mouse(), thus allowing Object-Oriented
 * people to customize GLVU's behavior by overriding the Mouse()
 * method.
 *
 * @see Mouse, DefaultDisplayFunc, DefaultInertiaFunc, DefaultKeyboardFunc,
 *     DefaultMotionFunc, DefaultReshapeFunc 
 */
void SDLVU::DefaultMouseFunc(const SDL_MouseButtonEvent & event)
{
  GetSDLVU()->Mouse(event);
}

/// Handler for mouse clicks (called when a button is pressed or released)
/**
 * This method can be overridden to perform application specific
 * funcionality (e.g. picking). The default implementation does some
 * important work for handling camera manipulation (world
 * navigation), so if you override this method you should always call
 * SDLVU::Mouse() from your override.
 * 
 * Users not interested in creating an object-oriented app can simply
 * call SDLVU's \c SetMouseFunc to set a callback directly. If you
 * do so you can still call GetCurrent()->Mouse() or 
 * SDLVU::DefaultMouseFunc(), in your handler to get the default 
 * behavior back.
 *
 */
void SDLVU::Mouse(const SDL_MouseButtonEvent & event)
{
  SDLMod mod = SDL_GetModState();
  CtrlPressed = (mod & KMOD_CTRL);
  AltPressed = (mod & KMOD_ALT);
  ShiftPressed = (mod & KMOD_SHIFT);

  // SET APPROPRIATE FLAGS FOR A LEFT-BUTTON MOUSE EVENT
  if (event.button == SDL_BUTTON_LEFT) {

    // STORE THE NEW MOUSE POS
    NewX=event.x; NewY=event.y;

    if (event.state==SDL_PRESSED)  // LEFT-BUTTON DOWN
    {
      OldX=event.x; OldY=event.y;      // RESET THE OLD TO THE CURRENT (starting over)
      LeftButtonDown=true; // SET LEFT-BUTTON-DOWN FLAG
      SetInertiaOn(0);     // TURN-OFF INERTIA WHEN USER PRESSES LEFT-BUTTON
    }
    else if (LeftButtonDown)    // LEFT-BUTTON UP after LEFT-BUTTON DOWN
    {
      LeftButtonDown=false;    // UNSET LEFT-BUTTON-DOWN FLAG

      // INVOKE THE INERTIAL SYSTEM IF THE LEFT BUTTON IS BEING RELEASED, THE
      // AMOUNT OF MOVEMENT IF "ENOUGH" AS DEFINED BY MINMOVE (IN PIXELS), AND
      // THE GLOBAL InertiaEnabled FLAG IS SET (CAN BE SET BY SetInertiaEnabled).
      if ((ABS(NewX-OldX) >= MINMOVE) || (ABS(NewY-OldY) >= MINMOVE)) 
        if (InertiaEnabled)
          SetInertiaOn(1);
    }
  }
  
  else if (event.button==SDL_BUTTON_RIGHT) {
    if (event.state==SDL_PRESSED) RightButtonDown = true;
    else RightButtonDown = false;
  }
  else if (event.button==SDL_BUTTON_MIDDLE) {
    if (event.state==SDL_PRESSED) MiddleButtonDown = true;
    else MiddleButtonDown = false;
  }
  if (event.state == SDL_PRESSED) {
    OldX = event.x;
    OldY = event.y;
  }
}

/// The default SDL motion function implementation
/**
 * This just calls GetGLVU()->Motion(), thus allowing Object-Oriented
 * people to customize GLVU's behavior by overriding the Motion() method.
 *
 * @see Motion, DefaultDisplayFunc, DefaultInertiaFunc, DefaultKeyboardFunc,
 *     DefaultMouseFunc, DefaultReshapeFunc 
*/
void SDLVU::DefaultMotionFunc(const SDL_MouseMotionEvent & event)
{
  GetSDLVU()->Motion(event);
}

/// Handler for 'active' mouse drag events, i.e. dragging with a button pressed.
/**
 *  This method can be overridden to perform application specific
 *  funcionality (e.g. direct manipulation of scene objects).  The
 *  default implementation does some important work for handling
 *  camera manipulation (world navigation), so if you override this
 *  method you should always call SDLVU::Mouse() from your override if
 *  you wish to preserve the built-in navigation capabilities.  
 *
 *  The exact effect the default implementation has on the current 
 *  camera depends upon the current world navigation mode.  
 *  See SetWorldNavMode().
 *
 *  Users not interested in creating an object-oriented app can simply
 *  call SDLVU's \c SetMotionFunc to set a callback directly. If you
 *  do so you can still call GetCurrent()->Motion() or 
 *  SDLVU::DefaultMotionFunc(), in your handler to get the default 
 *  behavior back.
 *
 *  @param event The most recent SDL_MouseMotionEvent.
 * 
 *  @see Motion, SetWorldNavMode 
 */
void SDLVU::Motion(const SDL_MouseMotionEvent & event)
{
  // STORE PREVIOUS NEW MOUSE POSITION (OLD)
  OldX=NewX; OldY=NewY;

  if (LeftButtonDown)
  {
    // STORE THE NEW MOUSE POSITION
    NewX=event.x; NewY=event.y;

    SDL_Surface * surface = SDL_GetVideoSurface();
    int WW = surface->w;
    int WH = surface->h;

    switch(GetWorldNavMode())
    {
      // -------  WORLD NAVIGATION -------
      case NAV_MODE_TRACKBALL:
        if (CtrlPressed) { TrackBallX(OldX,NewX,WW); DriveY(OldY,NewY,WH); }
        else { TrackBallX(OldX,NewX,WW); TrackBallY(OldY,NewY,WH); }
        break;
      case NAV_MODE_HYPERBALL:
        if (CtrlPressed) { 
          HyperBall(OldX,OldY,NewX,OldY,WW,WH); DriveY(OldY,NewY,WH); }
        else { HyperBall(OldX,OldY,NewX,NewY,WW,WH); }
        break;
      case NAV_MODE_DRIVE:
        if (CtrlPressed) { TranslateX(NewX,OldX,WW); TranslateY(NewY,OldY,WH); }
        else if (AltPressed) { LookX(OldX,NewX,WW); LookY(OldY,NewY,WH); }
        else { LookX(OldX,NewX,WW); DriveY(OldY,NewY,WH); }
        break;
      case NAV_MODE_TRANSLATE:
        TranslateX(OldX,NewX,WW); 
        if (CtrlPressed) DriveY(OldY,NewY,WH); else TranslateY(OldY,NewY,WH);
        break;
      case NAV_MODE_LOOK:
        if (CtrlPressed) { TranslateX(OldX,NewX,WW); TranslateY(OldY,NewY,WH); }
        else if (AltPressed) { LookX(OldX,NewX,WW); DriveY(OldY,NewY,WH); }
        else { LookX(OldX,NewX,WW); LookY(OldY,NewY,WH); }
        break;
    };
  }
}

// callback functions (use if you don't want to subclass SDLVU)

void 
SDLVU::SetMouseFunc(SDLVU::MouseFunc f)
{
  if (NULL == f) {
    pfMouse = DefaultMouseFunc;
  }
  else {
    pfMouse = f;
  }
}

void 
SDLVU::SetMotionFunc(SDLVU::MotionFunc f)
{
  if (NULL == f) {
    pfMotion = DefaultMotionFunc;
  }
  else {
    pfMotion = f;
  }
}

//----------------------------------------------------------------------------
// INERTIAL SYSTEM
//----------------------------------------------------------------------------

/// The default intertia function implementation
/**
 * This just calls GetSDLVU()->Inertia(), thus allowing Object-Oriented
 * people to customize GLVU's behavior by overriding the Inertia()
 * method instead of by dealing with callbacks.
 *
 * @see Inertia, DefaultDisplayFunc, DefaultReshapeFunc, DefaultKeyboardFunc,
 *     DefaultMouseFunc, DefaultMotionFunc 
 */
void SDLVU::DefaultInertiaFunc(int x, int y)
{
  GetSDLVU()->Inertia(x, y);
}

/// Handler for inertia events
/**
 * The default implementation calls the Motion() method, causing the
 * camera to move a little more in the direction it was moving when
 * inertia kicked in.
 *
 * This method only gets called when inertia has been triggered by an
 * appropriate mouse drag and release action.
 *
 * @param x, y 
 * @see DefaultInertiaFunc, Motion, Mouse, Keyboard, Reshape, Display 
 */
void SDLVU::Inertia(int x, int y)
{
  SDL_MouseMotionEvent e;
  e.type = SDL_MOUSEMOTION;
  e.state = SDL_PRESSED;
  e.x = x;
  e.y = y;
  SDLVU::Motion(e);
}

/// Call the inertia handler after some setup
/**
 * The way inertia is handled is to trick SDLVU into thinking it is
 * getting the exact same mouse motion again and again, i.e, that the
 * mouse was dragged from x1,y1 to x2,y2 repeatedly.  This method munges the 
 * various internal data members as necessary to pull this trick off, then 
 * calls the inertia callback.
 * 
 * Returns TRUE (1) if inertia is enabled, FALSE (0) otherwise.
 * @see SetInertiaOn, Inertia
 */
//----------------------------------------------------------------------------
int SDLVU::DoInertia()
{
  if (InertiaOn)
  {
    // SIMPLY REPEATEDLY CALL THE MOUSE MOVE HANDLER (MUST SET APPROPRIATE STATES)
    int tNewX=NewX, tNewY=NewY;          // COPY NEW VALUES TO TEMPS
    NewX=OldX; NewY=OldY;                // COPY OLD TO NEW (MOUSE MOVE COPIES)
    LeftButtonDown=true;                 // "PRETEND" BUTTON IS PRESSED
    if (UserInertiaFunc)
      UserInertiaFunc(tNewX, tNewY);     // CALL MOUSE MOVE HANDLER (INDIRECTLY)
    LeftButtonDown=false;                // RESET BUTTON STATE
    return(1);
  }
  return(0);
}

/// Turn inertia on or off.
/**
 * This is not about whether inertia is \em enabled or not, but whether 
 * it is currently active.  Usually called internally only.
 *
 * @see Inertia, DoInertia, SetInertiaEnabled
 */

SDL_TimerID SDLVU::timer;

void SDLVU::SetInertiaOn(int onOrOff)
{
  InertiaOn = onOrOff;
  if (InertiaOn) {
    SDLVU *g = SDLVUs[0];
    timer = SDL_AddTimer(g->InertiaDelay, InertialTimerFunc, (void*)g->WindowID); 
  } else {
    SDL_RemoveTimer(timer);
  }
}

Uint32 SDLVU::InertialTimerFunc(Uint32 interval, void * param)
{
  //FIXME: actually this is an int, but on you cant cast void* to int as
  //int may be small than void* (as on 64bit x86 linux)
  intptr_t winID = (intptr_t)param;
  SDLVU *g = SDLVUs[winID];
  g->DoInertia();
  return interval;
}
