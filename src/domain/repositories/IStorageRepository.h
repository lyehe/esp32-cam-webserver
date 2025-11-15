/**
 * @file IStorageRepository.h
 * @brief Interface for storage repository
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef I_STORAGE_REPOSITORY_H
#define I_STORAGE_REPOSITORY_H

#include "../entities/CameraSettings.h"
#include "../entities/User.h"
#include "../../core/Result.h"
#include <ArduinoJson.h>

/**
 * @brief Storage repository interface
 *
 * Provides persistent storage operations for application data.
 */
class IStorageRepository {
public:
    virtual ~IStorageRepository() = default;

    /**
     * @brief Initialize storage system
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> initialize() = 0;

    /**
     * @brief Check if storage is initialized
     *
     * @return bool True if initialized, false otherwise
     */
    virtual bool isInitialized() const = 0;

    // ========================================================================
    // Camera Settings Storage
    // ========================================================================

    /**
     * @brief Save camera settings
     *
     * @param settings Settings to save
     * @return Result<void> Success or error
     */
    virtual Result<void> saveCameraSettings(const CameraSettings& settings) = 0;

    /**
     * @brief Load camera settings
     *
     * @return Result<CameraSettings> Settings or error
     */
    virtual Result<CameraSettings> loadCameraSettings() = 0;

    /**
     * @brief Delete camera settings
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> deleteCameraSettings() = 0;

    // ========================================================================
    // User Storage
    // ========================================================================

    /**
     * @brief Save user
     *
     * @param user User to save
     * @return Result<void> Success or error
     */
    virtual Result<void> saveUser(const User& user) = 0;

    /**
     * @brief Load user by username
     *
     * @param username Username to lookup
     * @return Result<User> User or error
     */
    virtual Result<User> loadUser(const String& username) = 0;

    /**
     * @brief Delete user
     *
     * @param username Username to delete
     * @return Result<void> Success or error
     */
    virtual Result<void> deleteUser(const String& username) = 0;

    /**
     * @brief Check if user exists
     *
     * @param username Username to check
     * @return bool True if exists, false otherwise
     */
    virtual bool userExists(const String& username) = 0;

    /**
     * @brief Get all usernames
     *
     * @param usernames Array to fill with usernames
     * @param count In: array size, Out: number of usernames
     * @return Result<void> Success or error
     */
    virtual Result<void> getAllUsernames(String* usernames, size_t& count) = 0;

    // ========================================================================
    // Generic Key-Value Storage
    // ========================================================================

    /**
     * @brief Save string value
     *
     * @param key Key
     * @param value Value
     * @return Result<void> Success or error
     */
    virtual Result<void> saveString(const String& key, const String& value) = 0;

    /**
     * @brief Load string value
     *
     * @param key Key
     * @return Result<String> Value or error
     */
    virtual Result<String> loadString(const String& key) = 0;

    /**
     * @brief Save integer value
     *
     * @param key Key
     * @param value Value
     * @return Result<void> Success or error
     */
    virtual Result<void> saveInt(const String& key, int value) = 0;

    /**
     * @brief Load integer value
     *
     * @param key Key
     * @return Result<int> Value or error
     */
    virtual Result<int> loadInt(const String& key) = 0;

    /**
     * @brief Check if key exists
     *
     * @param key Key to check
     * @return bool True if exists, false otherwise
     */
    virtual bool exists(const String& key) = 0;

    /**
     * @brief Delete key
     *
     * @param key Key to delete
     * @return Result<void> Success or error
     */
    virtual Result<void> deleteKey(const String& key) = 0;

    // ========================================================================
    // File Operations
    // ========================================================================

    /**
     * @brief Read file contents
     *
     * @param path File path
     * @return Result<String> File contents or error
     */
    virtual Result<String> readFile(const String& path) = 0;

    /**
     * @brief Write file contents
     *
     * @param path File path
     * @param contents Contents to write
     * @return Result<void> Success or error
     */
    virtual Result<void> writeFile(const String& path, const String& contents) = 0;

    /**
     * @brief Delete file
     *
     * @param path File path
     * @return Result<void> Success or error
     */
    virtual Result<void> deleteFile(const String& path) = 0;

    /**
     * @brief Check if file exists
     *
     * @param path File path
     * @return bool True if exists, false otherwise
     */
    virtual bool fileExists(const String& path) = 0;

    /**
     * @brief Get file size
     *
     * @param path File path
     * @return Result<size_t> File size or error
     */
    virtual Result<size_t> getFileSize(const String& path) = 0;

    // ========================================================================
    // System Information
    // ========================================================================

    /**
     * @brief Get total storage space
     *
     * @return size_t Total space in bytes
     */
    virtual size_t getTotalSpace() const = 0;

    /**
     * @brief Get used storage space
     *
     * @return size_t Used space in bytes
     */
    virtual size_t getUsedSpace() const = 0;

    /**
     * @brief Get free storage space
     *
     * @return size_t Free space in bytes
     */
    virtual size_t getFreeSpace() const = 0;

    /**
     * @brief Format storage (erase all data)
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> format() = 0;
};

#endif // I_STORAGE_REPOSITORY_H
