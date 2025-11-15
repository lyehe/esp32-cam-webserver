/**
 * @file AuthDTO.h
 * @brief Data Transfer Objects for authentication
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef AUTH_DTO_H
#define AUTH_DTO_H

#include <Arduino.h>
#include "../../domain/entities/User.h"

/**
 * @brief User DTO
 *
 * Public user information (no sensitive data).
 */
struct UserDTO {
    String id;
    String username;
    String role;
    uint32_t loginCount;
    uint32_t lastLoginAt;
    uint32_t createdAt;

    /**
     * @brief Create from domain User entity
     */
    static UserDTO fromEntity(const User& user) {
        UserDTO dto;
        dto.id = user.getId();
        dto.username = user.getUsername();

        // Convert role to string
        switch (user.getRole()) {
            case UserRole::GUEST:
                dto.role = "guest";
                break;
            case UserRole::USER:
                dto.role = "user";
                break;
            case UserRole::ADMIN:
                dto.role = "admin";
                break;
            default:
                dto.role = "unknown";
        }

        dto.loginCount = user.getLoginCount();
        dto.lastLoginAt = user.getLastLoginAt();
        dto.createdAt = user.getCreatedAt();

        return dto;
    }

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"id\":\"" + id + "\",";
        json += "\"username\":\"" + username + "\",";
        json += "\"role\":\"" + role + "\",";
        json += "\"loginCount\":" + String(loginCount) + ",";
        json += "\"lastLoginAt\":" + String(lastLoginAt) + ",";
        json += "\"createdAt\":" + String(createdAt);
        json += "}";
        return json;
    }
};

/**
 * @brief Authentication token DTO
 */
struct AuthTokenDTO {
    String token;
    String username;
    uint32_t expiresAt;
    uint32_t expiresIn; // seconds until expiry

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"token\":\"" + token + "\",";
        json += "\"username\":\"" + username + "\",";
        json += "\"expiresAt\":" + String(expiresAt) + ",";
        json += "\"expiresIn\":" + String(expiresIn);
        json += "}";
        return json;
    }
};

/**
 * @brief Login request DTO
 */
struct LoginRequestDTO {
    String username;
    String password;
};

/**
 * @brief Login response DTO
 */
struct LoginResponseDTO {
    bool success;
    String message;
    AuthTokenDTO token;
    UserDTO user;

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"success\":" + String(success ? "true" : "false") + ",";
        json += "\"message\":\"" + message + "\"";

        if (success) {
            json += ",\"token\":" + token.toJson();
            json += ",\"user\":" + user.toJson();
        }

        json += "}";
        return json;
    }
};

/**
 * @brief User permissions DTO
 */
struct UserPermissionsDTO {
    bool canViewStream;
    bool canCaptureImage;
    bool canModifySettings;
    bool canPerformOTA;
    bool canManageUsers;

    /**
     * @brief Create from domain User entity
     */
    static UserPermissionsDTO fromEntity(const User& user) {
        UserPermissionsDTO dto;
        dto.canViewStream = user.canViewStream();
        dto.canCaptureImage = user.canCaptureImage();
        dto.canModifySettings = user.canModifySettings();
        dto.canPerformOTA = user.canPerformOTA();
        dto.canManageUsers = user.canManageUsers();
        return dto;
    }

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"canViewStream\":" + String(canViewStream ? "true" : "false") + ",";
        json += "\"canCaptureImage\":" + String(canCaptureImage ? "true" : "false") + ",";
        json += "\"canModifySettings\":" + String(canModifySettings ? "true" : "false") + ",";
        json += "\"canPerformOTA\":" + String(canPerformOTA ? "true" : "false") + ",";
        json += "\"canManageUsers\":" + String(canManageUsers ? "true" : "false");
        json += "}";
        return json;
    }
};

#endif // AUTH_DTO_H
