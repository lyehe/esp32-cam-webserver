/**
 * @file User.h
 * @brief Domain entity representing a user
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef USER_ENTITY_H
#define USER_ENTITY_H

#include <Arduino.h>
#include "../value_objects/Credentials.h"

/**
 * @brief User role enumeration
 */
enum class UserRole {
    GUEST,       // Read-only access (view stream only)
    USER,        // Standard user (view + capture)
    ADMIN        // Full access (view + capture + settings + OTA)
};

/**
 * @brief User entity
 *
 * Represents an authenticated user with roles and permissions.
 */
class User {
private:
    String _id;
    String _username;
    String _passwordHash;
    UserRole _role;
    bool _active;
    uint32_t _createdAt;
    uint32_t _lastLoginAt;
    uint32_t _loginCount;

public:
    /**
     * @brief Constructor
     */
    User(const String& username = "", const String& passwordHash = "",
         UserRole role = UserRole::USER)
        : _id(generateId()),
          _username(username),
          _passwordHash(passwordHash),
          _role(role),
          _active(true),
          _createdAt(millis()),
          _lastLoginAt(0),
          _loginCount(0) {}

    /**
     * @brief Create user from credentials
     */
    static Result<User> create(const Credentials& credentials, UserRole role = UserRole::USER) {
        if (!credentials.isValid()) {
            return Result<User>::error("Invalid credentials");
        }

        User user(credentials.getUsername(),
                  credentials.getPasswordHash(),
                  role);

        return Result<User>::ok(user);
    }

    // ========================================================================
    // Identification
    // ========================================================================

    const String& getId() const { return _id; }

    const String& getUsername() const { return _username; }

    // ========================================================================
    // Authentication
    // ========================================================================

    const String& getPasswordHash() const { return _passwordHash; }

    void setPasswordHash(const String& hash) {
        _passwordHash = hash;
    }

    /**
     * @brief Verify password
     */
    bool verifyPassword(const Credentials& credentials) const {
        if (credentials.getUsername() != _username) {
            return false;
        }

        return credentials.getPasswordHash() == _passwordHash;
    }

    /**
     * @brief Update password
     */
    void updatePassword(const Credentials& newCredentials) {
        if (newCredentials.isValid()) {
            _passwordHash = newCredentials.getPasswordHash();
        }
    }

    // ========================================================================
    // Authorization (Role & Permissions)
    // ========================================================================

    UserRole getRole() const { return _role; }

    void setRole(UserRole role) { _role = role; }

    bool isGuest() const { return _role == UserRole::GUEST; }
    bool isUser() const { return _role == UserRole::USER; }
    bool isAdmin() const { return _role == UserRole::ADMIN; }

    /**
     * @brief Check if user can view stream
     */
    bool canViewStream() const {
        return _active;  // All roles can view if active
    }

    /**
     * @brief Check if user can capture images
     */
    bool canCaptureImage() const {
        return _active && (_role == UserRole::USER || _role == UserRole::ADMIN);
    }

    /**
     * @brief Check if user can modify settings
     */
    bool canModifySettings() const {
        return _active && _role == UserRole::ADMIN;
    }

    /**
     * @brief Check if user can perform OTA updates
     */
    bool canPerformOTA() const {
        return _active && _role == UserRole::ADMIN;
    }

    /**
     * @brief Check if user can manage other users
     */
    bool canManageUsers() const {
        return _active && _role == UserRole::ADMIN;
    }

    // ========================================================================
    // Account management
    // ========================================================================

    bool isActive() const { return _active; }

    void setActive(bool active) { _active = active; }

    void activate() { _active = true; }

    void deactivate() { _active = false; }

    // ========================================================================
    // Session management
    // ========================================================================

    uint32_t getCreatedAt() const { return _createdAt; }

    uint32_t getLastLoginAt() const { return _lastLoginAt; }

    uint32_t getLoginCount() const { return _loginCount; }

    /**
     * @brief Record login
     */
    void recordLogin() {
        _lastLoginAt = millis();
        _loginCount++;
    }

    /**
     * @brief Get account age in seconds
     */
    uint32_t getAccountAge() const {
        return (millis() - _createdAt) / 1000;
    }

    /**
     * @brief Get time since last login in seconds
     */
    uint32_t getTimeSinceLastLogin() const {
        if (_lastLoginAt == 0) return 0;
        return (millis() - _lastLoginAt) / 1000;
    }

    // ========================================================================
    // Utility methods
    // ========================================================================

    /**
     * @brief Get role as string
     */
    String getRoleString() const {
        switch (_role) {
            case UserRole::GUEST: return "Guest";
            case UserRole::USER:  return "User";
            case UserRole::ADMIN: return "Admin";
            default:              return "Unknown";
        }
    }

    /**
     * @brief Convert to string for logging (no sensitive data)
     */
    String toString() const {
        char buffer[128];
        sprintf(buffer, "User[id=%s, username=%s, role=%s, active=%d, logins=%lu]",
                _id.c_str(),
                _username.c_str(),
                getRoleString().c_str(),
                _active,
                _loginCount);
        return String(buffer);
    }

    /**
     * @brief Equality comparison (by ID)
     */
    bool operator==(const User& other) const {
        return _id == other._id;
    }

    bool operator!=(const User& other) const {
        return !(*this == other);
    }

private:
    /**
     * @brief Generate unique user ID
     */
    static String generateId() {
        char id[16];
        sprintf(id, "%08lx", millis());
        return String("user_") + String(id);
    }
};

#endif // USER_ENTITY_H
