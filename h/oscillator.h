#pragma once
#include "raylib.h"
#include "stddef.h"

#define SAMPLE_RATE 44100
#define SAMPLE_DURATION (1.0f / SAMPLE_RATE)
#define STREAM_BUFFER_SIZE 1024
#define NUM_OSCILLATORS 32
#define BASE_NOTE_FREQ 440

typedef enum WaveShape {
  WaveShape_NONE = 0,
  WaveShape_SINE = 1,
  WaveShape_SAWTOOTH = 2,
  WaveShape_SQUARE = 3,
  WaveShape_TRIANGLE = 4,
  WaveShape_ROUNDEDSQUARE = 5
} WaveShape;

typedef struct Oscillator {
  float phase;
  float phase_dt;
  float freq;
  float amplitude;
  float shape_parameter_0;
} Oscillator;

typedef struct OscillatorArray {
  Oscillator *osc;
  size_t count;
} OscillatorArray;

typedef float (*WaveShapeFn)(const Oscillator);

typedef struct UIOscillator {
  float freq;
  float amplitude;
  float shape_parameter_0;
  WaveShape shape;
  bool is_dropdown_open;
  Rectangle shape_dropdown_rect;
} UIOscillator;


Oscillator *makeOscillator(OscillatorArray *oscArray);
void clearOscillatorArray(OscillatorArray *oscArray);
static float getFrequencyForSemitone(float semitone);
static float getSemitoneForFrequency(float freq);
void updateOsc(Oscillator *osc, float freq_modulation);
float bandlimitedRipple(float phase, float phase_dt);

// float (*WaveShapeFn)(const Oscillator)
float sineShape(const Oscillator osc);
float sawtoothShape(const Oscillator osc);
float triangleShape(const Oscillator osc);
float squareShape(const Oscillator osc);
float roundedSquareShape(const Oscillator osc);