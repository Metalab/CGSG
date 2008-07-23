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
// sdlvu_camview.cpp : camera-based viewing system. This modules stores a 
// collection of cameras and provides routines for manipulating them based 
// on mouse input and allows for recording and playback. These cameras can 
// directly drive the viewing main display routine to provide user-interaction.
// ============================================================================

// SDL port by Karin Kosina (vka kyrah) 20080718

#include "sdlvu.h"

extern "C" {
#include "trackball.h"
}

#include <iostream>

using namespace std;

//----------------------------------------------------------------------------
/// Set all cameras to the same viewing parameters
/**
// Most every SDLVU app will call this routine soon after Init().
// It's the best way to get the camera perspecitve matrix parameters all set
// up.
//
// This method also saves these camera parameters into a separate
// member, so that these camera settings can be restored with a call
// to AllCamsResetToOrig()
//
// @param ModelMin, ModelMax  axis-aligned bounding box of the world
// @param Eye   starting location of the camera origin (eye)
// @param LookAtCntr   starting world point to look towards (usually model center)
// @param ViewUp   starting world up vector (usually [0,1,0])
// @param Yfov   vertical field-of-view in degrees.
// @param Aspect   pixel width/height. 
// @param Nearfactor, Farfactor  determine the distances to the near and 
//        far planes as a factor times the world bounding sphere radius 
//        (sphere that surrounds the given (WorldMin,WorldMax).
// @note \p Eye cannot equal \p LookAtCntr!! Earth-shattering Kaboom!!
//
// @see AllCamsPerspectiveChange, AllCamsPerspectiveAspectChange,
//     AllCamsResetToOrig
*/
//----------------------------------------------------------------------------
void SDLVU::SetAllCams(const Vec3f& WorldMin, const Vec3f& WorldMax, 
                  const Vec3f& Eye, const Vec3f& LookAtCntr, const Vec3f& viewup,
                  float Yfov, float Aspect, float NearFactor, float FarFactor)
{
  // STORE WORLD BOUNDING SPHERE
  WorldCenter = (WorldMax+WorldMin)*0.5;
  Vec3f Diagonal = WorldMax-WorldCenter;
  WorldRadius = Diagonal.Length();

  // SET CAMERA PROJECTION
  float NearDist = WorldRadius * NearFactor;
  float FarDist = WorldRadius * FarFactor;
  AllCamsPerspectiveChange(Yfov,Aspect,NearDist,FarDist);

  // STORE THE UP VECTOR
  ViewUp = viewup;

  // SET CAMERA MODELVIEW
  for (int i=0; i<NumCams; i++) {
    Cams[i].LookAt(Eye,LookAtCntr,ViewUp);
  }

  SetOrigCam(&Cams[0]);
}


//----------------------------------------------------------------------------
/// Change some parameters of all camearas
/**
// 
// This is an easy way to change some key parameters that define the
// perspective transformation being used by every camera, without
// changing the location or orientation of the cameras.
//
// @param Yfov   vertical field-of-view in degrees.
// @param Aspect window width/height in pixels. 
// @param NDist, FDist  Near and far plane distances.
//
// @see SetAllCams, AllCamsPerspectiveAspectChange, AllCamsResetToOrig
*/
//----------------------------------------------------------------------------
void SDLVU::AllCamsPerspectiveChange(float Yfov, float Aspect, float Ndist, float Fdist)
{
  for (int i=0; i<NumCams; i++)
    Cams[i].Perspective(Yfov, Aspect, Ndist, Fdist);
}


//----------------------------------------------------------------------------
/// Change the aspect ratio of all cameras
/**
// Change the aspect ratio of the perspective transformation for every
// camera without changing any of the other parameters of the perspective 
// matrix.
//
// This is called by the default implementation of Reshape().
//
// @param NewAspect A new aspect ratio (width over height)
//
// @see SetAllCams, AllCamsPerspectiveChange, AllCamsResetToOrig, Reshape
*/
//----------------------------------------------------------------------------
void SDLVU::AllCamsPerspectiveAspectChange(float NewAspect)
{
  // PRESERVE OTHER PROJECTION SETTINGS, ONLY CHANGE ASPECT RATIO FOR
  // NEW WINDOW SIZES
  float Yfov, OldAspect, Ndist, Fdist;
  for (int i=0; i<NumCams; i++)
  {
    Cams[i].GetPerspectiveParams(&Yfov,&OldAspect,&Ndist,&Fdist);
    Cams[i].Perspective(Yfov,NewAspect,Ndist,Fdist);
  }
}

