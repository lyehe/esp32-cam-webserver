/**
 * @file ESP32CameraDriver.h
 * @brief ESP32 camera hardware driver for Arduino Core 3.3.4
 *
 * Implements low-level camera operations using the ESP32-CAM module.
 * Updated for ESP32 Arduino Core 3.3.4 with new LEDC API.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef ESP32_CAMERA_DRIVER_H
#define ESP32_CAMERA_DRIVER_H

#include <Arduino.h>
#include "esp_camera.h"
#include "../../core/Result.h"
#include "../../core/Logger.h"
#include "../../domain/entities/CameraSettings.h"
#include "../../domain/entities/Camera.h"
#include "../TypeMappers.h"

/**
 * @brief Pin configuration for different ESP32-CAM boards
 */
struct CameraPins {
    int8_t PWDN;
    int8_t RESET;
    int8_t XCLK;
    int8_t SIOD;
    int8_t SIOC;
    int8_t Y9;
    int8_t Y8;
    int8_t Y7;
    int8_t Y6;
    int8_t Y5;
    int8_t Y4;
    int8_t Y3;
    int8_t Y2;
    int8_t VSYNC;
    int8_t HREF;
    int8_t PCLK;
    int8_t LED_PIN;      // Flash LED pin
    uint8_t LED_CHANNEL; // LEDC channel for LED (Core 3.3.4)

    /**
     * @brief Default AI-Thinker ESP32-CAM configuration
     */
    static CameraPins AIThinker() {
        return {
            .PWDN     = 32,
            .RESET    = -1,
            .XCLK     = 0,
            .SIOD     = 26,
            .SIOC     = 27,
            .Y9       = 35,
            .Y8       = 34,
            .Y7       = 39,
            .Y6       = 36,
            .Y5       = 21,
            .Y4       = 19,
            .Y3       = 18,
            .Y2       = 5,
            .VSYNC    = 25,
            .HREF     = 23,
            .PCLK     = 22,
            .LED_PIN  = 4,
            .LED_CHANNEL = 7
        };
    }

    /**
     * @brief M5Stack ESP32-CAM configuration
     */
    static CameraPins M5Stack() {
        return {
            .PWDN     = -1,
            .RESET    = 15,
            .XCLK     = 27,
            .SIOD     = 25,
            .SIOC     = 23,
            .Y9       = 19,
            .Y8       = 36,
            .Y7       = 18,
            .Y6       = 39,
            .Y5       = 5,
            .Y4       = 34,
            .Y3       = 35,
            .Y2       = 32,
            .VSYNC    = 22,
            .HREF     = 26,
            .PCLK     = 21,
            .LED_PIN  = 14,
            .LED_CHANNEL = 7
        };
    }
};

/**
 * @brief ESP32 Camera Driver
 *
 * Low-level hardware driver for ESP32-CAM module.
 * Handles camera initialization, frame capture, and hardware control.
 */
class ESP32CameraDriver {
private:
    static constexpr const char* TAG = "CameraDriver";

    CameraPins _pins;
    bool _initialized;
    bool _lampInitialized;
    sensor_t* _sensor;
    uint32_t _xclkFreqHz;

    /**
     * @brief Initialize the flash LED using LEDC (Core 3.3.4 API)
     */
    Result<void> initializeLamp() {
        if (_pins.LED_PIN < 0) {
            Logger::getInstance().info(TAG, "No LED pin configured");
            return Result<void>::ok();
        }

        // Core 3.3.4: Use ledcAttach instead of ledcSetup/ledcAttachPin
        // ledcAttach(pin, freq, resolution) returns bool
        // Frequency: 5000 Hz, Resolution: 8-bit (0-255)
        bool success = ledcAttach(_pins.LED_PIN, 5000, 8);

        if (!success) {
            Logger::getInstance().error(TAG, "Failed to attach LEDC to LED pin");
            return Result<void>::error("LEDC attach failed");
        }

        // Initially off
        ledcWrite(_pins.LED_PIN, 0);

        _lampInitialized = true;
        Logger::getInstance().info(TAG, "Lamp initialized on pin " + String(_pins.LED_PIN));

        return Result<void>::ok();
    }

