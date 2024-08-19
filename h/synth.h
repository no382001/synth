#pragma once
#include "oscillator.h"

#define MAX_UI_OSCILLATORS 32

typedef struct Synth {
  OscillatorArray sineOsc;
  OscillatorArray sawtoothOsc;
  OscillatorArray triangleOsc;
  OscillatorArray squareOsc;
  OscillatorArray roundedSquareOsc;
  OscillatorArray keyOscillators;
  float *signal;
  size_t signal_length;
  float audio_frame_duration;
  float delta_time_last_frame;

  UIOscillator ui_oscillator[MAX_UI_OSCILLATORS];
  size_t ui_oscillator_count;
} Synth;

void zeroSignal(float *signal);

void updateOscArray(WaveShapeFn base_osc_shape_fn, Synth *synth,
                    OscillatorArray osc_array);
void handleAudioStream(AudioStream stream, Synth *synth);