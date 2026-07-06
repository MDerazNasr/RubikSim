# rwlls cmkae the min version it must have 
cmake_minimum_required(VERSION 3.20)
#project defines the project name.
#RubikSim is the project name.
#LANGUAGES CXX tells CMake this project uses C++.
#CXX is CMake’s label for C++.
project(RubikSim LANGUAGES CXX)

# makes code compile cpp 20
# 2 - cmake must make use of cpp20
# this diables compiler specifc lang extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# loads cmakes FetchContent module
include(FetchContent)
# describes a dependancy
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# means do not build documenatation 
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw glm)

find_package(OpenGL REQUIRED)

# creates executable target for RubikSim - a target is something cmake knows how to build - inside the brackets you list the source files that belong to that executable
add_executable(rubiksim
  src/main.cpp
  src/app/Application.cpp
  src/renderer/Mesh.cpp
)

# this tells the compiler where it can search for header files
# PRIVATE means this incklude path is only needed to build this target
target_include_directories(rubiksim PRIVATE src)

target_compile_definitions(rubiksim PRIVATE GL_SILENCE_DEPRECATION)
target_link_libraries(rubiksim PRIVATE glfw glm::glm OpenGL::GL)

#this is why we can write include... insetad of a longer path
