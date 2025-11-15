/**
 * @file HttpResponse.h
 * @brief HTTP response helpers
 *
 * Provides utilities for consistent HTTP responses.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "../../core/Logger.h"

/**
 * @brief HTTP Response Helper
 *
 * Provides consistent response formatting for the API.
 */
class HttpResponse {
private:
    static constexpr const char* TAG = "HttpResponse";

public:
    /**
     * @brief Send JSON success response
     */
    static void sendJson(AsyncWebServerRequest* request, const String& json, int statusCode = 200) {
        AsyncWebServerResponse* response = request->beginResponse(statusCode, "application/json", json);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    }

    /**
     * @brief Send success with data
     */
    static void sendSuccess(AsyncWebServerRequest* request, const String& data, int statusCode = 200) {
        String json = "{\"success\":true,\"data\":" + data + "}";
        sendJson(request, json, statusCode);
    }

    /**
     * @brief Send success with message
     */
    static void sendSuccessMessage(AsyncWebServerRequest* request, const String& message, int statusCode = 200) {
        String json = "{\"success\":true,\"message\":\"" + message + "\"}";
        sendJson(request, json, statusCode);
    }

    /**
     * @brief Send error response
     */
    static void sendError(AsyncWebServerRequest* request, const String& error, int statusCode = 500) {
        String json = "{\"success\":false,\"error\":\"" + error + "\"}";
        Logger::getInstance().warn(TAG, "Error response: " + error);
        sendJson(request, json, statusCode);
    }

    /**
     * @brief Send 400 Bad Request
     */
    static void sendBadRequest(AsyncWebServerRequest* request, const String& error = "Bad request") {
        sendError(request, error, 400);
    }

    /**
     * @brief Send 401 Unauthorized
     */
    static void sendUnauthorized(AsyncWebServerRequest* request, const String& error = "Unauthorized") {
        sendError(request, error, 401);
    }

    /**
     * @brief Send 403 Forbidden
     */
    static void sendForbidden(AsyncWebServerRequest* request, const String& error = "Forbidden") {
        sendError(request, error, 403);
    }

    /**
     * @brief Send 404 Not Found
     */
    static void sendNotFound(AsyncWebServerRequest* request, const String& error = "Not found") {
        sendError(request, error, 404);
    }

    /**
     * @brief Send 500 Internal Server Error
     */
    static void sendInternalError(AsyncWebServerRequest* request, const String& error = "Internal server error") {
        sendError(request, error, 500);
    }

    /**
     * @brief Send JPEG image
     */
    static void sendJpeg(AsyncWebServerRequest* request, const uint8_t* buffer, size_t length) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "image/jpeg", buffer, length);
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);
    }

    /**
     * @brief Send MJPEG stream headers
     */
    static void sendMjpegHeaders(AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginChunkedResponse(
            "multipart/x-mixed-replace; boundary=frame",
            [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
                // This will be handled by streaming callbacks
                return 0;
            }
        );
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);
    }

    /**
     * @brief Send plain text
     */
    static void sendText(AsyncWebServerRequest* request, const String& text, int statusCode = 200) {
        AsyncWebServerResponse* response = request->beginResponse(statusCode, "text/plain", text);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    }

    /**
     * @brief Send HTML
     */
    static void sendHtml(AsyncWebServerRequest* request, const String& html, int statusCode = 200) {
        AsyncWebServerResponse* response = request->beginResponse(statusCode, "text/html", html);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    }

    /**
     * @brief Get request parameter
     */
    static String getParam(AsyncWebServerRequest* request, const String& name, const String& defaultValue = "") {
        if (request->hasParam(name)) {
            return request->getParam(name)->value();
        }
        return defaultValue;
    }

    /**
     * @brief Get request body
     */
    static String getBody(AsyncWebServerRequest* request) {
        if (request->hasParam("plain", true)) {
            return request->getParam("plain", true)->value();
        }
        return "";
    }

    /**
     * @brief Get authorization header (Bearer token)
     */
    static String getAuthToken(AsyncWebServerRequest* request) {
        if (request->hasHeader("Authorization")) {
            String authHeader = request->header("Authorization");
            if (authHeader.startsWith("Bearer ")) {
                return authHeader.substring(7); // Remove "Bearer " prefix
            }
        }
        return "";
    }

    /**
     * @brief Check if request has valid JSON content type
     */
    static bool isJson(AsyncWebServerRequest* request) {
        if (request->hasHeader("Content-Type")) {
            String contentType = request->header("Content-Type");
            return contentType.indexOf("application/json") >= 0;
        }
        return false;
    }
};

#endif // HTTP_RESPONSE_H
