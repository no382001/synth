#include "utils.h"


static LogLevel current_log_level = DEBUG;

static LogState last_log = {.file = "", .line = -1, .message = ""};

static char *log_lvl_to_str(LogLevel l) {
  switch (l) {
  case INFO:
    return "[INFO] ";
  case WARNING:
    return "[WARNING] ";
  case ERROR:
    return "[ERROR] ";
  case DEBUG:
    return "[DEBUG] ";
  }
}

void log_message_impl(LogLevel level, const char *file, int line,
                      const char *format, ...) {
  if (level > current_log_level) {
    return;
  }

  char current_message[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(current_message, sizeof(current_message), format, args);
  va_end(args);
#ifndef NULL
  // current message is the same as the last one
  if (strcmp(last_log.file, file) == 0 && last_log.line == line) {
    last_log.repeat++;
    return;
  } else if (last_log.repeat != 0) { // not the first

    // print the prev message and the repeat count
    const char *level_str = log_lvl_to_str(last_log.level);
    size_t len = strlen(last_log.message);
    if (len > 0 && last_log.message[len - 1] == '\n') {
      last_log.message[len - 1] = '\0';
    }
    printf("%s%s:%d: %s \t[repeats x%d times]\n", level_str, last_log.file,
           last_log.line, last_log.message, last_log.repeat);
  }

  last_log.line = line;
  last_log.level = level;
  last_log.repeat = 0;
  last_log.file = file;
  strncpy(last_log.message, current_message, sizeof(last_log.message) - 1);
#endif
  const char *level_str = log_lvl_to_str(level);

  printf("%s%s:%d: %s", level_str, file, line, current_message);
}

void set_log_level(LogLevel level) { current_log_level = level; }
LogLevel get_current_log_level(void) { return current_log_level; }