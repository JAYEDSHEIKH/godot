#include "logger.h"
#include "core/os/time.h"
#include "core/os/os.h"

Vector<LogEntry> AILogger::log_buffer;
LogLevel         AILogger::min_level = LOG_DEBUG;

void AILogger::_log(LogLevel level, const String &tag, const String &msg) {
    if (level < min_level) return;
    LogEntry e;
    e.level        = level;
    e.tag          = tag;
    e.message      = msg;
    e.timestamp_ms = Time::get_singleton()->get_ticks_msec();
    log_buffer.push_back(e);
    if ((int)log_buffer.size() > max_buffer) log_buffer.remove_at(0);

    static const char *level_str[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    String full = String("[AIEngine/") + tag + "] " + level_str[level] + ": " + msg;
    if (level >= LOG_WARN) {
        if (level == LOG_ERROR) ERR_PRINT(full);
        else WARN_PRINT(full);
    } else {
        print_verbose(full);
    }
}

void AILogger::debug(const String &tag, const String &msg) { _log(LOG_DEBUG, tag, msg); }
void AILogger::info(const String &tag, const String &msg)  { _log(LOG_INFO,  tag, msg); }
void AILogger::warn(const String &tag, const String &msg)  { _log(LOG_WARN,  tag, msg); }
void AILogger::error(const String &tag, const String &msg) { _log(LOG_ERROR, tag, msg); }
