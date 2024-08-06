#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define SAMPLE_RATE 44100
#define SAMPLE_DURATION (1.0f / SAMPLE_RATE)
#define STREAM_BUFFER_SIZE 1024
#define NUM_OSCILLATORS 32
#define MAX_UI_OSCILLATORS 32
#define UI_PANEL_WIDTH 350
#define BASE_NOTE_FREQ 440

#define WAVE_SHAPE_OPTIONS "None;Sine;Sawtooth;Square;Triangle;Rounded Square"

typedef enum WaveShape {
  WaveShape_NONE = 0,
  WaveShape_SINE = 1,
  WaveShape_SAWTOOTH = 2,
  WaveShape_SQUARE = 3,
  WaveShape_TRIANGLE = 4,
  WaveShape_ROUNDEDSQUARE = 5
} WaveShape;

typedef struct UIOscillator {
  float freq;
  float amplitude;
  float shape_parameter_0;
  WaveShape shape;
  bool is_dropdown_open;
  Rectangle shape_dropdown_rect;
} UIOscillator;

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

typedef struct Synth {
  OscillatorArray sineOsc;
  OscillatorArray sawtoothOsc;
  OscillatorArray triangleOsc;
  OscillatorArray squareOsc;
  OscillatorArray roundedSquareOsc;
  float *signal;
  size_t signal_length;
  float audio_frame_duration;

  UIOscillator ui_oscillator[MAX_UI_OSCILLATORS];
  size_t ui_oscillator_count;
} Synth;

typedef float (*WaveShapeFn)(const Oscillator);

float log2f(float n) {
  // log(n)/log(2) is log2.
  return logf(n) / logf(2);
}

static float getFrequencyForSemitone(float semitone) {
  // fn = 2^(n/12) × 440 Hz
  return powf(2.f, semitone / 12.f) * BASE_NOTE_FREQ;
}

static float getSemitoneForFrequency(float freq) {
  // n = 12 × log2 (fn / 440 Hz).
  return 12.f * log2f(freq / BASE_NOTE_FREQ);
}

static Oscillator *makeOscillator(OscillatorArray *oscArray) {
  return oscArray->osc + (oscArray->count++);
}

