#pragma once
// ============================================================================
// logger.h — Lightweight serial logger with severity levels
// ============================================================================

#include <Arduino.h>

enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    // Set the minimum level that will be printed. Default: INFO.
    static void setMinLevel(LogLevel level);

    static void debug(const char* tag, const String& msg);
    static void info(const char* tag, const String& msg);
    static void warning(const char* tag, const String& msg);
    static void error(const char* tag, const String& msg);

private:
    static LogLevel minLevel_;
    static void log(LogLevel level, const char* tag, const String& msg);
    static const char* levelToStr(LogLevel level);
};
