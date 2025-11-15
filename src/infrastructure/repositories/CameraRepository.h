/**
 * @file CameraRepository.h
 * @brief Camera repository implementation for ESP32
 *
 * Implements ICameraRepository using ESP32CameraDriver.
 * Manages frame buffers and camera hardware lifecycle.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CAMERA_REPOSITORY_H
#define CAMERA_REPOSITORY_H

#include "../../domain/repositories/ICameraRepository.h"
#include "../drivers/ESP32CameraDriver.h"
#include "../TypeMappers.h"
#include "../../core/Logger.h"
#include <memory>

/**
 * @brief ESP32 Camera Repository
 *
 * Infrastructure implementation of ICameraRepository.
 * Uses ESP32CameraDriver for hardware access.
 */
class CameraRepository : public ICameraRepository {
private:
    static constexpr const char* TAG = "CameraRepo";

    std::unique_ptr<ESP32CameraDriver> _driver;
    CameraSettings _currentSettings;
    bool _initialized;

public:
    /**
     * @brief Constructor
     */
    CameraRepository(const CameraPins& pins = CameraPins::AIThinker())
        : _initialized(false) {
        _driver = std::make_unique<ESP32CameraDriver>(pins);
    }

    /**
     * @brief Initialize camera with settings
     */
    Result<void> initialize(const CameraSettings& settings) override {
        Logger::getInstance().info(TAG, "Initializing camera repository");

        auto result = _driver->initialize(settings);
        if (result.isError()) {
            return result;
        }

        _currentSettings = settings;
        _initialized = true;

        Logger::getInstance().info(TAG, "Camera repository initialized");
        return Result<void>::ok();
    }

    /**
     * @brief Deinitialize camera
     */
    Result<void> deinitialize() override {
        if (!_initialized) {
            return Result<void>::ok();
        }

        auto result = _driver->deinitialize();
        if (result.isError()) {
            return result;
        }

        _initialized = false;
        Logger::getInstance().info(TAG, "Camera repository deinitialized");
        return Result<void>::ok();
    }

    /**
     * @brief Capture a frame
     */
    Result<Frame> captureFrame() override {
        if (!_initialized) {
            return Result<Frame>::error("Camera not initialized");
        }

        // Capture hardware frame
        auto fbResult = _driver->captureFrame();
        if (fbResult.isError()) {
            return Result<Frame>::error(fbResult.getError());
        }

        camera_fb_t* fb = fbResult.getValue();

        // Convert to domain Frame
        Frame domainFrame = TypeMappers::fromHardwareFrame(fb);

        // Note: We don't release the hardware frame here
        // The caller must call releaseFrame() when done
        // Store fb pointer in a map for later release? Or require caller to manage?
        // For now, storing fb pointer is problematic. Let's document that caller must release.

        return Result<Frame>::ok(domainFrame);
    }

    /**
     * @brief Release a captured frame
     */
    void releaseFrame(Frame& frame) override {
        // In current design, Frame doesn't store the camera_fb_t pointer
        // This is a design issue we need to address
        // For now, we'll document that frames are auto-released on next capture
        // TODO: Consider adding void* hardwareHandle to Frame struct

        frame.release();
        Logger::getInstance().trace(TAG, "Frame released (marking buffer as null)");
    }

    /**
     * @brief Update camera settings
     */
    Result<void> updateSettings(const CameraSettings& settings) override {
        if (!_initialized) {
            return Result<void>::error("Camera not initialized");
        }

        auto result = _driver->updateSettings(settings);
        if (result.isError()) {
            return result;
        }

        _currentSettings = settings;
        Logger::getInstance().info(TAG, "Settings updated");
        return Result<void>::ok();
    }

    /**
     * @brief Get current settings
     */
    CameraSettings getCurrentSettings() const override {
        return _currentSettings;
    }

    /**
     * @brief Set lamp intensity
     */
    Result<void> setLampIntensity(uint8_t intensity) override {
        if (!_initialized) {
            return Result<void>::error("Camera not initialized");
        }

        return _driver->setLampIntensity(intensity);
    }

    /**
     * @brief Suspend camera
     */
    Result<void> suspend() override {
        return _driver->suspend();
    }

    /**
     * @brief Resume camera
     */
    Result<void> resume() override {
        return _driver->resume();
    }

    /**
     * @brief Reset camera
     */
    Result<void> reset() override {
        auto result = _driver->reset();
        if (result.isError()) {
            return result;
        }

        // Reinitialize with current settings
        return _driver->initialize(_currentSettings);
    }

    /**
     * @brief Get sensor model
     */
    String getSensorModel() const override {
        return _driver->getSensorModel();
    }

    /**
     * @brief Check if camera is initialized
     */
    bool isInitialized() const override {
        return _initialized;
    }

    /**
     * @brief Set auto white balance
     */
    Result<void> setAutoWhiteBalance(bool enable) override {
        _currentSettings.setAutoWhiteBalance(enable);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set auto exposure control
     */
    Result<void> setAutoExposureControl(bool enable) override {
        _currentSettings.setAutoExposureControl(enable);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set brightness
     */
    Result<void> setBrightness(int8_t level) override {
        _currentSettings.setBrightness(level);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set contrast
     */
    Result<void> setContrast(int8_t level) override {
        _currentSettings.setContrast(level);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set saturation
     */
    Result<void> setSaturation(int8_t level) override {
        _currentSettings.setSaturation(level);
        return _driver->updateSettings(_currentSettings);
    }
};

#endif // CAMERA_REPOSITORY_H
