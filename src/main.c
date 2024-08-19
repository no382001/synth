#include "assert.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_IMPLEMENTATION
#include "yaml.h"

#include "synth.h"
#include "ui.h"

static ADSR defaultEnvelope = {.attack_time = ENVELOPE_DEFAULT_ATTACK_TIME,
                               .decay_time = ENVELOPE_DEFAULT_DECAY_TIME,
                               .sustain_level = ENVELOPE_DEFAULT_SUSTAIN_LEVEL,
                               .release_time = ENVELOPE_DEFAULT_RELEASE_TIME,
                               .current_level = 0.0f,
                               .state = OFF};

static hash_t *config = NULL;

// takes a hash_t and if the value is proper sets it to the reference
static void hash_get_and_set_float(hash_t *h, char *cs, float* f) {
  char *str = hash_get(config, cs);
  char *eptr;

  float num = strtof(str, &eptr);

  if (*eptr == '\0') {
    *f = num;
  } else {
    TraceLog(LOG_ERROR, "%s could not be converted to float, ignoring",cs);
  }
}

void load_config() {
  config = yaml_read("conf/conf.yaml");
  if (!config) {
    TraceLog(LOG_ERROR, "conf/conf.yaml failed to load, using default values!");
    return;
  }
  hash_each(config, { TraceLog(LOG_INFO, "%s: %s\n", key, (char *)val); });

  hash_get_and_set_float(config,"envelope.attack_time",&defaultEnvelope.attack_time);
  hash_get_and_set_float(config,"envelope.decay_time",&defaultEnvelope.decay_time);
  hash_get_and_set_float(config,"envelope.sustain_level",&defaultEnvelope.sustain_level);
  hash_get_and_set_float(config,"envelope.release_time",&defaultEnvelope.release_time);
}

int main() {
  load_config();

  const int screen_width = 1024;
  const int screen_height = 768;
  InitWindow(screen_width, screen_height, "tins");
  SetTargetFPS(60);
  InitAudioDevice();

  GuiLoadStyle(".\\raygui\\styles\\jungle\\jungle.rgs");

  unsigned int sample_rate = SAMPLE_RATE;
  SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
  AudioStream synth_stream = LoadAudioStream(sample_rate, sizeof(float) * 8, 1);
  SetAudioStreamVolume(synth_stream, 0.01f);
  PlayAudioStream(synth_stream);

  Oscillator sineOsc[NUM_OSCILLATORS] = {0};
  Oscillator sawtoothOsc[NUM_OSCILLATORS] = {0};
  Oscillator triangleOsc[NUM_OSCILLATORS] = {0};
  Oscillator squareOsc[NUM_OSCILLATORS] = {0};
  Oscillator roundedSquareOsc[NUM_OSCILLATORS] = {0};
  Oscillator keyOscillators[NUM_OSCILLATORS] = {0};
  float signal[STREAM_BUFFER_SIZE] = {0};

  Synth synth = {.sineOsc = {.osc = sineOsc, .count = 0},
                 .sawtoothOsc = {.osc = sawtoothOsc, .count = 0},
                 .triangleOsc = {.osc = triangleOsc, .count = 0},
                 .squareOsc = {.osc = squareOsc, .count = 0},
                 .roundedSquareOsc = {.osc = roundedSquareOsc, .count = 0},
                 .keyOscillators = {.osc = keyOscillators, .count = 0},
                 .signal = signal,
                 .signal_length = STREAM_BUFFER_SIZE,
                 .audio_frame_duration = 0.0f,
                 .ui_oscillator_count = 0};

  // set oscillator amplitudes.
  for (size_t i = 0; i < NUM_OSCILLATORS; i++) {
    sineOsc[i].amplitude = 0.0f;
    sawtoothOsc[i].amplitude = 0.0f;
    triangleOsc[i].amplitude = 0.0f;
    squareOsc[i].amplitude = 0.0f;
    keyOscillators[i].amplitude = 0.0f;
  }

  for (size_t i = 0; i < NUM_KEYS; i++) { // make an osc for each key
    makeOscillator(&synth.keyOscillators);
    synth.keyOscillators.osc[i].envelope = defaultEnvelope;
  }

  while (!WindowShouldClose()) {
    synth.delta_time_last_frame = GetFrameTime();
    handleAudioStream(synth_stream, &synth);
    BeginDrawing();
    ClearBackground(BLACK);

    draw_ui(&synth);
    handle_keys(&synth);
    draw_signal(signal);

    const float total_frame_duration = GetFrameTime();
    DrawText(TextFormat("Frame time: %.3f%%, Audio budget: %.3f%%",
                        (100.0f / (total_frame_duration * 60.f)),
                        100.0f / ((1.0f / synth.audio_frame_duration) /
                                  ((float)SAMPLE_RATE / STREAM_BUFFER_SIZE))),
             UI_PANEL_WIDTH + 10, 10, 20, RED);
    EndDrawing();
  }

  UnloadAudioStream(synth_stream);
  CloseAudioDevice();
  CloseWindow();

  return 0;
}