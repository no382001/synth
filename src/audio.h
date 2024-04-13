#pragma once

#include <stdint.h>
#include <vector>
#include "utils.h"

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