cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(tinysynth VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

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

FetchContent_Declare(
    raygui
    GIT_REPOSITORY "https://github.com/raysan5/raygui.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raygui)


add_executable(tinysynth
    src/main.c
    src/oscillator.c
    src/synth.c
    3rdparty/hash.c
    src/yaml.c
    src/ui.c
)

target_include_directories(tinysynth PRIVATE
    h
    ${raylib_SOURCE_DIR}/src
    ${raygui_SOURCE_DIR}/src
    3rdparty
)

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(tinysynth m raylib)
else()
    target_link_libraries(tinysynth m raylib winmm)
endif()

target_compile_options(tinysynth PRIVATE -Wall -Wextra -pedantic -g)