cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)

project(glad)

find_package(OpenGL REQUIRED)

add_library(glad STATIC 
  src/glad.c
)

target_link_libraries(glad PUBLIC ${OPENGL_gl_LIBRARY})
target_include_directories(glad PUBLIC . src include)

add_library(glad::glad ALIAS glad)

if(UNIX AND NOT APPLE)
target_link_libraries(glad PUBLIC dl)
endif(UNIX AND NOT APPLE)