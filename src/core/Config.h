/**
 * @file Config.h
 * @brief Centralized configuration management system
 *
 * Manages application configuration from JSON files with type-safe access,
 * validation, and default values.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "Result.h"
#include "Logger.h"

/**
 * @brief Configuration management singleton
 *
 * Loads configuration from JSON file and provides type-safe access
 * to configuration values with validation and defaults.
 *
 * @example
 * Config& config = Config::getInstance();
 * if (config.load("/config.json").isOk()) {
 *     String ssid = config.getWiFiSSID();
 *     int port = config.getHttpPort();
 * }
 */
class Config {
private:
    StaticJsonDocument<4096> _doc;
    bool _loaded;
    String _configPath;

    /**
     * @brief Private constructor for singleton
     */
    Config() : _loaded(false), _configPath("/config.json") {}

    /**
     * @brief Set default configuration values
     */
    void setDefaults() {
        // WiFi defaults
        _doc["wifi"]["mode"] = "sta";  // sta, ap, or both
        _doc["wifi"]["sta"]["ssid"] = "ESP32-CAM-CONNECT";
        _doc["wifi"]["sta"]["password"] = "InsecurePassword";
        _doc["wifi"]["sta"]["dhcp"] = true;
        _doc["wifi"]["sta"]["static_ip"] = "";
        _doc["wifi"]["sta"]["gateway"] = "";
        _doc["wifi"]["sta"]["subnet"] = "";
        _doc["wifi"]["sta"]["dns"] = "";

        _doc["wifi"]["ap"]["ssid"] = "ESP32-CAM-AP";
        _doc["wifi"]["ap"]["password"] = "12345678";
        _doc["wifi"]["ap"]["channel"] = 1;
        _doc["wifi"]["ap"]["max_connections"] = 4;
        _doc["wifi"]["ap"]["hidden"] = false;

        // Camera defaults
        _doc["camera"]["model"] = "AI_THINKER";
        _doc["camera"]["framesize"] = 10;  // FRAMESIZE_SVGA
        _doc["camera"]["quality"] = 12;
        _doc["camera"]["brightness"] = 0;
        _doc["camera"]["contrast"] = 0;
        _doc["camera"]["saturation"] = 0;
        _doc["camera"]["xclk_mhz"] = 20;
        _doc["camera"]["fb_count"] = 2;

        // Server defaults
        _doc["server"]["http_port"] = 80;
        _doc["server"]["stream_port"] = 81;
        _doc["server"]["max_clients"] = 5;
        _doc["server"]["min_frame_time"] = 33;  // ~30 FPS

        // Security defaults
        _doc["security"]["enable_auth"] = false;
        _doc["security"]["jwt_secret"] = "change-me-in-production";
        _doc["security"]["token_lifetime"] = 3600;
        _doc["security"]["rate_limit_enabled"] = true;
        _doc["security"]["rate_limit_requests"] = 100;
        _doc["security"]["rate_limit_window"] = 60;

        // System defaults
        _doc["system"]["name"] = "ESP32-CAM";
        _doc["system"]["mdns_name"] = "esp32-cam";
        _doc["system"]["timezone"] = "UTC";
        _doc["system"]["ntp_server"] = "pool.ntp.org";
        _doc["system"]["log_level"] = "INFO";
        _doc["system"]["watchdog_timeout"] = 30;

        // OTA defaults
        _doc["ota"]["enabled"] = true;
        _doc["ota"]["password"] = "";
        _doc["ota"]["port"] = 3232;

        // Lamp defaults
        _doc["lamp"]["enabled"] = true;
        _doc["lamp"]["auto_mode"] = false;
        _doc["lamp"]["default_value"] = 0;
    }

public:
    /**
     * @brief Get singleton instance
     */
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    // Prevent copying and assignment
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    /**
     * @brief Load configuration from JSON file
     *
     * @param path Path to configuration file
     * @return Result<void> Success or error
     */
    Result<void> load(const String& path = "/config.json") {
        _configPath = path;

        // Set defaults first
        setDefaults();

        // Check if file exists
        if (!SPIFFS.exists(path)) {
            LOG_WARN("Config", "Config file not found, using defaults: " + path);
            _loaded = true;
            return Result<void>::ok();
        }

        // Open file
        File file = SPIFFS.open(path, FILE_READ);
        if (!file) {
            return Result<void>::error("Failed to open config file: " + path);
        }

        // Parse JSON
        DeserializationError error = deserializeJson(_doc, file);
        file.close();

        if (error) {
            return Result<void>::error("Failed to parse config JSON: " + String(error.c_str()));
        }

        _loaded = true;
        LOG_INFO("Config", "Configuration loaded from: " + path);
        return Result<void>::ok();
    }

