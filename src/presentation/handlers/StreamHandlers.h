/**
 * @file StreamHandlers.h
 * @brief HTTP handlers for streaming endpoints
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef STREAM_HANDLERS_H
#define STREAM_HANDLERS_H

#include <ESPAsyncWebServer.h>
#include "../../application/ApplicationFacade.h"
#include "../http/HttpResponse.h"
#include "../middleware/AuthMiddleware.h"
#include "../../core/Logger.h"

/**
 * @brief Stream HTTP Handlers
 */
class StreamHandlers {
private:
    static constexpr const char* TAG = "StreamHandlers";

public:
    /**
     * @brief GET /api/stream/mjpeg
     *
     * MJPEG streaming endpoint.
     * Requires: viewStream permission
     *
     * Starts streaming JPEG frames as multipart HTTP response.
     */
    static void handleMjpegStream(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        Logger::getInstance().info(TAG, "MJPEG stream requested by " + AuthMiddleware::getUsername(request));

        auto& app = ApplicationFacade::getInstance();

        // Start stream
        auto streamResult = app.stream().startStream();
        if (streamResult.isError()) {
            HttpResponse::sendInternalError(request, "Failed to start stream");
            return;
        }

        uint32_t streamId = streamResult.getValue();

        // Add client to stream
        IPAddress clientIp = request->client()->remoteIP();
        uint16_t clientPort = request->client()->remotePort();

        auto clientResult = app.stream().addClient(streamId, clientIp, clientPort);
        if (clientResult.isError()) {
            app.stream().stopStream(streamId);
            HttpResponse::sendInternalError(request, "Failed to add client to stream");
            return;
        }

        uint32_t clientId = clientResult.getValue();

        // Send chunked MJPEG response
        AsyncWebServerResponse* response = request->beginChunkedResponse(
            "multipart/x-mixed-replace; boundary=frame",
            [streamId, clientId](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
                // This callback is called repeatedly to send chunks
                auto& app = ApplicationFacade::getInstance();

                // Broadcast frame to stream
                auto result = app.stream().broadcastFrame(streamId);

                if (result.isError()) {
                    Logger::getInstance().error(TAG, "Broadcast failed: " + result.getError());
                    return 0; // End stream
                }

                // Note: In practice, we'd need to buffer frames and return them here
                // This is a simplified version
                // Consider using a frame queue or callback mechanism

                delay(33); // ~30 FPS (1000ms / 30 = 33ms)
                return 0; // Continue streaming
            }
        );

        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);

        Logger::getInstance().info(TAG, "MJPEG stream started: stream=" + String(streamId) +
                                       " client=" + String(clientId));
    }

    /**
     * @brief POST /api/stream/start
     *
     * Start a new stream.
     * Requires: viewStream permission
     *
     * Returns: {"streamId": <id>}
     */
    static void handleStartStream(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.stream().startStream();

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        uint32_t streamId = result.getValue();

        String json = "{\"streamId\":" + String(streamId) + "}";
        HttpResponse::sendSuccess(request, json, 201);
    }

    /**
     * @brief POST /api/stream/stop/:id
     *
     * Stop an active stream.
     * Requires: viewStream permission
     */
    static void handleStopStream(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        String streamIdStr = HttpResponse::getParam(request, "id", "0");
        uint32_t streamId = streamIdStr.toInt();

        if (streamId == 0) {
            HttpResponse::sendBadRequest(request, "Invalid stream ID");
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.stream().stopStream(streamId);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Stream stopped");
    }

    /**
     * @brief GET /api/stream/stats/:id
     *
     * Get stream statistics.
     * Requires: viewStream permission
     */
    static void handleGetStats(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        String streamIdStr = HttpResponse::getParam(request, "id", "0");
        uint32_t streamId = streamIdStr.toInt();

        if (streamId == 0) {
            HttpResponse::sendBadRequest(request, "Invalid stream ID");
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.stream().getStats(streamId);

        if (result.isError()) {
            HttpResponse::sendNotFound(request, "Stream not found");
            return;
        }

        StreamStatsDTO stats = result.getValue();
        HttpResponse::sendJson(request, stats.toJson());
    }

    /**
     * @brief POST /api/stream/framerate/:id
     *
     * Set stream frame rate.
     * Requires: modifySettings permission
     *
     * Query params: fps (1-60)
     */
    static void handleSetFrameRate(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "modifySettings")) {
            return;
        }

        String streamIdStr = HttpResponse::getParam(request, "id", "0");
        uint32_t streamId = streamIdStr.toInt();

        String fpsStr = HttpResponse::getParam(request, "fps", "25");
        float fps = fpsStr.toFloat();

        if (fps <= 0 || fps > 60) {
            HttpResponse::sendBadRequest(request, "FPS must be 1-60");
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.stream().setFrameRate(streamId, fps);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Frame rate set to " + String(fps, 1) + " FPS");
    }

    /**
     * @brief POST /api/stream/pause/:id
     *
     * Pause stream.
     * Requires: viewStream permission
     */
    static void handlePauseStream(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        String streamIdStr = HttpResponse::getParam(request, "id", "0");
        uint32_t streamId = streamIdStr.toInt();

        auto& app = ApplicationFacade::getInstance();
        auto result = app.stream().pauseStream(streamId);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Stream paused");
    }

    /**
     * @brief POST /api/stream/resume/:id
     *
     * Resume paused stream.
     * Requires: viewStream permission
     */
    static void handleResumeStream(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        String streamIdStr = HttpResponse::getParam(request, "id", "0");
        uint32_t streamId = streamIdStr.toInt();

        auto& app = ApplicationFacade::getInstance();
        auto result = app.stream().resumeStream(streamId);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Stream resumed");
    }

    /**
     * @brief GET /api/stream/list
     *
     * List all active streams.
     * Requires: viewStream permission
     */
    static void handleListStreams(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        size_t count = app.stream().getActiveStreamCount();

        String json = "{\"activeStreams\":" + String(count) + "}";
        HttpResponse::sendSuccess(request, json);
    }
};

#endif // STREAM_HANDLERS_H
