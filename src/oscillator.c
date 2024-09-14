#include "oscillator.h"
#include <math.h>

Oscillator *makeOscillator(OscillatorArray *oscArray) {
  // add a new osc to the array
  return oscArray->osc + (oscArray->count++);
}

void clearOscillatorArray(OscillatorArray *oscArray) {
  // reset the array
  oscArray->count = 0;
}

void updateOsc(Oscillator *osc, float freq_modulation) {
  // calc the phase increment for each sample based on the osc freq
  osc->phase_dt = ((osc->freq + freq_modulation) * SAMPLE_DURATION);
  // increment the phase
  osc->phase += osc->phase_dt;
  // wrap if it goes over
  if (osc->phase < 0.0f)
    osc->phase += 1.0f;
  if (osc->phase >= 1.0f)
    osc->phase -= 1.0f;
}

// generates a band-limited ripple to reduce aliasing in waveform generation
float bandlimitedRipple(float phase, float phase_dt) {
  // if phase is within the phase increment (start of the waveform cycle)
  if (phase < phase_dt) {
    // normalize phase to be within the range [0, 1]
    phase /= phase_dt;
    // calculate and return the ripple value for the start of the waveform cycle
    // the formula creates a smooth transition, quadratic + linear terms
    return (phase + phase) - (phase * phase) - 1.0f;

    // if phase is within the last phase increment (end of the waveform cycle)
  } else if (phase > 1.0f - phase_dt) {
    // normalize phase to be within the range [0, 1], but for the end of the
    // waveform cycle
    phase = (phase - 1.0f) / phase_dt;

    return (phase * phase) + (phase + phase) + 1.0f;

    // if phase is not within the first or last phase increment, no ripple is
    // needed
  } else {
    return 0.0f;
  }
}

// float (*WaveShapeFn)(const Oscillator)
float sineShape(const Oscillator osc) { return sinf(2.f * M_PI * osc.phase); }

// float (*WaveShapeFn)(const Oscillator)
float sawtoothShape(const Oscillator osc) {
  float sample = (osc.phase * 2.0f) - 1.0f;
  sample -= bandlimitedRipple(osc.phase, osc.phase_dt);
  return sample;
}

// float (*WaveShapeFn)(const Oscillator)
float squareShape(const Oscillator osc) {
  // the duty cycle is a parameter that defines the proportion of the waveform
  // period
  float duty_cycle = osc.shape_parameter_0;
  // determine the base sample value based on the phase and duty cycle
  // if the phase is less than the duty cycle, the sample is 1.0 (high part of
  // the square wave) otherwise, the sample is -1.0 (low part of the square
  // wave)
  float sample = (osc.phase < duty_cycle) ? 1.0f : -1.0f;

  // apply band limited ripple to the current phase
  sample += bandlimitedRipple(osc.phase, osc.phase_dt);

  // calculate the adjusted phase for the end of the duty cycle
  // `fmodf` ensures the phase wraps around correctly to stay within the [0, 1)
  // range
  sample -= bandlimitedRipple(fmodf(osc.phase + (1.f - duty_cycle), 1.0f),
                              osc.phase_dt);
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
float roundedSquareShape(const Oscillator osc) {
  // controls the waveform's rounding and sharpness
  float s = (osc.shape_parameter_0 * 8.f) + 2.f;

  float base = (float)fabs(s);

  // 'osc.phase * PI * 2' converts the phase to radians (0 to 2π)
  // 'sinf(osc.phase * PI * 2)' produces a value that oscillates between -1 and
  // 1 multiplying by 's' scales the oscillation
  float power = s * sinf(osc.phase * M_PI * 2);

  float denominator = powf(base, power) + 1.f;

  // '2.f / denominator' scales the value to be between 0 and 2
  // subtracting 1 shifts the range to be between -1 and 1
  float sample = (2.f / denominator) - 1.f;

  return sample;
}

// this function advances the state machine of the envelope
// the envelope is only used by the kb oscillators, pre initialized in main
// they gain amplitude and state when pressed, and release state is set if released
// TODO: refactoring
// there is no point in having an adsr for each oscillator, make a global one
void updateADSR(ADSR *envelope, float delta_time) {
  switch (envelope->state) {
  case OFF:
    break;
  case ATTACK:
    envelope->current_level += delta_time / envelope->attack_time;
    if (envelope->current_level >= 1.0f) {
      envelope->current_level = 1.0f;
      envelope->state = DECAY;
    }
    break;
  case DECAY:
    envelope->current_level -=
        delta_time / envelope->decay_time * (1.0f - envelope->sustain_level);
    if (envelope->current_level <= envelope->sustain_level) {
      envelope->current_level = envelope->sustain_level;
      envelope->state = SUSTAIN;
      envelope->sustain_time_elapsed = 0.0f;
    }
    break;
  case SUSTAIN:
    envelope->current_level = envelope->sustain_level;
    envelope->sustain_time_elapsed += delta_time;
    if (envelope->sustain_time_elapsed >= envelope->sustain_time) {
        envelope->state = RELEASE;
    }
    break;
  case RELEASE:
    envelope->current_level -=
        delta_time / envelope->release_time * envelope->sustain_level;
    if (envelope->current_level <= 0.0f) {
      envelope->current_level = 0.0f;
      envelope->state = OFF;
    }
    break;
  }
}