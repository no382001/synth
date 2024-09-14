#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>

#include "synth.h"
#include "utils.h"
#include "yaml.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "tk.h"
#include "tcl.h"

ADSR defaultEnvelope = {.attack_time = ENVELOPE_DEFAULT_ATTACK_TIME,
                        .decay_time = ENVELOPE_DEFAULT_DECAY_TIME,
                        .sustain_level = ENVELOPE_DEFAULT_SUSTAIN_LEVEL,
                        .release_time = ENVELOPE_DEFAULT_RELEASE_TIME,
                        .sustain_time = ENVELOPE_DEFAULT_SUSTAIN_TIME,
                        .current_level = 0.0f,
                        .state = OFF};

static hash_t *config = NULL;

// takes a hash_t and if the value is proper sets it to the reference
static void hash_get_and_set_float(hash_t *h, char *cs, float *f) {
  char *str = hash_get(config, cs);
  char *eptr;

  float num = strtof(str, &eptr);

  if (*eptr == '\0') {
    *f = num;
  } else {
    log_message(ERROR, "%s could not be converted to float, ignoring", cs);
  }
}

void load_config() {
  config = yaml_read("conf/conf.yaml");
  if (!config) {
    log_message(ERROR, "conf/conf.yaml failed to load, using default values!");
    return;
  }
  hash_each(config, { log_message(INFO, "%s: %s\n", key, (char *)val); });

  hash_get_and_set_float(config, "envelope.attack_time",
                         &defaultEnvelope.attack_time);
  hash_get_and_set_float(config, "envelope.decay_time",
                         &defaultEnvelope.decay_time);
  hash_get_and_set_float(config, "envelope.sustain_level",
                         &defaultEnvelope.sustain_level);
  hash_get_and_set_float(config, "envelope.sustain_time",
                         &defaultEnvelope.sustain_time);
  hash_get_and_set_float(config, "envelope.release_time",
                         &defaultEnvelope.release_time);

  hash_free(config);
  config = NULL;
}

#define SAMPLE_RATE 44100
#define NUM_KEYS 12

Synth *g_synth = NULL;

void audio_cb(ma_device *pDevice, void *pOutput, const void *pInput,
              ma_uint32 frameCount) {
  (void)pDevice;
  (void)pInput;
  if (!g_synth)
    return;

  float *out = (float *)pOutput;

  zeroSignal(g_synth->signal);
  updateOscArray(&sineShape, g_synth, g_synth->keyOscillators);

  for (ma_uint32 i = 0; i < frameCount; i++) {
    out[i] = g_synth->signal[i];
  }
}

void init_audio(ma_device* device){
  ma_result result;
  ma_device_config config;

  config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 1;
  config.sampleRate = SAMPLE_RATE;
  config.dataCallback = audio_cb;

  result = ma_device_init(NULL, &config, device);
  if (result != MA_SUCCESS) {
    printf("failed to initialize playback device.\n");
    exit(1);
  }

}

void tcl_thread(void) {
  Tcl_Interp *interp = Tcl_CreateInterp();

  if (Tcl_Init(interp) == TCL_ERROR || Tk_Init(interp) == TCL_ERROR) {
    fprintf(stderr, "tcl/tk initialization failed: %s\n",
            Tcl_GetStringResult(interp));
    exit(1);
  }

  if (Tcl_EvalFile(interp, "tcl/gui.tcl") != TCL_OK) {
    const char *error_message = Tcl_GetStringResult(interp);
    const char *stack_trace = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);

    fprintf(stderr, "%s\n", error_message);
    fprintf(stderr, "%s\n", stack_trace);

    exit(1);
  }

  Tk_MainLoop();

  Tcl_DeleteInterp(interp);
}

int main() {
  load_config();

  ma_device device;
  init_audio(&device);

  Oscillator keyOscillators[NUM_KEYS] = {0};
  float signal[STREAM_BUFFER_SIZE] = {0};
  Synth synth = {.keyOscillators = {.osc = keyOscillators, .count = 0},
                 .signal = &signal,
                 .signal_length = STREAM_BUFFER_SIZE,
                 .audio_frame_duration = 0.0f};
  g_synth = &synth;

  for (size_t i = 0; i < NUM_KEYS; i++) {
    Oscillator *o = makeOscillator(&synth.keyOscillators);
    o->envelope = defaultEnvelope;
  }

  pthread_t tcl_interpreter;
  pthread_create(&tcl_interpreter, NULL, tcl_thread, NULL);

  while (1) {
    pthread_join(tcl_interpreter, NULL);
  }


  return 0;
}