//----------------------------------------------------------------------------
/// Change the near and far distances of all cameras
/**
// Change the near and far distances of the perspective frustum for every
// camera without changing any of the other parameters of the perspective 
// matrix.
//
// @param Ndist,Fdist The new near and far distances
//
// @see SetAllCams, AllCamsPerspectiveChange, AllCamsResetToOrig
*/
//----------------------------------------------------------------------------
void SDLVU::AllCamsPerspectiveNearFarChange(float Ndist, float Fdist)
{
  // PRESERVE OTHER PROJECTION SETTINGS, ONLY CHANGE NEAR/FAR
  float Yfov, Aspect, OldNdist, OldFdist;
  for (int i=0; i<NumCams; i++)
  {
    Cams[i].GetPerspectiveParams(&Yfov,&Aspect,&OldNdist,&OldFdist);
    Cams[i].Perspective(Yfov,Aspect,Ndist,Fdist);
  }
}

//----------------------------------------------------------------------------
/// Set all the cameras back to the initial view
/**
// Set all the cameras back to the initial camera that was set with the last
// call to SetAllCams().
//
// @see SetAllCams, AllCamsPerspectiveChange
*/
//----------------------------------------------------------------------------
void SDLVU::AllCamsResetToOrig()
{
  for (int i=0; i<NumCams; i++) {
    Cams[i].Copy(OrigCam);
  }

  SDL_Surface * surface = SDL_GetVideoSurface();
  int WW = surface->w;
  int WH = surface->h;

  AllCamsPerspectiveAspectChange(WW/WH);
}

//----------------------------------------------------------------------------
/// Return the next modelview matrix
/**
// This gets the next modelview matrix that should be used for displaying.
// BeginFrame() calls this automatically, but if you aren't calling
// BeginFrame() from your Display() method, then you can call this
// directly to get the next modelview matrix to pass to OpenGL.  The
// matrix is in OpenGL format, so you can pass it right to \c glLoadMatrix().  
// Here is what BeginFrame() does with the matrix stack for example:
//
//  @code
    GLfloat M[16];
    glMatrixMode(GL_PROJECTION);
     glLoadMatrixf( GetProjectionMatrix(M) );
    glMatrixMode(GL_MODELVIEW);
     glLoadMatrixf( GetModelviewMatrix(M) );
    @endcode
//
// This innocuous-sounding getter function also plays a role in camera
// path playback.  If playback mode is engaged, then the matrix
// returned to you will not be based on the user's mouse wigglings,
// but on recorded camera data loaded in from a file on demand.  So
// this isn't your typical do-nothing getter function, it actually
// does some work to figure out what matrix to give you.
//
// @see GetProjectionMatrix, BeginFrame
*/
//----------------------------------------------------------------------------
float* SDLVU::GetModelviewMatrix(float* M)
{
  // WRITE ALL CAMERAS TO FILE IF RECORDING ON
  if (RecordingOn) 
  {
    for (int i=0; i<NumCams; i++)
      Cams[i].WriteToFile(RecordPlaybackFP);
  }

  // READ ALL CAMERAS FROM FILE IF PLAYBACK ON
  else if (PlaybackOn)
  {
    int ShouldEndPlayback=0;
    for (int i=0; i<NumCams; i++)
      if ( !(Cams[i].ReadFromFile(RecordPlaybackFP)) )
        ShouldEndPlayback=1;
    if (ShouldEndPlayback)
      EndPlayback();
  }

  return( Cam->GetModelviewMatrix(M) );
}

/// Return the OpenGL-style projection matrix from the current camera
/**
 * The matrix is an array of 16 floats, suitable for passing to
 * \c glLoadMatrixf()
 * 
 * @see GetModelviewMatrix, BeginFrame
 */
float* SDLVU::GetProjectionMatrix(float* M)
{
  return( Cam->GetProjectionMatrix(M) );
}

/// Prepare the current frame for drawing in display routine
/**
 * Sets up the modelview and projection matrices based on the current 
 * camera, and updates the frame rate timer.
 * 
 * Unless you need to do some fancy customized manipulation of the
 * OpenGL matrix stack, just calling BeginFrame() is a convenient and effective way 
 * to set things up so you can just do your application-specific OpenGL drawing. 
 *
 * @see EndFrame, GetModelviewMatrix, GetProjectionMatrix, Display
 */
void SDLVU::BeginFrame()
{
  GLfloat M[16];
  glMatrixMode(GL_PROJECTION);
   glLoadMatrixf( GetProjectionMatrix(M) );
  glMatrixMode(GL_MODELVIEW);
   glLoadMatrixf( GetModelviewMatrix(M) );
  UpdateFPS();
}

