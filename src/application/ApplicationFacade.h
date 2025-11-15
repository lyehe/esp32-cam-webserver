/**
 * @file ApplicationFacade.h
 * @brief Unified facade for all application services
 *
 * Provides a single entry point for the presentation layer to access
 * all application functionality. Implements the Facade pattern.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef APPLICATION_FACADE_H
#define APPLICATION_FACADE_H

#include "services/CameraService.h"
#include "services/StreamService.h"
#include "services/AuthService.h"
#include "../core/DependencyContainer.h"
#include "../core/Logger.h"
#include <memory>

/**
 * @brief Application Facade
 *
 * Unified interface to all application services.
 * Simplifies access for the presentation layer (HTTP handlers).
 *
 * @example
 * // Initialize
 * ApplicationFacade::getInstance().initialize();
 *
 * // Use services
 * auto& app = ApplicationFacade::getInstance();
 * auto statusResult = app.camera().getStatus();
 * auto loginResult = app.auth().login("admin", "password");
 * auto streamId = app.stream().startStream();
 */
class ApplicationFacade {
private:
    static constexpr const char* TAG = "AppFacade";

    bool _initialized;

    // Services (lazy-initialized)
    std::unique_ptr<CameraService> _cameraService;
    std::unique_ptr<StreamService> _streamService;
    std::unique_ptr<AuthService> _authService;

    /**
     * @brief Private constructor (Singleton)
     */
    ApplicationFacade() : _initialized(false) {}

public:
    /**
     * @brief Get singleton instance
     */
    static ApplicationFacade& getInstance() {
        static ApplicationFacade instance;
        return instance;
    }

    // Prevent copying
    ApplicationFacade(const ApplicationFacade&) = delete;
    ApplicationFacade& operator=(const ApplicationFacade&) = delete;

    /**
     * @brief Initialize application layer
     *
     * Must be called after DependencyContainer is initialized.
     */
    bool initialize() {
        if (_initialized) {
            Logger::getInstance().warn(TAG, "Application facade already initialized");
            return true;
        }

        Logger::getInstance().info(TAG, "Initializing application facade...");

        // Get dependency container
        DependencyContainer& container = DependencyContainer::getInstance();

        if (!container.isInitialized()) {
            Logger::getInstance().error(TAG, "Dependency container not initialized");
            return false;
        }

        // Create services
        Logger::getInstance().info(TAG, "Creating application services...");

        _cameraService = std::make_unique<CameraService>(
            container.getCameraRepository(),
            container.getStorageRepository()
        );

        _streamService = std::make_unique<StreamService>(
            container.getStreamRepository(),
            container.getCameraRepository()
        );

        _authService = std::make_unique<AuthService>(
            container.getAuthRepository()
        );

        _initialized = true;

        Logger::getInstance().info(TAG, "Application facade initialized successfully");
        return true;
    }

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const {
        return _initialized;
    }

    /**
     * @brief Get camera service
     */
    CameraService& camera() {
        if (!_cameraService) {
            Logger::getInstance().fatal(TAG, "Camera service not initialized");
        }
        return *_cameraService;
    }

    /**
     * @brief Get stream service
     */
    StreamService& stream() {
        if (!_streamService) {
            Logger::getInstance().fatal(TAG, "Stream service not initialized");
        }
        return *_streamService;
    }

    /**
     * @brief Get auth service
     */
    AuthService& auth() {
        if (!_authService) {
            Logger::getInstance().fatal(TAG, "Auth service not initialized");
        }
        return *_authService;
    }

    /**
     * @brief Cleanup and shutdown
     */
    void shutdown() {
        Logger::getInstance().info(TAG, "Shutting down application facade...");

        // Stop all active streams
        if (_streamService) {
            _streamService->stopAllStreams();
        }

        // Release services
        _authService.reset();
        _streamService.reset();
        _cameraService.reset();

        _initialized = false;

        Logger::getInstance().info(TAG, "Application facade shut down");
    }

    /**
     * @brief Health check
     *
     * Verifies all services are operational.
     */
    bool healthCheck() {
        if (!_initialized) {
            return false;
        }

        bool healthy = true;

        // Check camera service
        if (!_cameraService || !_cameraService->isInitialized()) {
            Logger::getInstance().error(TAG, "Camera service unhealthy");
            healthy = false;
        }

        // Check stream service
        if (!_streamService) {
            Logger::getInstance().error(TAG, "Stream service unhealthy");
            healthy = false;
        }

        // Check auth service
        if (!_authService) {
            Logger::getInstance().error(TAG, "Auth service unhealthy");
            healthy = false;
        }

        if (healthy) {
            Logger::getInstance().info(TAG, "Health check: OK");
        } else {
            Logger::getInstance().error(TAG, "Health check: FAILED");
        }

        return healthy;
    }

    /**
     * @brief Get application statistics
     */
    String getStats() {
        String stats = "Application Statistics\n";
        stats += "=====================\n\n";

        if (_cameraService) {
            auto statusResult = _cameraService->getStatus();
            if (statusResult.isOk()) {
                CameraStatusDTO status = statusResult.getValue();
                stats += "Camera: " + status.state + "\n";
                stats += "Sensor: " + status.model + "\n";
                stats += "Frames Captured: " + String(status.framesCaptured) + "\n";
                stats += "Current FPS: " + String(status.currentFPS, 2) + "\n\n";
            }
        }

        if (_streamService) {
            stats += "Active Streams: " + String(_streamService->getActiveStreamCount()) + "\n\n";
        }

        if (_authService) {
            stats += "Active Sessions: " + String(_authService->getActiveSessionCount()) + "\n";
        }

        return stats;
    }

    /**
     * @brief Get application info as JSON
     */
    String getInfoJson() {
        String json = "{";
        json += "\"initialized\":" + String(_initialized ? "true" : "false") + ",";

        if (_cameraService) {
            json += "\"cameraInitialized\":" + String(_cameraService->isInitialized() ? "true" : "false") + ",";
            json += "\"sensorModel\":\"" + _cameraService->getSensorModel() + "\",";
        }

        if (_streamService) {
            json += "\"activeStreams\":" + String(_streamService->getActiveStreamCount()) + ",";
        }

        if (_authService) {
            json += "\"activeSessions\":" + String(_authService->getActiveSessionCount());
        }

        json += "}";
        return json;
    }
};

#endif // APPLICATION_FACADE_H
