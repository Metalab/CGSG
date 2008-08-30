#LIBS = -lcv -lhighgui

#  my (borken) OS X 
CFLAGS = `sdl-config --cflags` -I/opt/local/include -framework OpenGL -lglut -I../../../sdlvu/viewer/ -I$(HOME)/Downloads/rtaudio-4.0.4 -I../../../../user/scripty/vorbisFFT/
LIBS = -L../../../sdlvu/viewer/ -L/opt/local/lib `pkg-config opencv --libs` -lsdlvu `sdl-config --libs` -lSDL_image -lGLEW -lportaudio -lvorbisfile -L$(HOME)/Downloads/rtaudio-4.0.4 -l rtaudio -framework CoreAudio -lfftw3


%:%.cpp
	g++ map.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp $^ $(CFLAGS) $(LIBS) -o $@

mapview:mapview.cpp map.cpp
	g++ mapview.cpp map.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview: fftmapview.cpp
	g++ map.cpp fftmapview.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview_live: fftmapview_live.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp
	g++ fftmapview_live.cpp map.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp $(LIBS) $(CFLAGS) -o $@

