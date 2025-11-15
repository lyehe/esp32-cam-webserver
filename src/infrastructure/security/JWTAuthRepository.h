/**
 * @file JWTAuthRepository.h
 * @brief JWT-based authentication repository implementation
 *
 * Implements IAuthRepository using JWT tokens with mbedtls.
 * Provides secure authentication and authorization.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef JWT_AUTH_REPOSITORY_H
#define JWT_AUTH_REPOSITORY_H

#include "../../domain/repositories/IAuthRepository.h"
#include "../../domain/repositories/IStorageRepository.h"
#include "../../core/Logger.h"
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <map>

/**
 * @brief JWT Authentication Repository
 *
 * Implements authentication using JWT tokens.
 * Uses mbedtls for HMAC-SHA256 signing.
 */
class JWTAuthRepository : public IAuthRepository {
private:
    static constexpr const char* TAG = "JWTAuthRepo";
    static constexpr uint32_t DEFAULT_TOKEN_EXPIRY_SECONDS = 3600; // 1 hour
    static constexpr uint32_t MAX_TOKEN_CACHE_SIZE = 100;

    String _jwtSecret;
    IStorageRepository* _storage;
    bool _initialized;

    // Token cache (username -> token)
    std::map<String, AuthToken> _tokenCache;

    /**
     * @brief Base64URL encode (RFC 4648)
     */
    String base64UrlEncode(const uint8_t* data, size_t length) {
        // Calculate output buffer size
        size_t outLen = 0;
        mbedtls_base64_encode(nullptr, 0, &outLen, data, length);

        // Encode
        std::vector<uint8_t> output(outLen);
        mbedtls_base64_encode(output.data(), output.size(), &outLen, data, length);

        String result = String((char*)output.data());

        // Convert to base64url: replace +/= with -_
        result.replace('+', '-');
        result.replace('/', '_');
        result.replace("=", "");

        return result;
    }

    /**
     * @brief Base64URL decode
     */
    Result<std::vector<uint8_t>> base64UrlDecode(const String& encoded) {
        String input = encoded;

        // Convert from base64url to base64
        input.replace('-', '+');
        input.replace('_', '/');

        // Add padding if needed
        while (input.length() % 4 != 0) {
            input += '=';
        }

        // Calculate output buffer size
        size_t outLen = 0;
        int ret = mbedtls_base64_decode(nullptr, 0, &outLen,
                                        (const uint8_t*)input.c_str(), input.length());

        if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL && ret != 0) {
            return Result<std::vector<uint8_t>>::error("Base64 decode size calculation failed");
        }

        // Decode
        std::vector<uint8_t> output(outLen);
        ret = mbedtls_base64_decode(output.data(), output.size(), &outLen,
                                     (const uint8_t*)input.c_str(), input.length());

        if (ret != 0) {
            return Result<std::vector<uint8_t>>::error("Base64 decode failed");
        }

