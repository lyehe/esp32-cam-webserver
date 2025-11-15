/**
 * @file DependencyContainer.h
 * @brief Dependency Injection container for managing application dependencies
 *
 * Provides a centralized registry for creating and managing dependencies
 * following the Dependency Inversion Principle.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef DEPENDENCY_CONTAINER_H
#define DEPENDENCY_CONTAINER_H

#include <memory>
#include "Logger.h"

// Forward declarations - interfaces will be defined in domain layer
class ICameraRepository;
class IStreamRepository;
class IStorageRepository;
class IAuthRepository;
class INetworkManager;

// Forward declarations - implementations from infrastructure layer
class ESP32CameraDriver;
class CameraRepository;
class StreamRepository;
class SPIFFSStorageRepository;
class JWTAuthRepository;
class WiFiManager;

/**
 * @brief Singleton dependency injection container
 *
 * Manages the lifecycle and wiring of all application dependencies.
 * Uses shared_ptr for automatic memory management.
 *
 * @example
 * // Initialize container
 * DependencyContainer::getInstance().initialize();
 *
 * // Get dependencies
 * auto& cameraRepo = DependencyContainer::getInstance().getCameraRepository();
 * auto result = cameraRepo.captureFrame();
 */
class DependencyContainer {
private:
    // Repository instances (domain layer abstractions)
    std::shared_ptr<ICameraRepository> _cameraRepository;
    std::shared_ptr<IStreamRepository> _streamRepository;
    std::shared_ptr<IStorageRepository> _storageRepository;
    std::shared_ptr<IAuthRepository> _authRepository;

    // Infrastructure services
    std::shared_ptr<INetworkManager> _networkManager;

    // Hardware drivers
    std::shared_ptr<ESP32CameraDriver> _cameraDriver;

    bool _initialized;

    /**
     * @brief Private constructor for singleton
     */
    DependencyContainer() : _initialized(false) {}

public:
    /**
     * @brief Get singleton instance
     */
    static DependencyContainer& getInstance() {
        static DependencyContainer instance;
        return instance;
    }

    // Prevent copying and assignment
    DependencyContainer(const DependencyContainer&) = delete;
    DependencyContainer& operator=(const DependencyContainer&) = delete;

    /**
     * @brief Initialize all dependencies
     *
     * Creates instances and wires up dependencies.
     * Must be called before accessing any dependencies.
     *
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Check if container is initialized
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief Get camera repository
     *
     * @return ICameraRepository& Reference to camera repository
     */
    ICameraRepository& getCameraRepository() {
        if (!_cameraRepository) {
            LOG_FATAL("DI", "Camera repository not initialized");
        }
        return *_cameraRepository;
    }

    /**
     * @brief Get stream repository
     *
     * @return IStreamRepository& Reference to stream repository
     */
    IStreamRepository& getStreamRepository() {
        if (!_streamRepository) {
            LOG_FATAL("DI", "Stream repository not initialized");
        }
        return *_streamRepository;
    }

    /**
     * @brief Get storage repository
     *
     * @return IStorageRepository& Reference to storage repository
     */
    IStorageRepository& getStorageRepository() {
        if (!_storageRepository) {
            LOG_FATAL("DI", "Storage repository not initialized");
        }
        return *_storageRepository;
    }

    /**
     * @brief Get authentication repository
     *
     * @return IAuthRepository& Reference to auth repository
     */
    IAuthRepository& getAuthRepository() {
        if (!_authRepository) {
            LOG_FATAL("DI", "Auth repository not initialized");
        }
        return *_authRepository;
    }

    /**
     * @brief Get network manager
     *
     * @return INetworkManager& Reference to network manager
     */
    INetworkManager& getNetworkManager() {
        if (!_networkManager) {
            LOG_FATAL("DI", "Network manager not initialized");
        }
        return *_networkManager;
    }

    /**
     * @brief Cleanup and release all dependencies
     *
     * Should be called before shutdown
     */
    void cleanup() {
        LOG_INFO("DI", "Cleaning up dependency container");

        // Release in reverse order of initialization
        _authRepository.reset();
        _storageRepository.reset();
        _streamRepository.reset();
        _cameraRepository.reset();
        _networkManager.reset();
        _cameraDriver.reset();

        _initialized = false;
    }

    /**
     * @brief Reset dependencies (for testing)
     *
     * Allows injecting mock implementations for testing
     */
    void reset() {
        cleanup();
    }

    // ========================================================================
    // Test/Mock injection methods (only for testing)
    // ========================================================================

    #ifdef UNIT_TEST
    void setCameraRepository(std::shared_ptr<ICameraRepository> repo) {
        _cameraRepository = repo;
    }

    void setStreamRepository(std::shared_ptr<IStreamRepository> repo) {
        _streamRepository = repo;
    }

    void setStorageRepository(std::shared_ptr<IStorageRepository> repo) {
        _storageRepository = repo;
    }

    void setAuthRepository(std::shared_ptr<IAuthRepository> repo) {
        _authRepository = repo;
    }

    void setNetworkManager(std::shared_ptr<INetworkManager> manager) {
        _networkManager = manager;
    }
    #endif
};

#endif // DEPENDENCY_CONTAINER_H