    /**
     * @brief Save configuration to JSON file
     *
     * @param path Path to save configuration (optional, uses loaded path by default)
     * @return Result<void> Success or error
     */
    Result<void> save(const String& path = "") {
        String savePath = path.length() > 0 ? path : _configPath;

        File file = SPIFFS.open(savePath, FILE_WRITE);
        if (!file) {
            return Result<void>::error("Failed to open config file for writing: " + savePath);
        }

        if (serializeJsonPretty(_doc, file) == 0) {
            file.close();
            return Result<void>::error("Failed to write config to file");
        }

        file.close();
        LOG_INFO("Config", "Configuration saved to: " + savePath);
        return Result<void>::ok();
    }

    /**
     * @brief Check if configuration is loaded
     */
    bool isLoaded() const { return _loaded; }

    // ========================================================================
    // WiFi Configuration Accessors
    // ========================================================================

    String getWiFiMode() const { return _doc["wifi"]["mode"] | "sta"; }
    String getWiFiSSID() const { return _doc["wifi"]["sta"]["ssid"] | "ESP32-CAM-CONNECT"; }
    String getWiFiPassword() const { return _doc["wifi"]["sta"]["password"] | ""; }
    bool getWiFiDHCP() const { return _doc["wifi"]["sta"]["dhcp"] | true; }
    String getWiFiStaticIP() const { return _doc["wifi"]["sta"]["static_ip"] | ""; }
    String getWiFiGateway() const { return _doc["wifi"]["sta"]["gateway"] | ""; }
    String getWiFiSubnet() const { return _doc["wifi"]["sta"]["subnet"] | ""; }
    String getWiFiDNS() const { return _doc["wifi"]["sta"]["dns"] | ""; }

    String getAPSSID() const { return _doc["wifi"]["ap"]["ssid"] | "ESP32-CAM-AP"; }
    String getAPPassword() const { return _doc["wifi"]["ap"]["password"] | "12345678"; }
    int getAPChannel() const { return _doc["wifi"]["ap"]["channel"] | 1; }
    int getAPMaxConnections() const { return _doc["wifi"]["ap"]["max_connections"] | 4; }
    bool getAPHidden() const { return _doc["wifi"]["ap"]["hidden"] | false; }

    // ========================================================================
    // Camera Configuration Accessors
    // ========================================================================

    String getCameraModel() const { return _doc["camera"]["model"] | "AI_THINKER"; }
    int getCameraFrameSize() const { return _doc["camera"]["framesize"] | 10; }
    int getCameraQuality() const { return _doc["camera"]["quality"] | 12; }
    int getCameraBrightness() const { return _doc["camera"]["brightness"] | 0; }
    int getCameraContrast() const { return _doc["camera"]["contrast"] | 0; }
    int getCameraSaturation() const { return _doc["camera"]["saturation"] | 0; }
    int getCameraXCLK() const { return _doc["camera"]["xclk_mhz"] | 20; }
    int getCameraFBCount() const { return _doc["camera"]["fb_count"] | 2; }

    void setCameraFrameSize(int value) { _doc["camera"]["framesize"] = value; }
    void setCameraQuality(int value) { _doc["camera"]["quality"] = value; }
    void setCameraBrightness(int value) { _doc["camera"]["brightness"] = value; }
    void setCameraContrast(int value) { _doc["camera"]["contrast"] = value; }
    void setCameraSaturation(int value) { _doc["camera"]["saturation"] = value; }

    // ========================================================================
    // Server Configuration Accessors
    // ========================================================================

    int getHttpPort() const { return _doc["server"]["http_port"] | 80; }
    int getStreamPort() const { return _doc["server"]["stream_port"] | 81; }
    int getMaxClients() const { return _doc["server"]["max_clients"] | 5; }
    int getMinFrameTime() const { return _doc["server"]["min_frame_time"] | 33; }

    void setMinFrameTime(int value) { _doc["server"]["min_frame_time"] = value; }

