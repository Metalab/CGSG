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

// SDL port by Karin Kosina (vka kyrah) 20080715

#ifndef SDLVU_H_
#define SDLVU_H_

#include <time.h>            // FOR TIMED PATH PLAYBACK (glvu_camview)
#include <sys/timeb.h>       // FOR FRAME TIMING (glvu_camview)

// FIXME: use CMAKE to include correct headers depending on platform
#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif
#include <SDL.h>

#include <vec3f.hpp>
#include <camera.hpp>

#ifndef ABS
#define ABS(x) (((x)<0)?(-(x)):x)    // HANDY UNIVERSAL ABSOLUTE VALUE FUNC
#endif

#define INERTIA_MINMOVE 2            // AMOUNT (in pixels) TO MOVE TO INVOKE INERTIAL SYSTEM
#define MAX_GLVUS 33                 // MAX NUMBER OF WINDOWS PLUS 1 SUPPORTED IN ONE APP
#define FPS_INTEGRATE_INTERVAL 1.0f // MINIMUM NUMBER OF SECONDS TO INTEGRATE FRAME-RATE OVER

#define MAX_SDLVUS 1        // MAX NUMBER OF WINDOWS PLUS 1 SUPPORTED IN ONE APP

/**
 * @class SDLVU
 * @brief A convenient OpenGL/SDL based 3D viewer.
 *
 * SDLVU is a C++ library for creating OpenGL applications based on SDL.
 *
 * The viewer library was developed mainly for creating small research
 * applications, and as such implements much of the functionality
 * necessary in a small viewer for prototyping rendering algorithms.
 * Among these features are a flexible camera class [Camera], several
 * camera manipulation, or navigation, modes, frame rate calculation,
 * and the ability to record and play back camera paths.  All of these
 * are features likely to be needed at one time or another by most
 * anyone that wants to look at 3D geometric scene data with OpenGL,
 * and each requires some effor to implement.  In particular,
 * developing the mouse-based camera manipulation routines can be
 * quite time consuming, as how to construct a mapping from 2D to 3D
 * (6D, really) that is intuitive for the user is far from obvious.
 * 
 * Another nice feature supported by SDLVU is "inertia", or the ability
 * to start a model spinning and have it keep spinning even after the
 * user releases the mouse. 
 *
 * @see Camera 
 */

class SDLVU 
{
public:
  // The world navigation modes. See SetWorldNavMode()
  // These are the different modes for manipulating the current
  // camera with mouse input.
  enum WorldNavMode {
    NAV_MODE_TRACKBALL, 
    NAV_MODE_HYPERBALL,
    NAV_MODE_DRIVE, 
    NAV_MODE_TRANSLATE, 
    NAV_MODE_LOOK
  };
  
  // Camera IDs.  There are 4 predefined cameras in 
  // a GLVU that can be switched between.
  enum CameraID {
    CAMERA_1,
    CAMERA_2,
    CAMERA_3,
    CAMERA_4,
    CAMERA_NUM_CAMS
  };
  
  // A function type used by the 'inertia' handling system
  typedef void (*InertiaFunc)(int x, int y);

  // callback functions set by the user
  typedef void (*DisplayFunc)();
  typedef void (*MouseFunc)(const SDL_MouseButtonEvent & event); 
  typedef void (*MotionFunc)(const SDL_MouseMotionEvent & event);
  typedef void (*ReshapeFunc)(int WW, int WH);
  typedef void (*KeyboardFunc)(const SDL_KeyboardEvent & event);

  SDLVU();
  virtual ~SDLVU();

  int Init(const char * windowtitle, 
           int windowstartx, int windowstarty,
           int windowwidth, int windowheight, 
           unsigned int visualmode = 0);
  virtual int InitWin(int aPreviouslyCreatedGlutWindow /* unusued */);

  void BeginFrame();
  void EndFrame();

  // main loop
  int MainLoop();

