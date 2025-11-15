/**
 * @file StreamRepository.h
 * @brief Stream repository implementation for multi-client streaming
 *
 * Implements IStreamRepository with support for up to 5 concurrent clients.
 * Uses FreeRTOS tasks for asynchronous frame broadcasting.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef STREAM_REPOSITORY_H
#define STREAM_REPOSITORY_H

#include "../../domain/repositories/IStreamRepository.h"
#include "../../core/Logger.h"
#include <WiFi.h>
#include <map>
#include <vector>

/**
 * @brief Client connection state
 */
enum class ClientConnectionState {
    CONNECTED,
    STREAMING,
    DISCONNECTED,
    ERROR
};

/**
 * @brief Stream client connection info
 */
struct StreamClientConnection {
    uint32_t clientId;
    WiFiClient wifiClient;
    IPAddress ipAddress;
    uint16_t port;
    ClientConnectionState state;
    uint32_t framesSent;
    uint32_t framesDropped;
    uint32_t lastFrameTime;
    uint32_t connectedAt;

    StreamClientConnection()
        : clientId(0), port(0), state(ClientConnectionState::DISCONNECTED),
          framesSent(0), framesDropped(0), lastFrameTime(0), connectedAt(0) {}

    float getCurrentFPS() const {
        if (lastFrameTime == 0 || framesSent == 0) {
            return 0.0f;
        }
        uint32_t elapsed = millis() - lastFrameTime;
        if (elapsed == 0) return 0.0f;
        return 1000.0f / elapsed;
    }
};

/**
 * @brief Active stream info
 */
struct ActiveStream {
    uint32_t streamId;
    uint32_t startedAt;
    bool active;
    std::vector<StreamClientConnection> clients;
    uint16_t minFrameTime; // Minimum time between frames (ms)

    ActiveStream()
        : streamId(0), startedAt(0), active(false), minFrameTime(33) {} // ~30 FPS default
};

/**
 * @brief ESP32 Stream Repository
 *
 * Manages multiple concurrent streaming clients.
 * Supports up to 5 clients streaming simultaneously.
 */
class StreamRepository : public IStreamRepository {
private:
    static constexpr const char* TAG = "StreamRepo";
    static constexpr size_t MAX_STREAMS = 3;      // Maximum concurrent streams
    static constexpr size_t MAX_CLIENTS_PER_STREAM = 5; // Maximum clients per stream

    std::map<uint32_t, ActiveStream> _streams;
    uint32_t _nextStreamId;
    uint32_t _nextClientId;
    SemaphoreHandle_t _mutex;

    /**
     * @brief Generate next stream ID
     */
    uint32_t generateStreamId() {
        return _nextStreamId++;
    }

    /**
     * @brief Generate next client ID
     */
    uint32_t generateClientId() {
        return _nextClientId++;
    }

    /**
     * @brief Find stream by ID
     */
    ActiveStream* findStream(uint32_t streamId) {
        auto it = _streams.find(streamId);
        if (it != _streams.end()) {
            return &it->second;
        }
        return nullptr;
    }

    /**
     * @brief Find client in stream
     */
    StreamClientConnection* findClient(ActiveStream* stream, uint32_t clientId) {
        if (!stream) return nullptr;

        for (auto& client : stream->clients) {
            if (client.clientId == clientId) {
                return &client;
            }
        }
        return nullptr;
    }

    /**
     * @brief Remove disconnected clients from stream
     */
    void cleanupDisconnectedClients(ActiveStream* stream) {
        if (!stream) return;

        stream->clients.erase(
            std::remove_if(stream->clients.begin(), stream->clients.end(),
                [](const StreamClientConnection& client) {
                    return client.state == ClientConnectionState::DISCONNECTED ||
                           client.state == ClientConnectionState::ERROR;
                }),
            stream->clients.end()
        );
    }

public:
    /**
     * @brief Constructor
     */
    StreamRepository()
        : _nextStreamId(1), _nextClientId(1) {
        _mutex = xSemaphoreCreateMutex();
        Logger::getInstance().info(TAG, "Stream repository initialized");
    }

