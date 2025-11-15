/**
 * @file CameraHandlers.h
 * @brief HTTP handlers for camera endpoints
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CAMERA_HANDLERS_H
#define CAMERA_HANDLERS_H

#include <ESPAsyncWebServer.h>
#include "../../application/ApplicationFacade.h"
#include "../http/HttpResponse.h"
#include "../middleware/AuthMiddleware.h"
#include "../../core/Logger.h"

/**
 * @brief Camera HTTP Handlers
 */
class CameraHandlers {
private:
    static constexpr const char* TAG = "CameraHandlers";

public:
    /**
     * @brief GET /api/camera/status
     *
     * Get current camera status.
     * Requires: viewStream permission
     */
    static void handleGetStatus(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return; // Error response already sent
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.camera().getStatus();

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        CameraStatusDTO status = result.getValue();
        HttpResponse::sendJson(request, status.toJson());
    }

    /**
     * @brief GET /api/camera/settings
     *
     * Get current camera settings.
     * Requires: viewStream permission
     */
    static void handleGetSettings(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "viewStream")) {
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        CameraSettings settings = app.camera().getCurrentSettings();
        CameraSettingsDTO dto = CameraSettingsDTO::fromEntity(settings);

        HttpResponse::sendJson(request, dto.toJson());
    }

    /**
     * @brief POST /api/camera/settings
     *
     * Update camera settings.
     * Requires: modifySettings permission
     *
     * Body: JSON with camera settings
     */
    static void handleUpdateSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (!AuthMiddleware::authorize(request, "modifySettings")) {
            return;
        }

        // Parse JSON body
        // Note: Full JSON parsing would use ArduinoJson
        // For now, this is a placeholder
        // TODO: Implement JSON parsing for settings update

        HttpResponse::sendSuccessMessage(request, "Settings updated (JSON parsing not yet implemented)");
    }

    /**
     * @brief GET /api/camera/capture
     *
     * Capture a single JPEG image.
     * Requires: captureImage permission
     */
    static void handleCaptureImage(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "captureImage")) {
            return;
        }

        Logger::getInstance().info(TAG, "Capturing image for " + AuthMiddleware::getUsername(request));

        auto& app = ApplicationFacade::getInstance();
        auto result = app.camera().captureImage();

        if (result.isError()) {
            HttpResponse::sendInternalError(request, "Capture failed: " + result.getError());
            return;
        }

        Frame frame = result.getValue();
        HttpResponse::sendJpeg(request, frame.buffer, frame.length);

        // Release frame
        // Note: In current design, frame release is tricky
        // Consider adding frame pool management
    }

    /**
     * @brief POST /api/camera/lamp
     *
     * Set lamp (flash LED) intensity.
     * Requires: modifySettings permission
     *
     * Query params: intensity (0-100)
     */
    static void handleSetLamp(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "modifySettings")) {
            return;
        }

        String intensityStr = HttpResponse::getParam(request, "intensity", "0");
        uint8_t intensity = intensityStr.toInt();

        if (intensity > 100) {
            HttpResponse::sendBadRequest(request, "Intensity must be 0-100");
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.camera().setLampIntensity(intensity);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Lamp intensity set to " + String(intensity) + "%");
    }

    /**
     * @brief POST /api/camera/optimize/:type
     *
     * Apply optimization preset.
     * Requires: modifySettings permission
     *
     * Types: lowlight, speed, quality
     */
    static void handleOptimize(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "modifySettings")) {
            return;
        }

        String type = HttpResponse::getParam(request, "type", "");

        auto& app = ApplicationFacade::getInstance();
        Result<void> result = Result<void>::error("Unknown optimization type");

        if (type == "lowlight") {
            result = app.camera().optimizeForLowLight();
        } else if (type == "speed") {
            result = app.camera().optimizeForSpeed();
        } else if (type == "quality") {
            result = app.camera().optimizeForQuality();
        }

        if (result.isError()) {
            HttpResponse::sendBadRequest(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Camera optimized for " + type);
    }

    /**
     * @brief POST /api/camera/reset
     *
     * Reset camera hardware.
     * Requires: modifySettings permission
     */
    static void handleReset(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "modifySettings")) {
            return;
        }

        Logger::getInstance().warn(TAG, "Camera reset requested by " + AuthMiddleware::getUsername(request));

        auto& app = ApplicationFacade::getInstance();
        auto result = app.camera().reset();

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Camera reset successfully");
    }

    /**
     * @brief POST /api/camera/adjust
     *
     * Adjust individual camera parameters.
     * Requires: modifySettings permission
     *
     * Query params: brightness, contrast, saturation (-2 to +2)
     */
    static void handleAdjust(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "modifySettings")) {
            return;
        }

        auto& app = ApplicationFacade::getInstance();

        if (request->hasParam("brightness")) {
            int8_t value = HttpResponse::getParam(request, "brightness", "0").toInt();
            app.camera().setBrightness(value);
        }

        if (request->hasParam("contrast")) {
            int8_t value = HttpResponse::getParam(request, "contrast", "0").toInt();
            app.camera().setContrast(value);
        }

        if (request->hasParam("saturation")) {
            int8_t value = HttpResponse::getParam(request, "saturation", "0").toInt();
            app.camera().setSaturation(value);
        }

        if (request->hasParam("awb")) {
            bool enable = HttpResponse::getParam(request, "awb", "0") == "1";
            app.camera().setAutoWhiteBalance(enable);
        }

        if (request->hasParam("aec")) {
            bool enable = HttpResponse::getParam(request, "aec", "0") == "1";
            app.camera().setAutoExposureControl(enable);
        }

        HttpResponse::sendSuccessMessage(request, "Camera adjusted");
    }
};

#endif // CAMERA_HANDLERS_H