  // CAMVIEW
  int GetPlaybackOn() const;
  int GetRecordingOn() const;
  void AllCamsPerspectiveChange(float Yfov, float Aspect, 
                                float Ndist, float Fdist);
  void AllCamsPerspectiveNearFarChange(float Ndist, float Fdist);
  void AllCamsResetToOrig();
  float* GetModelviewMatrix(float* M);
  float* GetProjectionMatrix(float* M);
  void GetPixelRay(int sx, int sy, int ww, int wh, Vec3f *Start, Vec3f *Dir) const;
  void TranslateX(int OldX, int NewX, int WW);
  void TranslateY(int OldY, int NewY, int WH);
  void DriveY(int OldY, int NewY, int WH);
  void LookX(int OldX, int NewX, int WW);
  void LookY(int OldY, int NewY, int WH);
  void TrackBallX(int OldX, int NewX, int WW);
  void TrackBallY(int OldY, int NewY, int WH);
  void HyperBall(int OldX, int OldY, int NewX, int NewY,int WW, int WH);
  void StartRecording(const char *FileName = "path0.dat");
  void EndRecording();
  void StartPlayback(const char *FileName = "path0.dat");
  void EndPlayback();
  void StopRecordingPlayback();
  void DrawFPS(int xpos = 5, int ypos = 5);
  void UpdateFPS();  // USE ONLY IF YOU DO NOT CALL GLVU::BeginFrame()
  void AllCamsPerspectiveAspectChange(float Aspect);
     
  // CAMVIEW
  void SetAllCams(const Vec3f& worldmin, const Vec3f& worldmax, 
                  const Vec3f& eye, const Vec3f& lookatcntr, 
                  const Vec3f& viewup,
                  float yfov, float aspect, 
                  float nearfactor, float farfactor);
  void SelectCam(int WhichCam);
  int GetCurrentCamId() const;
  Camera* GetCurrentCam();
  void SetCurrentCam(Camera *NewCam);
  Camera* GetCam(int WhichCam);
  int GetInOutMode() const;
  void SetInOutMode(int Bool);
  void ToggleInOutMode();
  void SetOrigCam(Camera *Cam);
  const Vec3f& GetViewUp() const ;
  const Vec3f& GetWorldCenter() const ;
  void SetWorldCenter(const Vec3f& newCenter);
  float GetWorldRadius() const ;
  void SetWorldRadius(float newRadius) ;
  void SetViewUp(Vec3f viewup) ;
  void SetMoveSpeed(float speed) ;
  float GetMoveSpeed() const ;
  void StartFPSClock() ;
  void StopFPSClock() ;
  float GetFPS() const ;
  FILE* GetPathFile() const;

  int GetWorldNavMode() const ;
  void SetWorldNavMode(int mode) ;
  int GetCamDisplayOn(int WhichCam) const ;
  int IsWorldNavMode() const ;

  // MOUSE
  bool GetLeftButtonDown() const ;
  bool GetMiddleButtonDown() const ;
  bool GetRightButtonDown() const ;
  bool GetAltDown() const;
  bool GetShiftDown() const;
  bool GetCtrlDown() const;
  int GetMouseDeltaX(int curx) const;
  int GetMouseDeltaY(int cury) const;
  int GetInertiaOn() const ; 
  int GetInertiaEnabled() const ;
  void SetInertiaOn(int Bool);
  void SetInertiaEnabled(int Bool) ;
  int GetInertiaDelay() const ; 
  void SetInertiaDelay(int msec) ;
  void SetInertiaFunc(InertiaFunc f) ; 
  InertiaFunc GetInertiaFunc() const;

  // CALLBACK METHODS
  virtual void Keyboard(const SDL_KeyboardEvent & event);
  virtual void Mouse(const SDL_MouseButtonEvent & event);
  virtual void Motion(const SDL_MouseMotionEvent & event);
  virtual void Reshape(int WW, int WH);
  virtual void Display();
  virtual void Inertia(int x, int y);

  // DEFAULT CALLBACK FUNCTIONS (MOSTLY JUST CALL THE CALLBACK METHODS)
  static void DefaultDisplayFunc();
  static void DefaultMouseFunc(const SDL_MouseButtonEvent & event);
  static void DefaultMotionFunc(const SDL_MouseMotionEvent & event);
  static void DefaultReshapeFunc(int WW, int WH);
  static void DefaultKeyboardFunc(const SDL_KeyboardEvent & event);
  static void DefaultInertiaFunc(int x, int y);

  // callback functions to be set without having to subclass
  // FIXME: Implement Get*Func functions?
  void SetDisplayFunc(DisplayFunc f);
  void SetMouseFunc(MouseFunc f);
  void SetMotionFunc(MotionFunc f);
  void SetReshapeFunc(ReshapeFunc f);
  void SetKeyboardFunc(KeyboardFunc f);

  // MISC
  int GetWindowID() const;
  void MakeCurrent();
  static SDLVU* GetCurrent();
  static SDLVU* GetSDLVU(int WindowID);
  static SDLVU* GetSDLVU(); // deprecated

