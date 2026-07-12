#include "logger.h"

LogLevel Logger::minLevel_ = LogLevel::INFO;

void Logger::setMinLevel(LogLevel level) {
    minLevel_ = level;
}

const char* Logger::levelToStr(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "?????";
    }
}

void Logger::log(LogLevel level, const char* tag, const String& msg) {
    if (level < minLevel_) return;

    // [  12345][INFO   ][WiFi] Connected
    Serial.printf("[%8lu][%-7s][%s] %s\n",
                   static_cast<unsigned long>(millis()),
                   levelToStr(level),
                   tag,
                   msg.c_str());
}

void Logger::debug(const char* tag, const String& msg)   { log(LogLevel::DEBUG, tag, msg); }
void Logger::info(const char* tag, const String& msg)    { log(LogLevel::INFO, tag, msg); }
void Logger::warning(const char* tag, const String& msg) { log(LogLevel::WARNING, tag, msg); }
void Logger::error(const char* tag, const String& msg)   { log(LogLevel::ERROR, tag, msg); }
