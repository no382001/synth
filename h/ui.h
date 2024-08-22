#pragma once

#include "raygui.h"
#include "synth.h"
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

#define UI_PANEL_WIDTH 350
#define UI_PADDING 16
#define UI_ADSR_PANE_WIDTH 350
#define UI_ADSR_PANE_HEIGHT 200

#define UI_VIZ_PANE_WIDTH 610
#define UI_VIZ_PANE_HEIGHT 200

#define NUM_KEYS 12
#define BASE_SEMITONE 0 // A4 = 440 Hz

#define ADSR_VIZ_NUM_POINTS 200

void draw_signal(float *signal, Rectangle area);
void handle_keys(Synth *synth);
void draw_ui(Synth *synth);
void generateADSRPoints(ADSR envelope, Vector2 *points, int num_points,
                        float total_time, Rectangle area);