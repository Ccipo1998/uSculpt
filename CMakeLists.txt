cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(usculpt VERSION 0.1.0)

# set global variable to override runtime output directory of all the libs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(usculpt
${CMAKE_SOURCE_DIR}/uSculpt.cpp)
target_include_directories(usculpt PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/assimp/include
    ${CMAKE_SOURCE_DIR}/libs/glad/include/
    ${CMAKE_SOURCE_DIR}/libs/glfw/
    ${CMAKE_SOURCE_DIR}/libs/glm/
    ${CMAKE_SOURCE_DIR}/libs/imgui/
    ${CMAKE_SOURCE_DIR}/libs/imgui/imgui/
    ${CMAKE_SOURCE_DIR}/include/)
set_target_properties(usculpt
PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_subdirectory(libs/assimp)
add_subdirectory(libs/glad)
add_subdirectory(libs/glfw)
add_subdirectory(libs/glm/glm/)
add_subdirectory(libs/imgui)
target_link_libraries(usculpt
    PRIVATE
    assimp
    glad
    glfw
    glm
    imgui)
