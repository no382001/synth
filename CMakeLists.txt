
cmake_minimum_required(VERSION 3.5)
project(synth)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")

include_directories(
    h
    src/visuals/h)

# Adding Raylib
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raylib)

add_subdirectory(src/visuals)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)

target_include_directories(
    ${PROJECT_NAME} PRIVATE
    h)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    raylib
    fftw3
    visuals)