    // ========================================================================
    // Security Configuration Accessors
    // ========================================================================

    bool isAuthEnabled() const { return _doc["security"]["enable_auth"] | false; }
    String getJWTSecret() const { return _doc["security"]["jwt_secret"] | "change-me"; }
    int getTokenLifetime() const { return _doc["security"]["token_lifetime"] | 3600; }
    bool isRateLimitEnabled() const { return _doc["security"]["rate_limit_enabled"] | true; }
    int getRateLimitRequests() const { return _doc["security"]["rate_limit_requests"] | 100; }
    int getRateLimitWindow() const { return _doc["security"]["rate_limit_window"] | 60; }

    void setAuthEnabled(bool value) { _doc["security"]["enable_auth"] = value; }
    void setJWTSecret(const String& value) { _doc["security"]["jwt_secret"] = value; }

    // ========================================================================
    // System Configuration Accessors
    // ========================================================================

    String getSystemName() const { return _doc["system"]["name"] | "ESP32-CAM"; }
    String getMDNSName() const { return _doc["system"]["mdns_name"] | "esp32-cam"; }
    String getTimezone() const { return _doc["system"]["timezone"] | "UTC"; }
    String getNTPServer() const { return _doc["system"]["ntp_server"] | "pool.ntp.org"; }
    String getLogLevel() const { return _doc["system"]["log_level"] | "INFO"; }
    int getWatchdogTimeout() const { return _doc["system"]["watchdog_timeout"] | 30; }

    void setSystemName(const String& value) { _doc["system"]["name"] = value; }

    // ========================================================================
    // OTA Configuration Accessors
    // ========================================================================

    bool isOTAEnabled() const { return _doc["ota"]["enabled"] | true; }
    String getOTAPassword() const { return _doc["ota"]["password"] | ""; }
    int getOTAPort() const { return _doc["ota"]["port"] | 3232; }

    void setOTAEnabled(bool value) { _doc["ota"]["enabled"] = value; }
    void setOTAPassword(const String& value) { _doc["ota"]["password"] = value; }

    // ========================================================================
    // Lamp Configuration Accessors
    // ========================================================================

    bool isLampEnabled() const { return _doc["lamp"]["enabled"] | true; }
    bool isLampAutoMode() const { return _doc["lamp"]["auto_mode"] | false; }
    int getLampDefaultValue() const { return _doc["lamp"]["default_value"] | 0; }

    void setLampAutoMode(bool value) { _doc["lamp"]["auto_mode"] = value; }
    void setLampDefaultValue(int value) { _doc["lamp"]["default_value"] = value; }

    // ========================================================================
    // Direct JSON Access for Advanced Use
    // ========================================================================

    /**
     * @brief Get raw JSON document (read-only)
     */
    const JsonDocument& getDocument() const { return _doc; }

    /**
     * @brief Get raw JSON document (writable)
     */
    JsonDocument& getDocument() { return _doc; }

    /**
     * @brief Get configuration as JSON string
     */
    String toString() const {
        String output;
        serializeJsonPretty(_doc, output);
        return output;
    }

    /**
     * @brief Validate configuration
     *
     * Checks that all required fields are present and values are in valid ranges
     *
     * @return Result<void> Success or error with validation message
     */
    Result<void> validate() const {
        // Validate WiFi SSID length
        String ssid = getWiFiSSID();
        if (ssid.length() == 0 || ssid.length() > 32) {
            return Result<void>::error("Invalid WiFi SSID length");
        }

        // Validate ports
        int httpPort = getHttpPort();
        if (httpPort < 1 || httpPort > 65535) {
            return Result<void>::error("Invalid HTTP port");
        }

        int streamPort = getStreamPort();
        if (streamPort < 1 || streamPort > 65535) {
            return Result<void>::error("Invalid stream port");
        }

        // Validate camera settings
        int frameSize = getCameraFrameSize();
        if (frameSize < 0 || frameSize > 13) {
            return Result<void>::error("Invalid frame size");
        }

        int quality = getCameraQuality();
        if (quality < 0 || quality > 63) {
            return Result<void>::error("Invalid JPEG quality");
        }

        // Validate max clients
        int maxClients = getMaxClients();
        if (maxClients < 1 || maxClients > 10) {
            return Result<void>::error("Invalid max clients (must be 1-10)");
        }

        return Result<void>::ok();
    }
};

#endif // CONFIG_H
