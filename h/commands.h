#pragma once
#include "synth.h"

typedef void (*command_fn)(char *);

typedef struct command_map{
    char *command;
    command_fn f;
} command_map;

void key_pressed(char* userdata);
void key_released(char* userdata);
command_fn find_function_by_command(const char *command);
void handle_keys(Synth *synth);