  static void PrintVisualInfo();
  static void CheckForGLError( char *msg );

protected:

  // GL VIEWER STATE VARIABLES
  Camera *Cams;                // ARRAY OF 4 VIEWER CAMS
  Camera *Cam;                 // PTR TO CURRENT CAM (DEFAULT IS CAM 0)
  Camera OrigCam;              // THE ORIGINAL VIEW FOR RESETTING
  int RecordingOn, PlaybackOn; // CAMERA RECORDING/PLAYBACK FLAGS
  FILE *RecordPlaybackFP;      // RECORDING/PLAYBACK FILE POINTER
  Vec3f WorldCenter;           // WORLD BOUNDING-SPHERE CENTER
  float WorldRadius;           // WORLD BOUNDING-SPHERE RADIUS
  Vec3f ViewUp;                // THE WORLD UP-VECTOR
  int InsideLookingOutMode;    // NAVIGATION MODE (IN->OUT OR OUT->IN)
  clock_t PlaybackTime;        // FOR PATH PLAYBACK TIMING
  int NumCams;
  struct timeb lastFPSClock;
  int calcFPS;
  float lastFPS;
  int lastFPSCount;

  bool LeftButtonDown, MiddleButtonDown, RightButtonDown;
  mutable int OldX, OldY;
  int NewX, NewY;  // CURRENT OLD AND NEW MOUSE POSITIONS
  float moveSpeed;
  bool CtrlPressed, AltPressed, ShiftPressed;
  int InertiaOn, InertiaEnabled;
  int InertiaDelay;
  
  int WorldNavMode;            // WORLD NAVIGATION MODE
  int *CamDisplayOn;           // DISPLAY FLAG FOR EACH CAMERA

  // MOUSE 
  InertiaFunc UserInertiaFunc;
  int DoInertia();
  static Uint32 InertialTimerFunc(Uint32 interval, void * param);

  // keyboard helper functions
  void displayHelp();
  void toggleCameraDisplay(int value);
  void changeGlOptions(int value);
  void setNavigationMode(int value);

  static SDLVU *SDLVUs[MAX_SDLVUS];

  // glut WindowID 
  // FIXME: Probably useless for us
  int WindowID;

  DisplayFunc pfDisplay;
  MouseFunc pfMouse;
  MotionFunc pfMotion;
  ReshapeFunc pfReshape;
  KeyboardFunc pfKeyboard;

  static SDL_TimerID timer; 
};

//----------------------------------------------------------------------------

/// Return whether or not playback mode is currently active
/**
 * When playback mode is active, the modelview matrices for viewing
 * are read from a previously recorded file instead of coming from the 
 * user's mousing.
 *
 * @see GetRecordingOn, StartRecording, EndRecording, 
 *     StartPlayback, EndPlayback, GetModelviewMatrix
 */
inline int SDLVU::GetPlaybackOn() const
{
  return(PlaybackOn); 
}


/// Return whether or not record mode is currently active
/**
 * When record mode is active, every camera view change made by the user
 * (through mouse manipulation) is written to a file.  The resulting
 * path can be played back later using StartPlayback().
 *
 * @see GetPlaybackOn, StartRecording, EndRecording, 
 *     StartPlayback, EndPlayback, GetModelviewMatrix
 */
inline int SDLVU::GetRecordingOn() const
{
  return(RecordingOn); 
}

/// Switch to the specified camera
/** 
 * Changes the camera from which the \c SDLVU renders the scene.
 *
 * By default this method is bound to the 1,2,3, and 4 keys on the
 * keyboard.
 *
 * @param WhichCam can be one of the #CameraID enum values.
 * @see GetModelviewMatrix, BeginFrame, EndFrame 
*/
inline void SDLVU::SelectCam(int WhichCam)
{
  Cam = &Cams[WhichCam]; 
}

/// Return the index of the active camera.
inline int SDLVU::GetCurrentCamId() const
{
  return (Cam - &Cams[0]);
}


/// Return a pointer to the active camera.
inline Camera* SDLVU::GetCurrentCam() 
{
  return(Cam); 
}

/// Set the active camera.
/**
 *  @param NewCam the new camera to set.
 *
 *  @note The new camera will not be owned by SDLVU.  This sets the
 *  current camera \em temporarily to \p NewCam.
 *
 *  @see SelectCam
 */
