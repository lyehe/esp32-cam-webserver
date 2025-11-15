/**
 * @file AuthUseCases.h
 * @brief Authentication and authorization use cases
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef AUTH_USE_CASES_H
#define AUTH_USE_CASES_H

#include "../../domain/repositories/IAuthRepository.h"
#include "../../domain/value_objects/Credentials.h"
#include "../../core/Logger.h"
#include "../../core/Result.h"
#include "../dtos/AuthDTO.h"

/**
 * @brief Login use case
 */
class LoginUseCase {
private:
    static constexpr const char* TAG = "LoginUC";
    IAuthRepository& _authRepo;

public:
    LoginUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: Authenticate user and generate token
     */
    Result<LoginResponseDTO> execute(const String& username, const String& password) {
        Logger::getInstance().info(TAG, "Login attempt for user: " + username);

        LoginResponseDTO response;
        response.success = false;

        // Validate credentials format
        auto credResult = Credentials::create(username, password);
        if (credResult.isError()) {
            response.message = "Invalid credentials format: " + credResult.getError();
            Logger::getInstance().warn(TAG, response.message);
            return Result<LoginResponseDTO>::ok(response);
        }

        // Authenticate
        auto authResult = _authRepo.authenticate(credResult.getValue());
        if (authResult.isError()) {
            response.message = "Authentication failed: " + authResult.getError();
            Logger::getInstance().warn(TAG, response.message);
            return Result<LoginResponseDTO>::ok(response);
        }

        AuthToken authToken = authResult.getValue();

        // Get user info
        auto userResult = _authRepo.verifyToken(authToken.token);
        if (userResult.isError()) {
            response.message = "Failed to retrieve user info";
            Logger::getInstance().error(TAG, response.message);
            return Result<LoginResponseDTO>::ok(response);
        }

        User user = userResult.getValue();

        // Build response
        response.success = true;
        response.message = "Login successful";

        response.token.token = authToken.token;
        response.token.username = authToken.username;
        response.token.expiresAt = authToken.expiresAt;
        response.token.expiresIn = authToken.expiresAt - (millis() / 1000);

        response.user = UserDTO::fromEntity(user);

        Logger::getInstance().info(TAG, "User logged in successfully: " + username);

        return Result<LoginResponseDTO>::ok(response);
    }
};

/**
 * @brief Verify token use case
 */
class VerifyTokenUseCase {
private:
    static constexpr const char* TAG = "VerifyTokenUC";
    IAuthRepository& _authRepo;

public:
    VerifyTokenUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: Verify JWT token and return user
     */
    Result<UserDTO> execute(const String& token) {
        Logger::getInstance().trace(TAG, "Verifying token...");

        auto userResult = _authRepo.verifyToken(token);
        if (userResult.isError()) {
            Logger::getInstance().warn(TAG, "Token verification failed: " + userResult.getError());
            return Result<UserDTO>::error(userResult.getError());
        }

        User user = userResult.getValue();
        UserDTO dto = UserDTO::fromEntity(user);

        return Result<UserDTO>::ok(dto);
    }
};

/**
 * @brief Register user use case
 */
class RegisterUserUseCase {
private:
    static constexpr const char* TAG = "RegisterUserUC";
    IAuthRepository& _authRepo;

public:
    RegisterUserUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: Register a new user
     */
    Result<UserDTO> execute(const String& username, const String& password, UserRole role) {
        Logger::getInstance().info(TAG, "Registering user: " + username);

        // Validate credentials
        auto credResult = Credentials::create(username, password);
        if (credResult.isError()) {
            Logger::getInstance().warn(TAG, "Invalid credentials: " + credResult.getError());
            return Result<UserDTO>::error(credResult.getError());
        }

        // Register user
        auto userResult = _authRepo.registerUser(credResult.getValue(), role);
        if (userResult.isError()) {
            Logger::getInstance().error(TAG, "Registration failed: " + userResult.getError());
            return Result<UserDTO>::error(userResult.getError());
        }

        User user = userResult.getValue();
        UserDTO dto = UserDTO::fromEntity(user);

        Logger::getInstance().info(TAG, "User registered successfully: " + username);

        return Result<UserDTO>::ok(dto);
    }
};

/**
 * @brief Refresh token use case
 */
