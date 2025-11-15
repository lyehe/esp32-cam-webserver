/**
 * @file SPIFFSStorageRepository.h
 * @brief SPIFFS-based storage repository implementation
 *
 * Implements IStorageRepository using SPIFFS filesystem.
 * Handles persistent storage of settings, users, and files.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef SPIFFS_STORAGE_REPOSITORY_H
#define SPIFFS_STORAGE_REPOSITORY_H

#include "../../domain/repositories/IStorageRepository.h"
#include "../../core/Logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

/**
 * @brief SPIFFS Storage Repository
 *
 * Persists data to SPIFFS filesystem.
 * Uses JSON for structured data serialization.
 */
class SPIFFSStorageRepository : public IStorageRepository {
private:
    static constexpr const char* TAG = "SPIFFSRepo";
    static constexpr const char* CAMERA_SETTINGS_PATH = "/camera_settings.json";
    static constexpr const char* USERS_DIR = "/users/";

    bool _initialized;

    /**
     * @brief Ensure directory exists
     */
    Result<void> ensureDirectoryExists(const String& path) {
        // SPIFFS doesn't have real directories, but we can create marker files
        // or just rely on path naming convention
        return Result<void>::ok();
    }

    /**
     * @brief Serialize CameraSettings to JSON
     */
    String serializeCameraSettings(const CameraSettings& settings) {
        StaticJsonDocument<2048> doc;

        // Basic settings
        doc["resolution"] = static_cast<int>(settings.getResolution().getFrameSize());
        doc["pixelFormat"] = static_cast<int>(settings.getPixelFormat().getFormat());
        doc["quality"] = settings.getQuality();
        doc["frameBufferCount"] = settings.getFrameBufferCount();

        // Image adjustments
        doc["brightness"] = settings.getBrightness();
        doc["contrast"] = settings.getContrast();
        doc["saturation"] = settings.getSaturation();
        doc["sharpness"] = settings.getSharpness();
        doc["specialEffect"] = settings.getSpecialEffect();

        // White balance
        doc["whiteBalanceMode"] = settings.getWhiteBalanceMode();
        doc["autoWhiteBalance"] = settings.isAutoWhiteBalance();
        doc["autoWhiteBalanceGain"] = settings.isAutoWhiteBalanceGain();

        // Exposure
        doc["autoExposureControl"] = settings.isAutoExposureControl();
        doc["autoExposureControl2"] = settings.isAutoExposureControl2();
        doc["autoExposureLevel"] = settings.getAutoExposureLevel();
        doc["autoExposureValue"] = settings.getAutoExposureValue();

        // Gain
        doc["autoGainControl"] = settings.isAutoGainControl();
        doc["autoGainValue"] = settings.getAutoGainValue();
        doc["gainCeiling"] = settings.getGainCeiling();

        // Lens corrections
        doc["lensCorrectionEnabled"] = settings.isLensCorrectionEnabled();
        doc["blackPixelCorrection"] = settings.isBlackPixelCorrection();
        doc["whitePixelCorrection"] = settings.isWhitePixelCorrection();
        doc["gammaCorrection"] = settings.isGammaCorrection();

        // Image orientation
        doc["verticalFlip"] = settings.isVerticalFlip();
        doc["horizontalMirror"] = settings.isHorizontalMirror();
        doc["rotation"] = settings.getRotation();

        // Advanced
        doc["downscaleEnabled"] = settings.isDownscaleEnabled();
        doc["colorBarEnabled"] = settings.isColorBarEnabled();

        String output;
        serializeJson(doc, output);
        return output;
    }

    /**
     * @brief Deserialize JSON to CameraSettings
     */
    Result<CameraSettings> deserializeCameraSettings(const String& json) {
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            return Result<CameraSettings>::error("JSON parse error: " + String(error.c_str()));
        }

        CameraSettings settings;

        // Basic settings
        if (doc.containsKey("resolution")) {
            FrameSize fs = static_cast<FrameSize>(doc["resolution"].as<int>());
            settings.setResolution(Resolution(fs));
        }
        if (doc.containsKey("pixelFormat")) {
            Format fmt = static_cast<Format>(doc["pixelFormat"].as<int>());
            settings.setPixelFormat(PixelFormat(fmt));
        }
        if (doc.containsKey("quality")) {
            settings.setQuality(doc["quality"]);
        }
        if (doc.containsKey("frameBufferCount")) {
            settings.setFrameBufferCount(doc["frameBufferCount"]);
        }

