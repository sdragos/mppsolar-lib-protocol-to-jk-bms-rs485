#include <esp_log.h>

struct LogString;

#define LOG_STR(s) (reinterpret_cast<const LogString *>(s))
#define LOG_STR_ARG(s) (reinterpret_cast<const char *>(s))