/// Finish up the current OpenGL display frame.  
/**
 * This should be, or at least can be, called at the end of your Display() 
 * method.  
 *
 * Unless you need to swap buffers in the middle of a frame for some
 * reason, this is usually sufficient.  
 *
 * This method is also responsible for drawing the lines in space to
 * represent other cameras, so that you can see from one camera where
 * the others are located in space.  The default keys for switching
 * cameras are 1, 2, 3, and 4.  The default keys for toggling display
 * of camera frusta are Shift-1, Shift-2, Shift-3, and Shift-4.
 * 
 * @see BeginFrame, GetModelviewMatrix, GetProjectionMatrix, Display 
 */
void SDLVU::EndFrame()
{
  // DRAW CAMERAS (IF DISPLAY IS ON)
  for (int i=0; i<NumCams; i++) {
    if (GetCamDisplayOn(i)) {
      GetCam(i)->Display();
    }
  }

  SDL_GL_SwapBuffers();
  //printf("FPS: %.1f\n", GetFPS());
}

//----------------------------------------------------------------------------
/// Return the world space ray corresponding to a particular screen pixel
/** 
// Given a screen pixel location (sx,sy) w/ (0,0) at the lower-left, and the
// screen dimensions, return the ray (start,dir) of the ray in world coords.
//  
// A very handy routine for implementing ray-tracers.
//
// @see  Camera::GetPixelRay()
*/
//----------------------------------------------------------------------------
void SDLVU::GetPixelRay(int sx, int sy, int ww, int wh, 
                       Vec3f *Start, Vec3f *Dir) const
{
  Cam->GetPixelRay((float)sx,(float)sy,ww,wh,Start,Dir);
}


//----------------------------------------------------------------------------
// MOUSE-TO-CAMERA MOVEMENT ROUTINE: TRANSLATE
//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::TranslateX(int OldX, int NewX, int WW)
{
  float RelX = (NewX-OldX)/(float)WW;
  float dX;

  if (InsideLookingOutMode)
  {
    dX = RelX * (-WorldRadius*0.25f);
  }
  else // FOR OUTSIDE-LOOKING-IN
  {
    Vec3f EyeToWorldCntr = WorldCenter - Cam->Orig;
    float DistToCntr = EyeToWorldCntr.Length();
    if (DistToCntr<WorldRadius) DistToCntr=WorldRadius;

    float vpX = RelX * (Cam->wR-Cam->wL);
    dX = DistToCntr * vpX / -Cam->Near;
  }

  float M[16];
  Vec3f Trans = Cam->X * dX * moveSpeed;
  Translate16fv(M,Trans.x,Trans.y,Trans.z);
  Cam->Xform(M);
}

//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::TranslateY(int OldY, int NewY, int WH)
{
  float RelY = (NewY-OldY)/(float)WH;
  float dY;

  if (InsideLookingOutMode)
  {
    dY = RelY * (WorldRadius*0.25f);
  }
  else // FOR OUTSIDE-LOOKING-IN
  {
    Vec3f EyeToWorldCntr = WorldCenter - Cam->Orig;
    float DistToCntr = EyeToWorldCntr.Length();
    if (DistToCntr<WorldRadius) DistToCntr=WorldRadius;

    float vpY = RelY * (Cam->wT-Cam->wB);
    dY = DistToCntr * vpY / Cam->Near;
  }

  float M[16];
  Vec3f Trans = Cam->Y * dY * moveSpeed;
  Translate16fv(M,Trans.x,Trans.y,Trans.z);
  Cam->Xform(M);
}

//----------------------------------------------------------------------------
// MOUSE-TO-CAMERA MOVEMENT ROUTINE: DRIVE MODE
//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::DriveY(int OldY, int NewY, int WH)
{
  float RelY = (NewY-OldY)/(float)WH;
 
  Vec3f EyeToWorldCntr = WorldCenter - Cam->Orig;
  float DistToCntr = EyeToWorldCntr.Length();
  if (DistToCntr<WorldRadius) DistToCntr=WorldRadius;

  float M[16];
  Vec3f Trans = Cam->Z * (DistToCntr*RelY*2.0f) * moveSpeed;
  Translate16fv(M,Trans.x,Trans.y,Trans.z);
  Cam->Xform(M);
}


