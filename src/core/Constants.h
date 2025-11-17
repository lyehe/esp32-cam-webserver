/**
 * @file Constants.h
 * @brief Global constants for ESP32 CAM Webserver
 *
 * Centralizes magic numbers to improve maintainability and clarity.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

namespace Constants {
    // ========================================================================
    // Thread Safety
    // ========================================================================

    /** Default mutex timeout in milliseconds */
    constexpr uint32_t MUTEX_TIMEOUT_MS = 1000;

    /** Short mutex timeout for non-critical operations */
    constexpr uint32_t MUTEX_TIMEOUT_SHORT_MS = 100;

    /** Long mutex timeout for blocking operations */
    constexpr uint32_t MUTEX_TIMEOUT_LONG_MS = 5000;

    // ========================================================================
    // Frame Rate & Timing
    // ========================================================================

    /** Default streaming frame rate */
    constexpr uint32_t DEFAULT_FPS = 30;

    /** Frame interval in milliseconds for 30 FPS */
    constexpr uint32_t FRAME_INTERVAL_MS = 1000 / DEFAULT_FPS;  // 33ms

    /** Low frame rate for power saving */
    constexpr uint32_t LOW_POWER_FPS = 10;

    /** High frame rate for performance mode */
    constexpr uint32_t HIGH_PERF_FPS = 60;

    // ========================================================================
    // Authentication & Security
    // ========================================================================

    /** Default JWT token expiry in seconds (1 hour) */
    constexpr uint32_t TOKEN_EXPIRY_SECONDS = 3600;

    /** Extended token expiry for "remember me" (7 days) */
    constexpr uint32_t TOKEN_EXPIRY_EXTENDED_SECONDS = 7 * 24 * 3600;

    /** Minimum password length */
    constexpr uint8_t MIN_PASSWORD_LENGTH = 8;

    /** Maximum password length */
    constexpr uint8_t MAX_PASSWORD_LENGTH = 64;

    /** Maximum username length */
    constexpr uint8_t MAX_USERNAME_LENGTH = 32;

    // ========================================================================
    // Buffer Sizes
    // ========================================================================

    /** Small buffer size (256 bytes) */
    constexpr size_t BUFFER_SIZE_SMALL = 256;

    /** Medium buffer size (1 KB) */
    constexpr size_t BUFFER_SIZE_MEDIUM = 1024;

    /** Large buffer size (4 KB) */
    constexpr size_t BUFFER_SIZE_LARGE = 4096;

    /** JSON document size for configuration */
    constexpr size_t JSON_CONFIG_SIZE = 4096;

    /** JSON document size for camera settings */
    constexpr size_t JSON_CAMERA_SETTINGS_SIZE = 2048;

    /** JSON document size for user data */
    constexpr size_t JSON_USER_DATA_SIZE = 512;

    // ========================================================================
    // Streaming
    // ========================================================================

    /** Maximum concurrent streams */
    constexpr uint8_t MAX_STREAMS = 10;

    /** Maximum clients per stream */
    constexpr uint8_t MAX_CLIENTS_PER_STREAM = 5;

    /** Stream inactivity timeout in milliseconds */
    constexpr uint32_t STREAM_TIMEOUT_MS = 30000;  // 30 seconds

    /** Client heartbeat interval in milliseconds */
    constexpr uint32_t CLIENT_HEARTBEAT_MS = 5000;  // 5 seconds

    // ========================================================================
    // Camera Settings
    // ========================================================================

    /** JPEG quality range: minimum (worst quality, best compression) */
    constexpr uint8_t JPEG_QUALITY_MIN = 0;

    /** JPEG quality range: maximum (best quality, worst compression) */
    constexpr uint8_t JPEG_QUALITY_MAX = 63;

    /** Recommended JPEG quality for balanced mode */
    constexpr uint8_t JPEG_QUALITY_BALANCED = 12;

    /** JPEG quality for speed mode */
    constexpr uint8_t JPEG_QUALITY_SPEED = 20;

    /** JPEG quality for quality mode */
    constexpr uint8_t JPEG_QUALITY_BEST = 4;

    /** LED/Lamp intensity: minimum */
    constexpr uint8_t INTENSITY_MIN = 0;

    /** LED/Lamp intensity: maximum */
    constexpr uint8_t INTENSITY_MAX = 255;

    /** Camera adjustment range: minimum (brightness, contrast, saturation) */
    constexpr int8_t ADJUSTMENT_MIN = -2;

    /** Camera adjustment range: maximum (brightness, contrast, saturation) */
    constexpr int8_t ADJUSTMENT_MAX = 2;

    // ========================================================================
    // HTTP & Network
    // ========================================================================

    /** HTTP request timeout in milliseconds */
    constexpr uint32_t HTTP_TIMEOUT_MS = 10000;  // 10 seconds

    /** WebSocket ping interval in milliseconds */
    constexpr uint32_t WEBSOCKET_PING_MS = 30000;  // 30 seconds

    /** Maximum HTTP request body size */
    constexpr size_t HTTP_MAX_BODY_SIZE = 16384;  // 16 KB

    // ========================================================================
    // WiFi & Connectivity
    // ========================================================================

    /** WiFi connection timeout in milliseconds */
    constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;  // 30 seconds

    /** WiFi reconnect delay in milliseconds */
    constexpr uint32_t WIFI_RECONNECT_DELAY_MS = 5000;  // 5 seconds

    /** WiFi scan timeout in milliseconds */
    constexpr uint32_t WIFI_SCAN_TIMEOUT_MS = 10000;  // 10 seconds

    // ========================================================================
    // Storage & Filesystem
    // ========================================================================

    /** SPIFFS mount point */
    constexpr const char* SPIFFS_MOUNT_POINT = "/spiffs";

    /** Maximum filename length */
    constexpr uint8_t MAX_FILENAME_LENGTH = 64;

    /** Configuration file path */
    constexpr const char* CONFIG_FILE_PATH = "/config.json";

    /** Camera settings file path */
    constexpr const char* CAMERA_SETTINGS_PATH = "/camera_settings.json";

    /** Users database file path */
    constexpr const char* USERS_DB_PATH = "/users.json";

    // ========================================================================
    // Task Priorities (FreeRTOS)
    // ========================================================================

    /** Stream task priority */
    constexpr UBaseType_t STREAM_TASK_PRIORITY = 2;

    /** HTTP server task priority */
    constexpr UBaseType_t HTTP_TASK_PRIORITY = 1;

    /** WiFi manager task priority */
    constexpr UBaseType_t WIFI_TASK_PRIORITY = 1;

    // ========================================================================
    // Task Stack Sizes (FreeRTOS)
    // ========================================================================

    /** Stream task stack size in bytes */
    constexpr uint32_t STREAM_TASK_STACK_SIZE = 4096;

    /** HTTP handler task stack size in bytes */
    constexpr uint32_t HTTP_TASK_STACK_SIZE = 8192;

    /** WiFi manager task stack size in bytes */
    constexpr uint32_t WIFI_TASK_STACK_SIZE = 4096;

    // ========================================================================
    // Hardware
    // ========================================================================

    /** Minimum PSRAM size required (in bytes) */
    constexpr size_t MIN_PSRAM_SIZE = 2 * 1024 * 1024;  // 2 MB

    /** Camera I2C bus speed (Hz) */
    constexpr uint32_t CAMERA_I2C_SPEED = 100000;  // 100 kHz

    /** Camera XCLK frequency (Hz) */
    constexpr uint32_t CAMERA_XCLK_FREQ = 20000000;  // 20 MHz

    // ========================================================================
    // Logging
    // ========================================================================

    /** Maximum log message length */
    constexpr size_t MAX_LOG_MESSAGE_LENGTH = 256;

    /** Log file rotation size (bytes) */
    constexpr size_t LOG_FILE_MAX_SIZE = 1024 * 1024;  // 1 MB

    /** Maximum log files to keep */
    constexpr uint8_t MAX_LOG_FILES = 3;
}

#endif // CONSTANTS_H
