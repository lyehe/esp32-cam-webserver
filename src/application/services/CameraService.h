/**
 * @file CameraService.h
 * @brief Application service for camera operations
 *
 * Application services coordinate multiple use cases and provide
 * a high-level API for the presentation layer.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CAMERA_SERVICE_H
#define CAMERA_SERVICE_H

#include "../usecases/CameraUseCases.h"
#include "../../core/DependencyContainer.h"
#include "../../core/Logger.h"

/**
 * @brief Camera Application Service
 *
 * Provides high-level camera operations by coordinating use cases.
 * This is the main entry point for camera functionality from the presentation layer.
 */
class CameraService {
private:
    static constexpr const char* TAG = "CameraSvc";

    ICameraRepository& _cameraRepo;
    IStorageRepository& _storageRepo;

    // Use cases
    CaptureImageUseCase _captureImageUC;
    GetCameraStatusUseCase _getCameraStatusUC;
    UpdateCameraSettingsUseCase _updateSettingsUC;
    SetLampIntensityUseCase _setLampUC;
    OptimizeCameraUseCase _optimizeCameraUC;
    ResetCameraUseCase _resetCameraUC;

public:
    /**
     * @brief Constructor
     */
    CameraService(ICameraRepository& cameraRepo, IStorageRepository& storageRepo)
        : _cameraRepo(cameraRepo),
          _storageRepo(storageRepo),
          _captureImageUC(cameraRepo),
          _getCameraStatusUC(cameraRepo),
          _updateSettingsUC(cameraRepo, storageRepo),
          _setLampUC(cameraRepo),
          _optimizeCameraUC(cameraRepo, storageRepo),
          _resetCameraUC(cameraRepo) {
        Logger::getInstance().info(TAG, "Camera service initialized");
    }

    /**
     * @brief Capture a single image
     */
    Result<Frame> captureImage() {
        return _captureImageUC.execute();
    }

    /**
     * @brief Get current camera status
     */
    Result<CameraStatusDTO> getStatus() {
        return _getCameraStatusUC.execute();
    }

    /**
     * @brief Update camera settings
     */
    Result<void> updateSettings(const CameraSettings& settings) {
        return _updateSettingsUC.execute(settings);
    }

    /**
     * @brief Get current camera settings
     */
    CameraSettings getCurrentSettings() {
        return _cameraRepo.getCurrentSettings();
    }

    /**
     * @brief Set lamp intensity (0-100%)
     */
    Result<void> setLampIntensity(uint8_t intensity) {
        return _setLampUC.execute(intensity);
    }

    /**
     * @brief Optimize for low light conditions
     */
    Result<void> optimizeForLowLight() {
        return _optimizeCameraUC.execute(OptimizeCameraUseCase::OptimizationType::LOW_LIGHT);
    }

    /**
     * @brief Optimize for high-speed streaming
     */
    Result<void> optimizeForSpeed() {
        return _optimizeCameraUC.execute(OptimizeCameraUseCase::OptimizationType::SPEED);
    }

    /**
     * @brief Optimize for high quality
     */
    Result<void> optimizeForQuality() {
        return _optimizeCameraUC.execute(OptimizeCameraUseCase::OptimizationType::QUALITY);
    }

    /**
     * @brief Reset camera hardware
     */
    Result<void> reset() {
        return _resetCameraUC.execute();
    }

    /**
     * @brief Adjust specific camera parameters
     */
    Result<void> setBrightness(int8_t level) {
        return _cameraRepo.setBrightness(level);
    }

    Result<void> setContrast(int8_t level) {
        return _cameraRepo.setContrast(level);
    }

    Result<void> setSaturation(int8_t level) {
        return _cameraRepo.setSaturation(level);
    }

    Result<void> setAutoWhiteBalance(bool enable) {
        return _cameraRepo.setAutoWhiteBalance(enable);
    }

    Result<void> setAutoExposureControl(bool enable) {
        return _cameraRepo.setAutoExposureControl(enable);
    }

    /**
     * @brief Suspend camera (power saving)
     */
    Result<void> suspend() {
        Logger::getInstance().info(TAG, "Suspending camera");
        return _cameraRepo.suspend();
    }

    /**
     * @brief Resume camera from suspend
     */
    Result<void> resume() {
        Logger::getInstance().info(TAG, "Resuming camera");
        return _cameraRepo.resume();
    }

    /**
     * @brief Get sensor model
     */
    String getSensorModel() {
        return _cameraRepo.getSensorModel();
    }

    /**
     * @brief Check if camera is initialized
     */
    bool isInitialized() {
        return _cameraRepo.isInitialized();
    }
};

#endif // CAMERA_SERVICE_H
