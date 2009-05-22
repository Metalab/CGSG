#  my (borken) OS X 
# make sure glew via macports is up2date! (the linker will love you)
CFLAGS = -Wall -ggdb `sdl-config --cflags` -framework OpenGL -I/opt/local/include -I/Developer/FMOD\ Programmers\ API/api/inc
LIBS = -L/opt/local/lib `sdl-config --libs` -lSDL_image -framework CoreAudio -L/Developer/FMOD\ Programmers\ API/api/lib -lfmodex -lSDL_ttf -lglew `pkg-config opencv --libs`


#g++ src/*.cpp src/effects/*.cpp src/effects/Asdf/*.cpp src/effects/zoom/*.cpp $^ $(CFLAGS) $(LIBS) -o workingDir/acheron
#%:%.cpp
all:
	g++ src/*.cpp src/effects/*.cpp src/effects/*/*.cpp $^ $(CFLAGS) $(LIBS) -o workingDir/acheron


obj:
	g++ src/*.cpp src/effects/*.cpp src/effects/Asdf/*.cpp $^ $(CFLAGS) -c