class RefreshTokenUseCase {
private:
    static constexpr const char* TAG = "RefreshTokenUC";
    IAuthRepository& _authRepo;

public:
    RefreshTokenUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: Refresh JWT token
     */
    Result<AuthTokenDTO> execute(const String& oldToken) {
        Logger::getInstance().info(TAG, "Refreshing token...");

        auto result = _authRepo.refreshToken(oldToken);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Token refresh failed: " + result.getError());
            return Result<AuthTokenDTO>::error(result.getError());
        }

        AuthToken authToken = result.getValue();

        AuthTokenDTO dto;
        dto.token = authToken.token;
        dto.username = authToken.username;
        dto.expiresAt = authToken.expiresAt;
        dto.expiresIn = authToken.expiresAt - (millis() / 1000);

        Logger::getInstance().info(TAG, "Token refreshed successfully");

        return Result<AuthTokenDTO>::ok(dto);
    }
};

/**
 * @brief Logout use case
 */
class LogoutUseCase {
private:
    static constexpr const char* TAG = "LogoutUC";
    IAuthRepository& _authRepo;

public:
    LogoutUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: Logout user (revoke token)
     */
    Result<void> execute(const String& token) {
        Logger::getInstance().info(TAG, "Logging out user...");

        auto result = _authRepo.revokeToken(token);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Logout failed: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "User logged out successfully");
        return Result<void>::ok();
    }
};

/**
 * @brief Get user permissions use case
 */
class GetUserPermissionsUseCase {
private:
    static constexpr const char* TAG = "GetPermissionsUC";
    IAuthRepository& _authRepo;

public:
    GetUserPermissionsUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: Get user permissions from token
     */
    Result<UserPermissionsDTO> execute(const String& token) {
        Logger::getInstance().trace(TAG, "Getting user permissions...");

        auto userResult = _authRepo.verifyToken(token);
        if (userResult.isError()) {
            return Result<UserPermissionsDTO>::error(userResult.getError());
        }

        User user = userResult.getValue();
        UserPermissionsDTO dto = UserPermissionsDTO::fromEntity(user);

        return Result<UserPermissionsDTO>::ok(dto);
    }
};

/**
 * @brief Delete user use case
 */
class DeleteUserUseCase {
private:
    static constexpr const char* TAG = "DeleteUserUC";
    IAuthRepository& _authRepo;

public:
    DeleteUserUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: Delete a user (admin only)
     */
    Result<void> execute(const String& adminToken, const String& usernameToDelete) {
        Logger::getInstance().info(TAG, "Deleting user: " + usernameToDelete);

        // Verify admin has permission
        auto adminResult = _authRepo.verifyToken(adminToken);
        if (adminResult.isError()) {
            return Result<void>::error("Unauthorized");
        }

        User admin = adminResult.getValue();
        if (!admin.canManageUsers()) {
            Logger::getInstance().warn(TAG, "User lacks permission to delete users");
            return Result<void>::error("Insufficient permissions");
        }

        // Delete user
        auto result = _authRepo.deleteUser(usernameToDelete);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to delete user: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "User deleted successfully: " + usernameToDelete);
        return Result<void>::ok();
    }
};

/**
 * @brief List users use case
 */
class ListUsersUseCase {
private:
    static constexpr const char* TAG = "ListUsersUC";
    IAuthRepository& _authRepo;

public:
    ListUsersUseCase(IAuthRepository& authRepo)
        : _authRepo(authRepo) {}

    /**
     * @brief Execute: List all users (admin only)
     */
    Result<std::vector<String>> execute(const String& adminToken) {
        Logger::getInstance().info(TAG, "Listing users...");

        // Verify admin has permission
        auto adminResult = _authRepo.verifyToken(adminToken);
        if (adminResult.isError()) {
            return Result<std::vector<String>>::error("Unauthorized");
        }

        User admin = adminResult.getValue();
        if (!admin.canManageUsers()) {
            Logger::getInstance().warn(TAG, "User lacks permission to list users");
            return Result<std::vector<String>>::error("Insufficient permissions");
        }

        // List users
        auto result = _authRepo.listUsers();
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to list users: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "Retrieved " + String(result.getValue().size()) + " users");
        return result;
    }
};

#endif // AUTH_USE_CASES_H
