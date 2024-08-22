#include "ui.h"

extern ADSR defaultEnvelope;
extern Vector2 ADSR_points;

Rectangle ui_adsr_pos =
    (Rectangle){UI_PADDING, UI_PADDING, UI_PADDING + UI_ADSR_PANE_WIDTH,
                UI_PADDING + UI_ADSR_PANE_HEIGHT};
Rectangle ui_viz_pos =
    (Rectangle){(3 * UI_PADDING) + UI_ADSR_PANE_WIDTH, UI_PADDING,
                UI_VIZ_PANE_WIDTH, UI_PADDING + UI_VIZ_PANE_HEIGHT};

void draw_ui(Synth *synth) {
  GuiPanel(ui_adsr_pos, NULL);
  GuiPanel(ui_viz_pos, NULL);
  draw_signal(synth->signal, ui_viz_pos);
  DrawLineStrip(&ADSR_points, ADSR_VIZ_NUM_POINTS, RED);
}

static float lerp(float a, float b, float t) { return a + t * (b - a); }

void draw_signal(float *signal, Rectangle area) {
  const size_t SUBSAMPLE_RATE = 8;
  const size_t INTERPOLATION_STEPS = 8;

  size_t zero_crossing_index = 0;
  for (size_t i = 1; i < STREAM_BUFFER_SIZE; i++) {
    if (signal[i] >= 0.0f && signal[i - 1] < 0.0f) {
      zero_crossing_index = i;
      break;
    }
  }

  size_t num_subsampled_points = STREAM_BUFFER_SIZE / SUBSAMPLE_RATE;
  Vector2 signal_points[num_subsampled_points * INTERPOLATION_STEPS];
  const float screen_vertical_midpoint = area.height / 2.0f + area.y;

  float x_scale =
      area.width / (float)(num_subsampled_points * INTERPOLATION_STEPS);
  float y_scale = area.height / 400.0f;

  for (size_t point_idx = 0; point_idx < num_subsampled_points; point_idx++) {
    size_t signal_idx =
        (point_idx * SUBSAMPLE_RATE + zero_crossing_index) % STREAM_BUFFER_SIZE;
    float x1 = area.x + (float)(point_idx * INTERPOLATION_STEPS) * x_scale;
    float y1 = screen_vertical_midpoint - (signal[signal_idx] * 400 * y_scale);

    size_t next_signal_idx =
        ((point_idx + 1) * SUBSAMPLE_RATE + zero_crossing_index) %
        STREAM_BUFFER_SIZE;
    float x2 =
        area.x + (float)((point_idx + 1) * INTERPOLATION_STEPS) * x_scale;
    float y2 =
        screen_vertical_midpoint - (signal[next_signal_idx] * 400 * y_scale);

    signal_points[point_idx * INTERPOLATION_STEPS].x = x1;
    signal_points[point_idx * INTERPOLATION_STEPS].y = y1;

    for (int step = 1; step < INTERPOLATION_STEPS; step++) {
      float t = (float)step / (float)INTERPOLATION_STEPS;
      signal_points[point_idx * INTERPOLATION_STEPS + step].x = lerp(x1, x2, t);
      signal_points[point_idx * INTERPOLATION_STEPS + step].y = lerp(y1, y2, t);
    }
  }

  DrawLineStrip(signal_points, num_subsampled_points * INTERPOLATION_STEPS,
                RED);
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
    } else if (isKeyHeld && wasKeyHeld[i] && osc->envelope.state > SUSTAIN) {
      osc->envelope.state = SUSTAIN; // reset state
      osc->envelope.sustain_time_elapsed = 0.0f;
    }

    if (!isKeyHeld && wasKeyHeld[i]) { // key was released
      // state machine will fall into release
    }

    wasKeyHeld[i] = isKeyHeld;
  }
}

void generateADSRPoints(ADSR envelope, Vector2 *points, int num_points,
                        float total_time, Rectangle area) {
  float attack_time = envelope.attack_time;
  float decay_time = envelope.decay_time;
  float release_time = envelope.release_time;
  float sustain_time = envelope.sustain_time;

  if (sustain_time < 0)
    sustain_time = 0;

  int attack_points = (int)(attack_time / total_time * num_points);
  int decay_points = (int)(decay_time / total_time * num_points);
  int sustain_points = (int)(sustain_time / total_time * num_points);
  int release_points =
      num_points - (attack_points + decay_points + sustain_points);

  int point_index = 0;
  float current_level = 0.0f;
  float time_step;

  float x_scale = area.width / (float)(num_points - 1);
  float y_scale =
      area.height / 200.0f; // Assuming the ADSR level is scaled to [0, 1]

  time_step = attack_time / (attack_points > 0 ? attack_points : 1);
  for (int i = 0; i < attack_points; i++, point_index++) {
    current_level = (i * time_step) / attack_time;
    points[point_index] =
        (Vector2){area.x + (float)point_index * x_scale,
                  area.y + area.height - (current_level * area.height)};
  }

  time_step = decay_time / (decay_points > 0 ? decay_points : 1);
  for (int i = 0; i < decay_points; i++, point_index++) {
    float decay_progress = (i * time_step) / decay_time;
    current_level = 1.0f - (1.0f - envelope.sustain_level) * decay_progress;
    points[point_index] =
        (Vector2){area.x + (float)point_index * x_scale,
                  area.y + area.height - (current_level * area.height)};
  }

  for (int i = 0; i < sustain_points; i++, point_index++) {
    points[point_index] = (Vector2){area.x + (float)point_index * x_scale,
                                    area.y + area.height -
                                        (envelope.sustain_level * area.height)};
  }

  time_step = release_time / (release_points > 0 ? release_points : 1);
  for (int i = 0; i < release_points; i++, point_index++) {
    float release_progress = (i * time_step) / release_time;
    current_level = envelope.sustain_level * (1.0f - release_progress);
    points[point_index] =
        (Vector2){area.x + (float)point_index * x_scale,
                  area.y + area.height - (current_level * area.height)};
  }
}
