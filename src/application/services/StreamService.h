/**
 * @file StreamService.h
 * @brief Application service for streaming operations
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef STREAM_SERVICE_H
#define STREAM_SERVICE_H

#include "../usecases/StreamUseCases.h"
#include "../../core/Logger.h"
#include <map>

/**
 * @brief Stream Application Service
 *
 * Coordinates streaming operations and manages active streams.
 */
class StreamService {
private:
    static constexpr const char* TAG = "StreamSvc";

    IStreamRepository& _streamRepo;
    ICameraRepository& _cameraRepo;

    // Use cases
    StartStreamUseCase _startStreamUC;
    StopStreamUseCase _stopStreamUC;
    AddStreamClientUseCase _addClientUC;
    RemoveStreamClientUseCase _removeClientUC;
    BroadcastFrameUseCase _broadcastFrameUC;
    GetStreamStatsUseCase _getStatsUC;
    SetStreamFrameRateUseCase _setFrameRateUC;
    PauseStreamUseCase _pauseStreamUC;
    ResumeStreamUseCase _resumeStreamUC;

    // Active streams tracking
    std::map<uint32_t, uint32_t> _activeStreams; // streamId -> startTime

public:
    /**
     * @brief Constructor
     */
    StreamService(IStreamRepository& streamRepo, ICameraRepository& cameraRepo)
        : _streamRepo(streamRepo),
          _cameraRepo(cameraRepo),
          _startStreamUC(streamRepo, cameraRepo),
          _stopStreamUC(streamRepo),
          _addClientUC(streamRepo),
          _removeClientUC(streamRepo),
          _broadcastFrameUC(streamRepo, cameraRepo),
          _getStatsUC(streamRepo),
          _setFrameRateUC(streamRepo),
          _pauseStreamUC(streamRepo),
          _resumeStreamUC(streamRepo) {
        Logger::getInstance().info(TAG, "Stream service initialized");
    }

    /**
     * @brief Start a new stream
     */
    Result<uint32_t> startStream() {
        auto result = _startStreamUC.execute();
        if (result.isOk()) {
            uint32_t streamId = result.getValue();
            _activeStreams[streamId] = millis();
        }
        return result;
    }

    /**
     * @brief Stop an active stream
     */
    Result<void> stopStream(uint32_t streamId) {
        auto result = _stopStreamUC.execute(streamId);
        if (result.isOk()) {
            _activeStreams.erase(streamId);
        }
        return result;
    }

    /**
     * @brief Stop all active streams
     */
    Result<void> stopAllStreams() {
        Logger::getInstance().info(TAG, "Stopping all streams...");

        // Copy stream IDs to avoid modifying container during iteration
        std::vector<uint32_t> streamIds;
        for (const auto& pair : _activeStreams) {
            streamIds.push_back(pair.first);
        }

        // Stop each stream
        for (uint32_t streamId : streamIds) {
            stopStream(streamId);
        }

        Logger::getInstance().info(TAG, "All streams stopped");
        return Result<void>::ok();
    }

    /**
     * @brief Add client to stream
     */
    Result<uint32_t> addClient(uint32_t streamId, IPAddress clientIp, uint16_t clientPort) {
        return _addClientUC.execute(streamId, clientIp, clientPort);
    }

    /**
     * @brief Remove client from stream
     */
    Result<void> removeClient(uint32_t streamId, uint32_t clientId) {
        return _removeClientUC.execute(streamId, clientId);
    }

    /**
     * @brief Broadcast frame to stream (capture + send)
     */
    Result<void> broadcastFrame(uint32_t streamId) {
        return _broadcastFrameUC.execute(streamId);
    }

    /**
     * @brief Get stream statistics
     */
    Result<StreamStatsDTO> getStats(uint32_t streamId) {
        auto result = _getStatsUC.execute(streamId);

        // Add uptime if available
        if (result.isOk() && _activeStreams.count(streamId) > 0) {
            StreamStatsDTO dto = result.getValue();
            dto.uptime = (millis() - _activeStreams[streamId]) / 1000;
            return Result<StreamStatsDTO>::ok(dto);
        }

        return result;
    }

    /**
     * @brief Set target frame rate
     */
    Result<void> setFrameRate(uint32_t streamId, float fps) {
        return _setFrameRateUC.execute(streamId, fps);
    }

    /**
     * @brief Pause stream
     */
    Result<void> pauseStream(uint32_t streamId) {
        return _pauseStreamUC.execute(streamId);
    }

    /**
     * @brief Resume stream
     */
    Result<void> resumeStream(uint32_t streamId) {
        return _resumeStreamUC.execute(streamId);
    }

    /**
     * @brief Check if stream is active
     */
    bool isStreamActive(uint32_t streamId) {
        return _streamRepo.isStreamActive(streamId);
    }

    /**
     * @brief Get active client count
     */
    size_t getActiveClientCount(uint32_t streamId) {
        return _streamRepo.getActiveClientCount(streamId);
    }

    /**
     * @brief Get number of active streams
     */
    size_t getActiveStreamCount() {
        return _activeStreams.size();
    }

    /**
     * @brief Stream broadcasting loop
     *
     * Call this repeatedly to stream frames to all clients.
     * Typically called from a FreeRTOS task.
     */
    void streamLoop(uint32_t streamId) {
        if (!isStreamActive(streamId)) {
            return;
        }

        // Broadcast frame to all clients
        auto result = broadcastFrame(streamId);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Broadcast error: " + result.getError());
        }
    }

    /**
     * @brief Stream all active streams
     *
     * Broadcasts frames to all active streams.
     */
    void streamAllActive() {
        for (const auto& pair : _activeStreams) {
            streamLoop(pair.first);
        }
    }
};

#endif // STREAM_SERVICE_H
