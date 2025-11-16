/**
 * @file ImagePreprocessor.h
 * @brief Custom image preprocessing before compression
 *
 * WARNING: This is SLOW! Use sensor preprocessing when possible.
 * Only use this for custom filters not available in hardware.
 *
 * Performance impact:
 * - RAW frame: ~900 KB (VGA)
 * - Processing: 50-200ms depending on algorithm
 * - Software JPEG: 500-800ms
 * Total: ~1 FPS (vs 30 FPS with hardware)
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef IMAGE_PREPROCESSOR_H
#define IMAGE_PREPROCESSOR_H

#include <Arduino.h>
#include "esp_camera.h"
#include "../../core/Logger.h"

/**
 * @brief Image preprocessing utilities
 */
class ImagePreprocessor {
private:
    static constexpr const char* TAG = "ImgPreproc";

public:
    /**
     * @brief Apply grayscale conversion
     *
     * Converts RGB to grayscale using luminosity method.
     * Fast algorithm: Y = 0.299*R + 0.587*G + 0.114*B
     */
    static void toGrayscale(uint8_t* buffer, size_t width, size_t height) {
        size_t pixels = width * height;

        for (size_t i = 0; i < pixels; i++) {
            size_t idx = i * 3; // RGB888 format
            uint8_t r = buffer[idx];
            uint8_t g = buffer[idx + 1];
            uint8_t b = buffer[idx + 2];

            // Luminosity method (weighted)
            uint8_t gray = (r * 77 + g * 150 + b * 29) >> 8; // Fast division by 256

            buffer[idx] = gray;
            buffer[idx + 1] = gray;
            buffer[idx + 2] = gray;
        }
    }

    /**
     * @brief Apply brightness adjustment
     *
     * @param delta Brightness change (-255 to +255)
     */
    static void adjustBrightness(uint8_t* buffer, size_t width, size_t height, int16_t delta) {
        size_t total = width * height * 3; // RGB888

        for (size_t i = 0; i < total; i++) {
            int16_t value = buffer[i] + delta;
            buffer[i] = constrain(value, 0, 255);
        }
    }

    /**
     * @brief Apply contrast adjustment
     *
     * @param factor Contrast factor (0.5 = less, 1.0 = normal, 2.0 = more)
     */
    static void adjustContrast(uint8_t* buffer, size_t width, size_t height, float factor) {
        size_t total = width * height * 3; // RGB888

        for (size_t i = 0; i < total; i++) {
            float value = (buffer[i] - 128) * factor + 128;
            buffer[i] = constrain((int)value, 0, 255);
        }
    }

