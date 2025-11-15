/**
 * @file Credentials.h
 * @brief Value object representing authentication credentials
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <Arduino.h>
#include "../../core/Result.h"

/**
 * @brief Credentials value object
 *
 * Immutable value object representing user credentials with validation.
 */
class Credentials {
private:
    String _username;
    String _password;
    bool _valid;

    /**
     * @brief Validate credentials
     */
    static bool validate(const String& username, const String& password) {
        // Username validation
        if (username.length() < 3 || username.length() > 32) {
            return false;
        }

        // Password validation (minimum 8 characters for security)
        if (password.length() < 8 || password.length() > 64) {
            return false;
        }

        // Check for invalid characters
        for (size_t i = 0; i < username.length(); i++) {
            char c = username.charAt(i);
            if (!isalnum(c) && c != '_' && c != '-') {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Private constructor
     */
    Credentials(const String& username, const String& password, bool valid)
        : _username(username), _password(password), _valid(valid) {}

public:
    /**
     * @brief Create credentials with validation
     */
    static Result<Credentials> create(const String& username, const String& password) {
        if (!validate(username, password)) {
            return Result<Credentials>::error(
                "Invalid credentials: Username must be 3-32 alphanumeric chars, "
                "password must be 8-64 chars"
            );
        }

        return Result<Credentials>::ok(Credentials(username, password, true));
    }

    /**
     * @brief Create empty/invalid credentials
     */
    static Credentials empty() {
        return Credentials("", "", false);
    }

    // Accessors
    const String& getUsername() const { return _username; }
    const String& getPassword() const { return _password; }
    bool isValid() const { return _valid; }

    /**
     * @brief Get password hash (for storage)
     *
     * NOTE: This is a simple hash for demonstration.
     * In production, use proper password hashing (bcrypt, argon2, etc.)
     */
    String getPasswordHash() const {
        // Simple SHA-256-like hash for demonstration
        // In production, use mbedtls_md for proper hashing
        uint32_t hash = 0;
        for (size_t i = 0; i < _password.length(); i++) {
            hash = ((hash << 5) - hash) + _password.charAt(i);
        }
        char hashStr[32];
        sprintf(hashStr, "%08x", hash);
        return String(hashStr);
    }

    /**
     * @brief Verify password against hash
     */
    bool verifyPassword(const String& hash) const {
        return getPasswordHash() == hash;
    }

    /**
     * @brief Equality comparison (constant-time to prevent timing attacks)
     */
    bool operator==(const Credentials& other) const {
        bool usernameMatch = true;
        bool passwordMatch = true;

        // Constant-time string comparison
        size_t maxLen = max(_username.length(), other._username.length());
        for (size_t i = 0; i < maxLen; i++) {
            char a = i < _username.length() ? _username.charAt(i) : 0;
            char b = i < other._username.length() ? other._username.charAt(i) : 0;
            if (a != b) usernameMatch = false;
        }

        maxLen = max(_password.length(), other._password.length());
        for (size_t i = 0; i < maxLen; i++) {
            char a = i < _password.length() ? _password.charAt(i) : 0;
            char b = i < other._password.length() ? other._password.charAt(i) : 0;
            if (a != b) passwordMatch = false;
        }

        return usernameMatch && passwordMatch;
    }

    bool operator!=(const Credentials& other) const {
        return !(*this == other);
    }

    /**
     * @brief Convert to string (sanitized - no password)
     */
    String toString() const {
        return "Credentials[username=" + _username + "]";
    }
};

/**
 * @brief WiFi credentials value object
 */
class WiFiCredentials {
private:
    String _ssid;
    String _password;
    bool _valid;

    static bool validate(const String& ssid, const String& password) {
        // SSID validation (1-32 characters)
        if (ssid.length() == 0 || ssid.length() > 32) {
            return false;
        }

        // Password validation (0 for open network, or 8-63 for WPA/WPA2)
        if (password.length() > 0 &&
            (password.length() < 8 || password.length() > 63)) {
            return false;
        }

        return true;
    }

    WiFiCredentials(const String& ssid, const String& password, bool valid)
        : _ssid(ssid), _password(password), _valid(valid) {}

public:
    static Result<WiFiCredentials> create(const String& ssid, const String& password = "") {
        if (!validate(ssid, password)) {
            return Result<WiFiCredentials>::error(
                "Invalid WiFi credentials: SSID must be 1-32 chars, "
                "password must be 0 (open) or 8-63 chars"
            );
        }

        return Result<WiFiCredentials>::ok(WiFiCredentials(ssid, password, true));
    }

    static WiFiCredentials empty() {
        return WiFiCredentials("", "", false);
    }

    const String& getSSID() const { return _ssid; }
    const String& getPassword() const { return _password; }
    bool isValid() const { return _valid; }
    bool isOpenNetwork() const { return _password.length() == 0; }

    String toString() const {
        return "WiFiCredentials[ssid=" + _ssid +
               ", secured=" + String(isOpenNetwork() ? "false" : "true") + "]";
    }
};

#endif // CREDENTIALS_H
