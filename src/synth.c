#include "synth.h"
#include "oscillator.h"

void zeroSignal(float *signal) {
  for (size_t t = 0; t < STREAM_BUFFER_SIZE; t++) {
    signal[t] = 0.0f;
  }
}

void updateOscArray(WaveShapeFn base_osc_shape_fn, Synth *synth,
                    OscillatorArray osc_array) {
  for (size_t i = 0; i < osc_array.count; i++) {
    // skip oscillators with frequencies outside the Nyquist limit
    if (osc_array.osc[i].freq > (SAMPLE_RATE / 2) ||
        osc_array.osc[i].freq < -(SAMPLE_RATE / 2))
      continue;
    for (size_t t = 0; t < STREAM_BUFFER_SIZE; t++) {
      if (osc_array.osc[i].envelope.state == OFF)
        continue; // ignore if adsr is off

      updateADSR(&osc_array.osc[i].envelope, synth->delta_time_last_frame);

      updateOsc(&osc_array.osc[i], 0.0f);
      // generate the waveform sample and accumulate it into the signal buffer
      synth->signal[t] += base_osc_shape_fn(osc_array.osc[i]) *
                          osc_array.osc[i].amplitude *
                          osc_array.osc[i].envelope.current_level;
    }
  }
}