/**
 * @file Logger.h
 * @brief Centralized logging system with multiple log levels
 *
 * Provides a flexible logging facade that can output to Serial, file,
 * or remote syslog server.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "SPIFFS.h"

/**
 * @brief Log levels for filtering and categorizing log messages
 */
enum class LogLevel {
    TRACE = 0,   ///< Detailed diagnostic information
    DEBUG = 1,   ///< Debug information for developers
    INFO = 2,    ///< Informational messages
    WARN = 3,    ///< Warning messages for potential issues
    ERROR = 4,   ///< Error messages for failures
    FATAL = 5,   ///< Fatal errors requiring system restart
    NONE = 6     ///< Disable all logging
};

/**
 * @brief Singleton logger class for centralized logging
 *
 * Supports multiple log levels, colored output, timestamps,
 * and optional file logging.
 *
 * @example
 * Logger& log = Logger::getInstance();
 * log.setLevel(LogLevel::DEBUG);
 * log.info("Camera", "Initialized successfully");
 * log.error("WiFi", "Connection failed");
 */
class Logger {
private:
    LogLevel _minLevel;
    bool _enableColors;
    bool _enableTimestamps;
    bool _enableFileLogging;
    String _logFilePath;
    File _logFile;

    // ANSI color codes for terminal output
    static constexpr const char* COLOR_RESET = "\033[0m";
    static constexpr const char* COLOR_TRACE = "\033[90m";   // Gray
    static constexpr const char* COLOR_DEBUG = "\033[36m";   // Cyan
    static constexpr const char* COLOR_INFO = "\033[32m";    // Green
    static constexpr const char* COLOR_WARN = "\033[33m";    // Yellow
    static constexpr const char* COLOR_ERROR = "\033[31m";   // Red
    static constexpr const char* COLOR_FATAL = "\033[35;1m"; // Magenta bold

    /**
     * @brief Private constructor for singleton
     */
    Logger()
        : _minLevel(LogLevel::INFO),
          _enableColors(true),
          _enableTimestamps(true),
          _enableFileLogging(false),
          _logFilePath("/log/system.log") {}

    /**
     * @brief Get color code for log level
     */
    const char* getColor(LogLevel level) const {
        if (!_enableColors) return "";

        switch (level) {
            case LogLevel::TRACE: return COLOR_TRACE;
            case LogLevel::DEBUG: return COLOR_DEBUG;
            case LogLevel::INFO:  return COLOR_INFO;
            case LogLevel::WARN:  return COLOR_WARN;
            case LogLevel::ERROR: return COLOR_ERROR;
            case LogLevel::FATAL: return COLOR_FATAL;
            default: return "";
        }
    }

    /**
     * @brief Get string representation of log level
     */
    const char* getLevelString(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKN ";
        }
    }

    /**
     * @brief Get formatted timestamp
     */
    String getTimestamp() const {
        if (!_enableTimestamps) return "";

        unsigned long ms = millis();
        unsigned long secs = ms / 1000;
        unsigned long mins = secs / 60;
        unsigned long hours = mins / 60;

        char timestamp[32];
        sprintf(timestamp, "[%02lu:%02lu:%02lu.%03lu]",
                hours % 24, mins % 60, secs % 60, ms % 1000);

        return String(timestamp);
    }

    /**
     * @brief Write log message to outputs
     */
    void write(LogLevel level, const String& tag, const String& message) {
        if (level < _minLevel) return;

        String logLine;

        // Add timestamp
        if (_enableTimestamps) {
            logLine += getTimestamp();
            logLine += " ";
        }

        // Add level with color
        if (_enableColors) {
            logLine += getColor(level);
        }

        logLine += "[";
        logLine += getLevelString(level);
        logLine += "]";

        if (_enableColors) {
            logLine += COLOR_RESET;
        }

        // Add tag
        if (tag.length() > 0) {
            logLine += " [";
            logLine += tag;
            logLine += "]";
        }

        // Add message
        logLine += " ";
        logLine += message;

        // Output to Serial
        Serial.println(logLine);

        // Output to file if enabled
        if (_enableFileLogging && _logFile) {
            _logFile.println(logLine);
            _logFile.flush();  // Ensure data is written
        }
    }

