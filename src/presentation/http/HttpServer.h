/**
 * @file HttpServer.h
 * @brief HTTP server configuration and routing
 *
 * Sets up AsyncWebServer with all routes and middleware.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "../../application/ApplicationFacade.h"
#include "../handlers/CameraHandlers.h"
#include "../handlers/StreamHandlers.h"
#include "../handlers/AuthHandlers.h"
#include "../middleware/AuthMiddleware.h"
#include "../websocket/WebSocketHandler.h"
#include "HttpResponse.h"
#include "../../core/Logger.h"
#include "../../core/Config.h"

/**
 * @brief HTTP Server Manager
 *
 * Configures and manages the AsyncWebServer.
 */
class HttpServer {
private:
    static constexpr const char* TAG = "HttpServer";
    static constexpr uint16_t DEFAULT_PORT = 80;

    AsyncWebServer* _server;
    AsyncWebSocket* _ws;
    bool _initialized;

public:
    /**
     * @brief Constructor
     */
    HttpServer(uint16_t port = DEFAULT_PORT)
        : _initialized(false) {
        _server = new AsyncWebServer(port);
        _ws = new AsyncWebSocket("/ws");
    }

    /**
     * @brief Initialize and configure server
     */
    bool initialize() {
        if (_initialized) {
            Logger::getInstance().warn(TAG, "HTTP server already initialized");
            return true;
        }

        Logger::getInstance().info(TAG, "Initializing HTTP server...");

        // Initialize WebSocket handler
        WebSocketHandler::initialize(_ws);
        _server->addHandler(_ws);

        // Configure routes
        configureRoutes();

        // Configure static file serving
        configureStaticFiles();

        // Configure CORS
        configureCors();

        // Configure default handlers
        configureDefaultHandlers();

        _initialized = true;

        Logger::getInstance().info(TAG, "HTTP server initialized");
        return true;
    }

    /**
     * @brief Start the server
     */
    void start() {
        if (!_initialized) {
            Logger::getInstance().error(TAG, "Cannot start: server not initialized");
            return;
        }

        _server->begin();
        Logger::getInstance().info(TAG, "HTTP server started");
    }

    /**
     * @brief Stop the server
     */
    void stop() {
        _server->end();
        Logger::getInstance().info(TAG, "HTTP server stopped");
    }

