#  my (borken) OS X 
CFLAGS = -O2 -ggdb `sdl-config --cflags` -I/opt/local/include -framework OpenGL -lglut -I/Developer/FMOD\ Programmers\ API/api/inc
LIBS = -L/opt/local/lib `sdl-config --libs` -lSDL_image  -framework CoreAudio -L/Developer/FMOD\ Programmers\ API/api/lib -lfmodex -lSDL_ttf -lglew


#%:%.cpp
all:
	g++ src/*.cpp src/effects/*.cpp $^ $(CFLAGS) $(LIBS) -o workingDir/acheron
