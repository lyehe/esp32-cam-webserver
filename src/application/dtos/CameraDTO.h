/**
 * @file CameraDTO.h
 * @brief Data Transfer Objects for camera information
 *
 * DTOs provide a simplified view of domain entities for presentation layer.
 * They decouple the HTTP API from internal domain models.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CAMERA_DTO_H
#define CAMERA_DTO_H

#include <Arduino.h>
#include "../../domain/entities/Camera.h"
#include "../../domain/entities/CameraSettings.h"

/**
 * @brief Camera status DTO
 *
 * Lightweight representation of camera state for HTTP responses.
 */
struct CameraStatusDTO {
    String state;
    String model;
    String resolution;
    String pixelFormat;
    uint8_t quality;
    uint32_t framesCaptured;
    uint32_t framesDropped;
    float currentFPS;
    bool lampEnabled;
    uint8_t lampIntensity;
    String errorMessage;

    /**
     * @brief Create from domain Camera entity
     */
    static CameraStatusDTO fromEntity(const Camera& camera) {
        CameraStatusDTO dto;
        dto.state = camera.getStateString();
        dto.model = camera.getModel();
        dto.resolution = camera.getSettings().getResolution().toString();
        dto.pixelFormat = camera.getSettings().getPixelFormat().toString();
        dto.quality = camera.getSettings().getQuality();
        dto.framesCaptured = camera.getFramesCaptured();
        dto.framesDropped = camera.getFramesDropped();
        dto.currentFPS = camera.getCurrentFPS();
        dto.lampEnabled = camera.isLampEnabled();
        dto.lampIntensity = camera.getLampIntensity();
        dto.errorMessage = camera.getErrorMessage();
        return dto;
    }

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"state\":\"" + state + "\",";
        json += "\"model\":\"" + model + "\",";
        json += "\"resolution\":\"" + resolution + "\",";
        json += "\"pixelFormat\":\"" + pixelFormat + "\",";
        json += "\"quality\":" + String(quality) + ",";
        json += "\"framesCaptured\":" + String(framesCaptured) + ",";
        json += "\"framesDropped\":" + String(framesDropped) + ",";
        json += "\"currentFPS\":" + String(currentFPS, 2) + ",";
        json += "\"lampEnabled\":" + String(lampEnabled ? "true" : "false") + ",";
        json += "\"lampIntensity\":" + String(lampIntensity);
        if (errorMessage.length() > 0) {
            json += ",\"error\":\"" + errorMessage + "\"";
        }
        json += "}";
        return json;
    }
};

/**
 * @brief Camera settings DTO
 *
 * Simplified settings for HTTP API.
 */
struct CameraSettingsDTO {
    // Basic settings
    String resolution;
    String pixelFormat;
    uint8_t quality;
    uint8_t frameBufferCount;

    // Image adjustments
    int8_t brightness;
    int8_t contrast;
    int8_t saturation;

    // Auto controls
    bool autoWhiteBalance;
    bool autoExposureControl;
    bool autoGainControl;

    // Image orientation
    bool verticalFlip;
    bool horizontalMirror;

    /**
     * @brief Create from domain CameraSettings
     */
    static CameraSettingsDTO fromEntity(const CameraSettings& settings) {
        CameraSettingsDTO dto;
        dto.resolution = settings.getResolution().toString();
        dto.pixelFormat = settings.getPixelFormat().toString();
        dto.quality = settings.getQuality();
        dto.frameBufferCount = settings.getFrameBufferCount();
        dto.brightness = settings.getBrightness();
        dto.contrast = settings.getContrast();
        dto.saturation = settings.getSaturation();
        dto.autoWhiteBalance = settings.isAutoWhiteBalance();
        dto.autoExposureControl = settings.isAutoExposureControl();
        dto.autoGainControl = settings.isAutoGainControl();
        dto.verticalFlip = settings.isVerticalFlip();
        dto.horizontalMirror = settings.isHorizontalMirror();
        return dto;
    }

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"resolution\":\"" + resolution + "\",";
        json += "\"pixelFormat\":\"" + pixelFormat + "\",";
        json += "\"quality\":" + String(quality) + ",";
        json += "\"frameBufferCount\":" + String(frameBufferCount) + ",";
        json += "\"brightness\":" + String(brightness) + ",";
        json += "\"contrast\":" + String(contrast) + ",";
        json += "\"saturation\":" + String(saturation) + ",";
        json += "\"autoWhiteBalance\":" + String(autoWhiteBalance ? "true" : "false") + ",";
        json += "\"autoExposureControl\":" + String(autoExposureControl ? "true" : "false") + ",";
        json += "\"autoGainControl\":" + String(autoGainControl ? "true" : "false") + ",";
        json += "\"verticalFlip\":" + String(verticalFlip ? "true" : "false") + ",";
        json += "\"horizontalMirror\":" + String(horizontalMirror ? "true" : "false");
        json += "}";
        return json;
    }
};

/**
 * @brief Frame info DTO
 *
 * Metadata about a captured frame.
 */
struct FrameInfoDTO {
    uint32_t width;
    uint32_t height;
    size_t length;
    String format;
    uint32_t timestamp;

    /**
     * @brief Create from domain Frame
     */
    static FrameInfoDTO fromEntity(const Frame& frame) {
        FrameInfoDTO dto;
        dto.width = frame.width;
        dto.height = frame.height;
        dto.length = frame.length;
        dto.format = frame.format.toString();
        dto.timestamp = frame.timestamp;
        return dto;
    }

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"width\":" + String(width) + ",";
        json += "\"height\":" + String(height) + ",";
        json += "\"length\":" + String(length) + ",";
        json += "\"format\":\"" + format + "\",";
        json += "\"timestamp\":" + String(timestamp);
        json += "}";
        return json;
    }
};

#endif // CAMERA_DTO_H