        // Image adjustments
        if (doc.containsKey("brightness")) settings.setBrightness(doc["brightness"]);
        if (doc.containsKey("contrast")) settings.setContrast(doc["contrast"]);
        if (doc.containsKey("saturation")) settings.setSaturation(doc["saturation"]);
        if (doc.containsKey("sharpness")) settings.setSharpness(doc["sharpness"]);
        if (doc.containsKey("specialEffect")) settings.setSpecialEffect(doc["specialEffect"]);

        // White balance
        if (doc.containsKey("whiteBalanceMode")) settings.setWhiteBalanceMode(doc["whiteBalanceMode"]);
        if (doc.containsKey("autoWhiteBalance")) settings.setAutoWhiteBalance(doc["autoWhiteBalance"]);
        if (doc.containsKey("autoWhiteBalanceGain")) settings.setAutoWhiteBalanceGain(doc["autoWhiteBalanceGain"]);

        // Exposure
        if (doc.containsKey("autoExposureControl")) settings.setAutoExposureControl(doc["autoExposureControl"]);
        if (doc.containsKey("autoExposureControl2")) settings.setAutoExposureControl2(doc["autoExposureControl2"]);
        if (doc.containsKey("autoExposureLevel")) settings.setAutoExposureLevel(doc["autoExposureLevel"]);
        if (doc.containsKey("autoExposureValue")) settings.setAutoExposureValue(doc["autoExposureValue"]);

        // Gain
        if (doc.containsKey("autoGainControl")) settings.setAutoGainControl(doc["autoGainControl"]);
        if (doc.containsKey("autoGainValue")) settings.setAutoGainValue(doc["autoGainValue"]);
        if (doc.containsKey("gainCeiling")) settings.setGainCeiling(doc["gainCeiling"]);

        // Lens corrections
        if (doc.containsKey("lensCorrectionEnabled")) settings.setLensCorrectionEnabled(doc["lensCorrectionEnabled"]);
        if (doc.containsKey("blackPixelCorrection")) settings.setBlackPixelCorrection(doc["blackPixelCorrection"]);
        if (doc.containsKey("whitePixelCorrection")) settings.setWhitePixelCorrection(doc["whitePixelCorrection"]);
        if (doc.containsKey("gammaCorrection")) settings.setGammaCorrection(doc["gammaCorrection"]);

        // Image orientation
        if (doc.containsKey("verticalFlip")) settings.setVerticalFlip(doc["verticalFlip"]);
        if (doc.containsKey("horizontalMirror")) settings.setHorizontalMirror(doc["horizontalMirror"]);
        if (doc.containsKey("rotation")) settings.setRotation(doc["rotation"]);

        // Advanced
        if (doc.containsKey("downscaleEnabled")) settings.setDownscaleEnabled(doc["downscaleEnabled"]);
        if (doc.containsKey("colorBarEnabled")) settings.setColorBarEnabled(doc["colorBarEnabled"]);

        return Result<CameraSettings>::ok(settings);
    }

    /**
     * @brief Serialize User to JSON
     */
    String serializeUser(const User& user) {
        StaticJsonDocument<1024> doc;

        doc["id"] = user.getId();
        doc["username"] = user.getUsername();
        doc["passwordHash"] = user.getPasswordHash();
        doc["role"] = static_cast<int>(user.getRole());
        doc["createdAt"] = user.getCreatedAt();
        doc["loginCount"] = user.getLoginCount();
        doc["lastLoginAt"] = user.getLastLoginAt();

        String output;
        serializeJson(doc, output);
        return output;
    }

    /**
     * @brief Deserialize JSON to User
     */
    Result<User> deserializeUser(const String& json) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            return Result<User>::error("JSON parse error: " + String(error.c_str()));
        }

        // Create user from stored data
        String username = doc["username"] | "";
        String passwordHash = doc["passwordHash"] | "";
        UserRole role = static_cast<UserRole>(doc["role"].as<int>());

        // Use factory method with dummy credentials (we'll set hash later)
        auto credResult = Credentials::create(username, "dummy123"); // Dummy password
        if (credResult.isError()) {
            return Result<User>::error(credResult.getError());
        }

        auto userResult = User::create(credResult.getValue(), role);
        if (userResult.isError()) {
            return Result<User>::error(userResult.getError());
        }

        User user = userResult.getValue();

        // Now set the actual password hash and other fields via reflection
        // Since User doesn't expose setters, we need to reconstruct it properly
        // This is a limitation of the current design - we'll document this

        // TODO: Add deserialization constructor to User entity
        // For now, return error
        return Result<User>::error("User deserialization not yet fully implemented");
    }

