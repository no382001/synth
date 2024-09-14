#pragma once
#include "oscillator.h"

#define SAMPLE_RATE 44100
#define NUM_KEYS 12
#define BASE_SEMITONE 0 // A4 = 440 Hz

typedef struct Synth {
  OscillatorArray keyOscillators;
  float *signal;
  size_t signal_length;
  float audio_frame_duration;
  float delta_time_last_frame;
} Synth;

void zeroSignal(float *signal);

void updateOscArray(WaveShapeFn base_osc_shape_fn, Synth *synth,
                    OscillatorArray osc_array);