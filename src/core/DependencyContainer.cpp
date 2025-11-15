/**
 * @file DependencyContainer.cpp
 * @brief Implementation of Dependency Injection container
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#include "DependencyContainer.h"
#include "Config.h"
#include "../domain/repositories/ICameraRepository.h"
#include "../domain/repositories/IStreamRepository.h"
#include "../domain/repositories/IStorageRepository.h"
#include "../domain/repositories/IAuthRepository.h"
#include "../infrastructure/repositories/CameraRepository.h"
#include "../infrastructure/repositories/StreamRepository.h"
#include "../infrastructure/storage/SPIFFSStorageRepository.h"
#include "../infrastructure/security/JWTAuthRepository.h"
#include "../infrastructure/drivers/ESP32CameraDriver.h"

/**
 * @brief Initialize all dependencies
 */
bool DependencyContainer::initialize() {
    if (_initialized) {
        Logger::getInstance().warn("DI", "Dependency container already initialized");
        return true;
    }

    Logger::getInstance().info("DI", "Initializing dependency container...");

    // ========================================================================
    // Step 1: Initialize Storage Repository (required by others)
    // ========================================================================

    Logger::getInstance().info("DI", "Creating storage repository...");
    auto storageRepo = std::make_shared<SPIFFSStorageRepository>();

    auto storageResult = storageRepo->initialize();
    if (storageResult.isError()) {
        Logger::getInstance().error("DI", "Storage init failed: " + storageResult.getError());
        return false;
    }

    _storageRepository = storageRepo;
    Logger::getInstance().info("DI", "Storage repository initialized");

    // ========================================================================
    // Step 2: Initialize Camera Repository
    // ========================================================================

    Logger::getInstance().info("DI", "Creating camera repository...");

    // Use AI-Thinker pin configuration (default)
    auto cameraRepo = std::make_shared<CameraRepository>(CameraPins::AIThinker());

    // Load camera settings from storage or use defaults
    auto settingsResult = _storageRepository->loadCameraSettings();
    CameraSettings settings;
    if (settingsResult.isOk()) {
        settings = settingsResult.getValue();
        Logger::getInstance().info("DI", "Loaded camera settings from storage");
    } else {
        Logger::getInstance().info("DI", "Using default camera settings");
    }

    // Initialize camera with settings
    auto cameraResult = cameraRepo->initialize(settings);
    if (cameraResult.isError()) {
        Logger::getInstance().error("DI", "Camera init failed: " + cameraResult.getError());
        return false;
    }

    _cameraRepository = cameraRepo;
    Logger::getInstance().info("DI", "Camera repository initialized");

    // ========================================================================
    // Step 3: Initialize Stream Repository
    // ========================================================================

    Logger::getInstance().info("DI", "Creating stream repository...");
    auto streamRepo = std::make_shared<StreamRepository>();

    _streamRepository = streamRepo;
    Logger::getInstance().info("DI", "Stream repository initialized");

    // ========================================================================
    // Step 4: Initialize Auth Repository
    // ========================================================================

    Logger::getInstance().info("DI", "Creating auth repository...");

    // Create auth repository with storage dependency
    auto authRepo = std::make_shared<JWTAuthRepository>(_storageRepository.get());

    // Get JWT secret from config
    Config& config = Config::getInstance();
    String jwtSecret = config.getJWTSecret();

    // If no JWT secret in config, generate a random one (not secure for production!)
    if (jwtSecret.length() < 32) {
        Logger::getInstance().warn("DI", "No JWT secret in config, generating random (INSECURE!)");
        jwtSecret = "";
        for (int i = 0; i < 64; i++) {
            jwtSecret += char(random(33, 126)); // Random printable ASCII
        }
    }

    auto authResult = authRepo->initialize(jwtSecret);
    if (authResult.isError()) {
        Logger::getInstance().error("DI", "Auth init failed: " + authResult.getError());
        return false;
    }

    _authRepository = authRepo;
    Logger::getInstance().info("DI", "Auth repository initialized");

    // ========================================================================
    // Step 5: Create default admin user if none exists
    // ========================================================================

    if (!_authRepository->userExists("admin")) {
        Logger::getInstance().info("DI", "Creating default admin user...");

        auto credResult = Credentials::create("admin", "admin123");
        if (credResult.isOk()) {
            auto userResult = _authRepository->registerUser(credResult.getValue(), UserRole::ADMIN);
            if (userResult.isOk()) {
                Logger::getInstance().warn("DI", "Default admin user created (username: admin, password: admin123)");
                Logger::getInstance().warn("DI", "*** CHANGE PASSWORD IMMEDIATELY FOR SECURITY ***");
            } else {
                Logger::getInstance().error("DI", "Failed to create admin user: " + userResult.getError());
            }
        }
    }

    // ========================================================================
    // Initialization complete
    // ========================================================================

    _initialized = true;
    Logger::getInstance().info("DI", "Dependency container initialized successfully");

    return true;
}
