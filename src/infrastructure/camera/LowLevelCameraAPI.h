/**
 * @file LowLevelCameraAPI.h
 * @brief Direct hardware access to camera sensor
 *
 * Provides three levels of camera control:
 * 1. High-Level: CameraSettings (recommended)
 * 2. Mid-Level: camera_config_t manipulation
 * 3. Low-Level: Direct sensor register access (advanced)
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef LOW_LEVEL_CAMERA_API_H
#define LOW_LEVEL_CAMERA_API_H

#include <Arduino.h>
#include "esp_camera.h"
#include "../../core/Logger.h"

/**
 * @brief Low-level camera hardware access
 *
 * WARNING: These APIs bypass safety checks!
 * Use only if you know what you're doing.
 */
class LowLevelCameraAPI {
private:
    static constexpr const char* TAG = "LowLevelAPI";

public:
    /**
     * @brief Get direct sensor access
     *
     * Returns sensor_t* for complete hardware control.
     * Available after esp_camera_init().
     */
    static sensor_t* getSensor() {
        sensor_t* sensor = esp_camera_sensor_get();

        if (!sensor) {
            Logger::getInstance().error(TAG, "Sensor not initialized");
            return nullptr;
        }

        Logger::getInstance().debug(TAG, "Sensor PID: 0x" + String(sensor->id.PID, HEX));
        return sensor;
    }

    /**
     * @brief Check which sensor is connected
     */
    static String getSensorModel() {
        sensor_t* sensor = getSensor();
        if (!sensor) return "UNKNOWN";

        switch (sensor->id.PID) {
            case OV2640_PID: return "OV2640";
            case OV3660_PID: return "OV3660";
            case OV5640_PID: return "OV5640";
            case OV7670_PID: return "OV7670";
            case OV7725_PID: return "OV7725";
            default: return "UNKNOWN (0x" + String(sensor->id.PID, HEX) + ")";
        }
    }

    // ========================================================================
    // LEVEL 3: Direct Sensor Register Access (Lowest Level)
    // ========================================================================

    /**
     * @brief Set individual sensor register
     *
     * WARNING: Can brick sensor if wrong values used!
     *
     * @param reg Register address (e.g., 0xFF for bank select)
     * @param value Register value
     */
    static bool setRegister(uint8_t reg, uint8_t value) {
        sensor_t* sensor = getSensor();
        if (!sensor) return false;

        int result = sensor->set_reg(sensor, reg, 0xFF, value);

        if (result != 0) {
            Logger::getInstance().error(TAG, "Failed to set register 0x" + String(reg, HEX));
            return false;
        }

        Logger::getInstance().debug(TAG, "Set reg 0x" + String(reg, HEX) + " = 0x" + String(value, HEX));
        return true;
    }

    /**
     * @brief Read individual sensor register
     */
    static uint8_t getRegister(uint8_t reg) {
        sensor_t* sensor = getSensor();
        if (!sensor) return 0;

        uint8_t value = sensor->get_reg(sensor, reg, 0xFF);
        Logger::getInstance().debug(TAG, "Read reg 0x" + String(reg, HEX) + " = 0x" + String(value, HEX));

        return value;
    }

    /**
     * @brief Dump all sensor registers (for debugging)
     */
    static void dumpRegisters() {
        sensor_t* sensor = getSensor();
        if (!sensor) return;

        Logger::getInstance().info(TAG, "=== Sensor Register Dump ===");

        // Common register banks (varies by sensor)
        for (uint16_t reg = 0x00; reg <= 0xFF; reg++) {
            uint8_t value = sensor->get_reg(sensor, reg, 0xFF);
            Serial.printf("0x%02X: 0x%02X\n", reg, value);
        }
    }

    // ========================================================================
    // LEVEL 3: Advanced Sensor Controls (Using sensor_t API)
    // ========================================================================