    /**
     * @brief Periodic update (call from loop)
     */
    void update() {
        if (!_initialized) {
            return;
        }

        // Broadcast WebSocket frames
        WebSocketHandler::broadcastLoop();

        // Cleanup inactive WebSocket clients
        WebSocketHandler::cleanup();
    }

private:
    /**
     * @brief Configure API routes
     */
    void configureRoutes() {
        Logger::getInstance().info(TAG, "Configuring routes...");

        // ====================================================================
        // Health & Info Routes (public)
        // ====================================================================

        _server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            HttpResponse::sendHtml(request, "<h1>ESP32-CAM Webserver v5.0</h1><p>Ready</p>");
        });

        _server->on("/api/health", HTTP_GET, [](AsyncWebServerRequest* request) {
            auto& app = ApplicationFacade::getInstance();
            bool healthy = app.healthCheck();

            if (healthy) {
                HttpResponse::sendSuccessMessage(request, "OK");
            } else {
                HttpResponse::sendInternalError(request, "Health check failed");
            }
        });

        _server->on("/api/info", HTTP_GET, [](AsyncWebServerRequest* request) {
            auto& app = ApplicationFacade::getInstance();
            String json = app.getInfoJson();
            HttpResponse::sendJson(request, json);
        });

        // ====================================================================
        // Authentication Routes
        // ====================================================================

        _server->on("/api/auth/login", HTTP_POST, [](AsyncWebServerRequest* request) {},
                   nullptr, AuthHandlers::handleLogin);

        _server->on("/api/auth/logout", HTTP_POST, AuthHandlers::handleLogout);

        _server->on("/api/auth/refresh", HTTP_POST, AuthHandlers::handleRefresh);

        _server->on("/api/auth/me", HTTP_GET, AuthHandlers::handleGetCurrentUser);

        _server->on("/api/auth/permissions", HTTP_GET, AuthHandlers::handleGetPermissions);

        _server->on("/api/auth/register", HTTP_POST, [](AsyncWebServerRequest* request) {},
                   nullptr, AuthHandlers::handleRegister);

        _server->on("/api/auth/users", HTTP_GET, AuthHandlers::handleListUsers);

        // ====================================================================
        // Camera Routes (protected)
        // ====================================================================

        _server->on("/api/camera/status", HTTP_GET, CameraHandlers::handleGetStatus);

        _server->on("/api/camera/settings", HTTP_GET, CameraHandlers::handleGetSettings);

        _server->on("/api/camera/settings", HTTP_POST, [](AsyncWebServerRequest* request) {},
                   nullptr, CameraHandlers::handleUpdateSettings);

        _server->on("/api/camera/capture", HTTP_GET, CameraHandlers::handleCaptureImage);

        _server->on("/api/camera/lamp", HTTP_POST, CameraHandlers::handleSetLamp);

        _server->on("/api/camera/optimize", HTTP_POST, CameraHandlers::handleOptimize);

        _server->on("/api/camera/reset", HTTP_POST, CameraHandlers::handleReset);

        _server->on("/api/camera/adjust", HTTP_POST, CameraHandlers::handleAdjust);

        // ====================================================================
        // Streaming Routes (protected)
        // ====================================================================

        _server->on("/api/stream/mjpeg", HTTP_GET, StreamHandlers::handleMjpegStream);

        _server->on("/api/stream/start", HTTP_POST, StreamHandlers::handleStartStream);

        _server->on("/api/stream/stop", HTTP_POST, StreamHandlers::handleStopStream);

        _server->on("/api/stream/stats", HTTP_GET, StreamHandlers::handleGetStats);

        _server->on("/api/stream/framerate", HTTP_POST, StreamHandlers::handleSetFrameRate);

        _server->on("/api/stream/pause", HTTP_POST, StreamHandlers::handlePauseStream);

        _server->on("/api/stream/resume", HTTP_POST, StreamHandlers::handleResumeStream);

        _server->on("/api/stream/list", HTTP_GET, StreamHandlers::handleListStreams);

        Logger::getInstance().info(TAG, "Routes configured");
    }

    /**
     * @brief Configure static file serving from SPIFFS
     */
    void configureStaticFiles() {
        Logger::getInstance().info(TAG, "Configuring static files...");

        // Serve static files from SPIFFS /www directory
        _server->serveStatic("/", SPIFFS, "/www/")
            .setDefaultFile("index.html")
            .setCacheControl("max-age=600");

        // Specific file routes
        _server->serveStatic("/favicon.ico", SPIFFS, "/www/favicon.ico");

        Logger::getInstance().info(TAG, "Static files configured");
    }

    /**
     * @brief Configure CORS headers
     */
    void configureCors() {
        Logger::getInstance().info(TAG, "Configuring CORS...");

        // Handle CORS preflight requests
        _server->on("/*", HTTP_OPTIONS, AuthMiddleware::handleCorsPreflightRequest);

        // CORS headers are added by HttpResponse helpers

        Logger::getInstance().info(TAG, "CORS configured");
    }

    /**
     * @brief Configure default handlers (404, etc.)
     */
    void configureDefaultHandlers() {
        Logger::getInstance().info(TAG, "Configuring default handlers...");

        // 404 handler
        _server->onNotFound([](AsyncWebServerRequest* request) {
            Logger::getInstance().warn(TAG, "404: " + request->url());
            HttpResponse::sendNotFound(request, "Endpoint not found: " + request->url());
        });

        Logger::getInstance().info(TAG, "Default handlers configured");
    }
};

#endif // HTTP_SERVER_H
