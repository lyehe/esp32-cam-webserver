/**
 * @file Resolution.h
 * @brief Value object representing camera resolution
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef RESOLUTION_H
#define RESOLUTION_H

#include <Arduino.h>

/**
 * @brief Frame size enumeration (domain-level, framework-agnostic)
 *
 * These values correspond to common camera resolutions but are not
 * tied to any specific hardware implementation.
 */
enum class FrameSize {
    SIZE_96X96 = 0,     // 96x96
    SIZE_QQVGA,         // 160x120
    SIZE_QCIF,          // 176x144
    SIZE_HQVGA,         // 240x176
    SIZE_240X240,       // 240x240
    SIZE_QVGA,          // 320x240
    SIZE_CIF,           // 400x296
    SIZE_HVGA,          // 480x320
    SIZE_VGA,           // 640x480
    SIZE_SVGA,          // 800x600
    SIZE_XGA,           // 1024x768
    SIZE_HD,            // 1280x720
    SIZE_SXGA,          // 1280x1024
    SIZE_UXGA           // 1600x1200
};

/**
 * @brief Resolution value object
 *
 * Immutable value object representing camera resolution with validation.
 * Framework-agnostic - does not depend on any hardware libraries.
 */
class Resolution {
private:
    FrameSize _frameSize;
    uint16_t _width;
    uint16_t _height;

    /**
     * @brief Get dimensions for frame size
     */
    void setDimensions() {
        switch (_frameSize) {
            case FrameSize::SIZE_96X96:    _width = 96;   _height = 96;   break;
            case FrameSize::SIZE_QQVGA:    _width = 160;  _height = 120;  break;
            case FrameSize::SIZE_QCIF:     _width = 176;  _height = 144;  break;
            case FrameSize::SIZE_HQVGA:    _width = 240;  _height = 176;  break;
            case FrameSize::SIZE_240X240:  _width = 240;  _height = 240;  break;
            case FrameSize::SIZE_QVGA:     _width = 320;  _height = 240;  break;
            case FrameSize::SIZE_CIF:      _width = 400;  _height = 296;  break;
            case FrameSize::SIZE_HVGA:     _width = 480;  _height = 320;  break;
            case FrameSize::SIZE_VGA:      _width = 640;  _height = 480;  break;
            case FrameSize::SIZE_SVGA:     _width = 800;  _height = 600;  break;
            case FrameSize::SIZE_XGA:      _width = 1024; _height = 768;  break;
            case FrameSize::SIZE_HD:       _width = 1280; _height = 720;  break;
            case FrameSize::SIZE_SXGA:     _width = 1280; _height = 1024; break;
            case FrameSize::SIZE_UXGA:     _width = 1600; _height = 1200; break;
            default:                       _width = 800;  _height = 600;  break;
        }
    }

public:
    /**
     * @brief Construct from framesize enum
     */
    explicit Resolution(FrameSize size = FrameSize::SIZE_SVGA)
        : _frameSize(size) {
        setDimensions();
    }

    /**
     * @brief Construct from dimensions
     */
    Resolution(uint16_t width, uint16_t height)
        : _width(width), _height(height) {
        // Find closest matching framesize
        if (width <= 96 && height <= 96)         _frameSize = FrameSize::SIZE_96X96;
        else if (width <= 160 && height <= 120)  _frameSize = FrameSize::SIZE_QQVGA;
        else if (width <= 176 && height <= 144)  _frameSize = FrameSize::SIZE_QCIF;
        else if (width <= 240 && height <= 176)  _frameSize = FrameSize::SIZE_HQVGA;
        else if (width <= 240 && height <= 240)  _frameSize = FrameSize::SIZE_240X240;
        else if (width <= 320 && height <= 240)  _frameSize = FrameSize::SIZE_QVGA;
        else if (width <= 400 && height <= 296)  _frameSize = FrameSize::SIZE_CIF;
        else if (width <= 480 && height <= 320)  _frameSize = FrameSize::SIZE_HVGA;
        else if (width <= 640 && height <= 480)  _frameSize = FrameSize::SIZE_VGA;
        else if (width <= 800 && height <= 600)  _frameSize = FrameSize::SIZE_SVGA;
        else if (width <= 1024 && height <= 768) _frameSize = FrameSize::SIZE_XGA;
        else if (width <= 1280 && height <= 720) _frameSize = FrameSize::SIZE_HD;
        else if (width <= 1280 && height <= 1024) _frameSize = FrameSize::SIZE_SXGA;
        else _frameSize = FrameSize::SIZE_UXGA;
    }

    // Accessors
    FrameSize getFrameSize() const { return _frameSize; }
    uint16_t getWidth() const { return _width; }
    uint16_t getHeight() const { return _height; }
    uint32_t getPixelCount() const { return (uint32_t)_width * _height; }

    /**
     * @brief Check if resolution requires PSRAM
     *
     * High resolutions (>= XGA) typically require PSRAM for frame buffers
     */
    bool requiresPSRAM() const {
        return _frameSize >= FrameSize::SIZE_XGA;
    }

    /**
     * @brief Get human-readable name
     */
    String getName() const {
        switch (_frameSize) {
            case FrameSize::SIZE_96X96:    return "96x96";
            case FrameSize::SIZE_QQVGA:    return "QQVGA (160x120)";
            case FrameSize::SIZE_QCIF:     return "QCIF (176x144)";
            case FrameSize::SIZE_HQVGA:    return "HQVGA (240x176)";
            case FrameSize::SIZE_240X240:  return "240x240";
            case FrameSize::SIZE_QVGA:     return "QVGA (320x240)";
            case FrameSize::SIZE_CIF:      return "CIF (400x296)";
            case FrameSize::SIZE_HVGA:     return "HVGA (480x320)";
            case FrameSize::SIZE_VGA:      return "VGA (640x480)";
            case FrameSize::SIZE_SVGA:     return "SVGA (800x600)";
            case FrameSize::SIZE_XGA:      return "XGA (1024x768)";
            case FrameSize::SIZE_HD:       return "HD (1280x720)";
            case FrameSize::SIZE_SXGA:     return "SXGA (1280x1024)";
            case FrameSize::SIZE_UXGA:     return "UXGA (1600x1200)";
            default:                       return "Unknown";
        }
    }

    /**
     * @brief Equality comparison
     */
    bool operator==(const Resolution& other) const {
        return _frameSize == other._frameSize;
    }

    bool operator!=(const Resolution& other) const {
        return !(*this == other);
    }

    /**
     * @brief Convert to string for logging
     */
    String toString() const {
        char buffer[64];
        sprintf(buffer, "%s (%dx%d)", getName().c_str(), _width, _height);
        return String(buffer);
    }

    // Static factory methods for common resolutions
    static Resolution QQVGA() { return Resolution(FrameSize::SIZE_QQVGA); }
    static Resolution QVGA()  { return Resolution(FrameSize::SIZE_QVGA); }
    static Resolution VGA()   { return Resolution(FrameSize::SIZE_VGA); }
    static Resolution SVGA()  { return Resolution(FrameSize::SIZE_SVGA); }
    static Resolution XGA()   { return Resolution(FrameSize::SIZE_XGA); }
    static Resolution HD()    { return Resolution(FrameSize::SIZE_HD); }
    static Resolution SXGA()  { return Resolution(FrameSize::SIZE_SXGA); }
    static Resolution UXGA()  { return Resolution(FrameSize::SIZE_UXGA); }
};

#endif // RESOLUTION_H
