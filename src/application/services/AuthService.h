/**
 * @file AuthService.h
 * @brief Application service for authentication and authorization
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "../usecases/AuthUseCases.h"
#include "../../core/Logger.h"
#include <map>

/**
 * @brief Authentication Application Service
 *
 * Coordinates authentication and authorization operations.
 */
class AuthService {
private:
    static constexpr const char* TAG = "AuthSvc";

    IAuthRepository& _authRepo;

    // Use cases
    LoginUseCase _loginUC;
    VerifyTokenUseCase _verifyTokenUC;
    RegisterUserUseCase _registerUserUC;
    RefreshTokenUseCase _refreshTokenUC;
    LogoutUseCase _logoutUC;
    GetUserPermissionsUseCase _getPermissionsUC;
    DeleteUserUseCase _deleteUserUC;
    ListUsersUseCase _listUsersUC;

    // Session tracking (optional - for stats)
    std::map<String, uint32_t> _activeSessions; // username -> login time

public:
    /**
     * @brief Constructor
     */
    AuthService(IAuthRepository& authRepo)
        : _authRepo(authRepo),
          _loginUC(authRepo),
          _verifyTokenUC(authRepo),
          _registerUserUC(authRepo),
          _refreshTokenUC(authRepo),
          _logoutUC(authRepo),
          _getPermissionsUC(authRepo),
          _deleteUserUC(authRepo),
          _listUsersUC(authRepo) {
        Logger::getInstance().info(TAG, "Auth service initialized");
    }

    /**
     * @brief Authenticate user (login)
     */
    Result<LoginResponseDTO> login(const String& username, const String& password) {
        auto result = _loginUC.execute(username, password);

        if (result.isOk() && result.getValue().success) {
            _activeSessions[username] = millis();
        }

        return result;
    }

    /**
     * @brief Verify JWT token
     */
    Result<UserDTO> verifyToken(const String& token) {
        return _verifyTokenUC.execute(token);
    }

    /**
     * @brief Register new user
     */
    Result<UserDTO> registerUser(const String& username, const String& password, UserRole role) {
        return _registerUserUC.execute(username, password, role);
    }

    /**
     * @brief Refresh authentication token
     */
    Result<AuthTokenDTO> refreshToken(const String& oldToken) {
        return _refreshTokenUC.execute(oldToken);
    }

    /**
     * @brief Logout user (revoke token)
     */
    Result<void> logout(const String& token) {
        // Get username from token before revoking
        auto userResult = _verifyTokenUC.execute(token);
        if (userResult.isOk()) {
            String username = userResult.getValue().username;
            _activeSessions.erase(username);
        }

        return _logoutUC.execute(token);
    }

    /**
     * @brief Get user permissions from token
     */
    Result<UserPermissionsDTO> getPermissions(const String& token) {
        return _getPermissionsUC.execute(token);
    }

    /**
     * @brief Check if user has specific permission
     */
    Result<bool> hasPermission(const String& token, const String& permission) {
        auto permResult = getPermissions(token);
        if (permResult.isError()) {
            return Result<bool>::error(permResult.getError());
        }

        UserPermissionsDTO perms = permResult.getValue();

        if (permission == "viewStream") {
            return Result<bool>::ok(perms.canViewStream);
        } else if (permission == "captureImage") {
            return Result<bool>::ok(perms.canCaptureImage);
        } else if (permission == "modifySettings") {
            return Result<bool>::ok(perms.canModifySettings);
        } else if (permission == "performOTA") {
            return Result<bool>::ok(perms.canPerformOTA);
        } else if (permission == "manageUsers") {
            return Result<bool>::ok(perms.canManageUsers);
        }

        return Result<bool>::error("Unknown permission: " + permission);
    }

    /**
     * @brief Delete user (admin only)
     */
    Result<void> deleteUser(const String& adminToken, const String& usernameToDelete) {
        auto result = _deleteUserUC.execute(adminToken, usernameToDelete);

        if (result.isOk()) {
            _activeSessions.erase(usernameToDelete);
        }

        return result;
    }

    /**
     * @brief List all users (admin only)
     */
    Result<std::vector<String>> listUsers(const String& adminToken) {
        return _listUsersUC.execute(adminToken);
    }

    /**
     * @brief Check if user exists
     */
    bool userExists(const String& username) {
        return _authRepo.userExists(username);
    }

    /**
     * @brief Get active session count
     */
    size_t getActiveSessionCount() {
        return _activeSessions.size();
    }

    /**
     * @brief Get session duration for user (seconds)
     */
    uint32_t getSessionDuration(const String& username) {
        if (_activeSessions.count(username) > 0) {
            return (millis() - _activeSessions[username]) / 1000;
        }
        return 0;
    }

    /**
     * @brief Authorization middleware helper
     *
     * Verifies token and checks permission in one call.
     * Returns user if authorized, error otherwise.
     */
    Result<UserDTO> authorize(const String& token, const String& requiredPermission) {
        Logger::getInstance().trace(TAG, "Authorizing for permission: " + requiredPermission);

        // Verify token
        auto userResult = verifyToken(token);
        if (userResult.isError()) {
            Logger::getInstance().warn(TAG, "Token verification failed");
            return Result<UserDTO>::error("Unauthorized: " + userResult.getError());
        }

        // Check permission
        auto permResult = hasPermission(token, requiredPermission);
        if (permResult.isError() || !permResult.getValue()) {
            Logger::getInstance().warn(TAG, "Permission denied: " + requiredPermission);
            return Result<UserDTO>::error("Forbidden: Insufficient permissions");
        }

        return userResult;
    }

    /**
     * @brief Quick authorization check (bool result)
     */
    bool isAuthorized(const String& token, const String& requiredPermission) {
        auto result = authorize(token, requiredPermission);
        return result.isOk();
    }

    /**
     * @brief Extract username from token (without full verification)
     *
     * Useful for logging/stats. Still verifies token is valid.
     */
    Result<String> getUsernameFromToken(const String& token) {
        auto userResult = verifyToken(token);
        if (userResult.isError()) {
            return Result<String>::error(userResult.getError());
        }

        return Result<String>::ok(userResult.getValue().username);
    }
};

#endif // AUTH_SERVICE_H