public:
    /**
     * @brief Get singleton instance
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Prevent copying and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Set minimum log level
     *
     * Messages below this level will be filtered out.
     *
     * @param level Minimum log level
     */
    void setLevel(LogLevel level) {
        _minLevel = level;
    }

    /**
     * @brief Get current log level
     */
    LogLevel getLevel() const {
        return _minLevel;
    }

    /**
     * @brief Enable or disable colored output
     */
    void setColors(bool enable) {
        _enableColors = enable;
    }

    /**
     * @brief Enable or disable timestamps
     */
    void setTimestamps(bool enable) {
        _enableTimestamps = enable;
    }

    /**
     * @brief Enable file logging
     *
     * @param enable Enable file logging
     * @param path Path to log file (default: /log/system.log)
     * @return true if successful, false otherwise
     */
    bool setFileLogging(bool enable, const String& path = "/log/system.log") {
        if (enable) {
            _logFilePath = path;

            // Ensure directory exists
            String dir = path.substring(0, path.lastIndexOf('/'));
            if (!SPIFFS.exists(dir)) {
                SPIFFS.mkdir(dir);
            }

            _logFile = SPIFFS.open(path, FILE_APPEND);
            if (!_logFile) {
                Serial.println("Failed to open log file");
                return false;
            }

            _enableFileLogging = true;
            info("Logger", "File logging enabled: " + path);
        } else {
            if (_logFile) {
                _logFile.close();
            }
            _enableFileLogging = false;
        }

        return true;
    }

    /**
     * @brief Log TRACE level message
     */
    void trace(const String& tag, const String& message) {
        write(LogLevel::TRACE, tag, message);
    }

    /**
     * @brief Log DEBUG level message
     */
    void debug(const String& tag, const String& message) {
        write(LogLevel::DEBUG, tag, message);
    }

    /**
     * @brief Log INFO level message
     */
    void info(const String& tag, const String& message) {
        write(LogLevel::INFO, tag, message);
    }

    /**
     * @brief Log WARN level message
     */
    void warn(const String& tag, const String& message) {
        write(LogLevel::WARN, tag, message);
    }

    /**
     * @brief Log ERROR level message
     */
    void error(const String& tag, const String& message) {
        write(LogLevel::ERROR, tag, message);
    }

    /**
     * @brief Log FATAL level message
     */
    void fatal(const String& tag, const String& message) {
        write(LogLevel::FATAL, tag, message);
    }

    /**
     * @brief Log formatted message (printf-style)
     */
    void logf(LogLevel level, const String& tag, const char* format, ...) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        write(level, tag, String(buffer));
    }

    /**
     * @brief Clear log file
     */
    void clearLogFile() {
        if (_logFile) {
            _logFile.close();
        }

        SPIFFS.remove(_logFilePath);

        if (_enableFileLogging) {
            _logFile = SPIFFS.open(_logFilePath, FILE_WRITE);
        }
    }

    /**
     * @brief Get log file path
     */
    String getLogFilePath() const {
        return _logFilePath;
    }

    /**
     * @brief Close logger (call on shutdown)
     */
    void close() {
        if (_logFile) {
            _logFile.close();
        }
    }
};

// Convenience macros for cleaner logging
#define LOG_TRACE(tag, msg) Logger::getInstance().trace(tag, msg)
#define LOG_DEBUG(tag, msg) Logger::getInstance().debug(tag, msg)
#define LOG_INFO(tag, msg) Logger::getInstance().info(tag, msg)
#define LOG_WARN(tag, msg) Logger::getInstance().warn(tag, msg)
#define LOG_ERROR(tag, msg) Logger::getInstance().error(tag, msg)
#define LOG_FATAL(tag, msg) Logger::getInstance().fatal(tag, msg)

#endif // LOGGER_H
