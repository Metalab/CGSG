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
// snapshot.h : TAKES A "SNAPSHOT" OF THE WINDOW (WRITES THE FRAMEBUFFER
// TO A .PPM FILE)
//============================================================================

// SDL port by Karin Kosina (vka kyrah) 20080717.

#ifndef SDLVU_SNAPSHOT_H_ 
#define SDLVU_SNAPSHOT_H_ 

/**
 * Writes a snapshot of the current screen in ppm format. 
 * The file written is a file in the current working directory 
 * with a name of the form "snap*.ppm".  Each time SnapShot is 
 * called a search is performed to find a name of that for that 
 * doesn't already exist, and that is what is written to.
 *
 * Implemented using glReadPixels.
 */
void SnapShot(void);

#endif // SDLVU_SNAPSHOT_H_ 
