#pragma once
#include "SDL2/SDL_audio.h"
#include "SDL2/SDL_timer.h"
#include <SDL2/SDL.h>

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