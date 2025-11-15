/**
 * @file Stream.h
 * @brief Domain entity representing a video stream session
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef STREAM_ENTITY_H
#define STREAM_ENTITY_H

#include <Arduino.h>
#include <IPAddress.h>

/**
 * @brief Stream state enumeration
 */
enum class StreamState {
    INACTIVE,
    STARTING,
    ACTIVE,
    PAUSED,
    STOPPING,
    ERROR
};

/**
 * @brief Stream client information
 */
struct StreamClient {
    uint32_t id;
    IPAddress ipAddress;
    uint16_t port;
    uint32_t connectedAt;
    uint32_t framesSent;
    uint32_t framesDropped;
    uint32_t lastFrameTime;
    uint8_t quality;  // JPEG quality for this client
    bool active;

    StreamClient()
        : id(0), port(0), connectedAt(0),
          framesSent(0), framesDropped(0), lastFrameTime(0),
          quality(12), active(false) {}

    StreamClient(uint32_t clientId, IPAddress ip, uint16_t p)
        : id(clientId), ipAddress(ip), port(p),
          connectedAt(millis()), framesSent(0), framesDropped(0),
          lastFrameTime(millis()), quality(12), active(true) {}

    uint32_t getConnectionDuration() const {
        if (!active) return 0;
        return millis() - connectedAt;
    }

    float getCurrentFPS() const {
        if (!active || lastFrameTime == 0 || framesSent == 0) {
            return 0.0f;
        }

        uint32_t elapsed = millis() - lastFrameTime;
        if (elapsed == 0) return 0.0f;

        return 1000.0f / elapsed;
    }

    String toString() const {
        char buffer[128];
        sprintf(buffer, "Client[id=%lu, ip=%s:%d, frames=%lu, fps=%.1f]",
                id, ipAddress.toString().c_str(), port,
                framesSent, getCurrentFPS());
        return String(buffer);
    }
};

/**
 * @brief Stream entity
 *
 * Represents a video streaming session with multiple clients.
 * Manages stream state, clients, and statistics.
 */
class Stream {
private:
    uint32_t _streamId;
    StreamState _state;
    String _errorMessage;

    static constexpr size_t MAX_CLIENTS = 5;
    StreamClient _clients[MAX_CLIENTS];
    size_t _clientCount;

    uint32_t _startedAt;
    uint32_t _totalFramesSent;
    uint32_t _totalFramesDropped;

