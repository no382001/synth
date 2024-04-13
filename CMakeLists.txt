
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

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp)

target_include_directories(
    ${PROJECT_NAME} PRIVATE
    h)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    SDL2
    SDL2_ttf
    pthread)