    /**
     * @brief Apply camera settings to sensor
     */
    Result<void> applySettings(const CameraSettings& settings) {
        if (!_sensor) {
            return Result<void>::error("Sensor not available");
        }

        // Image adjustments
        _sensor->set_brightness(_sensor, settings.getBrightness());
        _sensor->set_contrast(_sensor, settings.getContrast());
        _sensor->set_saturation(_sensor, settings.getSaturation());
        _sensor->set_sharpness(_sensor, settings.getSharpness());
        _sensor->set_special_effect(_sensor, settings.getSpecialEffect());

        // White balance
        _sensor->set_wb_mode(_sensor, settings.getWhiteBalanceMode());
        _sensor->set_whitebal(_sensor, settings.isAutoWhiteBalance() ? 1 : 0);
        _sensor->set_awb_gain(_sensor, settings.isAutoWhiteBalanceGain() ? 1 : 0);

        // Exposure
        _sensor->set_exposure_ctrl(_sensor, settings.isAutoExposureControl() ? 1 : 0);
        _sensor->set_aec2(_sensor, settings.isAutoExposureControl2() ? 1 : 0);
        _sensor->set_ae_level(_sensor, settings.getAutoExposureLevel());
        _sensor->set_aec_value(_sensor, settings.getAutoExposureValue());

        // Gain
        _sensor->set_gain_ctrl(_sensor, settings.isAutoGainControl() ? 1 : 0);
        _sensor->set_agc_gain(_sensor, settings.getAutoGainValue());
        _sensor->set_gainceiling(_sensor, (gainceiling_t)settings.getGainCeiling());

        // Lens corrections
        _sensor->set_lenc(_sensor, settings.isLensCorrectionEnabled() ? 1 : 0);
        _sensor->set_bpc(_sensor, settings.isBlackPixelCorrection() ? 1 : 0);
        _sensor->set_wpc(_sensor, settings.isWhitePixelCorrection() ? 1 : 0);
        _sensor->set_dcw(_sensor, settings.isDownscaleEnabled() ? 1 : 0);
        _sensor->set_colorbar(_sensor, settings.isColorBarEnabled() ? 1 : 0);

        // Image orientation
        _sensor->set_vflip(_sensor, settings.isVerticalFlip() ? 1 : 0);
        _sensor->set_hmirror(_sensor, settings.isHorizontalMirror() ? 1 : 0);

        Logger::getInstance().debug(TAG, "Applied settings to sensor");
        return Result<void>::ok();
    }

public:
    /**
     * @brief Constructor
     */
    ESP32CameraDriver(const CameraPins& pins = CameraPins::AIThinker(),
                     uint32_t xclkFreq = 20000000)
        : _pins(pins),
          _initialized(false),
          _lampInitialized(false),
          _sensor(nullptr),
          _xclkFreqHz(xclkFreq) {}

    /**
     * @brief Initialize camera with settings
     */
    Result<void> initialize(const CameraSettings& settings) {
        if (_initialized) {
            Logger::getInstance().warn(TAG, "Camera already initialized");
            return Result<void>::ok();
        }

        Logger::getInstance().info(TAG, "Initializing camera...");

        // Configure camera
        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer   = LEDC_TIMER_0;
        config.pin_d0       = _pins.Y2;
        config.pin_d1       = _pins.Y3;
        config.pin_d2       = _pins.Y4;
        config.pin_d3       = _pins.Y5;
        config.pin_d4       = _pins.Y6;
        config.pin_d5       = _pins.Y7;
        config.pin_d6       = _pins.Y8;
        config.pin_d7       = _pins.Y9;
        config.pin_xclk     = _pins.XCLK;
        config.pin_pclk     = _pins.PCLK;
        config.pin_vsync    = _pins.VSYNC;
        config.pin_href     = _pins.HREF;
        config.pin_sccb_sda = _pins.SIOD;
        config.pin_sccb_scl = _pins.SIOC;
        config.pin_pwdn     = _pins.PWDN;
        config.pin_reset    = _pins.RESET;
        config.xclk_freq_hz = _xclkFreqHz;

        // Apply domain settings
        config.pixel_format = TypeMappers::toHardwarePixelFormat(settings.getPixelFormat());
        config.frame_size   = TypeMappers::toHardwareFrameSize(settings.getResolution().getFrameSize());
        config.jpeg_quality = settings.getQuality();
        config.fb_count     = settings.getFrameBufferCount();

        // PSRAM configuration
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST; // Always get latest frame

        // Initialize camera
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            String errorMsg = "Camera init failed: 0x" + String(err, HEX);
            Logger::getInstance().error(TAG, errorMsg);
            return Result<void>::error(errorMsg, err);
        }

        // Get sensor
        _sensor = esp_camera_sensor_get();
        if (!_sensor) {
            Logger::getInstance().error(TAG, "Failed to get sensor");
            return Result<void>::error("Sensor not found");
        }

        // Apply additional settings
        auto result = applySettings(settings);
        if (result.isError()) {
            return result;
        }

        // Initialize lamp
        auto lampResult = initializeLamp();
        if (lampResult.isError()) {
            Logger::getInstance().warn(TAG, "Lamp init failed: " + lampResult.getError());
            // Not critical, continue
        }

        _initialized = true;

        Logger::getInstance().info(TAG, "Camera initialized successfully");
        Logger::getInstance().info(TAG, "Sensor: " + String(getSensorModel()));
        Logger::getInstance().info(TAG, "Resolution: " + settings.getResolution().toString());
        Logger::getInstance().info(TAG, "Format: " + settings.getPixelFormat().toString());