//----------------------------------------------------------------------------
// MOUSE-TO-CAMERA MOVEMENT ROUTINE: LOOK MODE
//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::LookX(int OldX, int NewX, int WW)
{
  float RelX = (NewX-OldX)/(float)WW;
 
  float M[16], N[16];
  Translate16fv(M,Cam->Orig.x,Cam->Orig.y,Cam->Orig.z);
  Rotate16fv(N,-RelX*90,&(ViewUp.x));
  Mult16fv(M,M,N);
  Translate16fv(N,-Cam->Orig.x,-Cam->Orig.y,-Cam->Orig.z);
  Mult16fv(M,M,N);
  Cam->Xform(M);
}

//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::LookY(int OldY, int NewY, int WH)
{
  float RelY = (NewY-OldY)/(float)WH;
 
  float M[16], N[16];
  Translate16fv(M,Cam->Orig.x,Cam->Orig.y,Cam->Orig.z);
  Rotate16fv(N,-RelY*90,&(Cam->X.x));
  Mult16fv(M,M,N);
  Translate16fv(N,-Cam->Orig.x,-Cam->Orig.y,-Cam->Orig.z);
  Mult16fv(M,M,N);
  Cam->Xform(M);
}


//----------------------------------------------------------------------------
// MOUSE-TO-CAMERA MOVEMENT ROUTINE: TRACKBALL MODE
//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::TrackBallX(int OldX, int NewX, int WW)
{
  float RelX = (NewX-OldX)/(float)WW;
  Vec3f Up = InsideLookingOutMode ? ViewUp : Cam->Y;
  float M[16], N[16];
  Translate16fv(M,WorldCenter.x,WorldCenter.y,WorldCenter.z);
  Rotate16fv(N,-RelX*180,&(Up.x));
  Mult16fv(M,M,N);
  Translate16fv(N,-WorldCenter.x,-WorldCenter.y,-WorldCenter.z);
  Mult16fv(M,M,N);
  Cam->Xform(M);
}

//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::TrackBallY(int OldY, int NewY, int WH)
{
  float RelY = (NewY-OldY)/(float)WH;
  float M[16], N[16];
  Translate16fv(M,WorldCenter.x,WorldCenter.y,WorldCenter.z);
  Rotate16fv(N,-RelY*180,&(Cam->X.x));
  Mult16fv(M,M,N);
  Translate16fv(N,-WorldCenter.x,-WorldCenter.y,-WorldCenter.z);
  Mult16fv(M,M,N);
  Cam->Xform(M);
}


//----------------------------------------------------------------------------
// MOUSE-TO-CAMERA MOVEMENT ROUTINE: SGI "HYPERBOLIC SHEET" TRACKBALL MODE
//----------------------------------------------------------------------------
/// One of the core routines used to translate mouse motion into camera motion
//----------------------------------------------------------------------------
void SDLVU::HyperBall(int OldX, int OldY, int NewX, int NewY, int WW, int WH)
{
  // SCALE SCREEN COORDS TO [-1.0,1.0 range] for trackball() call
  float RelX0 = 2.0*(OldX/(float)WW)-1.0;
  float RelX1 = 2.0*(NewX/(float)WW)-1.0;
  float RelY0 = 1.0-2.0*(OldY/(float)WH);
  float RelY1 = 1.0-2.0*(NewY/(float)WH);
  float q[4];
  trackball(q, RelX0, RelY0, RelX1, RelY1);
  float Q[4][4], M[16], N[16];
  
  Translate16fv(M,WorldCenter.x,WorldCenter.y,WorldCenter.z);
  invViewing16fv(N,&(Cam->X.x),&(Cam->Y.x),&(Cam->Z.x),&(Vec3f::ZERO.x));
  Mult16fv(M, M, N);
  build_rotmatrix(Q, q);
  Transpose16fv((float*)Q);
  Mult16fv(M, M, (float*)Q);

  Viewing16fv(N,&(Cam->X.x),&(Cam->Y.x),&(Cam->Z.x),&(Vec3f::ZERO.x));
  Vec3f tmp(Cam->Orig), nV;
  tmp -= WorldCenter;
  Mult16fv3fv(&(nV.x), N, &(tmp.x));

  Cam->LoadIdentityXform();
  Cam->Orig.Set((const Vec3f)nV);
  Cam->Xform(M);
}

//----------------------------------------------------------------------------
// CAMERA RECORDING AND PLAYBACK
//----------------------------------------------------------------------------
/**
 * Begins path recording.  Records the position and orientation of all four 
 * Cameras to the file specified with SetPathFilename(), or to "path0.dat" if 
 * none is specified.
 * @param FileName The name of the file to open and write to.
 * @see EndRecording, StartPlayback, EndPlayback, SelectCam
 */
