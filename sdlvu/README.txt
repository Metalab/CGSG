
                         S D L V U 


OpenGL/SDL viewer library, offering a bunch of nice convenience
features such as trackball-style navigation, camera path recording
and playback, the usual linear algebra classes etc. 

SDLVU is a straightforward SDL port of a subset of the GLVU set of
libraries [http://www.cs.unc.edu/~walk/software/glvu/index.html] 
created by the Walkthru group at UNC.

GLVU is (c) 1997-2002 The University of North Carolina at Chapel Hill,
SDL port and some minor fixes by Karin Kosina (vka kyrah) 

Personal note: Yes I also think it's horrible to have uppercase
function names, but I decided to keep this for the sake of easier
back-porting of my changes into the GLVU framework.

The CMake-based build system is work in progress and currently
probably won't work on anything other than Mac OS X. (It should be
easy to extend though, so feel free to adapt it to your platform.)

This is NOT production-quality code and there are a whole bunch of 
issues that need to be ironed out (most notably that the camera 
path playback is very jerky at times... plus the code needs some
major cleanup in general).

That said... it should be a nice starting point, so have fun with it!


Building:
---------

SDLVU uses the CMake build system generator [http://www.cmake.org],
so you need to install cmake before building. For Ubuntu 8.04 Hardy
use the backported 2.6 version of cmake obtainable from
https://code.launchpad.net/ubuntu/hardy/amd64/cmake/2.6.0-4ubuntu1~hardy1

Unix instructions:

> mkdir build
> cd build
> cmake ..
> make

This should build the SDLVU static library in the viewer subdirectory.


Examples:
----------

There are some very basic examples included in this directory that
demonstrate how you can use SDLVU. To build them go the examples/ 
subdirectory (in your build directory): 

> cd examples
> make

As the examples show, there are basically two ways to use SDLVU:
either by instantiating a viewer and setting callback functions 
(see e.g. example_basic.cpp) or by creating your own viewer
subclass and overriding the methods you want to customize (see
e.g. example_basic_oo.cpp).



Features:
----------

SDLVU contains a bunch of nice features, available through the
following default keyboard shortcuts:

z: navigation = trackball
h: navigation = hyperball; 
x: navigation = drive; 
c: navigation = translate; 
v: navigation = look; 
=: make snapshot
o: toggle in/out viewing mode
i: toggle inertia
d: dump current camera parameters
w: solid/lines/point
l: toggle lighting 
b: toggle backface culling
n: switch front/back face
m: toggle materials
r: start recording camera path
s: stop recording camera path
p: play back camera path
0: reset cameras
1: select camera 1
2: select camera 2
3: select camera 3
4: select camera 4
!: toggle camera 1 display
@: toggle camera 2 display
#: toggle camera 3 display
$: toggle camera 4 display


License:
--------

GLVU is licensed under a very libraral MIT-style license (meaning you
can basically do with it what you want as long as you don't remove
their copyright notice):

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
