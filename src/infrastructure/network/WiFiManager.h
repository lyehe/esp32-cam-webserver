/**
 * @file WiFiManager.h
 * @brief WiFi connection manager with state machine
 *
 * Manages WiFi connectivity with automatic reconnection and state tracking.
 * Supports both station (STA) and access point (AP) modes.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "../../core/Logger.h"
#include "../../core/Result.h"
#include "../../domain/value_objects/Credentials.h"

/**
 * @brief WiFi connection state
 */
enum class WiFiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED,
    AP_MODE
};

/**
 * @brief WiFi configuration mode
 */
enum class WiFiMode {
    STATION,        // Connect to existing network
    ACCESS_POINT,   // Create access point
    BOTH            // Both STA and AP
};

/**
 * @brief WiFi Manager
 *
 * Manages WiFi connectivity with automatic reconnection.
 * Implements state machine for robust connection handling.
 */
class WiFiManager {
private:
    static constexpr const char* TAG = "WiFiMgr";
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 5000;
    static constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 10;
    static constexpr const char* DEFAULT_AP_SSID = "ESP32-CAM";
    static constexpr const char* DEFAULT_AP_PASSWORD = "esp32cam123";

    WiFiState _state;
    WiFiMode _mode;
    WiFiCredentials _staCredentials;
    WiFiCredentials _apCredentials;
    uint32_t _lastConnectionAttempt;
    uint8_t _reconnectAttempts;
    IPAddress _ipAddress;
    IPAddress _apIpAddress;
    String _hostname;

    /**
     * @brief WiFi event handler
     */
    static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
        // Note: Static method, can't access instance members directly
        // Consider using singleton pattern or callback registration
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                Logger::getInstance().info(TAG, "Got IP: " + WiFi.localIP().toString());
                break;

            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                Logger::getInstance().warn(TAG, "WiFi disconnected");
                break;

            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Logger::getInstance().info(TAG, "WiFi connected");
                break;

            default:
                break;
        }
    }

