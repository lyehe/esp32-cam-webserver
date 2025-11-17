/**
 * @file CameraRepository.h
 * @brief Camera repository implementation for ESP32
 *
 * Implements ICameraRepository using ESP32CameraDriver.
 * Manages frame buffers and camera hardware lifecycle.
 * Thread-safe with FreeRTOS mutex protection.
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
#include "../../core/ScopedMutex.h"
#include "../../core/Constants.h"
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief ESP32 Camera Repository
 *
 * Infrastructure implementation of ICameraRepository.
 * Uses ESP32CameraDriver for hardware access.
 *
 * Thread Safety: All public methods are thread-safe using FreeRTOS mutex.
 * Multiple tasks can safely call any method concurrently.
 */
class CameraRepository : public ICameraRepository {
private:
    static constexpr const char* TAG = "CameraRepo";

    std::unique_ptr<ESP32CameraDriver> _driver;
    CameraSettings _currentSettings;
    bool _initialized;
    uint8_t _lampIntensity;
    SemaphoreHandle_t _mutex;

public:
    /**
     * @brief Constructor
     *
     * Creates camera repository with specified pin configuration.
     * Initializes mutex for thread safety.
     *
     * @param pins Camera pin configuration (default: AI-Thinker)
     */
    CameraRepository(const CameraPins& pins = CameraPins::AIThinker())
        : _initialized(false)
        , _lampIntensity(0)
        , _mutex(nullptr) {

        _driver = std::make_unique<ESP32CameraDriver>(pins);
        _mutex = xSemaphoreCreateMutex();

        if (!_mutex) {
            Logger::getInstance().error(TAG, "Failed to create mutex - thread safety compromised!");
        }
    }

    /**
     * @brief Destructor
     *
     * Cleans up mutex resources automatically.
     */
    ~CameraRepository() {
        if (_mutex) {
            vSemaphoreDelete(_mutex);
            _mutex = nullptr;
        }
    }

    /**
     * @brief Initialize camera with settings (thread-safe)
     *
     * @param settings Camera configuration to apply
     * @return Result<void> Success or error with message
     */
    Result<void> initialize(const CameraSettings& settings) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_LONG_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        Logger::getInstance().info(TAG, "Initializing camera repository");

        auto result = _driver->initialize(settings);
        if (result.isError()) {
            return result;
        }

        _currentSettings = settings;
        _initialized = true;

