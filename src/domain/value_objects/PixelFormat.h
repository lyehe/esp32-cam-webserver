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

/**
 * @brief Pixel format enumeration (domain-level, framework-agnostic)
 *
 * These values correspond to common image/pixel formats but are not
 * tied to any specific hardware implementation.
 */
enum class Format {
    RGB565 = 0,     // 2 bytes per pixel
    YUV422,         // YUYV format
    GRAYSCALE,      // Y (grayscale) 1 byte per pixel
    JPEG,           // JPEG compressed
    RGB888,         // 3 bytes per pixel
    RAW,            // RAW sensor data
    RGB444,         // 2 bytes per pixel
    RGB555          // 2 bytes per pixel
};

/**
 * @brief Pixel format value object
 *
 * Immutable value object representing camera pixel/image format.
 * Framework-agnostic - does not depend on any hardware libraries.
 */
class PixelFormat {
private:
    Format _format;

public:
    /**
     * @brief Construct from format enum
     */
    explicit PixelFormat(Format format = Format::JPEG)
        : _format(format) {}

    // Accessors
    Format getFormat() const { return _format; }

    /**
     * @brief Check if format is JPEG
     */
    bool isJPEG() const { return _format == Format::JPEG; }

    /**
     * @brief Check if format is RGB
     */
    bool isRGB() const {
        return _format == Format::RGB565 ||
               _format == Format::RGB888 ||
               _format == Format::RGB444 ||
               _format == Format::RGB555;
    }

    /**
     * @brief Check if format is YUV
     */
    bool isYUV() const {
        return _format == Format::YUV422;
    }

    /**
     * @brief Check if format is grayscale
     */
    bool isGrayscale() const {
        return _format == Format::GRAYSCALE;
    }

    /**
     * @brief Get bytes per pixel
     */
    uint8_t getBytesPerPixel() const {
        switch (_format) {
            case Format::GRAYSCALE: return 1;
            case Format::RGB565:
            case Format::RGB444:
            case Format::RGB555:
            case Format::YUV422:    return 2;
            case Format::RGB888:    return 3;
            case Format::JPEG:      return 0; // Variable
            case Format::RAW:       return 0; // Variable
            default:                return 0;
        }
    }

    /**
     * @brief Get human-readable name
     */
    String getName() const {
        switch (_format) {
            case Format::RGB565:    return "RGB565";
            case Format::YUV422:    return "YUV422";
            case Format::GRAYSCALE: return "Grayscale";
            case Format::JPEG:      return "JPEG";
            case Format::RGB888:    return "RGB888";
            case Format::RAW:       return "RAW";
            case Format::RGB444:    return "RGB444";
            case Format::RGB555:    return "RGB555";
            default:                return "Unknown";
        }
    }

    /**
     * @brief Check if format is supported for streaming
     */
    bool isSupportedForStreaming() const {
        // JPEG is the most efficient for streaming
        // Some formats may not work on all cores
        return _format == Format::JPEG;
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
    static PixelFormat JPEG()      { return PixelFormat(Format::JPEG); }
    static PixelFormat RGB565()    { return PixelFormat(Format::RGB565); }
    static PixelFormat RGB888()    { return PixelFormat(Format::RGB888); }
    static PixelFormat YUV422()    { return PixelFormat(Format::YUV422); }
    static PixelFormat Grayscale() { return PixelFormat(Format::GRAYSCALE); }
};

#endif // PIXEL_FORMAT_H