        return Result<void>::ok();
    }

    /**
     * @brief Deinitialize camera
     */
    Result<void> deinitialize() {
        if (!_initialized) {
            return Result<void>::ok();
        }

        // Turn off lamp
        if (_lampInitialized) {
            ledcWrite(_pins.LED_PIN, 0);
            ledcDetach(_pins.LED_PIN); // Core 3.3.4 API
        }

        esp_err_t err = esp_camera_deinit();
        if (err != ESP_OK) {
            String errorMsg = "Camera deinit failed: 0x" + String(err, HEX);
            Logger::getInstance().error(TAG, errorMsg);
            return Result<void>::error(errorMsg, err);
        }

        _initialized = false;
        _lampInitialized = false;
        _sensor = nullptr;

        Logger::getInstance().info(TAG, "Camera deinitialized");
        return Result<void>::ok();
    }

    /**
     * @brief Capture a frame from camera
     */
    Result<camera_fb_t*> captureFrame() {
        if (!_initialized) {
            return Result<camera_fb_t*>::error("Camera not initialized");
        }

        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            Logger::getInstance().error(TAG, "Frame capture failed");
            return Result<camera_fb_t*>::error("Frame buffer allocation failed");
        }

        return Result<camera_fb_t*>::ok(fb);
    }

    /**
     * @brief Release a captured frame
     */
    void releaseFrame(camera_fb_t* fb) {
        if (fb) {
            esp_camera_fb_return(fb);
        }
    }

    /**
     * @brief Update camera settings
     */
    Result<void> updateSettings(const CameraSettings& settings) {
        if (!_initialized) {
            return Result<void>::error("Camera not initialized");
        }

        // Update frame size if changed
        framesize_t newSize = TypeMappers::toHardwareFrameSize(settings.getResolution().getFrameSize());
        if (_sensor->status.framesize != newSize) {
            _sensor->set_framesize(_sensor, newSize);
        }

        // Update pixel format if changed
        pixformat_t newFormat = TypeMappers::toHardwarePixelFormat(settings.getPixelFormat());
        if (_sensor->pixformat != newFormat) {
            _sensor->set_pixformat(_sensor, newFormat);
        }

        // Update quality
        _sensor->set_quality(_sensor, settings.getQuality());

        // Apply other settings
        return applySettings(settings);
    }

    /**
     * @brief Set lamp intensity (0-100%)
     */
    Result<void> setLampIntensity(uint8_t intensity) {
        if (!_lampInitialized) {
            return Result<void>::error("Lamp not initialized");
        }

        // Convert 0-100 to 0-255
        uint8_t duty = map(intensity, 0, 100, 0, 255);
        ledcWrite(_pins.LED_PIN, duty);

        Logger::getInstance().debug(TAG, "Lamp intensity: " + String(intensity) + "%");
        return Result<void>::ok();
    }

    /**
     * @brief Suspend camera (power saving)
     */
    Result<void> suspend() {
        if (!_initialized) {
            return Result<void>::ok();
        }

        // Turn off lamp
        if (_lampInitialized) {
            ledcWrite(_pins.LED_PIN, 0);
        }

        // Power down camera
        if (_pins.PWDN >= 0) {
            pinMode(_pins.PWDN, OUTPUT);
            digitalWrite(_pins.PWDN, HIGH);
        }

        Logger::getInstance().info(TAG, "Camera suspended");
        return Result<void>::ok();
    }

    /**
     * @brief Resume camera from suspend
     */
    Result<void> resume() {
        if (!_initialized) {
            return Result<void>::error("Camera not initialized");
        }

        // Power up camera
        if (_pins.PWDN >= 0) {
            digitalWrite(_pins.PWDN, LOW);
            delay(10); // Allow camera to stabilize
        }

        Logger::getInstance().info(TAG, "Camera resumed");
        return Result<void>::ok();
    }

    /**
     * @brief Reset camera
     */
    Result<void> reset() {
        if (!_initialized) {
            return Result<void>::error("Camera not initialized");
        }

        auto deinitResult = deinitialize();
        if (deinitResult.isError()) {
            return deinitResult;
        }

        delay(100); // Wait for hardware to reset

        // Will need to reinitialize with settings
        Logger::getInstance().info(TAG, "Camera reset");
        return Result<void>::ok();
    }

    /**
     * @brief Get sensor model string
     */
    String getSensorModel() const {
        if (!_sensor) {
            return "Unknown";
        }

        switch (_sensor->id.PID) {
            case OV2640_PID:  return "OV2640";
            case OV3660_PID:  return "OV3660";
            case OV5640_PID:  return "OV5640";
            case OV7670_PID:  return "OV7670";
            default:          return "Unknown (0x" + String(_sensor->id.PID, HEX) + ")";
        }
    }

    /**
     * @brief Check if camera is initialized
     */
    bool isInitialized() const {
        return _initialized;
    }

    /**
     * @brief Get sensor pointer (for advanced operations)
     */
    sensor_t* getSensor() const {
        return _sensor;
    }
};

#endif // ESP32_CAMERA_DRIVER_H
