cmake_minimum_required(VERSION 3.5.0)

project(imgui)

add_library(imgui STATIC 
  imgui/imgui_draw.cpp
  imgui/imgui_impl_glfw.cpp
  imgui/imgui_impl_opengl3.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui.cpp
)

# include directories inside imgui sources files
target_include_directories(imgui PRIVATE
  ${CMAKE_SOURCE_DIR}/libs/imgui/imgui/
  ${CMAKE_SOURCE_DIR}/libs/glfw/
)

target_link_libraries(imgui PUBLIC
    glfw)