inline void SDLVU::SetCurrentCam(Camera *NewCam) 
{
  Cam=NewCam; 
}

/// Return a pointer to the specified camera
/**
 * @param WhichCam can be one of the \c CameraID identifiers \c
 * CAMERA_1, \c CAMERA_2, \c CAMERA_3, or \c CAMERA_4, or just a
 * number between zero and 3.  
 */
inline Camera* SDLVU::GetCam(int WhichCam) 
{
  return(&Cams[WhichCam]);
}

/**
 * Returns a boolean specifying whether the navigation mode is "inside looking 
 * out" (true) or "outside looking in" (false, the default).  
 * @see SetInOutMode, ToggleInOutMode
 */   
inline int SDLVU::GetInOutMode() const 
{
  return(InsideLookingOutMode); 
}

/**
 * Sets the navigation mode to either "inside looking out" (true) or
 * "outside looking in" (false, the default).  
 * @see SetInOutMode, ToggleInOutMode.
 */
inline void SDLVU::SetInOutMode(int Bool) 
{
  InsideLookingOutMode=Bool; 
}

/**
 * Toggles the display mode between "inside looking out" 
 * and "outside looking in".
 * @see SetInOutMode, GetInOutMode
 */
inline void SDLVU::ToggleInOutMode() 
{
  if (InsideLookingOutMode) InsideLookingOutMode=0;
  else InsideLookingOutMode=1; 
}

/**
 * Stores a copy of \p Cam so that Cameras can be reset to its parameters
 * at a later time using AllCamsResetToOrig().  Called from SetAllCams().
 * @see AllCamsResetToOrig, SetAllCams
 */
inline void SDLVU::SetOrigCam(Camera *Cam) 
{
  OrigCam.Copy(*Cam); 
}

/** 
 * Returns the global "up" vector, as set by SetViewUp() or SetAllCams().
 * This has some effect on the operation of certain mouse navigation modes.
 * @see SetWorldNavMode
 */
inline const Vec3f& SDLVU::GetViewUp() const 
{
  return(ViewUp); 
}

/**
 * Returns the global "center" of the world, as set by SetWorldCenter() or
 * (indirectly) by SetAllCams().
 */
inline const Vec3f& SDLVU::GetWorldCenter() const 
{
  return WorldCenter; 
}

/**
 * Sets the global "center" of the world.  Also set (indirectly) by SetAllCams().
 * @see GetWorldCenter, SetWorldNavMode
 */
inline void SDLVU::SetWorldCenter(const Vec3f& center)
{
  WorldCenter = center;
}

/**
 * Returns the global "radius" of the world, as set by SetWorldRadius() or
 * (indirectly) by SetAllCams().
 */
inline float SDLVU::GetWorldRadius() const 
{
  return WorldRadius; 
}

/**
 * Sets the global "radius" of the world.
 * @see GetWorldRadius
 */
inline void SDLVU::SetWorldRadius(float newRadius) 
{
  WorldRadius = newRadius; 
}

/**
 * Sets the global "up" vector.
 * @see GetViewUp
 */
inline void SDLVU::SetViewUp(Vec3f viewup) 
{
  ViewUp=viewup; 
}

/**
 * Sets the gain factor used in translating mouse motion in pixels
 * into world units.
 * @see GetMoveSpeed, SetWorldNavMode
 */
inline void SDLVU::SetMoveSpeed(float speed) 
{
  moveSpeed = speed; 
}

/**
 * Gets the gain factor used in translating mouse motion in pixels
 * into world units.
 * @see SetMoveSpeed, SetWorldNavMode
 */
inline float SDLVU::GetMoveSpeed() const
{
  return moveSpeed; 
}


/**
 * Starts the frame timer and resets the frame counter.  You must call
 * this once after creating the SDLVU if you would like to take
 * advantage of the built-in frame timing capabilities.
 * @see StopFPSClock, UpdateFPS, BeginFrame 
 */
inline void SDLVU::StartFPSClock() 
{
  calcFPS = 1; ftime(&lastFPSClock); lastFPSCount=0;
}

/**
 *   Stops the frame rate timer.
 * @see StartFPSClock
 */
inline void SDLVU::StopFPSClock() 
{
  calcFPS = 0; 
}

/**
 * Returns the last calculated Frames Per Second measurement.
 * @see StartFPSClock, StopFPSClock, UpdateFPS
 */
inline float SDLVU::GetFPS() const 
{
  return lastFPS; 
}

