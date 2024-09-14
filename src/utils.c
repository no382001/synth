#include "utils.h"
#include <time.h>
static LogLevel current_log_level = DEBUG;

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
  return "";
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

    char time_str[100];
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    strftime(time_str, sizeof(time_str), "[%a %b %d %H:%M:%S %Z %Y]", local_time);

    const char *level_str = log_lvl_to_str(level);

    printf("%s %s%s:%d: %s\n", time_str, level_str, file, line, current_message);

}

void set_log_level(LogLevel level) { current_log_level = level; }
LogLevel get_current_log_level(void) { return current_log_level; }