        output.resize(outLen);
        return Result<std::vector<uint8_t>>::ok(output);
    }

    /**
     * @brief HMAC-SHA256 signature
     */
    Result<String> hmacSha256(const String& data, const String& key) {
        uint8_t output[32]; // SHA256 = 32 bytes

        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);

        const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (info == nullptr) {
            mbedtls_md_free(&ctx);
            return Result<String>::error("Failed to get SHA256 info");
        }

        int ret = mbedtls_md_setup(&ctx, info, 1); // 1 = HMAC
        if (ret != 0) {
            mbedtls_md_free(&ctx);
            return Result<String>::error("HMAC setup failed");
        }

        ret = mbedtls_md_hmac_starts(&ctx, (const uint8_t*)key.c_str(), key.length());
        if (ret != 0) {
            mbedtls_md_free(&ctx);
            return Result<String>::error("HMAC start failed");
        }

        ret = mbedtls_md_hmac_update(&ctx, (const uint8_t*)data.c_str(), data.length());
        if (ret != 0) {
            mbedtls_md_free(&ctx);
            return Result<String>::error("HMAC update failed");
        }

        ret = mbedtls_md_hmac_finish(&ctx, output);
        if (ret != 0) {
            mbedtls_md_free(&ctx);
            return Result<String>::error("HMAC finish failed");
        }

        mbedtls_md_free(&ctx);

        // Base64URL encode the signature
        return Result<String>::ok(base64UrlEncode(output, 32));
    }

    /**
     * @brief Create JWT header (base64url encoded)
     */
    String createJwtHeader() {
        String header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
        return base64UrlEncode((const uint8_t*)header.c_str(), header.length());
    }

    /**
     * @brief Create JWT payload (base64url encoded)
     */
    String createJwtPayload(const String& username, UserRole role, uint32_t expirySeconds) {
        uint32_t now = millis() / 1000; // Unix timestamp (approximation)
        uint32_t exp = now + expirySeconds;

        String payload = "{";
        payload += "\"username\":\"" + username + "\",";
        payload += "\"role\":" + String(static_cast<int>(role)) + ",";
        payload += "\"iat\":" + String(now) + ",";
        payload += "\"exp\":" + String(exp);
        payload += "}";

        return base64UrlEncode((const uint8_t*)payload.c_str(), payload.length());
    }

    /**
     * @brief Generate JWT token
     */
    Result<String> generateToken(const String& username, UserRole role, uint32_t expirySeconds) {
        String header = createJwtHeader();
        String payload = createJwtPayload(username, role, expirySeconds);

        String message = header + "." + payload;

        auto signatureResult = hmacSha256(message, _jwtSecret);
        if (signatureResult.isError()) {
            return Result<String>::error("Failed to sign token: " + signatureResult.getError());
        }

        String token = message + "." + signatureResult.getValue();
        return Result<String>::ok(token);
    }

    /**
     * @brief Parse JWT payload from token
     */
    Result<String> parsePayload(const String& token) {
        int firstDot = token.indexOf('.');
        int secondDot = token.indexOf('.', firstDot + 1);

        if (firstDot < 0 || secondDot < 0) {
            return Result<String>::error("Invalid token format");
        }

        String payloadEncoded = token.substring(firstDot + 1, secondDot);
        auto payloadBytes = base64UrlDecode(payloadEncoded);

        if (payloadBytes.isError()) {
            return Result<String>::error("Failed to decode payload");
        }

        String payload = String((char*)payloadBytes.getValue().data());
        return Result<String>::ok(payload);
    }

    /**
     * @brief Verify JWT signature
     */
    Result<bool> verifySignature(const String& token) {
        int firstDot = token.indexOf('.');
        int secondDot = token.indexOf('.', firstDot + 1);

        if (firstDot < 0 || secondDot < 0) {
            return Result<bool>::error("Invalid token format");
        }

        String message = token.substring(0, secondDot);
        String signatureProvided = token.substring(secondDot + 1);

        auto signatureResult = hmacSha256(message, _jwtSecret);
        if (signatureResult.isError()) {
            return Result<bool>::error("Failed to compute signature");
        }

        bool valid = (signatureResult.getValue() == signatureProvided);
        return Result<bool>::ok(valid);
    }

    /**
     * @brief Extract username from token payload
     */
    Result<String> extractUsername(const String& payload) {
        // Simple JSON parsing (could use ArduinoJson for robustness)
        int usernameStart = payload.indexOf("\"username\":\"");
        if (usernameStart < 0) {
            return Result<String>::error("Username not found in token");
        }

        usernameStart += 12; // Length of "\"username\":\""
        int usernameEnd = payload.indexOf("\"", usernameStart);

        if (usernameEnd < 0) {
            return Result<String>::error("Malformed username in token");
        }

        String username = payload.substring(usernameStart, usernameEnd);
        return Result<String>::ok(username);
    }

    /**
     * @brief Extract role from token payload
     */
    Result<UserRole> extractRole(const String& payload) {
        int roleStart = payload.indexOf("\"role\":");
        if (roleStart < 0) {
            return Result<UserRole>::error("Role not found in token");
        }

        roleStart += 7; // Length of "\"role\":"
        int roleEnd = payload.indexOf(",", roleStart);
        if (roleEnd < 0) {
            roleEnd = payload.indexOf("}", roleStart);
        }

        if (roleEnd < 0) {
            return Result<UserRole>::error("Malformed role in token");
        }

        String roleStr = payload.substring(roleStart, roleEnd);
        roleStr.trim();

        int roleInt = roleStr.toInt();
        return Result<UserRole>::ok(static_cast<UserRole>(roleInt));
    }

    /**
     * @brief Extract expiry from token payload
     */
    Result<uint32_t> extractExpiry(const String& payload) {
        int expStart = payload.indexOf("\"exp\":");
        if (expStart < 0) {
            return Result<uint32_t>::error("Expiry not found in token");
        }

        expStart += 6; // Length of "\"exp\":"
        int expEnd = payload.indexOf(",", expStart);
        if (expEnd < 0) {
            expEnd = payload.indexOf("}", expStart);
        }

        if (expEnd < 0) {
            return Result<uint32_t>::error("Malformed expiry in token");
        }

        String expStr = payload.substring(expStart, expEnd);
        expStr.trim();

        uint32_t expiry = expStr.toInt();
        return Result<uint32_t>::ok(expiry);
    }

