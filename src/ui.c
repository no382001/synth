#include "ui.h"

void draw_ui(Synth *synth) {
  const int panel_x_start = 0;
  const int panel_y_start = 0;
  const int panel_width = UI_PANEL_WIDTH;
  const int panel_height = SCREEN_WIDTH;

  bool is_shape_dropdown_open = false;
  int shape_index = 0;

  Vector2 mouseCell = {0};
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

    // frequency slider
    char freq_slider_label[16];
    sprintf(freq_slider_label, "%.1fHz", ui_osc->freq);
    float log_freq = log10f(ui_osc->freq);
    GuiSlider(el_rect, freq_slider_label, "",
              &log_freq, //?
              0.0f, log10f((float)(SAMPLE_RATE / 2)));
    ui_osc->freq = powf(10.f, log_freq);
    el_rect.y += el_rect.height + el_spacing;

    // amplitude slider
    float decibels = (20.f * log10f(ui_osc->amplitude));
    char amp_slider_label[32];
    sprintf(amp_slider_label, "%.1f dB", decibels);
    GuiSlider(el_rect, amp_slider_label, "",
              &decibels, //?
              -60.0f, 0.0f);
    ui_osc->amplitude = powf(10.f, decibels * (1.f / 20.f));
    el_rect.y += el_rect.height + el_spacing;

    // shape parameter slider
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

    // defer shape drop-down box.
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
    // shape select
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
      osc->amplitude = ui_osc->amplitude;
      osc->shape_parameter_0 = ui_osc->shape_parameter_0;
    }
  }
}

void draw_signal(float *signal) {
  size_t zero_crossing_index = 0;
  for (size_t i = 1; i < STREAM_BUFFER_SIZE; i++) {
    if (signal[i] >= 0.0f && signal[i - 1] < 0.0f) // zero-crossing
    {
      zero_crossing_index = i;
      break;
    }
  }

  Vector2 signal_points[STREAM_BUFFER_SIZE];
  const float screen_vertical_midpoint = (SCREEN_HEIGHT / 2);
  for (size_t point_idx = 0; point_idx < STREAM_BUFFER_SIZE; point_idx++) {
    const size_t signal_idx =
        (point_idx + zero_crossing_index) % STREAM_BUFFER_SIZE;
    signal_points[point_idx].x = (float)point_idx + UI_PANEL_WIDTH;
    signal_points[point_idx].y =
        screen_vertical_midpoint + (int)(signal[signal_idx] * 400);
  }
  DrawLineStrip(signal_points, STREAM_BUFFER_SIZE - zero_crossing_index, RED);
}

int keyMappings[NUM_KEYS] = {KEY_A, KEY_S,         KEY_D,         KEY_F,
                             KEY_G, KEY_H,         KEY_J,         KEY_K,
                             KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE};
int semitoneOffsets[NUM_KEYS] = {-9, -7, -5, -4, -2, 0, 2, 3, 5, 7, 8, 10};
bool wasKeyHeld[NUM_KEYS] = {false};

void handle_keys(Synth *synth) {
  for (int i = 0; i < NUM_KEYS; i++) {
    Oscillator *osc = &synth->keyOscillators.osc[i];

    bool isKeyHeld = IsKeyDown(keyMappings[i]);

    if (isKeyHeld && !wasKeyHeld[i]) { // key was just pressed
      float freq = getFrequencyForSemitone(BASE_SEMITONE + semitoneOffsets[i]);
      if (osc->envelope.state == OFF) { // start attack
        osc->freq = freq;
        osc->amplitude = 0.5f;
        osc->shape_parameter_0 = 1.0f;

        osc->envelope.state = ATTACK;
      }
    }

    if (!isKeyHeld && wasKeyHeld[i]) { // key was released
      osc->envelope.state = RELEASE;   // start release
    }

    wasKeyHeld[i] = isKeyHeld;

    if (isKeyHeld) {
      // sustain takes care of this, no need for state change
    }
  }
}
