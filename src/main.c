#include "assert.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_IMPLEMENTATION
#include "yaml.h"

#include "synth.h"
#include "ui.h"

ADSR defaultEnvelope = {.attack_time = ENVELOPE_DEFAULT_ATTACK_TIME,
                               .decay_time = ENVELOPE_DEFAULT_DECAY_TIME,
                               .sustain_level = ENVELOPE_DEFAULT_SUSTAIN_LEVEL,
                               .release_time = ENVELOPE_DEFAULT_RELEASE_TIME,
                               .sustain_time = ENVELOPE_DEFAULT_SUSTAIN_TIME,
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
  hash_get_and_set_float(config,"envelope.sustain_time",&defaultEnvelope.sustain_time);
  hash_get_and_set_float(config,"envelope.release_time",&defaultEnvelope.release_time);

  hash_free(config);
  config = NULL;
}

Vector2 ADSR_points[ADSR_VIZ_NUM_POINTS];
extern Rectangle ui_adsr_pos;
int main() {
  load_config();

  float total_time = defaultEnvelope.attack_time + defaultEnvelope.decay_time + defaultEnvelope.release_time + defaultEnvelope.sustain_time;
  generateADSRPoints(defaultEnvelope, ADSR_points, ADSR_VIZ_NUM_POINTS, total_time, ui_adsr_pos);

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

  Oscillator keyOscillators[NUM_KEYS] = {0};
  float signal[STREAM_BUFFER_SIZE] = {0};

  Synth synth = {.keyOscillators = {.osc = keyOscillators, .count = 0},
                 .signal = signal,
                 .signal_length = STREAM_BUFFER_SIZE,
                 .audio_frame_duration = 0.0f};

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
    
    EndDrawing();
  }

  UnloadAudioStream(synth_stream);
  CloseAudioDevice();
  CloseWindow();

  return 0;
}