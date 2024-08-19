#pragma once

#include "synth.h"
#include "raygui.h"
#include <string.h>
#include <stdio.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define UI_PANEL_WIDTH 350
#define WAVE_SHAPE_OPTIONS "None;Sine;Sawtooth;Square;Triangle;Rounded Square"
#define NUM_KEYS 12
#define BASE_SEMITONE 0 // A4 = 440 Hz

void draw_signal(float *signal);
void handle_keys(Synth *synth);
void draw_ui(Synth *synth);