/**
 * Returns the current world navigation mode, (i.e. camera control mode).
 * Return value is one of the #WorldNavMode enum constants.
 * @see SetWorldNavMode
 */
inline int SDLVU::GetWorldNavMode() const 
{
  return(WorldNavMode); 
}

/**
 * Sets the current world navigation mode, (i.e. camera control mode).
 * @param mode One of the #WorldNavMode enum constants.
 *
 * - \c NAV_MODE_TRACKBALL: a simple trackball.  This is primarily for
 *           rotating the model, but by holding down CTRL you can move
 *           in and out as well.  The rotation generated by the
 *           trackball mode depends only upon the relative motion of
 *           the mouse, not the absolute location.  The drawback is
 *           that this mode has no good way to make a model upright.
 *           The Hyperball is better for that.
 *
 * - \c NAV_MODE_HYPERBALL: an "SGI-style" trackball. The effect of
 *           this trackball is different depending on where on the
 *           screen you drag the mouse.  On the edge of the screen,
 *           rotation happens mostly in the screen plane, but near
 *           the middle of the window, rotation happens perpendicular
 *           to the screen plane.  Hold down CTRL to move in and out.
 *
 * - \c NAV_MODE_DRIVE: Left / Right to steer, Up / Down to move fore and aft.
 * - \c NAV_MODE_TRANSLATE: translate the camera parallel to the view plane.
 * - \c NAV_MODE_LOOK: Rotation about a fixed eye position.
 *
 * The outside-looking-in rotational modes (\c NAV_MODE_TRACKBALL,
 * \c NAV_MODE_HYPERBALL) use the "world center" as the center of rotation.
 * See SetWorldCenter() and SetAllCams() for more information about the 
 * world center and camera settings.
 *
 * SDLVU uses the following default keyboard accelerators to switch between modes:
 *
 * - 'z' : \c NAV_MODE_TRACKBALL
 * - 'x' : \c NAV_MODE_DRIVE
 * - 'c' : \c NAV_MODE_TRANSLATE
 * - 'v' : \c NAV_MODE_LOOK
 * - 'h' : \c NAV_MODE_TRACKBALL
 *
 * @see GetWorldNavMode */
inline void SDLVU::SetWorldNavMode(int mode) 
{
  WorldNavMode=mode; 
}

/**
 *  Returns the display status of the specified Camera.
 *  Camera display refers to rendering of some lines that show the extents of a 
 *  camera's frustum.   SDLVU has some functionality to do this automically.
 *
 *  @see EndFrame
*/
inline int SDLVU::GetCamDisplayOn(int WhichCam) const 
{
  return(CamDisplayOn[WhichCam]); 
}
/**
 *  Returns true (nonzero) if the current world nav mode set is valid
 *  false (0) otherwise.
 *
 *  @see SetWorldNavMode
*/
inline int SDLVU::IsWorldNavMode() const 
{
  return(WorldNavMode>=0 && WorldNavMode<=3); 
}

/**
 * Returns true if the left button was down
 * last time the Mouse() or Motion() callback got called.
 */
inline bool SDLVU::GetLeftButtonDown() const 
{
  return(LeftButtonDown); 
}

/**
 * Returns true if the middle button was down
 * last time the Mouse() or Motion() callback got called.
 */
inline bool SDLVU::GetMiddleButtonDown() const 
{
  return(MiddleButtonDown); 
}

/**
 * Returns true if the right button was down
 * last time the Mouse() or Motion() callback got called.
 */
inline bool SDLVU::GetRightButtonDown() const 
{
  return(RightButtonDown); 
}

/**
 * Returns true if the Alt key was down
 * last time the Mouse() or Keyboard() callback got called.
 */
inline bool SDLVU::GetAltDown() const
{
  return (AltPressed);
}
/**
 * Returns true if the Shift key was down
 * last time the Mouse() or Keyboard() callback got called.
 */
inline bool SDLVU::GetShiftDown() const
{
  return (ShiftPressed);
}
/**
 * Returns true if the Control key was down
 * last time the Mouse() or Keyboard() callback got called.
 */
inline bool SDLVU::GetCtrlDown() const
{
  return (CtrlPressed);
}

/**
 * Returns the difference between the current x value, curx, passed in
 * and the last mouse X value SDLVU got.
 */
inline int SDLVU::GetMouseDeltaX(int curx) const
{
  int dx = curx - OldX;
  OldX = curx;
  return dx;
}

