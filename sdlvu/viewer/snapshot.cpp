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
// snapshot.cpp : TAKES A "SNAPSHOT" OF THE WINDOW (WRITES THE FRAMEBUFFER
// TO A .PPM FILE)
//============================================================================

// SDL port by Karin Kosina (vka kyrah) 20080717.

#include <stdio.h>

#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif
#include <SDL.h>

void SnapShot(void)
{
  FILE *fp;

  // GENERATE A UNIQUE FILENAME
  char FileName[20];
  static int SnapShotNum=100;
  int UniqueFound=0;  
  do { sprintf(FileName,"snap%d.ppm",SnapShotNum);
    if ((fp=fopen(FileName,"r"))) fclose(fp); else UniqueFound=1;
       SnapShotNum++;
     } while(!UniqueFound);

  GLint OldReadBuffer;
  glGetIntegerv(GL_READ_BUFFER,&OldReadBuffer);
  glReadBuffer(GL_FRONT);

  GLint OldPackAlignment;
  glGetIntegerv(GL_PACK_ALIGNMENT,&OldPackAlignment); 
  glPixelStorei(GL_PACK_ALIGNMENT,1);

  const SDL_VideoInfo *info = SDL_GetVideoInfo();
  int WW = info->current_w;
  int WH = info->current_h;
  int NumPixels = WW*WH;
  GLubyte* Pixels = new GLubyte[NumPixels*3];
  if (Pixels==NULL) { printf("UNABLE TO ALLOC PIXEL READ ARRAY!\n"); return; }
  glReadPixels(0,0,WW,WH,GL_RGB,GL_UNSIGNED_BYTE,Pixels);

  fp = fopen(FileName, "wb");
  fprintf(fp, "P6\n%d %d\n255\n", WW, WH);
  fwrite(Pixels,1,NumPixels*3,fp);
  fclose(fp);
  delete[] Pixels;

  glPixelStorei(GL_PACK_ALIGNMENT,OldPackAlignment);
  glReadBuffer((GLenum)OldReadBuffer);
}
