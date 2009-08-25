#LIBS = -lcv -lhighgui

SDLVUDIR=../../../sdlvu/viewer/

#  my (borken) OS X 
# for egypt callgraph generation use:
#  -dr in CFLAGS 
# print callgraph with: ./egypt --include-external ../*.00.expand | /Applications/Graphviz.app/Contents/MacOS/dot -Tps -o callgraph.ps -Gpage=40,40 -Gsize=40,8 -Grankdir=LR -s36
myCFLAGS = `sdl-config --cflags` -I/opt/local/include -framework OpenGL -lglut -I$(SDLVUDIR) -I/Developer/FMOD\ Programmers\ API/api/inc -Ioscpack/

#performance test build, x86-64bit
# -mtune=nocona
#CFLAGS = -O2 -mtune=nocona $(myCFLAGS)
#std with debugging etc
CFLAGS = -O2 -dr -ggdb $(myCFLAGS)

LIBS = -L$(SDLVUDIR) -L/opt/local/lib `pkg-config opencv --libs` -lsdlvu `sdl-config --libs` -lSDL_image  -framework CoreAudio -L/Developer/FMOD\ Programmers\ API/api/lib -lfmodex -lSDL_console -Loscpack/ -loscpack.so.1.0.2

%:%.cpp
	g++ map.cpp $^ $(CFLAGS) $(LIBS) -o $@

osc_test: osc_test.cpp
	g++ oscReceive.cpp oscmsgqueue.cpp $^ $(CFLAGS) $(LIBS) -o $@

event_osc_test: event_osc_test.cpp
	g++ oscReceive.cpp oscmsgqueue.cpp asdf_eventhandler.cpp $^ $(CFLAGS) $(LIBS) -o $@

asdf: augenkrach.cpp
	g++ map_augenkrach.cpp oscReceive.cpp oscmsgqueue.cpp asdf_eventhandler.cpp $^ $(CFLAGS) $(LIBS) -o $@
		
augenkrach: augenkrach.cpp
	g++ map_augenkrach.cpp krach_console.c split.c oscReceive.cpp oscmsgqueue.cpp asdf_eventhandler.cpp $^ $(CFLAGS) $(LIBS) -o $@

mapview:mapview.cpp map.cpp
	g++ mapview.cpp map.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview: fftmapview.cpp
	g++ map.cpp fftmapview.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp $(LIBS) $(CFLAGS) -o $@

fftmapview_live: fftmapview_live.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp
	g++ fftmapview_live.cpp map.cpp ../../../../user/scripty/vorbisFFT/vorbis_fft.cpp $(LIBS) $(CFLAGS) -o $@

