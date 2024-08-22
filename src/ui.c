#include "ui.h"

void draw_ui(Synth *synth) {
  const int panel_x_start = 0;
  const int panel_y_start = 0;
  const int panel_width = UI_PANEL_WIDTH;
  const int panel_height = SCREEN_WIDTH;

  Vector2 mouseCell = {0};
  GuiGrid((Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, "", SCREEN_HEIGHT / 8,
          2, &mouseCell);
}

static float lerp(float a, float b, float t) { return a + t * (b - a); }

void draw_signal(float *signal) {
  const size_t SUBSAMPLE_RATE = 8; // take every 8th point from the signal
  const size_t INTERPOLATION_STEPS =
      8; // number of interpolated points between each subsampled point

  size_t zero_crossing_index = 0;
  for (size_t i = 1; i < STREAM_BUFFER_SIZE; i++) {
    if (signal[i] >= 0.0f && signal[i - 1] < 0.0f) // zero-crossing
    {
      zero_crossing_index = i;
      break;
    }
  }

  // nalculate the number of subsampled points
  size_t num_subsampled_points = STREAM_BUFFER_SIZE / SUBSAMPLE_RATE;
  Vector2 signal_points[num_subsampled_points * INTERPOLATION_STEPS];
  const float screen_vertical_midpoint = (SCREEN_HEIGHT / 2);

  for (size_t point_idx = 0; point_idx < num_subsampled_points; point_idx++) {
    // subsampled point index
    size_t signal_idx =
        (point_idx * SUBSAMPLE_RATE + zero_crossing_index) % STREAM_BUFFER_SIZE;

    // original subsampled point
    float x1 = (float)(point_idx * INTERPOLATION_STEPS) + UI_PANEL_WIDTH;
    float y1 = screen_vertical_midpoint + (int)(signal[signal_idx] * 400);

    // next subsampled point
    size_t next_signal_idx =
        ((point_idx + 1) * SUBSAMPLE_RATE + zero_crossing_index) %
        STREAM_BUFFER_SIZE;
    float x2 = (float)((point_idx + 1) * INTERPOLATION_STEPS) + UI_PANEL_WIDTH;
    float y2 = screen_vertical_midpoint + (int)(signal[next_signal_idx] * 400);

    // add the original subsampled point
    signal_points[point_idx * INTERPOLATION_STEPS].x = x1;
    signal_points[point_idx * INTERPOLATION_STEPS].y = y1;

    // add interpolated points
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
                        float total_time) {
  float attack_time = envelope.attack_time;
  float decay_time = envelope.decay_time;
  float release_time = envelope.release_time;
  float sustain_time = envelope.sustain_time;

  // ensure the sustain time isn't negative
  if (sustain_time < 0)
    sustain_time = 0;

  // calculate how many points to allocate to each phase
  int attack_points = (int)(attack_time / total_time * num_points);
  int decay_points = (int)(decay_time / total_time * num_points);
  int sustain_points = (int)(sustain_time / total_time * num_points);
  int release_points =
      num_points - (attack_points + decay_points + sustain_points);

  int point_index = 0;
  float current_level = 0.0f;
  float time_step;

  // attack phase
  time_step = attack_time / (attack_points > 0 ? attack_points : 1);
  for (int i = 0; i < attack_points; i++, point_index++) {
    current_level = (i * time_step) / attack_time;
    points[point_index] = (Vector2){(float)point_index / (num_points - 1) * 500,
                                    200 - current_level * 200};
  }

  // decay phase
  time_step = decay_time / (decay_points > 0 ? decay_points : 1);
  for (int i = 0; i < decay_points; i++, point_index++) {
    float decay_progress = (i * time_step) / decay_time;
    current_level = 1.0f - (1.0f - envelope.sustain_level) * decay_progress;
    points[point_index] = (Vector2){(float)point_index / (num_points - 1) * 500,
                                    200 - current_level * 200};
  }

  // sustain phase
  for (int i = 0; i < sustain_points; i++, point_index++) {
    points[point_index] = (Vector2){(float)point_index / (num_points - 1) * 500,
                                    200 - envelope.sustain_level * 200};
  }

  // release phase
  time_step = release_time / (release_points > 0 ? release_points : 1);
  for (int i = 0; i < release_points; i++, point_index++) {
    float release_progress = (i * time_step) / release_time;
    current_level = envelope.sustain_level * (1.0f - release_progress);
    points[point_index] = (Vector2){(float)point_index / (num_points - 1) * 500,
                                    200 - current_level * 200};
  }
}
