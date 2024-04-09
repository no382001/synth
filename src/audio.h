#pragma once

#include <stdint.h>
#include <vector>
#include "utils.h"

extern std::vector<oscillator*> oscv;
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


void init_sine_wave(std::vector<atomic_sample>& atomics){
    if (!atomics.empty()){
        atomics.clear();
    }

    const int n = 2;
    atomics.reserve(n);

    // for 50 cycles fill the buffer with a continuous sine wave 
    for (int i = 0; i < n; i++) {
        atomic_sample as = {512, new float[512]};
        for (int j = 0; j < 512; j++) {
            as.buffer[j] = next(oscv[0]);
        }
        atomics.push_back(as);
    }
}