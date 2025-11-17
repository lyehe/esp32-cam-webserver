/**
 * @file ScopedMutex.h
 * @brief RAII wrapper for FreeRTOS mutexes
 *
 * Provides automatic mutex lock/unlock using C++ RAII pattern.
 * Eliminates manual xSemaphoreGive calls and prevents lock leaks.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef SCOPED_MUTEX_H
#define SCOPED_MUTEX_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief RAII mutex guard for automatic lock/unlock
 *
 * Usage:
 * @code
 * {
 *     ScopedMutex lock(_mutex, 1000);
 *     if (!lock.isAcquired()) {
 *         return Result<T>::error("Timeout");
 *     }
 *     // ... critical section ...
 * }  // Mutex automatically released here
 * @endcode
 */
class ScopedMutex {
private:
    SemaphoreHandle_t _mutex;
    bool _acquired;

    // Non-copyable
    ScopedMutex(const ScopedMutex&) = delete;
    ScopedMutex& operator=(const ScopedMutex&) = delete;

public:
    /**
     * @brief Construct and acquire mutex
     *
     * @param mutex FreeRTOS mutex handle
     * @param timeout_ms Timeout in milliseconds (default 1000ms)
     */
    explicit ScopedMutex(SemaphoreHandle_t mutex, uint32_t timeout_ms = 1000)
        : _mutex(mutex)
        , _acquired(false) {

        if (_mutex) {
            _acquired = (xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
        }
    }

    /**
     * @brief Destructor - automatically releases mutex
     */
    ~ScopedMutex() {
        if (_acquired && _mutex) {
            xSemaphoreGive(_mutex);
        }
    }

    /**
     * @brief Check if mutex was successfully acquired
     *
     * @return true if mutex is held, false if timeout or invalid
     */
    bool isAcquired() const {
        return _acquired;
    }

    /**
     * @brief Explicit operator bool for convenient checking
     *
     * Usage: if (ScopedMutex lock(_mutex)) { ... }
     */
    explicit operator bool() const {
        return _acquired;
    }
};

#endif // SCOPED_MUTEX_H
