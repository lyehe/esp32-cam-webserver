/**
 * @file TypeMappers.h
 * @brief Type mapping utilities between domain and hardware types
 *
 * These mappers enable Clean Architecture by translating between:
 * - Domain types (framework-agnostic value objects)
 * - Hardware types (ESP32-specific enums and structures)
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef TYPE_MAPPERS_H
#define TYPE_MAPPERS_H

#include "esp_camera.h"
#include "../domain/value_objects/Resolution.h"
#include "../domain/value_objects/PixelFormat.h"
#include "../core/Result.h"

/**
 * @brief Type mapping utilities namespace
 */
namespace TypeMappers {

    // ========================================================================
    // Resolution Mapping (FrameSize <-> framesize_t)
    // ========================================================================

    /**
     * @brief Convert domain FrameSize to hardware framesize_t
     */
    inline framesize_t toHardwareFrameSize(FrameSize domainSize) {
        switch (domainSize) {
            case FrameSize::SIZE_96X96:    return FRAMESIZE_96X96;
            case FrameSize::SIZE_QQVGA:    return FRAMESIZE_QQVGA;
            case FrameSize::SIZE_QCIF:     return FRAMESIZE_QCIF;
            case FrameSize::SIZE_HQVGA:    return FRAMESIZE_HQVGA;
            case FrameSize::SIZE_240X240:  return FRAMESIZE_240X240;
            case FrameSize::SIZE_QVGA:     return FRAMESIZE_QVGA;
            case FrameSize::SIZE_CIF:      return FRAMESIZE_CIF;
            case FrameSize::SIZE_HVGA:     return FRAMESIZE_HVGA;
            case FrameSize::SIZE_VGA:      return FRAMESIZE_VGA;
            case FrameSize::SIZE_SVGA:     return FRAMESIZE_SVGA;
            case FrameSize::SIZE_XGA:      return FRAMESIZE_XGA;
            case FrameSize::SIZE_HD:       return FRAMESIZE_HD;
            case FrameSize::SIZE_SXGA:     return FRAMESIZE_SXGA;
            case FrameSize::SIZE_UXGA:     return FRAMESIZE_UXGA;
            default:                       return FRAMESIZE_SVGA;
        }
    }

    /**
     * @brief Convert hardware framesize_t to domain FrameSize
     */
    inline FrameSize toDomainFrameSize(framesize_t hardwareSize) {
        switch (hardwareSize) {
            case FRAMESIZE_96X96:    return FrameSize::SIZE_96X96;
            case FRAMESIZE_QQVGA:    return FrameSize::SIZE_QQVGA;
            case FRAMESIZE_QCIF:     return FrameSize::SIZE_QCIF;
            case FRAMESIZE_HQVGA:    return FrameSize::SIZE_HQVGA;
            case FRAMESIZE_240X240:  return FrameSize::SIZE_240X240;
            case FRAMESIZE_QVGA:     return FrameSize::SIZE_QVGA;
            case FRAMESIZE_CIF:      return FrameSize::SIZE_CIF;
            case FRAMESIZE_HVGA:     return FrameSize::SIZE_HVGA;
            case FRAMESIZE_VGA:      return FrameSize::SIZE_VGA;
            case FRAMESIZE_SVGA:     return FrameSize::SIZE_SVGA;
            case FRAMESIZE_XGA:      return FrameSize::SIZE_XGA;
            case FRAMESIZE_HD:       return FrameSize::SIZE_HD;
            case FRAMESIZE_SXGA:     return FrameSize::SIZE_SXGA;
            case FRAMESIZE_UXGA:     return FrameSize::SIZE_UXGA;
            default:                 return FrameSize::SIZE_SVGA;
        }
    }

    // ========================================================================
    // Pixel Format Mapping (Format <-> pixformat_t)
    // ========================================================================

    /**
     * @brief Convert domain Format to hardware pixformat_t
     */
    inline pixformat_t toHardwarePixelFormat(Format domainFormat) {
        switch (domainFormat) {
            case Format::RGB565:    return PIXFORMAT_RGB565;
            case Format::YUV422:    return PIXFORMAT_YUV422;
            case Format::GRAYSCALE: return PIXFORMAT_GRAYSCALE;
            case Format::JPEG:      return PIXFORMAT_JPEG;
            case Format::RGB888:    return PIXFORMAT_RGB888;
            case Format::RAW:       return PIXFORMAT_RAW;
            case Format::RGB444:    return PIXFORMAT_RGB444;
            case Format::RGB555:    return PIXFORMAT_RGB555;
            default:                return PIXFORMAT_JPEG;
        }
    }

    /**
     * @brief Convert hardware pixformat_t to domain Format
     */
    inline Format toDomainPixelFormat(pixformat_t hardwareFormat) {
        switch (hardwareFormat) {
            case PIXFORMAT_RGB565:    return Format::RGB565;
            case PIXFORMAT_YUV422:    return Format::YUV422;
            case PIXFORMAT_GRAYSCALE: return Format::GRAYSCALE;
            case PIXFORMAT_JPEG:      return Format::JPEG;
            case PIXFORMAT_RGB888:    return Format::RGB888;
            case PIXFORMAT_RAW:       return Format::RAW;
            case PIXFORMAT_RGB444:    return Format::RGB444;
            case PIXFORMAT_RGB555:    return Format::RGB555;
            default:                  return Format::JPEG;
        }
    }

    /**
     * @brief Convert PixelFormat value object to hardware type
     */
    inline pixformat_t toHardwarePixelFormat(const PixelFormat& domainPixelFormat) {
        return toHardwarePixelFormat(domainPixelFormat.getFormat());
    }

    /**
     * @brief Convert hardware type to PixelFormat value object
     */
    inline PixelFormat toDomainPixelFormat(pixformat_t hardwareFormat, bool createValueObject) {
        return PixelFormat(toDomainPixelFormat(hardwareFormat));
    }

    // ========================================================================
    // Frame Conversion
    // ========================================================================

    /**
     * @brief Convert hardware camera_fb_t to domain Frame
     */
    inline Frame fromHardwareFrame(camera_fb_t* fb) {
        if (!fb) {
            return Frame(); // Invalid frame
        }

        PixelFormat format = toDomainPixelFormat(fb->format, true);
        return Frame(fb->buf, fb->len, fb->width, fb->height, format);
    }

} // namespace TypeMappers

#endif // TYPE_MAPPERS_H
