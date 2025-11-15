/**
 * @file AuthHandlers.h
 * @brief HTTP handlers for authentication endpoints
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef AUTH_HANDLERS_H
#define AUTH_HANDLERS_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "../../application/ApplicationFacade.h"
#include "../http/HttpResponse.h"
#include "../middleware/AuthMiddleware.h"
#include "../../core/Logger.h"

/**
 * @brief Authentication HTTP Handlers
 */
class AuthHandlers {
private:
    static constexpr const char* TAG = "AuthHandlers";

public:
    /**
     * @brief POST /api/auth/login
     *
     * Authenticate user and receive JWT token.
     *
     * Body: {"username": "...", "password": "..."}
     */
    static void handleLogin(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        // Parse JSON body
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, data, len);

        if (error) {
            HttpResponse::sendBadRequest(request, "Invalid JSON");
            return;
        }

        String username = doc["username"] | "";
        String password = doc["password"] | "";

        if (username.length() == 0 || password.length() == 0) {
            HttpResponse::sendBadRequest(request, "Username and password required");
            return;
        }

        Logger::getInstance().info(TAG, "Login attempt: " + username);

        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().login(username, password);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        LoginResponseDTO response = result.getValue();

        if (!response.success) {
            HttpResponse::sendUnauthorized(request, response.message);
            return;
        }

        Logger::getInstance().info(TAG, "Login successful: " + username);
        HttpResponse::sendJson(request, response.toJson());
    }

    /**
     * @brief POST /api/auth/logout
     *
     * Logout user (revoke token).
     * Requires: valid token
     */
    static void handleLogout(AsyncWebServerRequest* request) {
        String token = HttpResponse::getAuthToken(request);

        if (token.length() == 0) {
            HttpResponse::sendBadRequest(request, "No token provided");
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().logout(token);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        HttpResponse::sendSuccessMessage(request, "Logged out successfully");
    }

    /**
     * @brief POST /api/auth/refresh
     *
     * Refresh JWT token.
     * Requires: valid (possibly expired) token
     */
    static void handleRefresh(AsyncWebServerRequest* request) {
        String token = HttpResponse::getAuthToken(request);

        if (token.length() == 0) {
            HttpResponse::sendBadRequest(request, "No token provided");
            return;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().refreshToken(token);

        if (result.isError()) {
            HttpResponse::sendUnauthorized(request, "Token refresh failed");
            return;
        }

        AuthTokenDTO newToken = result.getValue();
        HttpResponse::sendJson(request, newToken.toJson());
    }

    /**
     * @brief GET /api/auth/me
     *
     * Get current user info from token.
     * Requires: valid token
     */
    static void handleGetCurrentUser(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::verifyToken(request)) {
            return;
        }

        String token = HttpResponse::getAuthToken(request);
        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().verifyToken(token);

        if (result.isError()) {
            HttpResponse::sendUnauthorized(request);
            return;
        }

        UserDTO user = result.getValue();
        HttpResponse::sendJson(request, user.toJson());
    }

    /**
     * @brief GET /api/auth/permissions
     *
     * Get current user's permissions.
     * Requires: valid token
     */
    static void handleGetPermissions(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::verifyToken(request)) {
            return;
        }

        String token = HttpResponse::getAuthToken(request);
        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().getPermissions(token);

        if (result.isError()) {
            HttpResponse::sendUnauthorized(request);
            return;
        }

        UserPermissionsDTO perms = result.getValue();
        HttpResponse::sendJson(request, perms.toJson());
    }

    /**
     * @brief POST /api/auth/register
     *
     * Register new user.
     * Requires: manageUsers permission
     *
     * Body: {"username": "...", "password": "...", "role": "user|admin"}
     */
    static void handleRegister(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (!AuthMiddleware::authorize(request, "manageUsers")) {
            return;
        }

        // Parse JSON body
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, data, len);

        if (error) {
            HttpResponse::sendBadRequest(request, "Invalid JSON");
            return;
        }

        String username = doc["username"] | "";
        String password = doc["password"] | "";
        String roleStr = doc["role"] | "user";

        if (username.length() == 0 || password.length() == 0) {
            HttpResponse::sendBadRequest(request, "Username and password required");
            return;
        }

        // Parse role
        UserRole role = UserRole::USER;
        if (roleStr == "admin") {
            role = UserRole::ADMIN;
        } else if (roleStr == "guest") {
            role = UserRole::GUEST;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().registerUser(username, password, role);

        if (result.isError()) {
            HttpResponse::sendBadRequest(request, result.getError());
            return;
        }

        UserDTO user = result.getValue();
        Logger::getInstance().info(TAG, "User registered: " + username);

        HttpResponse::sendJson(request, user.toJson(), 201);
    }

    /**
     * @brief DELETE /api/auth/users/:username
     *
     * Delete user.
     * Requires: manageUsers permission
     */
    static void handleDeleteUser(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "manageUsers")) {
            return;
        }

        String username = HttpResponse::getParam(request, "username", "");

        if (username.length() == 0) {
            HttpResponse::sendBadRequest(request, "Username required");
            return;
        }

        String token = HttpResponse::getAuthToken(request);
        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().deleteUser(token, username);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        Logger::getInstance().info(TAG, "User deleted: " + username);
        HttpResponse::sendSuccessMessage(request, "User deleted");
    }

    /**
     * @brief GET /api/auth/users
     *
     * List all users.
     * Requires: manageUsers permission
     */
    static void handleListUsers(AsyncWebServerRequest* request) {
        if (!AuthMiddleware::authorize(request, "manageUsers")) {
            return;
        }

        String token = HttpResponse::getAuthToken(request);
        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().listUsers(token);

        if (result.isError()) {
            HttpResponse::sendInternalError(request, result.getError());
            return;
        }

        std::vector<String> users = result.getValue();

        // Build JSON array
        String json = "[";
        for (size_t i = 0; i < users.size(); i++) {
            json += "\"" + users[i] + "\"";
            if (i < users.size() - 1) {
                json += ",";
            }
        }
        json += "]";

        HttpResponse::sendSuccess(request, json);
    }
};

#endif // AUTH_HANDLERS_H
