/**
 * @file ICameraRepository.h
 * @brief Interface for camera repository
 *
 * Defines the contract for camera operations following the Repository pattern.
 * Infrastructure layer will provide concrete implementation.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef I_CAMERA_REPOSITORY_H
#define I_CAMERA_REPOSITORY_H

#include "../entities/Camera.h"
#include "../../core/Result.h"

/**
 * @brief Camera repository interface
 *
 * Provides an abstraction for camera hardware operations.
 * Follows Dependency Inversion Principle - high-level code depends on this interface,
 * not on concrete hardware implementations.
 */
class ICameraRepository {
public:
    virtual ~ICameraRepository() = default;

    /**
     * @brief Initialize camera with settings
     *
     * @param settings Camera configuration
     * @return Result<void> Success or error
     */
    virtual Result<void> initialize(const CameraSettings& settings) = 0;

    /**
     * @brief Deinitialize camera
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> deinitialize() = 0;

    /**
     * @brief Check if camera is initialized
     *
     * @return true if initialized, false otherwise
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Capture a single frame
     *
     * @return Result<Frame> Frame data or error
     */
    virtual Result<Frame> captureFrame() = 0;

    /**
     * @brief Release a frame buffer
     *
     * Must be called after processing a frame to return buffer to pool.
     *
     * @param frame Frame to release
     */
    virtual void releaseFrame(Frame& frame) = 0;

    /**
     * @brief Update camera settings
     *
     * @param settings New camera settings
     * @return Result<void> Success or error
     */
    virtual Result<void> updateSettings(const CameraSettings& settings) = 0;

    /**
     * @brief Get current camera settings
     *
     * @return CameraSettings Current settings
     */
    virtual CameraSettings getCurrentSettings() const = 0;

    /**
     * @brief Get camera information
     *
     * @return Camera Camera entity with current state
     */
    virtual Camera getCameraInfo() const = 0;

    /**
     * @brief Set camera lamp intensity
     *
     * @param intensity Intensity (0-100)
     * @return Result<void> Success or error
     */
    virtual Result<void> setLampIntensity(uint8_t intensity) = 0;

    /**
     * @brief Get current lamp intensity
     *
     * @return uint8_t Intensity (0-100)
     */
    virtual uint8_t getLampIntensity() const = 0;

    /**
     * @brief Suspend camera (low power mode)
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> suspend() = 0;

    /**
     * @brief Resume camera from suspend
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> resume() = 0;

    /**
     * @brief Reset camera (reinitialize with same settings)
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> reset() = 0;

    /**
     * @brief Get sensor name/model
     *
     * @return String Sensor model (e.g., "OV2640", "OV3660")
     */
    virtual String getSensorModel() const = 0;

    /**
     * @brief Get sensor ID
     *
     * @return uint8_t Sensor PID
     */
    virtual uint8_t getSensorId() const = 0;
};

#endif // I_CAMERA_REPOSITORY_H
