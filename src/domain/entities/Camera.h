/**
 * @file Camera.h
 * @brief Domain entity representing the camera device
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CAMERA_ENTITY_H
#define CAMERA_ENTITY_H

#include <Arduino.h>
#include "CameraSettings.h"
#include "../value_objects/PixelFormat.h"
#include "../../core/Result.h"

/**
 * @brief Camera state enumeration
 */
enum class CameraState {
    UNINITIALIZED,
    INITIALIZING,
    READY,
    CAPTURING,
    STREAMING,
    ERROR,
    SUSPENDED
};

/**
 * @brief Frame data structure
 */
struct Frame {
    uint8_t* buffer;
    size_t length;
    uint32_t width;
    uint32_t height;
    PixelFormat format;
    uint32_t timestamp;

    Frame()
        : buffer(nullptr), length(0), width(0), height(0),
          format(PixelFormat::JPEG()), timestamp(0) {}

    Frame(uint8_t* buf, size_t len, uint32_t w, uint32_t h, PixelFormat fmt)
        : buffer(buf), length(len), width(w), height(h),
          format(fmt), timestamp(millis()) {}

    bool isValid() const {
        return buffer != nullptr && length > 0;
    }

    void release() {
        buffer = nullptr;
        length = 0;
    }
};

/**
 * @brief Camera entity
 *
 * Represents the camera device with its state and configuration.
 * This is a domain entity that models the camera as a business concept,
 * independent of the underlying hardware implementation.
 */
class Camera {
private:
    CameraState _state;
    CameraSettings _settings;
    String _model;
    String _errorMessage;
    uint32_t _framesCaptured;
    uint32_t _framesDropped;
    uint32_t _lastFrameTime;
    bool _lampEnabled;
    uint8_t _lampIntensity;

public:
    /**
     * @brief Constructor
     */
    Camera(const String& model = "Unknown")
        : _state(CameraState::UNINITIALIZED),
          _model(model),
          _framesCaptured(0),
          _framesDropped(0),
          _lastFrameTime(0),
          _lampEnabled(false),
          _lampIntensity(0) {}

    // ========================================================================
    // State management
    // ========================================================================

    CameraState getState() const { return _state; }

    bool isReady() const {
        return _state == CameraState::READY || _state == CameraState::STREAMING;
    }

    bool isStreaming() const {
        return _state == CameraState::STREAMING;
    }

    bool hasError() const {
        return _state == CameraState::ERROR;
    }

    const String& getErrorMessage() const {
        return _errorMessage;
    }

    void setState(CameraState state) {
        _state = state;
        if (state != CameraState::ERROR) {
            _errorMessage = "";
        }
    }

    void setError(const String& message) {
        _state = CameraState::ERROR;
        _errorMessage = message;
    }

    // ========================================================================
    // Settings management
    // ========================================================================

    const CameraSettings& getSettings() const { return _settings; }

    CameraSettings& getSettings() { return _settings; }

    void updateSettings(const CameraSettings& settings) {
        _settings = settings;
    }

    // ========================================================================
    // Camera information
    // ========================================================================

    const String& getModel() const { return _model; }

    void setModel(const String& model) { _model = model; }

    /**
     * @brief Get sensor information string
     */
    String getSensorInfo() const {
        return _model + " - " + _settings.getResolution().toString() +
               " @ " + _settings.getPixelFormat().toString();
    }

    // ========================================================================
    // Statistics
    // ========================================================================

    uint32_t getFramesCaptured() const { return _framesCaptured; }

    uint32_t getFramesDropped() const { return _framesDropped; }

    void incrementFramesCaptured() { _framesCaptured++; }

    void incrementFramesDropped() { _framesDropped++; }

    uint32_t getLastFrameTime() const { return _lastFrameTime; }

    void updateFrameTime() { _lastFrameTime = millis(); }

    /**
     * @brief Get current frame rate (calculated)
     */
    float getCurrentFPS() const {
        if (_lastFrameTime == 0 || _framesCaptured == 0) {
            return 0.0f;
        }

        uint32_t elapsed = millis() - _lastFrameTime;
        if (elapsed == 0) return 0.0f;

        return 1000.0f / elapsed;
    }

    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        _framesCaptured = 0;
        _framesDropped = 0;
        _lastFrameTime = 0;
    }

    // ========================================================================
    // Lamp control
    // ========================================================================

    bool isLampEnabled() const { return _lampEnabled; }

    uint8_t getLampIntensity() const { return _lampIntensity; }

    void setLampEnabled(bool enabled) { _lampEnabled = enabled; }

    void setLampIntensity(uint8_t intensity) {
        _lampIntensity = constrain(intensity, 0, 100);
    }

    // ========================================================================
    // Utility methods
    // ========================================================================

    /**
     * @brief Get state as string
     */
    String getStateString() const {
        switch (_state) {
            case CameraState::UNINITIALIZED: return "Uninitialized";
            case CameraState::INITIALIZING:  return "Initializing";
            case CameraState::READY:         return "Ready";
            case CameraState::CAPTURING:     return "Capturing";
            case CameraState::STREAMING:     return "Streaming";
            case CameraState::ERROR:         return "Error";
            case CameraState::SUSPENDED:     return "Suspended";
            default:                         return "Unknown";
        }
    }

    /**
     * @brief Convert to string for logging
     */
    String toString() const {
        char buffer[256];
        sprintf(buffer,
                "Camera[model=%s, state=%s, frames=%lu, fps=%.1f, lamp=%d%%]",
                _model.c_str(),
                getStateString().c_str(),
                _framesCaptured,
                getCurrentFPS(),
                _lampIntensity);
        return String(buffer);
    }

    /**
     * @brief Check if camera requires PSRAM
     */
    bool requiresPSRAM() const {
        return _settings.getResolution().requiresPSRAM();
    }
};

#endif // CAMERA_ENTITY_H
