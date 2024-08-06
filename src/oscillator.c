#include "oscillator.h"
#include "math.h"

Oscillator *makeOscillator(OscillatorArray *oscArray) {
  return oscArray->osc + (oscArray->count++);
}

void clearOscillatorArray(OscillatorArray *oscArray) {
  oscArray->count = 0;
}

static float getFrequencyForSemitone(float semitone) {
  // fn = 2^(n/12) × 440 Hz
  return powf(2.f, semitone / 12.f) * BASE_NOTE_FREQ;
}

static float getSemitoneForFrequency(float freq) {
  // n = 12 × log2 (fn / 440 Hz).
  return 12.f * log2f(freq / BASE_NOTE_FREQ);
}

void updateOsc(Oscillator *osc, float freq_modulation) {
  osc->phase_dt = ((osc->freq + freq_modulation) * SAMPLE_DURATION);
  osc->phase += osc->phase_dt;
  if (osc->phase < 0.0f)
    osc->phase += 1.0f;
  if (osc->phase >= 1.0f)
    osc->phase -= 1.0f;
}

float bandlimitedRipple(float phase, float phase_dt) {
  if (phase < phase_dt) {
    phase /= phase_dt;
    return (phase + phase) - (phase * phase) - 1.0f;
  } else if (phase > 1.0f - phase_dt) {
    phase = (phase - 1.0f) / phase_dt;
    return (phase * phase) + (phase + phase) + 1.0f;
  } else
    return 0.0f;
}

// float (*WaveShapeFn)(const Oscillator)
float sineShape(const Oscillator osc) { return sinf(2.f * PI * osc.phase); }

// float (*WaveShapeFn)(const Oscillator)
float sawtoothShape(const Oscillator osc) {
  float sample = (osc.phase * 2.0f) - 1.0f;
  sample -= bandlimitedRipple(osc.phase, osc.phase_dt);
  return sample;
}

// float (*WaveShapeFn)(const Oscillator)
float triangleShape(const Oscillator osc) {
  // TODO: Make this band-limited.
  if (osc.phase < 0.5f)
    return (osc.phase * 4.0f) - 1.0f;
  else
    return (osc.phase * -4.0f) + 3.0f;
}

// float (*WaveShapeFn)(const Oscillator)
float squareShape(const Oscillator osc) {
  float duty_cycle = osc.shape_parameter_0;
  float sample = (osc.phase < duty_cycle) ? 1.0f : -1.0f;
  sample += bandlimitedRipple(osc.phase, osc.phase_dt);
  sample -= bandlimitedRipple(fmodf(osc.phase + (1.f - duty_cycle), 1.0f),
                              osc.phase_dt);
  return sample;
}

// float (*WaveShapeFn)(const Oscillator)
float roundedSquareShape(const Oscillator osc) {
  float s = (osc.shape_parameter_0 * 8.f) + 2.f;
  float base = (float)fabs(s);
  float power = s * sinf(osc.phase * PI * 2);
  float denominator = powf(base, power) + 1.f;
  float sample = (2.f / denominator) - 1.f;
  return sample;
}