    /**
     * @brief Destructor
     */
    ~StreamRepository() {
        if (_mutex) {
            vSemaphoreDelete(_mutex);
        }
    }

    /**
     * @brief Start a new stream
     */
    Result<uint32_t> startStream() override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        if (_streams.size() >= MAX_STREAMS) {
            xSemaphoreGive(_mutex);
            return Result<uint32_t>::error("Maximum number of streams reached");
        }

        uint32_t streamId = generateStreamId();
        ActiveStream stream;
        stream.streamId = streamId;
        stream.startedAt = millis();
        stream.active = true;

        _streams[streamId] = stream;

        xSemaphoreGive(_mutex);

        Logger::getInstance().info(TAG, "Stream started: " + String(streamId));
        return Result<uint32_t>::ok(streamId);
    }

    /**
     * @brief Stop a stream
     */
    Result<void> stopStream(uint32_t streamId) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Stream not found");
        }

        // Disconnect all clients
        for (auto& client : stream->clients) {
            if (client.wifiClient.connected()) {
                client.wifiClient.stop();
            }
            client.state = ClientConnectionState::DISCONNECTED;
        }

        stream->active = false;
        _streams.erase(streamId);

        xSemaphoreGive(_mutex);

        Logger::getInstance().info(TAG, "Stream stopped: " + String(streamId));
        return Result<void>::ok();
    }

    /**
     * @brief Add a client to stream
     */
    Result<uint32_t> addClient(uint32_t streamId, IPAddress clientIp, uint16_t clientPort) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<uint32_t>::error("Stream not found");
        }

        // Clean up disconnected clients first
        cleanupDisconnectedClients(stream);

        if (stream->clients.size() >= MAX_CLIENTS_PER_STREAM) {
            xSemaphoreGive(_mutex);
            return Result<uint32_t>::error("Maximum clients reached for this stream");
        }

        StreamClientConnection client;
        client.clientId = generateClientId();
        client.ipAddress = clientIp;
        client.port = clientPort;
        client.state = ClientConnectionState::CONNECTED;
        client.connectedAt = millis();

        stream->clients.push_back(client);

        xSemaphoreGive(_mutex);

        Logger::getInstance().info(TAG, "Client added to stream " + String(streamId) +
                                       ": " + clientIp.toString() + ":" + String(clientPort));

        return Result<uint32_t>::ok(client.clientId);
    }

    /**
     * @brief Remove a client from stream
     */
    Result<void> removeClient(uint32_t streamId, uint32_t clientId) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Stream not found");
        }

        auto client = findClient(stream, clientId);
        if (!client) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Client not found");
        }

        // Disconnect WiFi client
        if (client->wifiClient.connected()) {
            client->wifiClient.stop();
        }

        client->state = ClientConnectionState::DISCONNECTED;

        // Remove from list
        stream->clients.erase(
            std::remove_if(stream->clients.begin(), stream->clients.end(),
                [clientId](const StreamClientConnection& c) {
                    return c.clientId == clientId;
                }),
            stream->clients.end()
        );

        xSemaphoreGive(_mutex);

        Logger::getInstance().info(TAG, "Client removed: " + String(clientId));
        return Result<void>::ok();
    }

    /**
     * @brief Broadcast frame to all clients in stream
     */
    Result<void> broadcastFrame(uint32_t streamId, const Frame& frame) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Stream not found");
        }

        if (!frame.isValid()) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Invalid frame");
        }

        // Check minimum frame time (rate limiting)
        uint32_t now = millis();
        uint32_t elapsed = now - frame.timestamp;
        if (stream->clients.size() > 0 && elapsed < stream->minFrameTime) {
            xSemaphoreGive(_mutex);
            return Result<void>::ok(); // Skip frame, too fast
        }

        // Broadcast to all connected clients
        size_t successCount = 0;
        for (auto& client : stream->clients) {
            if (client.state != ClientConnectionState::STREAMING &&
                client.state != ClientConnectionState::CONNECTED) {
                continue;
            }

            // Send frame to this client
            auto result = sendFrameToClient(streamId, client.clientId, frame);
            if (result.isOk()) {
                successCount++;
            }
        }

        xSemaphoreGive(_mutex);

        Logger::getInstance().trace(TAG, "Frame broadcast to " + String(successCount) +
                                        " clients in stream " + String(streamId));

        return Result<void>::ok();
    }

    /**
     * @brief Send frame to specific client
     */
    Result<void> sendFrameToClient(uint32_t streamId, uint32_t clientId, const Frame& frame) override {
        // Note: Assumes mutex is already held by caller (broadcastFrame)

        auto stream = findStream(streamId);
        if (!stream) {
            return Result<void>::error("Stream not found");
        }

        auto client = findClient(stream, clientId);
        if (!client) {
            return Result<void>::error("Client not found");
        }

        if (!client->wifiClient.connected()) {
            client->state = ClientConnectionState::DISCONNECTED;
            return Result<void>::error("Client disconnected");
        }

        if (!frame.isValid()) {
            return Result<void>::error("Invalid frame");
        }

        // Send MJPEG multipart headers
        String header = "--frame\r\n";
        header += "Content-Type: image/jpeg\r\n";
        header += "Content-Length: " + String(frame.length) + "\r\n\r\n";

        size_t written = client->wifiClient.write((const uint8_t*)header.c_str(), header.length());
        if (written != header.length()) {
            client->framesDropped++;
            return Result<void>::error("Failed to write header");
        }

        // Send frame data
        written = client->wifiClient.write(frame.buffer, frame.length);
        if (written != frame.length) {
            client->framesDropped++;
            return Result<void>::error("Failed to write frame data");
        }

        // Send boundary
        client->wifiClient.write((const uint8_t*)"\r\n", 2);

        client->framesSent++;
        client->lastFrameTime = millis();
        client->state = ClientConnectionState::STREAMING;

        return Result<void>::ok();
    }

    /**
     * @brief Get stream statistics
     */
    Result<Stream> getStreamStats(uint32_t streamId) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<Stream>::error("Stream not found");
        }

        // Create domain Stream entity with stats
        Stream domainStream;
        domainStream.setMinFrameTime(stream->minFrameTime);

        // Add clients
        for (const auto& client : stream->clients) {
            if (client.state != ClientConnectionState::DISCONNECTED) {
                domainStream.addClient(client.ipAddress, client.port);
            }
        }

        xSemaphoreGive(_mutex);

        return Result<Stream>::ok(domainStream);
    }

    /**
     * @brief Get active client count
     */
    size_t getActiveClientCount(uint32_t streamId) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return 0;
        }

        cleanupDisconnectedClients(stream);
        size_t count = stream->clients.size();

        xSemaphoreGive(_mutex);

        return count;
    }

    /**
     * @brief Check if stream is active
     */
    bool isStreamActive(uint32_t streamId) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        bool active = (stream != nullptr && stream->active);

        xSemaphoreGive(_mutex);

        return active;
    }

    /**
     * @brief Set minimum frame time (rate limiting)
     */
    Result<void> setMinFrameTime(uint32_t streamId, uint16_t minFrameTimeMs) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Stream not found");
        }

        stream->minFrameTime = minFrameTimeMs;

        xSemaphoreGive(_mutex);

        Logger::getInstance().info(TAG, "Stream " + String(streamId) +
                                       " min frame time set to " + String(minFrameTimeMs) + "ms");

        return Result<void>::ok();
    }

    /**
     * @brief Pause stream
     */
    Result<void> pauseStream(uint32_t streamId) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Stream not found");
        }

        stream->active = false;

        xSemaphoreGive(_mutex);

        Logger::getInstance().info(TAG, "Stream paused: " + String(streamId));
        return Result<void>::ok();
    }

    /**
     * @brief Resume stream
     */
    Result<void> resumeStream(uint32_t streamId) override {
        xSemaphoreTake(_mutex, portMAX_DELAY);

        auto stream = findStream(streamId);
        if (!stream) {
            xSemaphoreGive(_mutex);
            return Result<void>::error("Stream not found");
        }

        stream->active = true;

        xSemaphoreGive(_mutex);

        Logger::getInstance().info(TAG, "Stream resumed: " + String(streamId));
        return Result<void>::ok();
    }
};

#endif // STREAM_REPOSITORY_H
