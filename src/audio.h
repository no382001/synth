#pragma once

#ifdef WIN32
#include "SDL_audio.h"
#include "SDL_timer.h"
#include <SDL.h>
#else
#include "SDL2/SDL_audio.h"
#include "SDL2/SDL_timer.h"
#include <SDL2/SDL.h>
#endif

#include <stdint.h>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <complex>
#include <mutex>
#include <errno.h>
#include <stdbool.h>
#include <iostream>
#include <vector>
#include <numeric>


typedef struct {
  float current_step;
  float step_size;
  float volume;
} oscillator;

float next(oscillator *os);


// an atomic part of the audio sample
// it should be the same length as the AudioSpec callback provided buffer size
struct atomic_sample {
    int size;
    float* buffer; // the audio thread will convert it into uint8_t
};

// should this be the same size as the size of the audio buffer?
// - no but it needs to be 0 inited, and the mixer should now not to average anymore,
//   so maybe its easier if it had to contain a fux len non null buffer
void init_sine_wave(std::vector<atomic_sample>& atomics);