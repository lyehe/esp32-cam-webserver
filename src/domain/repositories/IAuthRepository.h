/**
 * @file IAuthRepository.h
 * @brief Interface for authentication repository
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef I_AUTH_REPOSITORY_H
#define I_AUTH_REPOSITORY_H

#include "../entities/User.h"
#include "../value_objects/Credentials.h"
#include "../../core/Result.h"

/**
 * @brief Authentication token
 */
struct AuthToken {
    String token;
    String username;
    uint32_t issuedAt;
    uint32_t expiresAt;

    AuthToken() : issuedAt(0), expiresAt(0) {}

    AuthToken(const String& t, const String& user, uint32_t issued, uint32_t expires)
        : token(t), username(user), issuedAt(issued), expiresAt(expires) {}

    bool isValid() const {
        return token.length() > 0 && millis() < expiresAt;
    }

    bool isExpired() const {
        return millis() >= expiresAt;
    }

    uint32_t getTimeToExpiry() const {
        if (isExpired()) return 0;
        return expiresAt - millis();
    }

    String toString() const {
        return "Token[user=" + username + ", valid=" + String(isValid()) + "]";
    }
};

/**
 * @brief Authentication repository interface
 *
 * Manages user authentication, authorization, and token generation.
 */
class IAuthRepository {
public:
    virtual ~IAuthRepository() = default;

    /**
     * @brief Initialize authentication system
     *
     * @param jwtSecret Secret key for JWT signing
     * @return Result<void> Success or error
     */
    virtual Result<void> initialize(const String& jwtSecret) = 0;

    // ========================================================================
    // Authentication
    // ========================================================================

    /**
     * @brief Authenticate user with credentials
     *
     * @param credentials User credentials
     * @return Result<AuthToken> Auth token or error
     */
    virtual Result<AuthToken> authenticate(const Credentials& credentials) = 0;

    /**
     * @brief Verify authentication token
     *
     * @param token Token to verify
     * @return Result<User> User if valid, error otherwise
     */
    virtual Result<User> verifyToken(const String& token) = 0;

    /**
     * @brief Refresh authentication token
     *
     * @param token Current token
     * @return Result<AuthToken> New token or error
     */
    virtual Result<AuthToken> refreshToken(const String& token) = 0;

    /**
     * @brief Revoke authentication token
     *
     * @param token Token to revoke
     * @return Result<void> Success or error
     */
    virtual Result<void> revokeToken(const String& token) = 0;

    /**
     * @brief Check if token is valid
     *
     * @param token Token to check
     * @return bool True if valid, false otherwise
     */
    virtual bool isTokenValid(const String& token) = 0;

    // ========================================================================
    // User Management
    // ========================================================================

    /**
     * @brief Register new user
     *
     * @param credentials User credentials
     * @param role User role
     * @return Result<User> Created user or error
     */
    virtual Result<User> registerUser(const Credentials& credentials, UserRole role = UserRole::USER) = 0;

    /**
     * @brief Update user password
     *
     * @param username Username
     * @param oldCredentials Old credentials
     * @param newCredentials New credentials
     * @return Result<void> Success or error
     */
    virtual Result<void> updatePassword(const String& username,
                                       const Credentials& oldCredentials,
                                       const Credentials& newCredentials) = 0;

    /**
     * @brief Delete user
     *
     * @param username Username to delete
     * @return Result<void> Success or error
     */
    virtual Result<void> deleteUser(const String& username) = 0;

    /**
     * @brief Get user by username
     *
     * @param username Username
     * @return Result<User> User or error
     */
    virtual Result<User> getUser(const String& username) = 0;

    /**
     * @brief Check if user exists
     *
     * @param username Username
     * @return bool True if exists, false otherwise
     */
    virtual bool userExists(const String& username) = 0;

    // ========================================================================
    // Authorization
    // ========================================================================

    /**
     * @brief Check if user has permission
     *
     * @param username Username
     * @param permission Permission to check
     * @return bool True if has permission, false otherwise
     */
    virtual bool hasPermission(const String& username, const String& permission) = 0;

    /**
     * @brief Update user role
     *
     * @param username Username
     * @param role New role
     * @return Result<void> Success or error
     */
    virtual Result<void> updateUserRole(const String& username, UserRole role) = 0;

    /**
     * @brief Activate user account
     *
     * @param username Username
     * @return Result<void> Success or error
     */
    virtual Result<void> activateUser(const String& username) = 0;

    /**
     * @brief Deactivate user account
     *
     * @param username Username
     * @return Result<void> Success or error
     */
    virtual Result<void> deactivateUser(const String& username) = 0;

    // ========================================================================
    // Token Management
    // ========================================================================

    /**
     * @brief Set token lifetime
     *
     * @param lifetime Lifetime in seconds
     */
    virtual void setTokenLifetime(uint32_t lifetime) = 0;

    /**
     * @brief Get token lifetime
     *
     * @return uint32_t Lifetime in seconds
     */
    virtual uint32_t getTokenLifetime() const = 0;

    /**
     * @brief Revoke all tokens for user
     *
     * @param username Username
     * @return Result<void> Success or error
     */
    virtual Result<void> revokeAllUserTokens(const String& username) = 0;

    /**
     * @brief Get active token count for user
     *
     * @param username Username
     * @return size_t Number of active tokens
     */
    virtual size_t getActiveTokenCount(const String& username) = 0;
};

#endif // I_AUTH_REPOSITORY_H
