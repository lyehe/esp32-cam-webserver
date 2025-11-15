/**
 * @file CameraSettings.h
 * @brief Entity representing camera configuration settings
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CAMERA_SETTINGS_H
#define CAMERA_SETTINGS_H

#include <Arduino.h>
#include "../value_objects/Resolution.h"
#include "../value_objects/PixelFormat.h"

/**
 * @brief Camera settings entity
 *
 * Mutable entity representing the complete configuration state of the camera.
 * Includes both basic settings and advanced sensor controls.
 */
class CameraSettings {
private:
    // Basic settings
    Resolution _resolution;
    PixelFormat _pixelFormat;
    uint8_t _quality;          // JPEG quality (0-63, lower is better)
    uint8_t _frameBufferCount; // Number of frame buffers (1-2)

    // Image adjustments
    int8_t _brightness;   // -2 to +2
    int8_t _contrast;     // -2 to +2
    int8_t _saturation;   // -2 to +2
    int8_t _sharpness;    // -2 to +2
    uint8_t _specialEffect; // 0-6 (Normal, Negative, Grayscale, etc.)

    // White balance
    uint8_t _whiteBalanceMode; // 0-4 (Auto, Sunny, Cloudy, Office, Home)
    bool _autoWhiteBalance;
    bool _autoWhiteBalanceGain;

    // Exposure
    bool _autoExposureControl;
    bool _autoExposureControl2;  // DSP auto exposure
    int8_t _autoExposureLevel;   // -2 to +2
    uint16_t _autoExposureValue; // 0-1200

    // Gain
    bool _autoGainControl;
    uint8_t _autoGainValue;      // 0-30
    uint8_t _gainCeiling;        // 0-6 (2x to 128x)

    // Lens corrections
    bool _lensCorrectionEnabled;
    bool _blackPixelCorrection;
    bool _whitePixelCorrection;
    bool _gammaCorrection;

    // Image orientation
    bool _verticalFlip;
    bool _horizontalMirror;
    int16_t _rotation;  // 0, 90, 180, 270 degrees

    // Advanced
    bool _downscaleEnable;  // Downscale to half size
    bool _colorBar;         // Test pattern

public:
    /**
     * @brief Default constructor with sensible defaults
     */
    CameraSettings()
        : _resolution(Resolution::SVGA()),
          _pixelFormat(PixelFormat::JPEG()),
          _quality(12),
          _frameBufferCount(2),
          _brightness(0),
          _contrast(0),
          _saturation(0),
          _sharpness(0),
          _specialEffect(0),
          _whiteBalanceMode(0),
          _autoWhiteBalance(true),
          _autoWhiteBalanceGain(true),
          _autoExposureControl(true),
          _autoExposureControl2(false),
          _autoExposureLevel(0),
          _autoExposureValue(300),
          _autoGainControl(true),
          _autoGainValue(0),
          _gainCeiling(0),
          _lensCorrectionEnabled(true),
          _blackPixelCorrection(true),
          _whitePixelCorrection(true),
          _gammaCorrection(false),
          _verticalFlip(false),
          _horizontalMirror(false),
          _rotation(0),
          _downscaleEnable(false),
          _colorBar(false) {}

    // ========================================================================
    // Getters
    // ========================================================================

    const Resolution& getResolution() const { return _resolution; }
    const PixelFormat& getPixelFormat() const { return _pixelFormat; }
    uint8_t getQuality() const { return _quality; }
    uint8_t getFrameBufferCount() const { return _frameBufferCount; }

    int8_t getBrightness() const { return _brightness; }
    int8_t getContrast() const { return _contrast; }
    int8_t getSaturation() const { return _saturation; }
    int8_t getSharpness() const { return _sharpness; }
    uint8_t getSpecialEffect() const { return _specialEffect; }

    uint8_t getWhiteBalanceMode() const { return _whiteBalanceMode; }
    bool isAutoWhiteBalance() const { return _autoWhiteBalance; }
    bool isAutoWhiteBalanceGain() const { return _autoWhiteBalanceGain; }

    bool isAutoExposureControl() const { return _autoExposureControl; }
    bool isAutoExposureControl2() const { return _autoExposureControl2; }
    int8_t getAutoExposureLevel() const { return _autoExposureLevel; }
    uint16_t getAutoExposureValue() const { return _autoExposureValue; }

    bool isAutoGainControl() const { return _autoGainControl; }
    uint8_t getAutoGainValue() const { return _autoGainValue; }
    uint8_t getGainCeiling() const { return _gainCeiling; }

    bool isLensCorrectionEnabled() const { return _lensCorrectionEnabled; }
    bool isBlackPixelCorrection() const { return _blackPixelCorrection; }
    bool isWhitePixelCorrection() const { return _whitePixelCorrection; }
    bool isGammaCorrection() const { return _gammaCorrection; }

    bool isVerticalFlip() const { return _verticalFlip; }
    bool isHorizontalMirror() const { return _horizontalMirror; }
    int16_t getRotation() const { return _rotation; }

