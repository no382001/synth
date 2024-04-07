
cmake_minimum_required(VERSION 3.5)
project(synth)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")

include_directories(
    res/SDL2/include)

link_directories(
    res/SDL2/bin
    res/SDL2/lib
)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
    src/utils.cpp)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    SDL2)