public:
    /**
     * @brief Constructor
     */
    WiFiManager(const String& hostname = "esp32-cam")
        : _state(WiFiState::DISCONNECTED),
          _mode(WiFiMode::STATION),
          _lastConnectionAttempt(0),
          _reconnectAttempts(0),
          _hostname(hostname) {}

    /**
     * @brief Initialize WiFi
     */
    Result<void> initialize() {
        Logger::getInstance().info(TAG, "Initializing WiFi manager");

        WiFi.mode(WIFI_STA);
        WiFi.setHostname(_hostname.c_str());

        // Register event handler
        WiFi.onEvent(onWiFiEvent);

        return Result<void>::ok();
    }

    /**
     * @brief Connect to WiFi network (Station mode)
     */
    Result<void> connect(const WiFiCredentials& credentials, uint32_t timeoutMs = 10000) {
        Logger::getInstance().info(TAG, "Connecting to WiFi: " + credentials.getSSID());

        _staCredentials = credentials;
        _state = WiFiState::CONNECTING;
        _reconnectAttempts = 0;

        WiFi.mode(WIFI_STA);
        WiFi.begin(credentials.getSSID().c_str(),
                   credentials.isOpenNetwork() ? nullptr : credentials.getPassword().c_str());

        uint32_t startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startTime > timeoutMs) {
                _state = WiFiState::FAILED;
                Logger::getInstance().error(TAG, "WiFi connection timeout");
                return Result<void>::error("Connection timeout");
            }

            delay(100);
        }

        _state = WiFiState::CONNECTED;
        _ipAddress = WiFi.localIP();
        _mode = WiFiMode::STATION;

        Logger::getInstance().info(TAG, "WiFi connected!");
        Logger::getInstance().info(TAG, "IP: " + _ipAddress.toString());
        Logger::getInstance().info(TAG, "RSSI: " + String(WiFi.RSSI()) + " dBm");

        return Result<void>::ok();
    }

    /**
     * @brief Start Access Point mode
     */
    Result<void> startAP(const WiFiCredentials& credentials = WiFiCredentials()) {
        WiFiCredentials apCreds = credentials;

        // Use defaults if not provided
        if (!apCreds.isValid()) {
            auto defaultResult = WiFiCredentials::create(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
            if (defaultResult.isError()) {
                return Result<void>::error("Failed to create default AP credentials");
            }
            apCreds = defaultResult.getValue();
        }

        _apCredentials = apCreds;

        Logger::getInstance().info(TAG, "Starting Access Point: " + apCreds.getSSID());

        WiFi.mode(WIFI_AP);
        bool success = WiFi.softAP(apCreds.getSSID().c_str(),
                                    apCreds.isOpenNetwork() ? nullptr : apCreds.getPassword().c_str());

        if (!success) {
            _state = WiFiState::FAILED;
            Logger::getInstance().error(TAG, "Failed to start AP");
            return Result<void>::error("AP start failed");
        }

        delay(100); // Allow AP to initialize

        _apIpAddress = WiFi.softAPIP();
        _state = WiFiState::AP_MODE;
        _mode = WiFiMode::ACCESS_POINT;

        Logger::getInstance().info(TAG, "Access Point started");
        Logger::getInstance().info(TAG, "AP IP: " + _apIpAddress.toString());

        return Result<void>::ok();
    }

    /**
     * @brief Start in both STA and AP mode
     */
    Result<void> startDual(const WiFiCredentials& staCredentials,
                          const WiFiCredentials& apCredentials = WiFiCredentials()) {
        Logger::getInstance().info(TAG, "Starting dual WiFi mode (STA + AP)");

        WiFi.mode(WIFI_AP_STA);

        // Start AP first
        WiFiCredentials apCreds = apCredentials;
        if (!apCreds.isValid()) {
            auto defaultResult = WiFiCredentials::create(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
            if (defaultResult.isError()) {
                return Result<void>::error("Failed to create default AP credentials");
            }
            apCreds = defaultResult.getValue();
        }

        _apCredentials = apCreds;
        WiFi.softAP(apCreds.getSSID().c_str(),
                    apCreds.isOpenNetwork() ? nullptr : apCreds.getPassword().c_str());

        _apIpAddress = WiFi.softAPIP();
        Logger::getInstance().info(TAG, "AP started: " + _apIpAddress.toString());

        // Connect to WiFi
        _staCredentials = staCredentials;
        WiFi.begin(staCredentials.getSSID().c_str(),
                   staCredentials.isOpenNetwork() ? nullptr : staCredentials.getPassword().c_str());

        // Wait for connection (non-blocking check)
        uint32_t startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            _state = WiFiState::CONNECTED;
            _ipAddress = WiFi.localIP();
            Logger::getInstance().info(TAG, "STA connected: " + _ipAddress.toString());
        } else {
            _state = WiFiState::AP_MODE;
            Logger::getInstance().warn(TAG, "STA connection failed, AP mode only");
        }

        _mode = WiFiMode::BOTH;
        return Result<void>::ok();
    }

    /**
     * @brief Disconnect from WiFi
     */
    Result<void> disconnect() {
        Logger::getInstance().info(TAG, "Disconnecting WiFi");

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        _state = WiFiState::DISCONNECTED;
        _ipAddress = IPAddress(0, 0, 0, 0);

        return Result<void>::ok();
    }

    /**
     * @brief Automatic reconnection handler
     *
     * Call this periodically from main loop to handle auto-reconnect
     */
    void update() {
        // Only auto-reconnect in station mode
        if (_mode == WiFiMode::ACCESS_POINT) {
            return;
        }

        // Check if we need to reconnect
        if (_state == WiFiState::CONNECTED && WiFi.status() != WL_CONNECTED) {
            Logger::getInstance().warn(TAG, "WiFi connection lost");
            _state = WiFiState::DISCONNECTED;
        }

        // Attempt reconnection
        if (_state == WiFiState::DISCONNECTED || _state == WiFiState::FAILED) {
            uint32_t now = millis();

            if (now - _lastConnectionAttempt >= RECONNECT_INTERVAL_MS) {
                if (_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                    Logger::getInstance().info(TAG, "Reconnection attempt " +
                                                   String(_reconnectAttempts + 1) + "/" +
                                                   String(MAX_RECONNECT_ATTEMPTS));

                    _lastConnectionAttempt = now;
                    _reconnectAttempts++;

                    // Try to reconnect
                    connect(_staCredentials, 5000);
                } else {
                    Logger::getInstance().error(TAG, "Max reconnection attempts reached");
                    _state = WiFiState::FAILED;
                    // Consider switching to AP mode as fallback
                }
            }
        }

        // Reset reconnect counter when connected
        if (_state == WiFiState::CONNECTED) {
            _reconnectAttempts = 0;
        }
    }

    /**
     * @brief Get current WiFi state
     */
    WiFiState getState() const {
        return _state;
    }

    /**
     * @brief Check if connected
     */
    bool isConnected() const {
        return _state == WiFiState::CONNECTED && WiFi.status() == WL_CONNECTED;
    }

    /**
     * @brief Get IP address (Station mode)
     */
    IPAddress getIPAddress() const {
        return _ipAddress;
    }

    /**
     * @brief Get AP IP address
     */
    IPAddress getAPIPAddress() const {
        return _apIpAddress;
    }

    /**
     * @brief Get RSSI (signal strength)
     */
    int32_t getRSSI() const {
        return WiFi.RSSI();
    }

    /**
     * @brief Get SSID
     */
    String getSSID() const {
        if (_mode == WiFiMode::STATION || _mode == WiFiMode::BOTH) {
            return WiFi.SSID();
        } else {
            return _apCredentials.getSSID();
        }
    }

    /**
     * @brief Get MAC address
     */
    String getMACAddress() const {
        return WiFi.macAddress();
    }

    /**
     * @brief Get hostname
     */
    String getHostname() const {
        return _hostname;
    }

    /**
     * @brief Set hostname
     */
    void setHostname(const String& hostname) {
        _hostname = hostname;
        WiFi.setHostname(hostname.c_str());
    }

    /**
     * @brief Scan for WiFi networks
     */
    Result<std::vector<String>> scanNetworks() {
        Logger::getInstance().info(TAG, "Scanning for WiFi networks...");

        int n = WiFi.scanNetworks();
        std::vector<String> networks;

        if (n < 0) {
            return Result<std::vector<String>>::error("Scan failed");
        }

        for (int i = 0; i < n; i++) {
            String network = WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
            if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
                network += " [OPEN]";
            }
            networks.push_back(network);
        }

        WiFi.scanDelete();

        Logger::getInstance().info(TAG, "Found " + String(n) + " networks");
        return Result<std::vector<String>>::ok(networks);
    }

    /**
     * @brief Get connection status string
     */
    String getStatusString() const {
        switch (_state) {
            case WiFiState::DISCONNECTED: return "Disconnected";
            case WiFiState::CONNECTING:   return "Connecting";
            case WiFiState::CONNECTED:    return "Connected";
            case WiFiState::FAILED:       return "Failed";
            case WiFiState::AP_MODE:      return "Access Point";
            default:                      return "Unknown";
        }
    }

    /**
     * @brief Get detailed connection info
     */
    String getConnectionInfo() const {
        String info = "WiFi Status: " + getStatusString() + "\n";

        if (_mode == WiFiMode::STATION || _mode == WiFiMode::BOTH) {
            if (_state == WiFiState::CONNECTED) {
                info += "SSID: " + WiFi.SSID() + "\n";
                info += "IP: " + _ipAddress.toString() + "\n";
                info += "RSSI: " + String(WiFi.RSSI()) + " dBm\n";
                info += "MAC: " + WiFi.macAddress() + "\n";
            }
        }

        if (_mode == WiFiMode::ACCESS_POINT || _mode == WiFiMode::BOTH) {
            info += "AP SSID: " + _apCredentials.getSSID() + "\n";
            info += "AP IP: " + _apIpAddress.toString() + "\n";
            info += "AP Clients: " + String(WiFi.softAPgetStationNum()) + "\n";
        }

        return info;
    }
};

#endif // WIFI_MANAGER_H
