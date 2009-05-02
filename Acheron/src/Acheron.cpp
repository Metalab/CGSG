//============================================================================
// Name        : Acheron.cpp
// Author      :
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <iostream>
#include "misc.h"

// Common OpenGL includes
#include <GL/glew.h>
#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
#endif
#include <SDL.h>

using namespace std;

// Shader utils
//#include "shader.h"

// Camera control:
#include "camera.h"
GLfloat rotation[4] = {0.0f, 0.0f, 0.0f, 0.0f};

#include "font.h"

#include "AnimateableObject.h"
#include "LinearInterpolatedAnimateable.h"

#include "Chronos.h"

#include "TimelineParseException.h"
#include "TimelineItemParser.h"
#include "TimelineReader.h"

#include "TextItemParser.h"
#include "effects/RingParser.h"
#include "effects/PhotoParser.h"

#include "Context.h"

Context *context;

Chronos *chronos;
TimelineReader *timelineReader;


void resizeGL(int w, int h)
{
	//  SDL_SetVideoMode(w, h, 16, SDL_OPENGL | SDL_RESIZABLE);
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(110.0f, 1.0f*w/h, 0.1f, 1000.0f);
}

void initGL(int w, int h)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glClearDepth(1.0f);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	glShadeModel(GL_SMOOTH);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	GLfloat lightpos[] = { 15.0f, 10.0f, 10.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);


	GLfloat mat_specular[] = { 0.1f, 0.8f, 0.4f, 1.0f };
	GLfloat mat_ambient[] = { 0.0f, 0.3f, 0.0f, 1.0f };
	GLfloat mat_diffuse[] = { 0.8f, 0.2f, 0.0f, 1.0f };

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialf(GL_FRONT, GL_SHININESS, 30.0f);

	resizeGL(w,h);
}

void printUsage(char *name)
{
	fprintf(stderr, "Usage: %s [options]\n\n", name);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -f              Run in fullscreen mode\n");
	fprintf(stderr, "  -m <samples=4>  Enable multisample\n");
	fprintf(stderr, "  -lm             Write LED matrix frames to stdout\n");
	exit(1);
}


int main( int argc, char ** argv ) {
	bool fullscreen = false;

	int multisample = 0;
	for (int i=1;i<argc;i++) {
		if (!strcmp(argv[i], "-f")) {
			fullscreen = true;
		}
		else if (!strcmp(argv[i], "-m")) {
			if (i==argc-1 || argv[i+1][0] == '-') {
				multisample = 4;
			}
			else if (i<argc-1) {
				multisample = atol(argv[++i]);
				if (multisample < 0) {
					fprintf(stderr, "%s: Illegal value option: '%s %s'\n\n",
							argv[0], argv[i-1], argv[i]);
					printUsage(argv[0]);
				}
			}
		}
		else if (argv[i][0] == '-') {
			printUsage(argv[0]);
		}
		else {
			printUsage(argv[0]);
		}
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return -1;
	}

	TTF_Init();

	if (multisample > 0) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisample);
	}

	int width = 1280, height = 1024;
	Uint32 flags = SDL_OPENGL;
	if (fullscreen) {
		const SDL_VideoInfo *info = SDL_GetVideoInfo();
		width = info->current_w;
		height = info->current_h;
		flags |= SDL_FULLSCREEN;
	}
	//  else flags |= SDL_RESIZABLE;
	SDL_Surface *surface;
	if (!(surface = SDL_SetVideoMode(width, height, 0, flags))) {
		fprintf(stderr, "Unable to set SDL video mode: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}
	if ((surface->flags & flags) != flags) {
		fprintf(stderr, "Warning: Flags couldn't be set: %x\n", (surface->flags&flags)^flags);
	}

	SDL_WM_SetCaption("OpenGL demo", NULL);

	glewInit();
	initGL(width, height);
	if (multisample > 0) {
		glEnable(GL_MULTISAMPLE);
	}


	//initialize the timeline reader
	timelineReader = new TimelineReader();

	timelineReader->SetParser( "TEXT", 	new TextItemParser(new Font("arialbd.ttf", 14)));
	timelineReader->SetParser( "RING", 	new multiKa::RingParser() );
	timelineReader->SetParser( "PHOTO", new multiKa::PhotoParser() );

	timelineReader->Open("timeline.txt");

	chronos = new Chronos();

	context = new Context();
	chronos->setContext( context );

	AnimateableObject *obj;
	bool done = false;

	while (!done && (!timelineReader->IsAtEnd() || chronos->HasObjects())) {

		unsigned int now = SDL_GetTicks();

		//read new objects
		while( (obj = timelineReader->GetNextObject(now, 1000)) ) {
			chronos->AddObject(obj);
		}

		//draw
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0, 0, -5.0);
		glRotatef(rotation[0], rotation[1], rotation[2], rotation[3]);
		glColor3f(1,0,0);

		context->update();

		chronos->Draw(now);

		glPushMatrix();
		//glLoadIdentity();

		float mat_shininess[] = { 100.0 };
		glMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess);

		float mat_ambient_diffuse[] = { 1.0, 0.0, 0.0 };
		float mat_specular[] = { 1.0, 0.0, 0.0 };
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_ambient_diffuse);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);

		glBegin(GL_LINES);
		glColor4f(0.0,0.0,0.0,0.5);
		glVertex3f(-20.0,0.0,0.0);
		glColor4f(1.0,0.0,0.0,1.0);
		glVertex3f(20.0,0.0,0.0);
		glEnd();

		float mat_ambient_diffuse2[] = { 0.0, 1.0, 0.0 };
		float mat_specular2[] = { 0.0, 1.0, 0.0 };
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_ambient_diffuse2);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular2);
		glBegin(GL_LINES);
		glColor4f(0.0,0.0,0.0,0.5);
		glVertex3f(0,-20,0);
		glColor4f(0.0,1.0,0.0,1.0);
		glVertex3f(0,20,0);
		glEnd();

		float mat_ambient_diffuse3[] = { 0.0, 0.0, 1.0 };
		float mat_specular3[] = { 0.0, 0.0, 1.0 };
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_ambient_diffuse3);
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular3);
		glBegin(GL_LINES);
		glColor4f(0.0,0.0,0.0,0.5);
		glVertex3f(0,0,-20);
		glColor4f(0.0,0.0,1.0,1.0);
		glVertex3f(0,0,20);
		glEnd();

		glPopMatrix();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					done = true;
				break;
				//       case SDL_VIDEORESIZE:
				//         width = event.resize.w;
				//         height = event.resize.h;
				//         resizeGL(width, height);
				//         break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1) {
						trackballInit(event.button.x, height-event.button.y, width, height);
					}
				break;
				case SDL_MOUSEMOTION:
					if (event.motion.state & SDL_BUTTON(1)) {
						trackballRotate(event.motion.x, height-event.motion.y, rotation);
					}
				break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							done = true;
						break;
						default:
						break;
					}
				break;
				case SDL_KEYUP:
				//        switch (event.key.keysym.sym) {
				//        }
				break;
			}
		}

		SDL_GL_SwapBuffers();
	}

	delete timelineReader;
	delete chronos;
	delete context;

	SDL_Quit();

	return 0;
}
