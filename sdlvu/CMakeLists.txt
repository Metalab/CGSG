project(SDLVU)

cmake_minimum_required(VERSION 2.6)

set (CMAKE_CXX_FLAGS "-g -Wall -Werror")

include (FindOpenGL)
include (FindSDL)

include_directories (${SDLVU_SOURCE_DIR}/viewer)
include_directories (${OPENGL_INCLUDE_DIR})
include_directories (${SDL_INCLUDE_DIR})

add_subdirectory(viewer)
add_subdirectory(examples EXCLUDE_FROM_ALL)