    /**
     * @brief Advanced exposure control
     *
     * Lower level than CameraSettings::setExposure()
     *
     * @param value Exposure value (sensor-specific)
     */
    static bool setExposureRaw(uint16_t value) {
        sensor_t* sensor = getSensor();
        if (!sensor || !sensor->set_exposure_ctrl) return false;

        // Disable auto exposure first
        sensor->set_exposure_ctrl(sensor, 0);

        // Set raw exposure value
        if (sensor->set_aec_value) {
            sensor->set_aec_value(sensor, value);
            Logger::getInstance().info(TAG, "Exposure set to " + String(value));
            return true;
        }

        return false;
    }

    /**
     * @brief Advanced gain control
     *
     * @param value Gain value (0-30 typical)
     */
    static bool setGainRaw(uint8_t value) {
        sensor_t* sensor = getSensor();
        if (!sensor || !sensor->set_gain_ctrl) return false;

        // Disable auto gain
        sensor->set_gain_ctrl(sensor, 0);

        // Set raw gain
        if (sensor->set_agc_gain) {
            sensor->set_agc_gain(sensor, value);
            Logger::getInstance().info(TAG, "Gain set to " + String(value));
            return true;
        }

        return false;
    }

    /**
     * @brief Set binning mode (pixel binning for lower resolution)
     *
     * OV2640: Combine adjacent pixels for better low-light performance
     */
    static bool setBinning(bool enable) {
        sensor_t* sensor = getSensor();
        if (!sensor || !sensor->set_binning) return false;

        sensor->set_binning(sensor, enable);
        Logger::getInstance().info(TAG, "Binning: " + String(enable ? "ON" : "OFF"));
        return true;
    }

    /**
     * @brief Set test pattern
     *
     * Useful for debugging image pipeline
     * 0 = Disabled
     * 1 = Color bars
     * 2 = Gradual change at vertical
     * 3 = Gradual change at horizontal
     */
    static bool setTestPattern(uint8_t pattern) {
        sensor_t* sensor = getSensor();
        if (!sensor || !sensor->set_colorbar) return false;

        sensor->set_colorbar(sensor, pattern);
        Logger::getInstance().info(TAG, "Test pattern: " + String(pattern));
        return true;
    }

    /**
     * @brief Advanced white balance (Kelvin temperature)
     *
     * @param temp Color temperature in Kelvin (2000-8000)
     */
    static bool setWhiteBalanceKelvin(uint16_t temp) {
        sensor_t* sensor = getSensor();
        if (!sensor) return false;

        // Disable auto white balance
        if (sensor->set_whitebal) {
            sensor->set_whitebal(sensor, 0);
        }

        // Map Kelvin to RGB gains
        // Simplified conversion (proper conversion needs lookup table)
        if (temp < 3000) {
            // Warm (red-ish)
            if (sensor->set_wb_mode) {
                sensor->set_wb_mode(sensor, 3); // Cloudy/warm preset
            }
        } else if (temp > 6000) {
            // Cool (blue-ish)
            if (sensor->set_wb_mode) {
                sensor->set_wb_mode(sensor, 2); // Office/cool preset
            }
        } else {
            // Neutral
            if (sensor->set_wb_mode) {
                sensor->set_wb_mode(sensor, 0); // Auto
            }
        }

        Logger::getInstance().info(TAG, "White balance: " + String(temp) + "K");
        return true;
    }

    // ========================================================================
    // OV2640-Specific Features
    // ========================================================================

    /**
     * @brief Enable OV2640 night mode
     *
     * Reduces frame rate, increases exposure for low light
     */
    static bool setNightMode(bool enable) {
        sensor_t* sensor = getSensor();
        if (!sensor || sensor->id.PID != OV2640_PID) {
            Logger::getInstance().warn(TAG, "Night mode only for OV2640");
            return false;
        }

        // OV2640 night mode registers
        // Bank 0x01
        setRegister(0xFF, 0x01);  // Select DSP bank

        if (enable) {
            setRegister(0x2D, 0x00);  // Reduce frame rate
            setRegister(0x2E, 0x00);
            Logger::getInstance().info(TAG, "Night mode: ON");
        } else {
            setRegister(0x2D, 0x60);  // Normal frame rate
            setRegister(0x2E, 0x60);
            Logger::getInstance().info(TAG, "Night mode: OFF");
        }

        return true;
    }

