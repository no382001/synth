#include "commands.h"
#include "hash.h"
#include "synth.h"
#include "utils.h"
#include <stdatomic.h>

static char keyMappings[NUM_KEYS] = {'a', 's', 'd', 'f', 'g', 'h',
                                     'j', 'k', 'l', ';', '\''};
static int semitoneOffsets[NUM_KEYS] = {-9, -7, -5, -4, -2, 0,
                                        2,  3,  5,  7,  8,  10};
static key_state_t keyStates[NUM_KEYS] = {};

static void key_state_update(key_state_t *ks, bool pressed) {
  if (ks->val != pressed) {
    ks->val = pressed;
    ks->change = true;
  }
}

// refactor this so it can handle multiple arguments if needed
static command_map_t cmd_map[] = {
    {"set", key_pressed}, {"res", key_released}, {NULL, NULL}};

void set_key_pressed(char key, bool pressed) {
  for (int i = 0; i < NUM_KEYS; ++i) {
    if (keyMappings[i] == key) {
      key_state_update(&keyStates[i], pressed);
      return;
    }
  }
}

void key_pressed(char *userdata) {
  if (strlen(userdata) != 1) {
    log_message(ERROR, "invalid command tail: %s",userdata);
    return;
  }

  set_key_pressed(userdata[0], true);
}

void key_released(char *userdata) {
  if (strlen(userdata) != 1) {
    log_message(ERROR, "invalid command tail: %s",userdata);
    return;
  }

  set_key_pressed(userdata[0], false);
}

command_fn find_function_by_command(const char *command) {
  for (int i = 0; cmd_map[i].command != NULL; i++) {
    if (strcmp(cmd_map[i].command, command) == 0) {
      return cmd_map[i].f;
    }
  }
  return NULL;
}

void handle_keys(Synth *synth) {
  for (int i = 0; i < NUM_KEYS; i++) {
    Oscillator *osc = &synth->keyOscillators.osc[i];
    key_state_t *ks = &keyStates[i];

    if (ks->val && ks->change &&
        osc->envelope.state == OFF) { // key was just pressed, start attack
      osc->freq = getFrequencyForSemitone(BASE_SEMITONE + semitoneOffsets[i]);;
      osc->amplitude = 0.5f;
      osc->shape_parameter_0 = 1.0f;
      osc->envelope.state = ATTACK;
    }

    if (ks->val && !ks->change && osc->envelope.state >= SUSTAIN) {
      osc->envelope.state = SUSTAIN; // reset state
      osc->envelope.sustain_time_elapsed = 0.0f;
    }

    if (!ks->val && ks->change) { // key was just released
      // state machine will handle the rest
    }

    ks->change = false;
  }
}