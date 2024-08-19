#pragma once
#include "math.h"
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

typedef enum ADSR_state_t {
  OFF = 0,
  ATTACK,
  DECAY,
  SUSTAIN,
  RELEASE
} ADSR_state_t;

typedef struct ADSR {
  ADSR_state_t state;
  float attack_time;
  float decay_time;
  float release_time;
  float sustain_level;
  float sustain_time;
  float sustain_time_elapsed;
  float current_level;
} ADSR;

typedef struct Oscillator {
  float phase;
  float phase_dt;
  float freq;
  float amplitude;
  float shape_parameter_0;
  ADSR envelope;
} Oscillator;

typedef struct OscillatorArray {
  Oscillator *osc;
  size_t count;
} OscillatorArray;

typedef struct UIOscillator {
  float freq;
  float amplitude;
  float shape_parameter_0;
  WaveShape shape;
  bool is_dropdown_open;
  Rectangle shape_dropdown_rect;
} UIOscillator; // used by the gui, every frame the main loop creates an
                // oscillator based on this

Oscillator *makeOscillator(OscillatorArray *oscArray);
void updateOsc(Oscillator *osc, float freq_modulation);
void updateADSR(ADSR *envelope, float delta_time);
void clearOscillatorArray(OscillatorArray *oscArray);
float bandlimitedRipple(float phase, float phase_dt);

typedef float (*WaveShapeFn)(const Oscillator);

// float (*WaveShapeFn)(const Oscillator)
float sineShape(const Oscillator osc);
float sawtoothShape(const Oscillator osc);
float triangleShape(const Oscillator osc);
float squareShape(const Oscillator osc);
float roundedSquareShape(const Oscillator osc);

static float getFrequencyForSemitone(float semitone) {
  // fn = 2^(n/12) × 440 Hz
  // 2^(n/12) <-- semitone value to a freq ratio
  return powf(2.f, semitone / 12.f) * BASE_NOTE_FREQ;
}

static float getSemitoneForFrequency(float freq) {
  // n = 12 × log2 (fn / 440 Hz).
  return 12.f * log2f(freq / BASE_NOTE_FREQ);
}

#define ENVELOPE_DEFAULT_ATTACK_TIME (0.1f * 5) // 100 ms
#define ENVELOPE_DEFAULT_DECAY_TIME (0.2f * 5)  // 200 ms
#define ENVELOPE_DEFAULT_SUSTAIN_LEVEL 0.7f // 70% of the peak amplitude
#define ENVELOPE_DEFAULT_SUSTAIN_TIME (0.3f * 100)
#define ENVELOPE_DEFAULT_RELEASE_TIME (0.3f * 100) // 300 ms