void SDLVU::StartRecording(const char *FileName)
{
  if (RecordingOn) { printf("Already recording!\n"); return; }
  if (PlaybackOn) EndPlayback();
  RecordPlaybackFP = fopen(FileName,"w");
  if (!RecordPlaybackFP) { printf("ERROR (StartRecording) fp==NULL\n"); return; }
  RecordingOn=1;
}

/**
 * Stops path recording and closes the file being recorded to.
 * @see StartRecording, StartPlayback, EndPlayback, SelectCam
 */
void SDLVU::EndRecording()
{
  if (!RecordingOn) { printf("Not recording!\n"); return; }
  if (PlaybackOn) {   // NOT SUPPOSED TO HAPPEN EVER
    printf("Playback is on! Not recording!\n"); return; 
  }
  fclose(RecordPlaybackFP);
  RecordingOn=0;
}

/**
 * Begins path playback from the path file specified, or from "path0.dat"
 * if none is specified.
 * Updates the positions and orientations of all four Cameras.  This relies
 * on the use of GetModelviewMatrix() in the display routine.
 * @see EndPlayback, StartRecording, EndRecording
 */
void SDLVU::StartPlayback(const char *FileName)
{
  if (RecordingOn) EndRecording();
  if (PlaybackOn) EndPlayback();
  RecordPlaybackFP = fopen(FileName,"r");
  if (!RecordPlaybackFP) { printf("ERROR (StartPlayback) fp==NULL\n"); return; }
  PlaybackOn=1;

  printf("CAM PLAYBACK...\n");
  PlaybackTime = clock();
}

/**
 * Ends camera path playback, and closes the file that was being read from.
 * @see StartPlayback, StartRecording, EndRecording
 */
void SDLVU::EndPlayback()
{
  if (!PlaybackOn) { printf("Not playing back!\n"); return; }
  if (RecordingOn) { printf("Recording is on! Not playing back!\n"); return; } // NOT SUPPOSED TO HAPPEN
  fclose(RecordPlaybackFP);
  PlaybackOn=0;
  printf("%.2f seconds\n", (clock()-PlaybackTime)/(float)CLOCKS_PER_SEC);
}

/**
 * Stops both camera path recording and playback.
 * Equivalent to calling both EndPlayback() and EndRecording().  
 * Just a convenience method.
 * @see StartPlayback, StartRecording, EndPlayback, EndRecording
 */
void SDLVU::StopRecordingPlayback()
{
  if (PlaybackOn) EndPlayback();
  else if (RecordingOn) EndRecording();
  else printf("Not recording or playing back!\n");
}


//----------------------------------------------------------------------------
// FRAME TIMING
//----------------------------------------------------------------------------
inline float elapsed_ftime(timeb *tstart, timeb *tend)
{
  return (float)(tend->time - tstart->time)
    + ((float)(tend->millitm - tstart->millitm))/1000.0f;
}

/**
 * If FPS calculation is enabled (StartFPSClock()), then this function updates
 * the current Frames Per Second calculation.  Call only if you are not using
 * the default implementation of BeginFrame(), because it calls this for you.
 * @see StartFPSClock, StopFPSClock, BeginFrame, DrawFPS
 */
void SDLVU::UpdateFPS()
{
  if (calcFPS) {
    lastFPSCount++;
    struct timeb newClock;
    ftime(&newClock);
    float tdiff = elapsed_ftime(&lastFPSClock,&newClock);
    if (tdiff >= FPS_INTEGRATE_INTERVAL) {
      lastFPS = (float)(lastFPSCount)/tdiff;
      lastFPSClock.time = newClock.time;
      lastFPSClock.millitm = newClock.millitm;
      lastFPSCount = 0;
    }
  }
}

/**
 * Draw the current frame rate in frames per second at position \p x, \p y.
 *
 * This uses the current OpenGL drawing color, whatever that is.   Probably
 * best to set it explicitly yourself before calling this method.
 *
 * Frame rate calculation requires calling StartFPSClock() once, and either 
 * BeginFrame()/EndFrame() ever frame or manual calls to UpdateFPS().
 *
 * @param x,y The position for the lower left corner of the text, relative to the 
 *            lower left corner of the screen.
 *
 * @see StartFPSClock, StopFPSClock, BeginFrame, UpdateFPS
 */
void SDLVU::DrawFPS(int x, int y)
{
  // Draw the current fps number in the lower left corner of the screen.
  //glColor3f(1.0, 1.0, 1.0);
#if 0 // FIXME: Implement text rendering
  Text(x, y, "FPS: %.1f", GetFPS());
#endif
}

