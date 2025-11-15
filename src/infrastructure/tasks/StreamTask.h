/**
 * @file StreamTask.h
 * @brief FreeRTOS task for high-performance streaming
 *
 * Dedicated task running on Core 0 for camera capture and streaming.
 * Provides better performance than polling in main loop.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef STREAM_TASK_H
#define STREAM_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../application/ApplicationFacade.h"
#include "../core/Logger.h"

/**
 * @brief High-performance streaming task
 */
class StreamTask {
private:
    static constexpr const char* TAG = "StreamTask";
    static constexpr uint32_t TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t TASK_PRIORITY = 2; // Higher priority
    static constexpr BaseType_t TASK_CORE = 0; // Core 0 (same as camera)

    static TaskHandle_t _taskHandle;
    static bool _running;

    /**
     * @brief Streaming task function
     */
    static void streamTaskFunction(void* parameter) {
        Logger::getInstance().info(TAG, "Stream task started on core " + String(xPortGetCoreID()));

        auto& app = ApplicationFacade::getInstance();
        TickType_t lastWakeTime = xTaskGetTickCount();
        const TickType_t frequency = pdMS_TO_TICKS(33); // ~30 FPS

        while (_running) {
            // Stream to all active streams
            app.stream().streamAllActive();

            // Precise timing control
            vTaskDelayUntil(&lastWakeTime, frequency);
        }

        Logger::getInstance().info(TAG, "Stream task stopped");
        vTaskDelete(NULL);
    }

public:
    /**
     * @brief Start streaming task
     */
    static bool start() {
        if (_taskHandle != NULL) {
            Logger::getInstance().warn(TAG, "Stream task already running");
            return false;
        }

        _running = true;

        BaseType_t result = xTaskCreatePinnedToCore(
            streamTaskFunction,
            "StreamTask",
            TASK_STACK_SIZE,
            NULL,
            TASK_PRIORITY,
            &_taskHandle,
            TASK_CORE
        );

        if (result != pdPASS) {
            Logger::getInstance().error(TAG, "Failed to create stream task");
            _running = false;
            return false;
        }

        Logger::getInstance().info(TAG, "Stream task created successfully");
        return true;
    }

    /**
     * @brief Stop streaming task
     */
    static void stop() {
        if (_taskHandle == NULL) {
            return;
        }

        _running = false;
        delay(100); // Allow task to finish
        _taskHandle = NULL;
    }

    /**
     * @brief Check if task is running
     */
    static bool isRunning() {
        return _running && _taskHandle != NULL;
    }

    /**
     * @brief Set streaming frequency (FPS)
     */
    static void setFPS(uint8_t fps) {
        if (fps < 1 || fps > 60) {
            return;
        }

        // This would require modifying the task
        // For now, restart task with new frequency
        Logger::getInstance().info(TAG, "FPS change requires task restart");
    }
};

// Static member initialization
TaskHandle_t StreamTask::_taskHandle = NULL;
bool StreamTask::_running = false;

#endif // STREAM_TASK_H
