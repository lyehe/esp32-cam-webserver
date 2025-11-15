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
#include "esp_camera.h"

/**
 * @brief Resolution value object
 *
 * Immutable value object representing camera resolution with validation.
 */
class Resolution {
private:
    framesize_t _frameSize;
    uint16_t _width;
    uint16_t _height;

    /**
     * @brief Get dimensions for frame size
     */
    void setDimensions() {
        switch (_frameSize) {
            case FRAMESIZE_96X96:    _width = 96;   _height = 96;   break;
            case FRAMESIZE_QQVGA:    _width = 160;  _height = 120;  break;
            case FRAMESIZE_QCIF:     _width = 176;  _height = 144;  break;
            case FRAMESIZE_HQVGA:    _width = 240;  _height = 176;  break;
            case FRAMESIZE_240X240:  _width = 240;  _height = 240;  break;
            case FRAMESIZE_QVGA:     _width = 320;  _height = 240;  break;
            case FRAMESIZE_CIF:      _width = 400;  _height = 296;  break;
            case FRAMESIZE_HVGA:     _width = 480;  _height = 320;  break;
            case FRAMESIZE_VGA:      _width = 640;  _height = 480;  break;
            case FRAMESIZE_SVGA:     _width = 800;  _height = 600;  break;
            case FRAMESIZE_XGA:      _width = 1024; _height = 768;  break;
            case FRAMESIZE_HD:       _width = 1280; _height = 720;  break;
            case FRAMESIZE_SXGA:     _width = 1280; _height = 1024; break;
            case FRAMESIZE_UXGA:     _width = 1600; _height = 1200; break;
            default:                 _width = 800;  _height = 600;  break;
        }
    }

public:
    /**
     * @brief Construct from framesize enum
     */
    explicit Resolution(framesize_t size = FRAMESIZE_SVGA)
        : _frameSize(size) {
        setDimensions();
    }

    /**
     * @brief Construct from dimensions
     */
    Resolution(uint16_t width, uint16_t height)
        : _width(width), _height(height) {
        // Find closest matching framesize
        if (width <= 96 && height <= 96)         _frameSize = FRAMESIZE_96X96;
        else if (width <= 160 && height <= 120)  _frameSize = FRAMESIZE_QQVGA;
        else if (width <= 176 && height <= 144)  _frameSize = FRAMESIZE_QCIF;
        else if (width <= 240 && height <= 176)  _frameSize = FRAMESIZE_HQVGA;
        else if (width <= 240 && height <= 240)  _frameSize = FRAMESIZE_240X240;
        else if (width <= 320 && height <= 240)  _frameSize = FRAMESIZE_QVGA;
        else if (width <= 400 && height <= 296)  _frameSize = FRAMESIZE_CIF;
        else if (width <= 480 && height <= 320)  _frameSize = FRAMESIZE_HVGA;
        else if (width <= 640 && height <= 480)  _frameSize = FRAMESIZE_VGA;
        else if (width <= 800 && height <= 600)  _frameSize = FRAMESIZE_SVGA;
        else if (width <= 1024 && height <= 768) _frameSize = FRAMESIZE_XGA;
        else if (width <= 1280 && height <= 720) _frameSize = FRAMESIZE_HD;
        else if (width <= 1280 && height <= 1024) _frameSize = FRAMESIZE_SXGA;
        else _frameSize = FRAMESIZE_UXGA;
    }

    // Accessors
    framesize_t getFrameSize() const { return _frameSize; }
    uint16_t getWidth() const { return _width; }
    uint16_t getHeight() const { return _height; }
    uint32_t getPixelCount() const { return (uint32_t)_width * _height; }

    /**
     * @brief Check if resolution requires PSRAM
     */
    bool requiresPSRAM() const {
        return _frameSize >= FRAMESIZE_XGA;
    }

    /**
     * @brief Get human-readable name
     */
    String getName() const {
        switch (_frameSize) {
            case FRAMESIZE_96X96:    return "96x96";
            case FRAMESIZE_QQVGA:    return "QQVGA (160x120)";
            case FRAMESIZE_QCIF:     return "QCIF (176x144)";
            case FRAMESIZE_HQVGA:    return "HQVGA (240x176)";
            case FRAMESIZE_240X240:  return "240x240";
            case FRAMESIZE_QVGA:     return "QVGA (320x240)";
            case FRAMESIZE_CIF:      return "CIF (400x296)";
            case FRAMESIZE_HVGA:     return "HVGA (480x320)";
            case FRAMESIZE_VGA:      return "VGA (640x480)";
            case FRAMESIZE_SVGA:     return "SVGA (800x600)";
            case FRAMESIZE_XGA:      return "XGA (1024x768)";
            case FRAMESIZE_HD:       return "HD (1280x720)";
            case FRAMESIZE_SXGA:     return "SXGA (1280x1024)";
            case FRAMESIZE_UXGA:     return "UXGA (1600x1200)";
            default:                 return "Unknown";
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
    static Resolution QQVGA() { return Resolution(FRAMESIZE_QQVGA); }
    static Resolution QVGA()  { return Resolution(FRAMESIZE_QVGA); }
    static Resolution VGA()   { return Resolution(FRAMESIZE_VGA); }
    static Resolution SVGA()  { return Resolution(FRAMESIZE_SVGA); }
    static Resolution XGA()   { return Resolution(FRAMESIZE_XGA); }
    static Resolution HD()    { return Resolution(FRAMESIZE_HD); }
    static Resolution SXGA()  { return Resolution(FRAMESIZE_SXGA); }
    static Resolution UXGA()  { return Resolution(FRAMESIZE_UXGA); }
};

#endif // RESOLUTION_H