public:
    /**
     * @brief Constructor
     */
    SPIFFSStorageRepository()
        : _initialized(false) {}

    /**
     * @brief Initialize SPIFFS
     */
    Result<void> initialize() override {
        Logger::getInstance().info(TAG, "Initializing SPIFFS storage");

        if (!SPIFFS.begin(true)) { // true = format on fail
            Logger::getInstance().error(TAG, "SPIFFS mount failed");
            return Result<void>::error("SPIFFS mount failed");
        }

        _initialized = true;

        // Log filesystem info
        size_t total = SPIFFS.totalBytes();
        size_t used = SPIFFS.usedBytes();
        Logger::getInstance().info(TAG, "SPIFFS: " + String(used) + " / " + String(total) + " bytes used");

        return Result<void>::ok();
    }

    /**
     * @brief Save camera settings
     */
    Result<void> saveCameraSettings(const CameraSettings& settings) override {
        if (!_initialized) {
            return Result<void>::error("Storage not initialized");
        }

        String json = serializeCameraSettings(settings);

        File file = SPIFFS.open(CAMERA_SETTINGS_PATH, "w");
        if (!file) {
            return Result<void>::error("Failed to open file for writing");
        }

        size_t written = file.print(json);
        file.close();

        if (written == 0) {
            return Result<void>::error("Failed to write settings");
        }

        Logger::getInstance().info(TAG, "Camera settings saved");
        return Result<void>::ok();
    }

    /**
     * @brief Load camera settings
     */
    Result<CameraSettings> loadCameraSettings() override {
        if (!_initialized) {
            return Result<CameraSettings>::error("Storage not initialized");
        }

        if (!SPIFFS.exists(CAMERA_SETTINGS_PATH)) {
            Logger::getInstance().info(TAG, "No saved camera settings, using defaults");
            return Result<CameraSettings>::ok(CameraSettings());
        }

        File file = SPIFFS.open(CAMERA_SETTINGS_PATH, "r");
        if (!file) {
            return Result<CameraSettings>::error("Failed to open settings file");
        }

        String json = file.readString();
        file.close();

        return deserializeCameraSettings(json);
    }

    /**
     * @brief Save user
     */
    Result<void> saveUser(const User& user) override {
        if (!_initialized) {
            return Result<void>::error("Storage not initialized");
        }

        String filename = String(USERS_DIR) + user.getUsername() + ".json";
        String json = serializeUser(user);

        File file = SPIFFS.open(filename, "w");
        if (!file) {
            return Result<void>::error("Failed to open user file for writing");
        }

        size_t written = file.print(json);
        file.close();

        if (written == 0) {
            return Result<void>::error("Failed to write user data");
        }

        Logger::getInstance().info(TAG, "User saved: " + user.getUsername());
        return Result<void>::ok();
    }

    /**
     * @brief Load user
     */
    Result<User> loadUser(const String& username) override {
        if (!_initialized) {
            return Result<User>::error("Storage not initialized");
        }

        String filename = String(USERS_DIR) + username + ".json";

        if (!SPIFFS.exists(filename)) {
            return Result<User>::error("User not found");
        }

        File file = SPIFFS.open(filename, "r");
        if (!file) {
            return Result<User>::error("Failed to open user file");
        }

        String json = file.readString();
        file.close();

        return deserializeUser(json);
    }

    /**
     * @brief Delete user
     */
    Result<void> deleteUser(const String& username) override {
        if (!_initialized) {
            return Result<void>::error("Storage not initialized");
        }

        String filename = String(USERS_DIR) + username + ".json";

        if (!SPIFFS.exists(filename)) {
            return Result<void>::error("User not found");
        }

        if (!SPIFFS.remove(filename)) {
            return Result<void>::error("Failed to delete user file");
        }

        Logger::getInstance().info(TAG, "User deleted: " + username);
        return Result<void>::ok();
    }

    /**
     * @brief Check if user exists
     */
    bool userExists(const String& username) override {
        if (!_initialized) {
            return false;
        }

        String filename = String(USERS_DIR) + username + ".json";
        return SPIFFS.exists(filename);
    }

    /**
     * @brief List all users
     */
    Result<std::vector<String>> listUsers() override {
        if (!_initialized) {
            return Result<std::vector<String>>::error("Storage not initialized");
        }

        std::vector<String> users;

        File root = SPIFFS.open(USERS_DIR);
        if (!root || !root.isDirectory()) {
            return Result<std::vector<String>>::ok(users); // Empty list
        }

        File file = root.openNextFile();
        while (file) {
            String filename = file.name();
            if (filename.endsWith(".json")) {
                // Extract username from filename
                int lastSlash = filename.lastIndexOf('/');
                int dotJson = filename.lastIndexOf(".json");
                if (lastSlash >= 0 && dotJson > lastSlash) {
                    String username = filename.substring(lastSlash + 1, dotJson);
                    users.push_back(username);
                }
            }
            file = root.openNextFile();
        }

        return Result<std::vector<String>>::ok(users);
    }

    /**
     * @brief Read file
     */
    Result<String> readFile(const String& path) override {
        if (!_initialized) {
            return Result<String>::error("Storage not initialized");
        }

        if (!SPIFFS.exists(path)) {
            return Result<String>::error("File not found");
        }

        File file = SPIFFS.open(path, "r");
        if (!file) {
            return Result<String>::error("Failed to open file");
        }

        String contents = file.readString();
        file.close();

        return Result<String>::ok(contents);
    }

    /**
     * @brief Write file
     */
    Result<void> writeFile(const String& path, const String& contents) override {
        if (!_initialized) {
            return Result<void>::error("Storage not initialized");
        }

        File file = SPIFFS.open(path, "w");
        if (!file) {
            return Result<void>::error("Failed to open file for writing");
        }

        size_t written = file.print(contents);
        file.close();

        if (written == 0) {
            return Result<void>::error("Failed to write file");
        }

        Logger::getInstance().debug(TAG, "File written: " + path);
        return Result<void>::ok();
    }

    /**
     * @brief Delete file
     */
    Result<void> deleteFile(const String& path) override {
        if (!_initialized) {
            return Result<void>::error("Storage not initialized");
        }

        if (!SPIFFS.exists(path)) {
            return Result<void>::error("File not found");
        }

        if (!SPIFFS.remove(path)) {
            return Result<void>::error("Failed to delete file");
        }

        Logger::getInstance().debug(TAG, "File deleted: " + path);
        return Result<void>::ok();
    }

    /**
     * @brief Check if file exists
     */
    bool fileExists(const String& path) override {
        if (!_initialized) {
            return false;
        }

        return SPIFFS.exists(path);
    }

    /**
     * @brief Get total space
     */
    size_t getTotalSpace() const override {
        if (!_initialized) {
            return 0;
        }

        return SPIFFS.totalBytes();
    }

    /**
     * @brief Get used space
     */
    size_t getUsedSpace() const override {
        if (!_initialized) {
            return 0;
        }

        return SPIFFS.usedBytes();
    }

    /**
     * @brief Get free space
     */
    size_t getFreeSpace() const override {
        if (!_initialized) {
            return 0;
        }

        return SPIFFS.totalBytes() - SPIFFS.usedBytes();
    }

    /**
     * @brief Format filesystem
     */
    Result<void> format() override {
        Logger::getInstance().warn(TAG, "Formatting SPIFFS...");

        if (!SPIFFS.format()) {
            return Result<void>::error("Format failed");
        }

        Logger::getInstance().info(TAG, "SPIFFS formatted");
        return Result<void>::ok();
    }
};

#endif // SPIFFS_STORAGE_REPOSITORY_H
