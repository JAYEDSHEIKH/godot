#pragma once
#include "core/string/ustring.h"
#include "core/templates/vector.h"

enum LogLevel { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARN = 2, LOG_ERROR = 3 };

struct LogEntry {
    LogLevel level;
    String   tag;
    String   message;
    int64_t  timestamp_ms;
};

class AILogger {
    static Vector<LogEntry> log_buffer;
    static int              max_buffer = 2000;
    static LogLevel         min_level;

public:
    static void debug(const String &tag, const String &msg);
    static void info(const String &tag, const String &msg);
    static void warn(const String &tag, const String &msg);
    static void error(const String &tag, const String &msg);

    static void set_min_level(LogLevel level) { min_level = level; }
    static const Vector<LogEntry> &get_log() { return log_buffer; }
    static void clear_log() { log_buffer.clear(); }

private:
    static void _log(LogLevel level, const String &tag, const String &msg);
};