    /**
     * @brief Enable OV2640 JPEG quality override
     *
     * Bypass standard quality settings for custom compression
     */
    static bool setJPEGQualityRaw(uint8_t qScale) {
        sensor_t* sensor = getSensor();
        if (!sensor || sensor->id.PID != OV2640_PID) return false;

        // OV2640 JPEG Q-scale register
        setRegister(0xFF, 0x00);  // Bank 0
        setRegister(0x44, qScale);  // JPEG Q-scale

        Logger::getInstance().info(TAG, "JPEG Q-scale: " + String(qScale));
        return true;
    }

    // ========================================================================
    // LEVEL 2: Mid-Level Configuration Access
    // ========================================================================

    /**
     * @brief Get current camera configuration
     *
     * Returns the camera_config_t struct for direct manipulation
     */
    static camera_config_t getHardwareConfig() {
        camera_config_t config;
        memset(&config, 0, sizeof(camera_config_t));

        // This would need to be stored during initialization
        // For now, return default AI-Thinker config
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = 5;
        config.pin_d1 = 18;
        config.pin_d2 = 19;
        config.pin_d3 = 21;
        config.pin_d4 = 36;
        config.pin_d5 = 39;
        config.pin_d6 = 34;
        config.pin_d7 = 35;
        config.pin_xclk = 0;
        config.pin_pclk = 22;
        config.pin_vsync = 25;
        config.pin_href = 23;
        config.pin_sccb_sda = 26;
        config.pin_sccb_scl = 27;
        config.pin_pwdn = 32;
        config.pin_reset = -1;
        config.xclk_freq_hz = 20000000;
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;

        return config;
    }

    /**
     * @brief Reinitialize camera with custom config
     *
     * WARNING: Will stop current camera operation!
     */
    static bool reinitialize(const camera_config_t& config) {
        // Deinitialize current camera
        esp_camera_deinit();
        delay(100);

        // Reinitialize with new config
        esp_err_t err = esp_camera_init(&config);

        if (err != ESP_OK) {
            Logger::getInstance().error(TAG, "Camera reinit failed: " + String(err));
            return false;
        }

        Logger::getInstance().info(TAG, "Camera reinitialized");
        return true;
    }

    // ========================================================================
    // Diagnostic Functions
    // ========================================================================

    /**
     * @brief Get detailed sensor status
     */
    static String getSensorStatus() {
        sensor_t* sensor = getSensor();
        if (!sensor) return "Sensor not available";

        String status = "=== Sensor Status ===\n";
        status += "Model: " + getSensorModel() + "\n";
        status += "PID: 0x" + String(sensor->id.PID, HEX) + "\n";
        status += "VER: 0x" + String(sensor->id.VER, HEX) + "\n";
        status += "MIDL: 0x" + String(sensor->id.MIDL, HEX) + "\n";
        status += "MIDH: 0x" + String(sensor->id.MIDH, HEX) + "\n";

        // Current settings (if available)
        status += "\n=== Current Settings ===\n";
        status += "Binning: " + String(sensor->status.binning ? "ON" : "OFF") + "\n";
        status += "H-Mirror: " + String(sensor->status.hmirror ? "ON" : "OFF") + "\n";
        status += "V-Flip: " + String(sensor->status.vflip ? "ON" : "OFF") + "\n";
        status += "Colorbar: " + String(sensor->status.colorbar ? "ON" : "OFF") + "\n";

        return status;
    }