public:
    /**
     * @brief Constructor
     */
    JWTAuthRepository(IStorageRepository* storage)
        : _storage(storage), _initialized(false) {}

    /**
     * @brief Initialize with JWT secret
     */
    Result<void> initialize(const String& jwtSecret) override {
        if (jwtSecret.length() < 32) {
            return Result<void>::error("JWT secret must be at least 32 characters");
        }

        _jwtSecret = jwtSecret;
        _initialized = true;

        Logger::getInstance().info(TAG, "JWT auth repository initialized");
        return Result<void>::ok();
    }

    /**
     * @brief Authenticate user with credentials
     */
    Result<AuthToken> authenticate(const Credentials& credentials) override {
        if (!_initialized) {
            return Result<AuthToken>::error("Auth repository not initialized");
        }

        if (!_storage) {
            return Result<AuthToken>::error("Storage repository not available");
        }

        // Load user from storage
        auto userResult = _storage->loadUser(credentials.getUsername());
        if (userResult.isError()) {
            Logger::getInstance().warn(TAG, "Authentication failed for: " + credentials.getUsername());
            return Result<AuthToken>::error("Invalid credentials");
        }

        User user = userResult.getValue();

        // Verify password
        if (!credentials.verifyPassword(user.getPasswordHash())) {
            Logger::getInstance().warn(TAG, "Password verification failed for: " + credentials.getUsername());
            return Result<AuthToken>::error("Invalid credentials");
        }

        // Generate JWT token
        auto tokenResult = generateToken(user.getUsername(), user.getRole(), DEFAULT_TOKEN_EXPIRY_SECONDS);
        if (tokenResult.isError()) {
            return Result<AuthToken>::error("Failed to generate token");
        }

        uint32_t expiresAt = (millis() / 1000) + DEFAULT_TOKEN_EXPIRY_SECONDS;

        AuthToken authToken;
        authToken.token = tokenResult.getValue();
        authToken.username = user.getUsername();
        authToken.expiresAt = expiresAt;

        // Cache token
        _tokenCache[user.getUsername()] = authToken;

        // Update login statistics
        user.recordLogin();
        _storage->saveUser(user);

        Logger::getInstance().info(TAG, "User authenticated: " + user.getUsername());

        return Result<AuthToken>::ok(authToken);
    }

    /**
     * @brief Verify token and return user
     */
    Result<User> verifyToken(const String& token) override {
        if (!_initialized) {
            return Result<User>::error("Auth repository not initialized");
        }

        // Verify signature
        auto signatureResult = verifySignature(token);
        if (signatureResult.isError() || !signatureResult.getValue()) {
            return Result<User>::error("Invalid token signature");
        }

        // Parse payload
        auto payloadResult = parsePayload(token);
        if (payloadResult.isError()) {
            return Result<User>::error("Failed to parse token");
        }

        String payload = payloadResult.getValue();

        // Extract and verify expiry
        auto expiryResult = extractExpiry(payload);
        if (expiryResult.isError()) {
            return Result<User>::error("Invalid token expiry");
        }

        uint32_t now = millis() / 1000;
        if (now > expiryResult.getValue()) {
            return Result<User>::error("Token expired");
        }

        // Extract username
        auto usernameResult = extractUsername(payload);
        if (usernameResult.isError()) {
            return Result<User>::error("Invalid token username");
        }

        // Load user from storage
        if (!_storage) {
            return Result<User>::error("Storage repository not available");
        }

        return _storage->loadUser(usernameResult.getValue());
    }

    /**
     * @brief Refresh token (extend expiry)
     */
    Result<AuthToken> refreshToken(const String& token) override {
        // Verify current token
        auto userResult = verifyToken(token);
        if (userResult.isError()) {
            return Result<AuthToken>::error(userResult.getError());
        }

        User user = userResult.getValue();

        // Generate new token
        auto tokenResult = generateToken(user.getUsername(), user.getRole(), DEFAULT_TOKEN_EXPIRY_SECONDS);
        if (tokenResult.isError()) {
            return Result<AuthToken>::error("Failed to generate token");
        }

        uint32_t expiresAt = (millis() / 1000) + DEFAULT_TOKEN_EXPIRY_SECONDS;

        AuthToken authToken;
        authToken.token = tokenResult.getValue();
        authToken.username = user.getUsername();
        authToken.expiresAt = expiresAt;

        // Update cache
        _tokenCache[user.getUsername()] = authToken;

        Logger::getInstance().info(TAG, "Token refreshed for: " + user.getUsername());

        return Result<AuthToken>::ok(authToken);
    }

    /**
     * @brief Revoke token
     */
    Result<void> revokeToken(const String& token) override {
        // Extract username from token
        auto payloadResult = parsePayload(token);
        if (payloadResult.isError()) {
            return Result<void>::error("Failed to parse token");
        }

        auto usernameResult = extractUsername(payloadResult.getValue());
        if (usernameResult.isError()) {
            return Result<void>::error("Failed to extract username");
        }

        // Remove from cache
        _tokenCache.erase(usernameResult.getValue());

        Logger::getInstance().info(TAG, "Token revoked for: " + usernameResult.getValue());
        return Result<void>::ok();
    }

    /**
     * @brief Register new user
     */
    Result<User> registerUser(const Credentials& credentials, UserRole role) override {
        if (!_initialized) {
            return Result<User>::error("Auth repository not initialized");
        }

        if (!_storage) {
            return Result<User>::error("Storage repository not available");
        }

        // Check if user already exists
        if (_storage->userExists(credentials.getUsername())) {
            return Result<User>::error("User already exists");
        }

        // Create user
        auto userResult = User::create(credentials, role);
        if (userResult.isError()) {
            return Result<User>::error(userResult.getError());
        }

        User user = userResult.getValue();

        // Save to storage
        auto saveResult = _storage->saveUser(user);
        if (saveResult.isError()) {
            return Result<User>::error("Failed to save user: " + saveResult.getError());
        }

        Logger::getInstance().info(TAG, "User registered: " + user.getUsername());

        return Result<User>::ok(user);
    }

    /**
     * @brief Update user
     */
    Result<void> updateUser(const User& user) override {
        if (!_initialized) {
            return Result<void>::error("Auth repository not initialized");
        }

        if (!_storage) {
            return Result<void>::error("Storage repository not available");
        }

        auto result = _storage->saveUser(user);
        if (result.isError()) {
            return Result<void>::error("Failed to update user");
        }

        Logger::getInstance().info(TAG, "User updated: " + user.getUsername());
        return Result<void>::ok();
    }

    /**
     * @brief Delete user
     */
    Result<void> deleteUser(const String& username) override {
        if (!_initialized) {
            return Result<void>::error("Auth repository not initialized");
        }

        if (!_storage) {
            return Result<void>::error("Storage repository not available");
        }

        // Revoke any cached tokens
        _tokenCache.erase(username);

        auto result = _storage->deleteUser(username);
        if (result.isError()) {
            return Result<void>::error("Failed to delete user");
        }

        Logger::getInstance().info(TAG, "User deleted: " + username);
        return Result<void>::ok();
    }

    /**
     * @brief Check if user exists
     */
    bool userExists(const String& username) override {
        if (!_storage) {
            return false;
        }

        return _storage->userExists(username);
    }

    /**
     * @brief List all users
     */
    Result<std::vector<String>> listUsers() override {
        if (!_storage) {
            return Result<std::vector<String>>::error("Storage repository not available");
        }

        return _storage->listUsers();
    }

    /**
     * @brief Change user password
     */
    Result<void> changePassword(const String& username, const Credentials& newCredentials) override {
        if (!_initialized) {
            return Result<void>::error("Auth repository not initialized");
        }

        if (!_storage) {
            return Result<void>::error("Storage repository not available");
        }

        // Load user
        auto userResult = _storage->loadUser(username);
        if (userResult.isError()) {
            return Result<void>::error("User not found");
        }

        User user = userResult.getValue();

        // Update password (need to recreate user with new hash)
        // This is a limitation - User entity doesn't have setPasswordHash
        // TODO: Add password change method to User entity

        Logger::getInstance().info(TAG, "Password change requested for: " + username);
        return Result<void>::error("Password change not yet implemented in User entity");
    }
};

#endif // JWT_AUTH_REPOSITORY_H
