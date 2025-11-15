/**
 * @file WebSocketHandler.h
 * @brief WebSocket handler for real-time streaming
 *
 * Provides WebSocket-based streaming as an alternative to MJPEG.
 * Supports binary frame transmission with lower overhead.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "../../application/ApplicationFacade.h"
#include "../../core/Logger.h"
#include <map>

/**
 * @brief WebSocket streaming handler
 */
class WebSocketHandler {
private:
    static constexpr const char* TAG = "WebSocket";
    static constexpr uint32_t FRAME_INTERVAL_MS = 33; // ~30 FPS

    struct ClientInfo {
        uint32_t streamId;
        uint32_t clientId;
        uint32_t lastFrameTime;
        bool active;
    };

    static std::map<uint32_t, ClientInfo> _clients; // client ID -> info
    static AsyncWebSocket* _ws;

public:
    /**
     * @brief Initialize WebSocket handler
     */
    static void initialize(AsyncWebSocket* ws) {
        _ws = ws;

        _ws->onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client,
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
            handleWebSocketEvent(server, client, type, arg, data, len);
        });

        Logger::getInstance().info(TAG, "WebSocket handler initialized");
    }

    /**
     * @brief WebSocket event handler
     */
    static void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                     AwsEventType type, void* arg, uint8_t* data, size_t len) {
        switch (type) {
            case WS_EVT_CONNECT:
                handleConnect(client);
                break;

            case WS_EVT_DISCONNECT:
                handleDisconnect(client);
                break;

            case WS_EVT_DATA:
                handleData(client, arg, data, len);
                break;

            case WS_EVT_ERROR:
                Logger::getInstance().error(TAG, "WebSocket error for client " + String(client->id()));
                break;

            default:
                break;
        }
    }

    /**
     * @brief Handle client connection
     */
    static void handleConnect(AsyncWebSocketClient* client) {
        Logger::getInstance().info(TAG, "WebSocket client connected: " + String(client->id()));

        // Start stream for this client
        auto& app = ApplicationFacade::getInstance();
        auto streamResult = app.stream().startStream();

        if (streamResult.isError()) {
            Logger::getInstance().error(TAG, "Failed to start stream: " + streamResult.getError());
            client->close();
            return;
        }

        uint32_t streamId = streamResult.getValue();

        // Add client to stream
        IPAddress clientIp = client->remoteIP();
        uint16_t clientPort = client->remotePort();

        auto clientResult = app.stream().addClient(streamId, clientIp, clientPort);

        if (clientResult.isError()) {
            Logger::getInstance().error(TAG, "Failed to add client: " + clientResult.getError());
            app.stream().stopStream(streamId);
            client->close();
            return;
        }

        uint32_t clientId = clientResult.getValue();

        // Store client info
        ClientInfo info;
        info.streamId = streamId;
        info.clientId = clientId;
        info.lastFrameTime = 0;
        info.active = true;

        _clients[client->id()] = info;

        // Send welcome message
        String welcome = "{\"type\":\"connected\",\"streamId\":" + String(streamId) + "}";
        client->text(welcome);

        Logger::getInstance().info(TAG, "Stream started for WebSocket client: stream=" +
                                       String(streamId) + " client=" + String(clientId));
    }

    /**
     * @brief Handle client disconnection
     */
    static void handleDisconnect(AsyncWebSocketClient* client) {
        Logger::getInstance().info(TAG, "WebSocket client disconnected: " + String(client->id()));

        auto it = _clients.find(client->id());
        if (it != _clients.end()) {
            ClientInfo& info = it->second;

            // Remove from stream
            auto& app = ApplicationFacade::getInstance();
            app.stream().removeClient(info.streamId, info.clientId);
            app.stream().stopStream(info.streamId);

            _clients.erase(it);
        }
    }

    /**
     * @brief Handle incoming data from client
     */
    static void handleData(AsyncWebSocketClient* client, void* arg, uint8_t* data, size_t len) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;

        if (info->final && info->index == 0 && info->len == len) {
            // Complete message received
            if (info->opcode == WS_TEXT) {
                // Handle text commands
                String message = String((char*)data).substring(0, len);
                handleCommand(client, message);
            }
        }
    }

    /**
     * @brief Handle text commands from client
     */
    static void handleCommand(AsyncWebSocketClient* client, const String& command) {
        Logger::getInstance().debug(TAG, "WebSocket command: " + command);

        if (command == "pause") {
            auto it = _clients.find(client->id());
            if (it != _clients.end()) {
                it->second.active = false;
                client->text("{\"type\":\"paused\"}");
            }
        } else if (command == "resume") {
            auto it = _clients.find(client->id());
            if (it != _clients.end()) {
                it->second.active = true;
                client->text("{\"type\":\"resumed\"}");
            }
        } else if (command == "stop") {
            client->close();
        }
    }

    /**
     * @brief Broadcast frames to all active WebSocket clients
     *
     * Call this repeatedly (e.g., from main loop or FreeRTOS task)
     */
    static void broadcastLoop() {
        if (!_ws || _clients.empty()) {
            return;
        }

        uint32_t now = millis();
        auto& app = ApplicationFacade::getInstance();

        // Iterate through all clients
        for (auto& pair : _clients) {
            ClientInfo& info = pair.second;

            if (!info.active) {
                continue;
            }

            // Check frame interval
            if (now - info.lastFrameTime < FRAME_INTERVAL_MS) {
                continue;
            }

            // Capture and broadcast frame
            auto result = app.stream().broadcastFrame(info.streamId);

            if (result.isError()) {
                Logger::getInstance().error(TAG, "Broadcast failed: " + result.getError());
                continue;
            }

            info.lastFrameTime = now;

            // Note: We'd need to actually send the frame via WebSocket
            // This would require accessing the frame buffer
            // For now, this is a placeholder
            // TODO: Implement frame transmission via WebSocket binary messages
        }
    }

    /**
     * @brief Clean up inactive clients
     */
    static void cleanup() {
        if (!_ws) {
            return;
        }

        _ws->cleanupClients();
    }

    /**
     * @brief Get active client count
     */
    static size_t getClientCount() {
        return _clients.size();
    }
};

// Static member initialization
std::map<uint32_t, WebSocketHandler::ClientInfo> WebSocketHandler::_clients;
AsyncWebSocket* WebSocketHandler::_ws = nullptr;

#endif // WEBSOCKET_HANDLER_H