    /**
     * @brief Performance profiling for camera operations
     */
    static void profileCapture(uint32_t frameCount = 100) {
        Logger::getInstance().info(TAG, "Profiling " + String(frameCount) + " frames...");

        uint32_t start = millis();
        uint32_t totalSize = 0;

        for (uint32_t i = 0; i < frameCount; i++) {
            camera_fb_t* fb = esp_camera_fb_get();
            if (fb) {
                totalSize += fb->len;
                esp_camera_fb_return(fb);
            }
        }

        uint32_t duration = millis() - start;
        float fps = (frameCount * 1000.0) / duration;
        float avgSize = totalSize / (float)frameCount;

        Logger::getInstance().info(TAG, "=== Capture Profile ===");
        Logger::getInstance().info(TAG, "Frames: " + String(frameCount));
        Logger::getInstance().info(TAG, "Duration: " + String(duration) + " ms");
        Logger::getInstance().info(TAG, "FPS: " + String(fps, 2));
        Logger::getInstance().info(TAG, "Avg Size: " + String(avgSize / 1024.0, 2) + " KB");
        Logger::getInstance().info(TAG, "Total: " + String(totalSize / 1024.0, 2) + " KB");
    }
};

/**
 * @brief Usage examples for low-level API
 */
namespace LowLevelExamples {
    /**
     * @brief Example 1: Custom night mode for low light
     */
    void customNightMode() {
        sensor_t* sensor = LowLevelCameraAPI::getSensor();

        // Disable auto exposure and gain
        sensor->set_exposure_ctrl(sensor, 0);
        sensor->set_gain_ctrl(sensor, 0);

        // Set high exposure and gain for low light
        LowLevelCameraAPI::setExposureRaw(1200);  // High exposure
        LowLevelCameraAPI::setGainRaw(30);        // Max gain

        // Reduce noise with binning
        LowLevelCameraAPI::setBinning(true);

        // Enable OV2640 night mode (if available)
        LowLevelCameraAPI::setNightMode(true);

        Logger::getInstance().info("Example", "Custom night mode enabled");
    }

    /**
     * @brief Example 2: High-speed capture mode
     */
    void highSpeedMode() {
        sensor_t* sensor = LowLevelCameraAPI::getSensor();

        // Disable unnecessary processing
        sensor->set_awb_gain(sensor, 0);      // Disable AWB
        sensor->set_wb_mode(sensor, 0);       // Auto WB
        sensor->set_lenc(sensor, 0);          // Disable lens correction

        // Low exposure for fast capture
        LowLevelCameraAPI::setExposureRaw(50);

        // Disable binning for full speed
        LowLevelCameraAPI::setBinning(false);

        Logger::getInstance().info("Example", "High-speed mode enabled");
    }

    /**
     * @brief Example 3: Test pattern for debugging
     */
    void enableTestPattern() {
        LowLevelCameraAPI::setTestPattern(1);  // Color bars
        Logger::getInstance().info("Example", "Test pattern enabled - you should see color bars");
    }

    /**
     * @brief Example 4: Register-level tweaking
     */
    void customRegisterConfig() {
        // Example: OV2640 custom saturation via registers
        // (Normally done via sensor->set_saturation)

        LowLevelCameraAPI::setRegister(0xFF, 0x00);  // Bank DSP
        LowLevelCameraAPI::setRegister(0x7C, 0x00);  // SDE control
        LowLevelCameraAPI::setRegister(0x7D, 0x02);  // Enable saturation
        LowLevelCameraAPI::setRegister(0x7C, 0x03);  // Saturation U
        LowLevelCameraAPI::setRegister(0x7D, 0x80);  // High saturation
        LowLevelCameraAPI::setRegister(0x7D, 0x80);  // High saturation V

        Logger::getInstance().info("Example", "Custom saturation via registers");
    }

    /**
     * @brief Example 5: Performance profiling
     */
    void benchmarkCapture() {
        // Profile current settings
        LowLevelCameraAPI::profileCapture(100);

        // Try lower quality
        sensor_t* sensor = LowLevelCameraAPI::getSensor();
        sensor->set_quality(sensor, 20);

        Logger::getInstance().info("Example", "Testing with quality 20...");
        LowLevelCameraAPI::profileCapture(100);

        // Restore quality
        sensor->set_quality(sensor, 12);
    }
}

#endif // LOW_LEVEL_CAMERA_API_H
