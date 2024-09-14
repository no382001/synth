cmake_minimum_required(VERSION 3.10)

project(tinysynth VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(tinysynth
    src/main.c
    src/oscillator.c
    src/synth.c
    3rdparty/hash.c
    src/yaml.c
    src/utils.c
    src/networking.c
    src/commands.c
)

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_compile_definitions(SOURCE_PATH_SIZE=${SOURCE_PATH_SIZE})

target_include_directories(tinysynth PRIVATE
    /usr/include/tcl8.6
    h
    3rdparty
)

target_link_libraries(tinysynth m ao tcl tk)

target_compile_options(tinysynth PRIVATE -Wall -Wextra -pedantic -g)