/**
 * Returns the difference between the current y value, cury, passed in
 * and the last mouse Y value SDLVU got (inverts normal screen coordinate
 * system so that upward mouse motion gives a positive delta).
 */
inline int SDLVU::GetMouseDeltaY(int cury) const
{
  int dy = OldY - cury;
  OldY = cury;
  return dy;
}

/**
 * Returns true (nonzero) if inertia is currently active.
 * Note that this is different from whether or not it is \em enabled. 
 * @see SetInertiaEnabled
 */
inline int SDLVU::GetInertiaOn() const 
{
  return(InertiaOn); 
} 

/// Return whether inertia is enabled
/**
 * Returns TRUE (1) if inertia is enabled, FALSE (0) otherwise.
 * @see SetInertiaEnabled 
 */
inline int SDLVU::GetInertiaEnabled() const 
{
  return(InertiaEnabled);
}

/// Enable or disable the SDLVU's inertia feature
/**
 * @see GetInertiaEnabled
 */
inline void SDLVU::SetInertiaEnabled(int Bool) 
{
  InertiaEnabled=Bool;
}


/// Return the number of milliseconds between inertia callbacks
/**
 * Inertia callbacks are only made by SDLVU when inertia is active and
 * enabled.  But when triggered, inertia callbacks are made repeatedly
 * at regular intervals to animate the camera.
 *
 * @see SetInertiaDelay, GetInertiaOn, GetInertiaEnabled 
 */
inline int SDLVU::GetInertiaDelay() const 
{
  return(InertiaDelay);
} 

/// Set the number of milliseconds to wait between inertia callbacks
/**
 * Inertia callbacks are only made by SDLVU when inertia is active and
 * enabled.  But when triggered, inertia callbacks are made repeatedly
 * at regular intervals to animate the camera.
 *
 *
 * @see GetInertiaDelay, GetInertiaOn, GetInertiaEnabled 
 */
inline void SDLVU::SetInertiaDelay(int msec) 
{
  InertiaDelay=msec;
}

/// Set the function to use as an inertia callback
/**
 * The default implemntation works fine.  There's really no
 * reason to call this.
 */
inline void SDLVU::SetInertiaFunc(SDLVU::InertiaFunc f) 
{
  UserInertiaFunc=f;
} 

/// Return the function being uses as the inertia callback
/**
 * @see SetInertiaFunc
 */
inline SDLVU::InertiaFunc SDLVU::GetInertiaFunc() const
{
  return UserInertiaFunc;
}

  
/**
 * GLUT legacy stuff -- unused
 * @see MakeCurrent, GetSDLVU(int), GetSDLVU 
 */
inline int SDLVU::GetWindowID() const
{
  return WindowID;
}

/// Make \c this the currently active GL context
/**
 *  GLUT legacy stuff -- unused
 *  @see GetSDLVU(void) 
 */
inline void SDLVU::MakeCurrent()
{
  // FIXME: Do I need to handle this in SDL at all? 20080715 kyrah
  // glutSetWindow(WindowID);
}

/// Returns the currently active SDLVU window
/**
 * See MakeCurrent() for a discussion of what it means to be the active 
 * window, and why you should care.
 *
 * @see MakeCurrent, GetSDLVU(int)
 */
inline SDLVU* SDLVU::GetCurrent()
{
  return SDLVUs[0]; // only one window in SDL
  // return SDLVUs[ glutGetWindow() ];
}

/**
 * GLUT legacy stuff -- unused
 * @see MakeCurrent, GetCurrent, GetWindowID
 */
inline SDLVU* SDLVU::GetSDLVU(int WindowID)
{
  return SDLVUs[ WindowID ];
}

/// Returns the currently active SDLVU window
/**
 * @deprecated
 *
 * This does the same thing as GetCurrent().  You should use that instead.
 *
 * @see MakeCurrent
 */
inline SDLVU* SDLVU::GetSDLVU(void)
{
  return SDLVUs[0]; // only one window in SDL
}


/// Return the file pointer being used for playback or recording
/**
 * Return the file pointer being used for playback or recording.
 * This is 0 if neither path playback nor recording is currently active.
 *
 * @see GetPlaybackOn, StartRecording, EndRecording, 
 *     StartPlayback, EndPlayback, GetModelviewMatrix
 */
inline FILE* SDLVU::GetPathFile() const
{
  return RecordPlaybackFP;
}

#endif /* SDLVU_H_ */
