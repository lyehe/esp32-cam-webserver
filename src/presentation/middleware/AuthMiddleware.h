/**
 * @file AuthMiddleware.h
 * @brief Authentication middleware for HTTP requests
 *
 * Provides JWT token verification and authorization checks.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <ESPAsyncWebServer.h>
#include "../../application/ApplicationFacade.h"
#include "../http/HttpResponse.h"
#include "../../core/Logger.h"

/**
 * @brief Authentication Middleware
 *
 * Verifies JWT tokens and enforces permissions.
 */
class AuthMiddleware {
private:
    static constexpr const char* TAG = "AuthMW";

public:
    /**
     * @brief Verify token from request
     *
     * Checks Authorization header for valid JWT token.
     * Returns true if valid, false otherwise (sends error response).
     */
    static bool verifyToken(AsyncWebServerRequest* request) {
        String token = HttpResponse::getAuthToken(request);

        if (token.length() == 0) {
            Logger::getInstance().warn(TAG, "No token provided");
            HttpResponse::sendUnauthorized(request, "No authentication token provided");
            return false;
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().verifyToken(token);

        if (result.isError()) {
            Logger::getInstance().warn(TAG, "Token verification failed: " + result.getError());
            HttpResponse::sendUnauthorized(request, "Invalid or expired token");
            return false;
        }

        // Store user info in request for later use
        // Note: AsyncWebServer doesn't support request attributes directly
        // We'll need to verify token again in handlers if needed

        return true;
    }

    /**
     * @brief Verify token and check permission
     *
     * @param request HTTP request
     * @param permission Required permission (e.g., "viewStream", "modifySettings")
     * @return true if authorized, false otherwise
     */
    static bool authorize(AsyncWebServerRequest* request, const String& permission) {
        String token = HttpResponse::getAuthToken(request);

        if (token.length() == 0) {
            Logger::getInstance().warn(TAG, "No token provided");
            HttpResponse::sendUnauthorized(request, "No authentication token provided");
            return false;
        }

        auto& app = ApplicationFacade::getInstance();

        // Verify and check permission in one call
        auto result = app.auth().authorize(token, permission);

        if (result.isError()) {
            String error = result.getError();
            Logger::getInstance().warn(TAG, "Authorization failed: " + error);

            if (error.startsWith("Unauthorized")) {
                HttpResponse::sendUnauthorized(request, error);
            } else if (error.startsWith("Forbidden")) {
                HttpResponse::sendForbidden(request, error);
            } else {
                HttpResponse::sendUnauthorized(request, "Authentication failed");
            }

            return false;
        }

        return true;
    }

    /**
     * @brief Require authentication (any valid user)
     *
     * Use this as a filter for protected endpoints.
     */
    static ArRequestFilterFunction requireAuth() {
        return [](AsyncWebServerRequest* request) {
            return verifyToken(request);
        };
    }

    /**
     * @brief Require specific permission
     *
     * Use this as a filter for permission-protected endpoints.
     */
    static ArRequestFilterFunction requirePermission(const String& permission) {
        return [permission](AsyncWebServerRequest* request) {
            return authorize(request, permission);
        };
    }

    /**
     * @brief CORS preflight handler
     *
     * Handles OPTIONS requests for CORS.
     */
    static void handleCorsPreflightRequest(AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        response->addHeader("Access-Control-Max-Age", "86400");
        request->send(response);
    }

    /**
     * @brief Get username from token in request
     *
     * Helper to extract username for logging.
     */
    static String getUsername(AsyncWebServerRequest* request) {
        String token = HttpResponse::getAuthToken(request);

        if (token.length() == 0) {
            return "anonymous";
        }

        auto& app = ApplicationFacade::getInstance();
        auto result = app.auth().getUsernameFromToken(token);

        if (result.isError()) {
            return "unknown";
        }

        return result.getValue();
    }
};

#endif // AUTH_MIDDLEWARE_H
