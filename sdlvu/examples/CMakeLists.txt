include_directories(${CMAKE_INSTALL_PREFIX}/include/)

set(SDLVU_LIBS ${SDLVU_BINARY_DIR}/viewer/libsdlvu.a)

add_executable(example_basic example_basic.cpp)
target_link_libraries (example_basic ${SDLVU_LIBS} ${SDL_LIBRARY} ${OPENGL_LIBRARIES})

add_executable(example_keymouse example_keymouse.cpp)
target_link_libraries (example_keymouse ${SDLVU_LIBS} ${SDL_LIBRARY} ${OPENGL_LIBRARIES})

add_executable(example_keymouse_oo example_keymouse_oo.cpp)
target_link_libraries (example_keymouse_oo ${SDLVU_LIBS} ${SDL_LIBRARY} ${OPENGL_LIBRARIES})

add_executable(example_basic_oo example_basic_oo.cpp)
target_link_libraries (example_basic_oo ${SDLVU_LIBS} ${SDL_LIBRARY} ${OPENGL_LIBRARIES})

