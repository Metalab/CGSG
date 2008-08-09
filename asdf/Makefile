LIBS = -L../sdlvu/build/viewer -lcv -lhighgui `sdl-config --libs` -lsdlvu -lGL -lGLU -lGLEW
CFLAGS = `sdl-config --cflags` -O2
INCLUDES = -I../sdlvu/viewer

%:%.cpp
	g++ $^ $(LIBS) $(CFLAGS) -o $@

mapview:mapview.cpp map.cpp
	g++ mapview.cpp map.cpp $(LIBS) $(CFLAGS) $(INCLUDES) -o $@