    bool isDownscaleEnabled() const { return _downscaleEnable; }
    bool isColorBarEnabled() const { return _colorBar; }

    // ========================================================================
    // Setters with validation
    // ========================================================================

    void setResolution(const Resolution& resolution) { _resolution = resolution; }
    void setPixelFormat(const PixelFormat& format) { _pixelFormat = format; }

    void setQuality(uint8_t quality) {
        _quality = constrain(quality, 0, 63);
    }

    void setFrameBufferCount(uint8_t count) {
        _frameBufferCount = constrain(count, 1, 2);
    }

    void setBrightness(int8_t value) {
        _brightness = constrain(value, -2, 2);
    }

    void setContrast(int8_t value) {
        _contrast = constrain(value, -2, 2);
    }

    void setSaturation(int8_t value) {
        _saturation = constrain(value, -2, 2);
    }

    void setSharpness(int8_t value) {
        _sharpness = constrain(value, -2, 2);
    }

    void setSpecialEffect(uint8_t effect) {
        _specialEffect = constrain(effect, 0, 6);
    }

    void setWhiteBalanceMode(uint8_t mode) {
        _whiteBalanceMode = constrain(mode, 0, 4);
    }

    void setAutoWhiteBalance(bool enable) { _autoWhiteBalance = enable; }
    void setAutoWhiteBalanceGain(bool enable) { _autoWhiteBalanceGain = enable; }

    void setAutoExposureControl(bool enable) { _autoExposureControl = enable; }
    void setAutoExposureControl2(bool enable) { _autoExposureControl2 = enable; }

    void setAutoExposureLevel(int8_t level) {
        _autoExposureLevel = constrain(level, -2, 2);
    }

    void setAutoExposureValue(uint16_t value) {
        _autoExposureValue = constrain(value, 0, 1200);
    }

    void setAutoGainControl(bool enable) { _autoGainControl = enable; }

    void setAutoGainValue(uint8_t value) {
        _autoGainValue = constrain(value, 0, 30);
    }

    void setGainCeiling(uint8_t ceiling) {
        _gainCeiling = constrain(ceiling, 0, 6);
    }

    void setLensCorrectionEnabled(bool enable) { _lensCorrectionEnabled = enable; }
    void setBlackPixelCorrection(bool enable) { _blackPixelCorrection = enable; }
    void setWhitePixelCorrection(bool enable) { _whitePixelCorrection = enable; }
    void setGammaCorrection(bool enable) { _gammaCorrection = enable; }

    void setVerticalFlip(bool enable) { _verticalFlip = enable; }
    void setHorizontalMirror(bool enable) { _horizontalMirror = enable; }

    void setRotation(int16_t degrees) {
        // Normalize to 0, 90, 180, 270
        degrees = degrees % 360;
        if (degrees < 0) degrees += 360;

        // Round to nearest 90 degrees
        if (degrees < 45) _rotation = 0;
        else if (degrees < 135) _rotation = 90;
        else if (degrees < 225) _rotation = 180;
        else if (degrees < 315) _rotation = 270;
        else _rotation = 0;
    }

    void setDownscaleEnabled(bool enable) { _downscaleEnable = enable; }
    void setColorBarEnabled(bool enable) { _colorBar = enable; }

    // ========================================================================
    // Utility methods
    // ========================================================================

    /**
     * @brief Reset to default settings
     */
    void resetToDefaults() {
        *this = CameraSettings();
    }

    /**
     * @brief Check if settings are optimized for low light
     */
    bool isLowLightOptimized() const {
        return _brightness >= 1 &&
               _autoExposureLevel >= 1 &&
               _gainCeiling >= 4;
    }

    /**
     * @brief Apply low-light optimizations
     */
    void optimizeForLowLight() {
        _brightness = 1;
        _contrast = 1;
        _autoExposureLevel = 2;
        _gainCeiling = 6;  // Max gain
        _autoGainControl = true;
        _autoExposureControl = true;
    }

    /**
     * @brief Apply high-speed streaming optimizations
     */
    void optimizeForSpeed() {
        _resolution = Resolution::VGA();  // Lower resolution
        _quality = 20;  // Lower quality, faster compression
        _frameBufferCount = 2;  // Double buffering
    }

    /**
     * @brief Apply high-quality capture optimizations
     */
    void optimizeForQuality() {
        _resolution = Resolution::UXGA();  // Max resolution
        _quality = 4;  // High quality
        _lensCorrectionEnabled = true;
        _blackPixelCorrection = true;
        _whitePixelCorrection = true;
    }

    /**
     * @brief Convert to string for logging
     */
    String toString() const {
        return "CameraSettings["
               "resolution=" + _resolution.toString() +
               ", format=" + _pixelFormat.toString() +
               ", quality=" + String(_quality) +
               ", brightness=" + String(_brightness) +
               "]";
    }
};

#endif // CAMERA_SETTINGS_H
