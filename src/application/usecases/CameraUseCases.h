/**
 * @file CameraUseCases.h
 * @brief Camera-related use cases
 *
 * Use cases orchestrate business logic by coordinating between
 * repositories and domain entities. They represent application-specific
 * operations.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CAMERA_USE_CASES_H
#define CAMERA_USE_CASES_H

#include "../../domain/repositories/ICameraRepository.h"
#include "../../domain/repositories/IStorageRepository.h"
#include "../../domain/entities/Camera.h"
#include "../../core/Logger.h"
#include "../../core/Result.h"
#include "../dtos/CameraDTO.h"

/**
 * @brief Capture single image use case
 *
 * Captures a single frame from the camera.
 */
class CaptureImageUseCase {
private:
    static constexpr const char* TAG = "CaptureImageUC";
    ICameraRepository& _cameraRepo;

public:
    CaptureImageUseCase(ICameraRepository& cameraRepo)
        : _cameraRepo(cameraRepo) {}

    /**
     * @brief Execute: Capture a frame
     */
    Result<Frame> execute() {
        Logger::getInstance().debug(TAG, "Capturing image...");

        if (!_cameraRepo.isInitialized()) {
            return Result<Frame>::error("Camera not initialized");
        }

        auto result = _cameraRepo.captureFrame();
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Capture failed: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "Image captured successfully");
        return result;
    }
};

/**
 * @brief Get camera status use case
 */
class GetCameraStatusUseCase {
private:
    static constexpr const char* TAG = "GetCameraStatusUC";
    ICameraRepository& _cameraRepo;

public:
    GetCameraStatusUseCase(ICameraRepository& cameraRepo)
        : _cameraRepo(cameraRepo) {}

    /**
     * @brief Execute: Get current camera status
     */
    Result<CameraStatusDTO> execute() {
        Logger::getInstance().trace(TAG, "Getting camera status...");

        if (!_cameraRepo.isInitialized()) {
            return Result<CameraStatusDTO>::error("Camera not initialized");
        }

        // Create a Camera entity with current state
        Camera camera(_cameraRepo.getSensorModel());
        // Note: We'd need to expose more state from repository
        // For now, create DTO with available info

        CameraStatusDTO dto;
        dto.model = _cameraRepo.getSensorModel();
        dto.state = _cameraRepo.isInitialized() ? "Ready" : "Uninitialized";
        // Settings would need to be retrieved
        auto settings = _cameraRepo.getCurrentSettings();
        dto.resolution = settings.getResolution().toString();
        dto.pixelFormat = settings.getPixelFormat().toString();
        dto.quality = settings.getQuality();

        return Result<CameraStatusDTO>::ok(dto);
    }
};

/**
 * @brief Update camera settings use case
 */
class UpdateCameraSettingsUseCase {
private:
    static constexpr const char* TAG = "UpdateSettingsUC";
    ICameraRepository& _cameraRepo;
    IStorageRepository& _storageRepo;

public:
    UpdateCameraSettingsUseCase(ICameraRepository& cameraRepo,
                               IStorageRepository& storageRepo)
        : _cameraRepo(cameraRepo), _storageRepo(storageRepo) {}

    /**
     * @brief Execute: Update camera settings
     */
    Result<void> execute(const CameraSettings& newSettings) {
        Logger::getInstance().info(TAG, "Updating camera settings...");

        if (!_cameraRepo.isInitialized()) {
            return Result<void>::error("Camera not initialized");
        }

        // Apply settings to camera
        auto updateResult = _cameraRepo.updateSettings(newSettings);
        if (updateResult.isError()) {
            Logger::getInstance().error(TAG, "Failed to update settings: " + updateResult.getError());
            return updateResult;
        }

        // Persist settings to storage
        auto saveResult = _storageRepo.saveCameraSettings(newSettings);
        if (saveResult.isError()) {
            Logger::getInstance().warn(TAG, "Failed to save settings: " + saveResult.getError());
            // Not critical, settings are applied
        } else {
            Logger::getInstance().info(TAG, "Settings saved to storage");
        }

        Logger::getInstance().info(TAG, "Camera settings updated successfully");
        return Result<void>::ok();
    }
};

/**
 * @brief Set camera lamp intensity use case
 */
class SetLampIntensityUseCase {
private:
    static constexpr const char* TAG = "SetLampUC";
    ICameraRepository& _cameraRepo;

public:
    SetLampIntensityUseCase(ICameraRepository& cameraRepo)
        : _cameraRepo(cameraRepo) {}

    /**
     * @brief Execute: Set lamp intensity (0-100%)
     */
    Result<void> execute(uint8_t intensity) {
        if (intensity > 100) {
            return Result<void>::error("Intensity must be 0-100");
        }

        Logger::getInstance().debug(TAG, "Setting lamp intensity to " + String(intensity) + "%");

        auto result = _cameraRepo.setLampIntensity(intensity);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to set lamp: " + result.getError());
            return result;
        }

        return Result<void>::ok();
    }
};

/**
 * @brief Optimize camera for condition use case
 */
class OptimizeCameraUseCase {
private:
    static constexpr const char* TAG = "OptimizeCameraUC";
    ICameraRepository& _cameraRepo;
    IStorageRepository& _storageRepo;

public:
    enum class OptimizationType {
        LOW_LIGHT,
        SPEED,
        QUALITY
    };

    OptimizeCameraUseCase(ICameraRepository& cameraRepo,
                         IStorageRepository& storageRepo)
        : _cameraRepo(cameraRepo), _storageRepo(storageRepo) {}

    /**
     * @brief Execute: Optimize camera for specific condition
     */
    Result<void> execute(OptimizationType type) {
        Logger::getInstance().info(TAG, "Optimizing camera...");

        if (!_cameraRepo.isInitialized()) {
            return Result<void>::error("Camera not initialized");
        }

        // Get current settings
        CameraSettings settings = _cameraRepo.getCurrentSettings();

        // Apply optimization
        switch (type) {
            case OptimizationType::LOW_LIGHT:
                Logger::getInstance().info(TAG, "Applying low-light optimization");
                settings.optimizeForLowLight();
                break;

            case OptimizationType::SPEED:
                Logger::getInstance().info(TAG, "Applying speed optimization");
                settings.optimizeForSpeed();
                break;

            case OptimizationType::QUALITY:
                Logger::getInstance().info(TAG, "Applying quality optimization");
                settings.optimizeForQuality();
                break;
        }

        // Apply settings
        auto updateResult = _cameraRepo.updateSettings(settings);
        if (updateResult.isError()) {
            return updateResult;
        }

        // Save to storage
        _storageRepo.saveCameraSettings(settings);

        Logger::getInstance().info(TAG, "Camera optimized successfully");
        return Result<void>::ok();
    }
};

/**
 * @brief Reset camera use case
 */
class ResetCameraUseCase {
private:
    static constexpr const char* TAG = "ResetCameraUC";
    ICameraRepository& _cameraRepo;

public:
    ResetCameraUseCase(ICameraRepository& cameraRepo)
        : _cameraRepo(cameraRepo) {}

    /**
     * @brief Execute: Reset camera hardware
     */
    Result<void> execute() {
        Logger::getInstance().warn(TAG, "Resetting camera...");

        auto result = _cameraRepo.reset();
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Reset failed: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "Camera reset successfully");
        return Result<void>::ok();
    }
};

#endif // CAMERA_USE_CASES_H
