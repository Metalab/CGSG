#LIBS = -lcv -lhighgui

#  my (borken) OS X 
CFLAGS = -O2 -ggdb `sdl-config --cflags` -I/opt/local/include -framework OpenGL -lglut -I../../../sdlvu/viewer/ -I/Developer/FMOD\ Programmers\ API/api/inc -I/Users/scriptythekid/Downloads/portmidi/pm_common/ -Ioscpack/
LIBS = -L../../../sdlvu/viewer/ -L/opt/local/lib `pkg-config opencv --libs` -lsdlvu `sdl-config --libs` -lSDL_image  -framework CoreAudio -lfftw3 -L/Developer/FMOD\ Programmers\ API/api/lib -lfmodex -lSDL_console -Loscpack/ -loscpack.so.1.0.2


%:%.cpp
	g++ map.cpp $^ $(CFLAGS) $(LIBS) -o $@

osc_test: osc_test.cpp
	g++ oscReceive.cpp oscmsgqueue.cpp $^ $(CFLAGS) $(LIBS) -o $@

augenkrach: augenkrach.cpp
	g++ map_augenkrach.cpp krach_console.c split.c oscReceive.cpp $^ $(CFLAGS) $(LIBS) -o $@

mapview:mapview.cpp map.cpp
	g++ mapview.cpp map.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview: fftmapview.cpp
	g++ map.cpp fftmapview.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview_live: fftmapview_live.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp
	g++ fftmapview_live.cpp map.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp $(LIBS) $(CFLAGS) -o $@