static void clearOscillatorArray(OscillatorArray *oscArray) {
  oscArray->count = 0;
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

void updateOsc(Oscillator *osc, float freq_modulation) {
  osc->phase_dt = ((osc->freq + freq_modulation) * SAMPLE_DURATION);
  osc->phase += osc->phase_dt;
  if (osc->phase < 0.0f)
    osc->phase += 1.0f;
  if (osc->phase >= 1.0f)
    osc->phase -= 1.0f;
}

void zeroSignal(float *signal) {
  for (size_t t = 0; t < STREAM_BUFFER_SIZE; t++) {
    signal[t] = 0.0f;
  }
}

// @shapefn
float sineShape(const Oscillator osc) { return sinf(2.f * PI * osc.phase); }

// @shapefn
float sawtoothShape(const Oscillator osc) {
  float sample = (osc.phase * 2.0f) - 1.0f;
  sample -= bandlimitedRipple(osc.phase, osc.phase_dt);
  return sample;
}

// @shapefn
float triangleShape(const Oscillator osc) {
  // TODO: Make this band-limited.
  if (osc.phase < 0.5f)
    return (osc.phase * 4.0f) - 1.0f;
  else
    return (osc.phase * -4.0f) + 3.0f;
}

// @shapefn
float squareShape(const Oscillator osc) {
  float duty_cycle = osc.shape_parameter_0;
  float sample = (osc.phase < duty_cycle) ? 1.0f : -1.0f;
  sample += bandlimitedRipple(osc.phase, osc.phase_dt);
  sample -= bandlimitedRipple(fmodf(osc.phase + (1.f - duty_cycle), 1.0f),
                              osc.phase_dt);
  return sample;
}

// @shapefn
float roundedSquareShape(const Oscillator osc) {
  float s = (osc.shape_parameter_0 * 8.f) + 2.f;
  float base = (float)fabs(s);
  float power = s * sinf(osc.phase * PI * 2);
  float denominator = powf(base, power) + 1.f;
  float sample = (2.f / denominator) - 1.f;
  return sample;
}

void updateOscArray(WaveShapeFn base_osc_shape_fn, Synth *synth,
                    OscillatorArray osc_array) {
  for (size_t i = 0; i < osc_array.count; i++) {
    if (osc_array.osc[i].freq > (SAMPLE_RATE / 2) ||
        osc_array.osc[i].freq < -(SAMPLE_RATE / 2))
      continue;
    for (size_t t = 0; t < STREAM_BUFFER_SIZE; t++) {
      updateOsc(&osc_array.osc[i], 0.0f);
      synth->signal[t] +=
          base_osc_shape_fn(osc_array.osc[i]) * osc_array.osc[i].amplitude;
    }
  }
}

void handleAudioStream(AudioStream stream, Synth *synth) {
  Vector2 mouse_pos = GetMousePosition();
  // float normalized_mouse_x = (mouse_pos.x / SCREEN_WIDTH);
  // float normalized_mouse_y = (mouse_pos.y / SCREEN_HEIGHT);
  // float base_freq = 100.0f + (normalized_mouse_x*normalized_mouse_x *
  // 4000.0f); synth->lfo.freq = 0.05f + (normalized_mouse_y * 10.0f);
  float audio_frame_duration = 0.0f;

  if (IsAudioStreamProcessed(stream)) {
    const float audio_frame_start_time = GetTime();
    zeroSignal(synth->signal);
    updateOscArray(&sineShape, synth, synth->sineOsc);
    updateOscArray(&sawtoothShape, synth, synth->sawtoothOsc);
    updateOscArray(&triangleShape, synth, synth->triangleOsc);
    updateOscArray(&squareShape, synth, synth->squareOsc);
    updateOscArray(&roundedSquareShape, synth, synth->roundedSquareOsc);
    UpdateAudioStream(stream, synth->signal, synth->signal_length);
    synth->audio_frame_duration = GetTime() - audio_frame_start_time;
  }
}

void draw_ui(Synth *synth) {
  const int panel_x_start = 0;
  const int panel_y_start = 0;
  const int panel_width = UI_PANEL_WIDTH;
  const int panel_height = SCREEN_WIDTH;

  bool is_shape_dropdown_open = false;
  int shape_index = 0;

  Vector2 mouseCell = {0}; // Initialize the mouseCell variable
  GuiGrid((Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, "", SCREEN_HEIGHT / 8,
          2, &mouseCell);

  GuiPanel((Rectangle){panel_x_start, panel_y_start, panel_width, panel_height},
           "");

  bool click_add_oscillator = GuiButton(
      (Rectangle){panel_x_start + 10, panel_y_start + 10, panel_width - 20, 25},
      "Add Oscillator");
  if (click_add_oscillator) {
    synth->ui_oscillator_count += 1;
  }

  float panel_y_offset = 0;
  for (size_t ui_osc_i = 0; ui_osc_i < synth->ui_oscillator_count; ui_osc_i++) {
    UIOscillator *ui_osc = &synth->ui_oscillator[ui_osc_i];
    const bool has_shape_param = (ui_osc->shape == WaveShape_SQUARE ||
                                  ui_osc->shape == WaveShape_ROUNDEDSQUARE);

    const int osc_panel_width = panel_width - 20;
    const int osc_panel_height = has_shape_param ? 130 : 100;
    const int osc_panel_x = panel_x_start + 10;
    const int osc_panel_y = panel_y_start + 50 + panel_y_offset;
    panel_y_offset += osc_panel_height + 5;
    GuiPanel((Rectangle){osc_panel_x, osc_panel_y, osc_panel_width,
                         osc_panel_height},
             "");

    const float slider_padding = 50.f;
    const float el_spacing = 5.f;
    Rectangle el_rect = {.x = osc_panel_x + slider_padding + 30,
                         .y = osc_panel_y + 10,
                         .width = osc_panel_width - (slider_padding * 2),
                         .height = 25};

    // Frequency slider
    char freq_slider_label[16];
    sprintf(freq_slider_label, "%.1fHz", ui_osc->freq);
    float log_freq = log10f(ui_osc->freq);
    GuiSlider(el_rect, freq_slider_label, "",
              &log_freq, //?
              0.0f, log10f((float)(SAMPLE_RATE / 2)));
    ui_osc->freq = powf(10.f, log_freq);
    el_rect.y += el_rect.height + el_spacing;

    // Amplitude slider
    float decibels = (20.f * log10f(ui_osc->amplitude));
    char amp_slider_label[32];
    sprintf(amp_slider_label, "%.1f dB", decibels);
    GuiSlider(el_rect, amp_slider_label, "",
              &decibels, //?
              -60.0f, 0.0f);
    ui_osc->amplitude = powf(10.f, decibels * (1.f / 20.f));
    el_rect.y += el_rect.height + el_spacing;

    // Shape parameter slider
    if (has_shape_param) {
      float shape_param = ui_osc->shape_parameter_0;
      char shape_param_label[32];
      sprintf(shape_param_label, "%.1f", shape_param);
      GuiSlider(el_rect, shape_param_label, "",
                &shape_param, //?
                0.f, 1.f);
      ui_osc->shape_parameter_0 = shape_param;
      el_rect.y += el_rect.height + el_spacing;
    }

    // Defer shape drop-down box.
    ui_osc->shape_dropdown_rect = el_rect;
    el_rect.y += el_rect.height + el_spacing;

    Rectangle delete_button_rect = el_rect;
    delete_button_rect.x = osc_panel_x + 5;
    delete_button_rect.y -= el_rect.height + el_spacing;
    delete_button_rect.width = 30;
    bool is_delete_button_pressed = GuiButton(delete_button_rect, "X");
    if (is_delete_button_pressed) {
      memmove(synth->ui_oscillator + ui_osc_i,
              synth->ui_oscillator + ui_osc_i + 1,
              (synth->ui_oscillator_count - ui_osc_i) * sizeof(UIOscillator));
      synth->ui_oscillator_count -= 1;
    }
  }

  for (size_t ui_osc_i = 0; ui_osc_i < synth->ui_oscillator_count;
       ui_osc_i += 1) {
    UIOscillator *ui_osc = &synth->ui_oscillator[ui_osc_i];
    // Shape select
    int shape_index = (int)(ui_osc->shape);
    bool is_dropdown_click =
        GuiDropdownBox(ui_osc->shape_dropdown_rect, WAVE_SHAPE_OPTIONS,
                       &shape_index, ui_osc->is_dropdown_open);
    ui_osc->shape = (WaveShape)(shape_index);
    if (is_dropdown_click) {
      ui_osc->is_dropdown_open = !ui_osc->is_dropdown_open;
    }
    if (ui_osc->is_dropdown_open)
      break;
  }

  // Reset synth
  clearOscillatorArray(&synth->sineOsc);
  clearOscillatorArray(&synth->sawtoothOsc);
  clearOscillatorArray(&synth->triangleOsc);
  clearOscillatorArray(&synth->squareOsc);
  clearOscillatorArray(&synth->roundedSquareOsc);

  float note_freq = 0;

  for (size_t ui_osc_i = 0; ui_osc_i < synth->ui_oscillator_count; ui_osc_i++) {
    UIOscillator *ui_osc = &synth->ui_oscillator[ui_osc_i];
    Oscillator *osc = NULL;
    switch (ui_osc->shape) {
    case WaveShape_SINE: {
      osc = makeOscillator(&synth->sineOsc);
      break;
    }
    case WaveShape_SAWTOOTH: {
      osc = makeOscillator(&synth->sawtoothOsc);
      break;
    }
    case WaveShape_SQUARE: {
      osc = makeOscillator(&synth->squareOsc);
      break;
    }
    case WaveShape_TRIANGLE: {
      osc = makeOscillator(&synth->triangleOsc);
      break;
    }
    case WaveShape_ROUNDEDSQUARE: {
      osc = makeOscillator(&synth->roundedSquareOsc);
      break;
    }
    }
    if (osc != NULL) {
      osc->freq = ui_osc->freq;
      // osc->freq = note_freq;
      osc->amplitude = ui_osc->amplitude;
      osc->shape_parameter_0 = ui_osc->shape_parameter_0;
    }
  }
}

int main() {
  const int screen_width = 1024;
  const int screen_height = 768;
  InitWindow(screen_width, screen_height, "tiny");
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
  float signal[STREAM_BUFFER_SIZE] = {0};
  Synth synth = {.sineOsc = {.osc = sineOsc, .count = 0},
                 .sawtoothOsc = {.osc = sawtoothOsc, .count = 0},
                 .triangleOsc = {.osc = triangleOsc, .count = 0},
                 .squareOsc = {.osc = squareOsc, .count = 0},
                 .roundedSquareOsc = {.osc = roundedSquareOsc, .count = 0},
                 .signal = signal,
                 .signal_length = STREAM_BUFFER_SIZE,
                 .audio_frame_duration = 0.0f,
                 .ui_oscillator_count = 0};

  // Set oscillator amplitudes.
  for (size_t i = 0; i < NUM_OSCILLATORS; i++) {
    sineOsc[i].amplitude = 0.0f;
    sawtoothOsc[i].amplitude = 0.0f;
    triangleOsc[i].amplitude = 0.0f;
    squareOsc[i].amplitude = 0.0f;
  }

  while (!WindowShouldClose()) {
    handleAudioStream(synth_stream, &synth);
    BeginDrawing();
    ClearBackground(BLACK);

    draw_ui(&synth);

    // Drawing the signal.
    {
      size_t zero_crossing_index = 0;
#if 1
      for (size_t i = 1; i < STREAM_BUFFER_SIZE; i++) {
        if (signal[i] >= 0.0f && signal[i - 1] < 0.0f) // zero-crossing
        {
          zero_crossing_index = i;
          break;
        }
      }
#endif

      Vector2 signal_points[STREAM_BUFFER_SIZE];
      const float screen_vertical_midpoint = (SCREEN_HEIGHT / 2);
      for (size_t point_idx = 0; point_idx < STREAM_BUFFER_SIZE; point_idx++) {
        const size_t signal_idx =
            (point_idx + zero_crossing_index) % STREAM_BUFFER_SIZE;
        signal_points[point_idx].x = (float)point_idx + UI_PANEL_WIDTH;
        signal_points[point_idx].y =
            screen_vertical_midpoint + (int)(signal[signal_idx] * 400);
      }
      DrawLineStrip(signal_points, STREAM_BUFFER_SIZE - zero_crossing_index,
                    RED);

      DrawText(TextFormat("Fundemental Freq: %.1f", squareOsc[0].freq),
               UI_PANEL_WIDTH + 10, 30, 20, RED);
    }

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