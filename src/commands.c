#include "hash.h"
#include "utils.h"
#include "commands.h"
#include <stdbool.h>
#include <stdatomic.h>

static char keyMappings[12] = {'a', 's', 'd', 'f', 'g', 'h',
                               'j', 'k', 'l', ';', '\''};
static atomic_bool wasKeyHeld[12] = {false};

static command_map cmd_map[] = {
    {"set", key_pressed},
    {"res", key_released},
    {NULL, NULL}
};

void set_key_pressed(char key, bool pressed) {
  for (int i = 0; i < 12; ++i) {
    if (keyMappings[i] == key) {
      wasKeyHeld[i] = pressed;
      return;
    }
  }
}

void key_pressed(char* userdata){
    if (strlen(userdata) != 1){
        log_message(ERROR,"invalid command tail");
        return;
    }

    set_key_pressed(userdata[0],true);
}

void key_released(char* userdata){
    if (strlen(userdata) != 1){
        log_message(ERROR,"invalid command tail");
        return;
    }

    set_key_pressed(userdata[0],false);
}

command_fn find_function_by_command(const char *command) {
    for (int i = 0; cmd_map[i].command != NULL; i++) {
        if (strcmp(cmd_map[i].command, command) == 0) {
            return cmd_map[i].f;
        }
    }
    return NULL;
}