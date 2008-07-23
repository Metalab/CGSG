set (SOURCES sdlvu.cpp sdlvu_keyinput.cpp sdlvu_mouse.cpp
             sdlvu_camview.cpp snapshot.cpp trackball.c
             mat16fv.cpp vec3fv.cpp
             minmaxbox.cpp plane.cpp tri.cpp
             camdisplay.cpp camera.cpp camutils.cpp)
set (PUBLIC_HEADERS sdlvu.h
  mat16fv.hpp mat33.hpp mat33impl.hpp mat44.hpp mat44impl.hpp 
  vec2f.hpp vec3f.hpp vec3fv.hpp vec4f.hpp quat.hpp quatimpl.hpp
  minmaxbox.hpp plane.hpp tri.hpp
  camera.hpp camutils.hpp)

add_library(sdlvu STATIC ${SOURCES})

install (TARGETS sdlvu DESTINATION lib)
install (FILES ${PUBLIC_HEADERS} DESTINATION include)