    uint16_t _minFrameTime;  // Minimum time between frames (ms)
    bool _autoLampEnabled;   // Enable lamp during streaming

public:
    /**
     * @brief Constructor
     */
    Stream(uint32_t id = 0)
        : _streamId(id),
          _state(StreamState::INACTIVE),
          _clientCount(0),
          _startedAt(0),
          _totalFramesSent(0),
          _totalFramesDropped(0),
          _minFrameTime(33),  // ~30 FPS default
          _autoLampEnabled(false) {
        // Initialize all clients as inactive
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            _clients[i].active = false;
        }
    }

    // ========================================================================
    // State management
    // ========================================================================

    uint32_t getStreamId() const { return _streamId; }

    StreamState getState() const { return _state; }

    bool isActive() const {
        return _state == StreamState::ACTIVE || _state == StreamState::PAUSED;
    }

    bool hasError() const {
        return _state == StreamState::ERROR;
    }

    const String& getErrorMessage() const {
        return _errorMessage;
    }

    void setState(StreamState state) {
        _state = state;
        if (state == StreamState::ACTIVE && _startedAt == 0) {
            _startedAt = millis();
        }
        if (state != StreamState::ERROR) {
            _errorMessage = "";
        }
    }

    void setError(const String& message) {
        _state = StreamState::ERROR;
        _errorMessage = message;
    }

    // ========================================================================
    // Client management
    // ========================================================================

    size_t getClientCount() const { return _clientCount; }

    size_t getMaxClients() const { return MAX_CLIENTS; }

    bool isFull() const { return _clientCount >= MAX_CLIENTS; }

    /**
     * @brief Add a new client to the stream
     */
    Result<uint32_t> addClient(IPAddress ip, uint16_t port) {
        if (isFull()) {
            return Result<uint32_t>::error("Stream is full - max clients reached");
        }

        // Find free slot
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if (!_clients[i].active) {
                uint32_t clientId = millis();  // Use timestamp as ID
                _clients[i] = StreamClient(clientId, ip, port);
                _clientCount++;

                return Result<uint32_t>::ok(clientId);
            }
        }

        return Result<uint32_t>::error("No free client slots");
    }

    /**
     * @brief Remove a client from the stream
     */
    bool removeClient(uint32_t clientId) {
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].active && _clients[i].id == clientId) {
                _clients[i].active = false;
                _clientCount--;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get client by ID
     */
    StreamClient* getClient(uint32_t clientId) {
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].active && _clients[i].id == clientId) {
                return &_clients[i];
            }
        }
        return nullptr;
    }

    /**
     * @brief Get all active clients
     */
    void getActiveClients(StreamClient** clients, size_t& count) const {
        count = 0;
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].active) {
                clients[count++] = const_cast<StreamClient*>(&_clients[i]);
            }
        }
    }

    /**
     * @brief Remove all clients
     */
    void removeAllClients() {
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            _clients[i].active = false;
        }
        _clientCount = 0;
    }

    // ========================================================================
    // Stream configuration
    // ========================================================================

    uint16_t getMinFrameTime() const { return _minFrameTime; }

    void setMinFrameTime(uint16_t ms) {
        _minFrameTime = constrain(ms, 10, 1000);  // 10-1000ms (1-100 FPS)
    }

    float getTargetFPS() const {
        if (_minFrameTime == 0) return 0.0f;
        return 1000.0f / _minFrameTime;
    }

    bool isAutoLampEnabled() const { return _autoLampEnabled; }

    void setAutoLampEnabled(bool enabled) { _autoLampEnabled = enabled; }

    // ========================================================================
    // Statistics
    // ========================================================================

    uint32_t getStartedAt() const { return _startedAt; }

    uint32_t getDuration() const {
        if (_startedAt == 0) return 0;
        return millis() - _startedAt;
    }

    uint32_t getTotalFramesSent() const { return _totalFramesSent; }

    uint32_t getTotalFramesDropped() const { return _totalFramesDropped; }

    void incrementFramesSent(uint32_t clientId) {
        _totalFramesSent++;

        StreamClient* client = getClient(clientId);
        if (client) {
            client->framesSent++;
            client->lastFrameTime = millis();
        }
    }

    void incrementFramesDropped(uint32_t clientId) {
        _totalFramesDropped++;

        StreamClient* client = getClient(clientId);
        if (client) {
            client->framesDropped++;
        }
    }

    float getAverageFPS() const {
        uint32_t duration = getDuration();
        if (duration == 0 || _totalFramesSent == 0) {
            return 0.0f;
        }

        return (_totalFramesSent * 1000.0f) / duration;
    }

    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        _totalFramesSent = 0;
        _totalFramesDropped = 0;
        _startedAt = millis();

        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i].active) {
                _clients[i].framesSent = 0;
                _clients[i].framesDropped = 0;
            }
        }
    }

    // ========================================================================
    // Utility methods
    // ========================================================================

    /**
     * @brief Get state as string
     */
    String getStateString() const {
        switch (_state) {
            case StreamState::INACTIVE:  return "Inactive";
            case StreamState::STARTING:  return "Starting";
            case StreamState::ACTIVE:    return "Active";
            case StreamState::PAUSED:    return "Paused";
            case StreamState::STOPPING:  return "Stopping";
            case StreamState::ERROR:     return "Error";
            default:                     return "Unknown";
        }
    }

    /**
     * @brief Convert to string for logging
     */
    String toString() const {
        char buffer[256];
        sprintf(buffer,
                "Stream[id=%lu, state=%s, clients=%zu/%zu, frames=%lu, fps=%.1f]",
                _streamId,
                getStateString().c_str(),
                _clientCount,
                MAX_CLIENTS,
                _totalFramesSent,
                getAverageFPS());
        return String(buffer);
    }
};

#endif // STREAM_ENTITY_H