    /**
     * @brief Apply simple edge detection (Sobel-like)
     *
     * NOTE: Very slow! ~200ms for VGA
     */
    static void edgeDetect(uint8_t* buffer, size_t width, size_t height) {
        // Allocate temporary buffer
        uint8_t* temp = (uint8_t*)ps_malloc(width * height);
        if (!temp) {
            Logger::getInstance().error(TAG, "Failed to allocate edge detection buffer");
            return;
        }

        // Convert to grayscale first
        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                size_t idx = (y * width + x) * 3;
                temp[y * width + x] = (buffer[idx] + buffer[idx+1] + buffer[idx+2]) / 3;
            }
        }

        // Sobel operator
        for (size_t y = 1; y < height - 1; y++) {
            for (size_t x = 1; x < width - 1; x++) {
                // Gx gradient
                int gx = -temp[(y-1)*width + x-1] + temp[(y-1)*width + x+1]
                        -2*temp[y*width + x-1]    + 2*temp[y*width + x+1]
                        -temp[(y+1)*width + x-1]  + temp[(y+1)*width + x+1];

                // Gy gradient
                int gy = -temp[(y-1)*width + x-1] - 2*temp[(y-1)*width + x] - temp[(y-1)*width + x+1]
                        +temp[(y+1)*width + x-1]  + 2*temp[(y+1)*width + x] + temp[(y+1)*width + x+1];

                // Magnitude
                int magnitude = sqrt(gx*gx + gy*gy);
                uint8_t edge = constrain(magnitude, 0, 255);

                // Write back as grayscale
                size_t idx = (y * width + x) * 3;
                buffer[idx] = buffer[idx+1] = buffer[idx+2] = edge;
            }
        }

        free(temp);
    }

    /**
     * @brief Add text overlay
     *
     * Simple text rendering (requires font data)
     * For production, use ESP32 graphics library
     */
    static void addTextOverlay(uint8_t* buffer, size_t width, size_t height,
                               const String& text, size_t x, size_t y) {
        // TODO: Implement with graphics library
        // Libraries: TFT_eSPI, Adafruit_GFX, etc.
        Logger::getInstance().warn(TAG, "Text overlay not yet implemented");
    }

    /**
     * @brief Add timestamp overlay
     */
    static void addTimestamp(uint8_t* buffer, size_t width, size_t height) {
        // Get current time
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

        // Add as overlay (requires graphics library)
        addTextOverlay(buffer, width, height, timestamp, 10, height - 20);
    }

    /**
     * @brief Apply motion blur reduction
     *
     * Uses simple sharpening kernel
     */
    static void sharpen(uint8_t* buffer, size_t width, size_t height) {
        uint8_t* temp = (uint8_t*)ps_malloc(width * height * 3);
        if (!temp) return;

        memcpy(temp, buffer, width * height * 3);

        // Sharpening kernel
        int kernel[3][3] = {
            { 0, -1,  0},
            {-1,  5, -1},
            { 0, -1,  0}
        };

        for (size_t y = 1; y < height - 1; y++) {
            for (size_t x = 1; x < width - 1; x++) {
                for (int c = 0; c < 3; c++) { // RGB channels
                    int sum = 0;
                    for (int ky = -1; ky <= 1; ky++) {
                        for (int kx = -1; kx <= 1; kx++) {
                            size_t idx = ((y+ky) * width + (x+kx)) * 3 + c;
                            sum += temp[idx] * kernel[ky+1][kx+1];
                        }
                    }
                    size_t idx = (y * width + x) * 3 + c;
                    buffer[idx] = constrain(sum, 0, 255);
                }
            }
        }

        free(temp);
    }
};

/**
 * @brief Example usage with RAW capture
 */
class RAWPreprocessingExample {
public:
    static void captureAndProcess() {
        Logger::getInstance().warn("RAW", "WARNING: This is VERY SLOW (~1 FPS)");

        // 1. Configure camera for RAW (RGB888) format
        CameraSettings settings;
        settings.setPixelFormat(PixelFormat::RGB888()); // RAW RGB
        settings.setResolution(Resolution::QVGA());     // Start small!

        // Initialize camera
        // ... (use your existing camera repository)

        // 2. Capture RAW frame
        auto result = captureRawFrame();
        if (result.isError()) return;

        Frame rawFrame = result.getValue();

        // 3. Apply preprocessing
        uint32_t start = millis();

        // Example: Grayscale + Edge detection
        ImagePreprocessor::toGrayscale(rawFrame.buffer,
                                      rawFrame.width,
                                      rawFrame.height);

        ImagePreprocessor::edgeDetect(rawFrame.buffer,
                                     rawFrame.width,
                                     rawFrame.height);

        uint32_t preprocessTime = millis() - start;
        Logger::getInstance().info("RAW", "Preprocessing: " + String(preprocessTime) + "ms");

        // 4. Now you need SOFTWARE JPEG compression
        // This requires additional library (e.g., esp_jpg_encode)
        // And it's SLOW (500-800ms for VGA)

        // 5. Stream the result
        // ... (use your streaming repository)
    }

private:
    static Result<Frame> captureRawFrame() {
        // This would need to be implemented in camera repository
        // Currently only JPEG is supported
        return Result<Frame>::error("RAW capture not implemented");
    }
};

#endif // IMAGE_PREPROCESSOR_H