        Logger::getInstance().info(TAG, "Camera repository initialized successfully");
        return Result<void>::ok();
    }

    /**
     * @brief Deinitialize camera (thread-safe)
     *
     * @return Result<void> Success or error with message
     */
    Result<void> deinitialize() override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_LONG_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        if (!_initialized) {
            return Result<void>::ok();  // Already deinitialized
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
     * @brief Capture a frame (thread-safe)
     *
     * Thread Safety: Protected by mutex. Safe for concurrent calls from
     * multiple tasks (e.g., StreamTask + HTTP handlers).
     *
     * @return Result<Frame> Frame data or error
     */
    Result<Frame> captureFrame() override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<Frame>::error("Failed to acquire camera lock (timeout)");
        }

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

        // Return frame (caller must release via releaseFrame())
        return Result<Frame>::ok(domainFrame);
    }

    /**
     * @brief Release a captured frame
     *
     * Note: Frame buffer management is handled by ESP32CameraDriver.
     * This marks the frame as released in the domain layer.
     *
     * @param frame Frame to release
     */
    void releaseFrame(Frame& frame) override {
        frame.release();
        Logger::getInstance().trace(TAG, "Frame released");
    }

    /**
     * @brief Update camera settings (thread-safe)
     *
     * Thread Safety: Settings are atomically updated under mutex protection.
     * No partial state visible to other threads.
     *
     * @param settings New camera settings to apply
     * @return Result<void> Success or error with message
     */
    Result<void> updateSettings(const CameraSettings& settings) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_LONG_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        if (!_initialized) {
            return Result<void>::error("Camera not initialized");
        }

        auto result = _driver->updateSettings(settings);
        if (result.isError()) {
            return result;
        }

        _currentSettings = settings;
        Logger::getInstance().info(TAG, "Settings updated successfully");
        return Result<void>::ok();
    }

    /**
     * @brief Get current settings (thread-safe)
     *
     * Thread Safety: Returns a copy of settings under mutex protection.
     *
     * @return CameraSettings Copy of current settings
     */
    CameraSettings getCurrentSettings() const override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            Logger::getInstance().warn(TAG, "Failed to acquire lock for getCurrentSettings");
            return CameraSettings();  // Return default settings
        }

        return _currentSettings;
    }

    /**
     * @brief Set lamp intensity (thread-safe)
     *
     * @param intensity LED brightness (0-255)
     * @return Result<void> Success or error with message
     */
    Result<void> setLampIntensity(uint8_t intensity) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        if (!_initialized) {
            return Result<void>::error("Camera not initialized");
        }

        // Validate intensity
        if (intensity > Constants::INTENSITY_MAX) {
            return Result<void>::error("Invalid intensity value", intensity);
        }

        auto result = _driver->setLampIntensity(intensity);
        if (result.isOk()) {
            _lampIntensity = intensity;
        }

        return result;
    }

    /**
     * @brief Suspend camera (thread-safe)
     *
     * @return Result<void> Success or error with message
     */
    Result<void> suspend() override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        return _driver->suspend();
    }

    /**
     * @brief Resume camera (thread-safe)
     *
     * @return Result<void> Success or error with message
     */
    Result<void> resume() override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        return _driver->resume();
    }

    /**
     * @brief Reset camera (thread-safe)
     *
     * Resets hardware and reinitializes with current settings.
     *
     * @return Result<void> Success or error with message
     */
    Result<void> reset() override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_LONG_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        auto result = _driver->reset();
        if (result.isError()) {
            return result;
        }

        // Reinitialize with current settings
        return _driver->initialize(_currentSettings);
    }

    /**
     * @brief Get sensor model name (thread-safe)
     *
     * @return String Sensor model (e.g., "OV2640", "OV3660")
     */
    String getSensorModel() const override {
        // Driver method is const and thread-safe
        return _driver->getSensorModel();
    }

    /**
     * @brief Check if camera is initialized (thread-safe)
     *
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_SHORT_MS);
        if (!lock.isAcquired()) {
            Logger::getInstance().warn(TAG, "Failed to acquire lock for isInitialized");
            return false;
        }

        return _initialized;
    }

    /**
     * @brief Set auto white balance (thread-safe)
     *
     * Convenience method that updates settings and applies immediately.
     *
     * @param enable true to enable AWB, false to disable
     * @return Result<void> Success or error with message
     */
    Result<void> setAutoWhiteBalance(bool enable) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        _currentSettings.setAutoWhiteBalance(enable);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set auto exposure control (thread-safe)
     *
     * @param enable true to enable AEC, false to disable
     * @return Result<void> Success or error with message
     */
    Result<void> setAutoExposureControl(bool enable) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        _currentSettings.setAutoExposureControl(enable);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set brightness (thread-safe)
     *
     * @param level Brightness level (-2 to +2)
     * @return Result<void> Success or error with message
     */
    Result<void> setBrightness(int8_t level) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        // Validate range
        if (level < Constants::ADJUSTMENT_MIN || level > Constants::ADJUSTMENT_MAX) {
            return Result<void>::error("Brightness out of range [-2, +2]", level);
        }

        _currentSettings.setBrightness(level);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set contrast (thread-safe)
     *
     * @param level Contrast level (-2 to +2)
     * @return Result<void> Success or error with message
     */
    Result<void> setContrast(int8_t level) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        // Validate range
        if (level < Constants::ADJUSTMENT_MIN || level > Constants::ADJUSTMENT_MAX) {
            return Result<void>::error("Contrast out of range [-2, +2]", level);
        }

        _currentSettings.setContrast(level);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Set saturation (thread-safe)
     *
     * @param level Saturation level (-2 to +2)
     * @return Result<void> Success or error with message
     */
    Result<void> setSaturation(int8_t level) override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            return Result<void>::error("Failed to acquire camera lock (timeout)");
        }

        // Validate range
        if (level < Constants::ADJUSTMENT_MIN || level > Constants::ADJUSTMENT_MAX) {
            return Result<void>::error("Saturation out of range [-2, +2]", level);
        }

        _currentSettings.setSaturation(level);
        return _driver->updateSettings(_currentSettings);
    }

    /**
     * @brief Get camera info (thread-safe)
     *
     * Creates Camera entity with current state and settings.
     *
     * @return Camera Entity with current camera state
     */
    Camera getCameraInfo() const override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_MS);
        if (!lock.isAcquired()) {
            Logger::getInstance().warn(TAG, "Failed to acquire lock for getCameraInfo");
            Camera camera("UNKNOWN");
            camera.setState(CameraState::ERROR);
            return camera;
        }

        String model = _driver->getSensorModel();
        Camera camera(model);

        if (_initialized) {
            camera.updateSettings(_currentSettings);
            camera.setState(CameraState::READY);
        } else {
            camera.setState(CameraState::UNINITIALIZED);
        }

        return camera;
    }

    /**
     * @brief Get current lamp intensity (thread-safe)
     *
     * @return uint8_t Lamp intensity (0-255)
     */
    uint8_t getLampIntensity() const override {
        ScopedMutex lock(_mutex, Constants::MUTEX_TIMEOUT_SHORT_MS);
        if (!lock.isAcquired()) {
            Logger::getInstance().warn(TAG, "Failed to acquire lock for getLampIntensity");
            return 0;
        }

        return _lampIntensity;
    }

    /**
     * @brief Get sensor ID (thread-safe)
     *
     * @return uint8_t Sensor Product ID (PID)
     */
    uint8_t getSensorId() const override {
        // Driver method is thread-safe (sensor_t* is immutable after init)
        sensor_t* sensor = _driver->getSensor();
        return sensor ? sensor->id.PID : 0;
    }
};

#endif // CAMERA_REPOSITORY_H
