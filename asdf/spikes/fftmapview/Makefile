#LIBS = -lcv -lhighgui

#  my (borken) OS X 
CFLAGS = `sdl-config --cflags` -I/opt/local/include -framework OpenGL -lglut -I../../../sdlvu/viewer/ -I$(HOME)/Downloads/rtaudio-4.0.4
LIBS = -L../../../sdlvu/viewer/ -L/opt/local/lib `pkg-config opencv --libs` -lsdlvu `sdl-config --libs` -lSDL_image -lGLEW -lportaudio -lvorbisfile -L$(HOME)/Downloads/rtaudio-4.0.4 -l rtaudio -framework CoreAudio -lfftw3


%:%.cpp
	g++ $^ $(CFLAGS) $(LIBS) -o $@

mapview:mapview.cpp map.cpp
	g++ mapview.cpp map.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview: metafft.cpp map.cpp vorbis_fft.cpp fftmapview.cpp
	g++ metafft.cpp map.cpp vorbis_fft.cpp fftmapview.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview_live: metafft.cpp fftmapview_live.cpp map.cpp vorbis_fft.cpp
	g++ metafft.cpp fftmapview_live.cpp map.cpp vorbis_fft.cpp $(LIBS) $(CFLAGS) -o $@

