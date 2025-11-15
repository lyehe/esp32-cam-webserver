/**
 * @file PixelFormat.h
 * @brief Value object representing camera pixel format
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef PIXEL_FORMAT_H
#define PIXEL_FORMAT_H

#include <Arduino.h>
#include "esp_camera.h"

/**
 * @brief Pixel format value object
 *
 * Immutable value object representing camera pixel/image format.
 */
class PixelFormat {
private:
    pixformat_t _format;

public:
    /**
     * @brief Construct from pixformat enum
     */
    explicit PixelFormat(pixformat_t format = PIXFORMAT_JPEG)
        : _format(format) {}

    // Accessors
    pixformat_t getFormat() const { return _format; }

    /**
     * @brief Check if format is JPEG
     */
    bool isJPEG() const { return _format == PIXFORMAT_JPEG; }

    /**
     * @brief Check if format is RGB
     */
    bool isRGB() const {
        return _format == PIXFORMAT_RGB565 ||
               _format == PIXFORMAT_RGB888;
    }

    /**
     * @brief Check if format is YUV
     */
    bool isYUV() const {
        return _format == PIXFORMAT_YUV422;
    }

    /**
     * @brief Check if format is grayscale
     */
    bool isGrayscale() const {
        return _format == PIXFORMAT_GRAYSCALE;
    }

    /**
     * @brief Get bytes per pixel
     */
    uint8_t getBytesPerPixel() const {
        switch (_format) {
            case PIXFORMAT_GRAYSCALE: return 1;
            case PIXFORMAT_RGB565:
            case PIXFORMAT_YUV422:    return 2;
            case PIXFORMAT_RGB888:    return 3;
            case PIXFORMAT_JPEG:      return 0; // Variable
            default:                  return 0;
        }
    }

    /**
     * @brief Get human-readable name
     */
    String getName() const {
        switch (_format) {
            case PIXFORMAT_RGB565:    return "RGB565";
            case PIXFORMAT_YUV422:    return "YUV422";
            case PIXFORMAT_GRAYSCALE: return "Grayscale";
            case PIXFORMAT_JPEG:      return "JPEG";
            case PIXFORMAT_RGB888:    return "RGB888";
            case PIXFORMAT_RAW:       return "RAW";
            case PIXFORMAT_RGB444:    return "RGB444";
            case PIXFORMAT_RGB555:    return "RGB555";
            default:                  return "Unknown";
        }
    }

    /**
     * @brief Check if format is supported for streaming
     */
    bool isSupportedForStreaming() const {
        // JPEG is the most efficient for streaming
        // Some formats may not work on all cores
        return _format == PIXFORMAT_JPEG;
    }

    /**
     * @brief Equality comparison
     */
    bool operator==(const PixelFormat& other) const {
        return _format == other._format;
    }

    bool operator!=(const PixelFormat& other) const {
        return !(*this == other);
    }

    /**
     * @brief Convert to string for logging
     */
    String toString() const {
        return getName();
    }

    // Static factory methods for common formats
    static PixelFormat JPEG()      { return PixelFormat(PIXFORMAT_JPEG); }
    static PixelFormat RGB565()    { return PixelFormat(PIXFORMAT_RGB565); }
    static PixelFormat RGB888()    { return PixelFormat(PIXFORMAT_RGB888); }
    static PixelFormat YUV422()    { return PixelFormat(PIXFORMAT_YUV422); }
    static PixelFormat Grayscale() { return PixelFormat(PIXFORMAT_GRAYSCALE); }
};

#endif // PIXEL_FORMAT_H
