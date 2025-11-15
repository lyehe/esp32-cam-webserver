/**
 * @file StreamUseCases.h
 * @brief Streaming-related use cases
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef STREAM_USE_CASES_H
#define STREAM_USE_CASES_H

#include "../../domain/repositories/IStreamRepository.h"
#include "../../domain/repositories/ICameraRepository.h"
#include "../../core/Logger.h"
#include "../../core/Result.h"
#include "../dtos/StreamDTO.h"

/**
 * @brief Start streaming use case
 */
class StartStreamUseCase {
private:
    static constexpr const char* TAG = "StartStreamUC";
    IStreamRepository& _streamRepo;
    ICameraRepository& _cameraRepo;

public:
    StartStreamUseCase(IStreamRepository& streamRepo, ICameraRepository& cameraRepo)
        : _streamRepo(streamRepo), _cameraRepo(cameraRepo) {}

    /**
     * @brief Execute: Start a new stream
     */
    Result<uint32_t> execute() {
        Logger::getInstance().info(TAG, "Starting stream...");

        if (!_cameraRepo.isInitialized()) {
            return Result<uint32_t>::error("Camera not initialized");
        }

        auto result = _streamRepo.startStream();
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to start stream: " + result.getError());
            return result;
        }

        uint32_t streamId = result.getValue();
        Logger::getInstance().info(TAG, "Stream started with ID: " + String(streamId));

        return Result<uint32_t>::ok(streamId);
    }
};

/**
 * @brief Stop streaming use case
 */
class StopStreamUseCase {
private:
    static constexpr const char* TAG = "StopStreamUC";
    IStreamRepository& _streamRepo;

public:
    StopStreamUseCase(IStreamRepository& streamRepo)
        : _streamRepo(streamRepo) {}

    /**
     * @brief Execute: Stop an active stream
     */
    Result<void> execute(uint32_t streamId) {
        Logger::getInstance().info(TAG, "Stopping stream " + String(streamId) + "...");

        auto result = _streamRepo.stopStream(streamId);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to stop stream: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "Stream stopped successfully");
        return Result<void>::ok();
    }
};

/**
 * @brief Add client to stream use case
 */
class AddStreamClientUseCase {
private:
    static constexpr const char* TAG = "AddClientUC";
    IStreamRepository& _streamRepo;

public:
    AddStreamClientUseCase(IStreamRepository& streamRepo)
        : _streamRepo(streamRepo) {}

    /**
     * @brief Execute: Add client to stream
     */
    Result<uint32_t> execute(uint32_t streamId, IPAddress clientIp, uint16_t clientPort) {
        Logger::getInstance().info(TAG, "Adding client to stream " + String(streamId) +
                                       ": " + clientIp.toString());

        auto result = _streamRepo.addClient(streamId, clientIp, clientPort);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to add client: " + result.getError());
            return result;
        }

        uint32_t clientId = result.getValue();
        Logger::getInstance().info(TAG, "Client added with ID: " + String(clientId));

        return Result<uint32_t>::ok(clientId);
    }
};

/**
 * @brief Remove client from stream use case
 */
class RemoveStreamClientUseCase {
private:
    static constexpr const char* TAG = "RemoveClientUC";
    IStreamRepository& _streamRepo;

public:
    RemoveStreamClientUseCase(IStreamRepository& streamRepo)
        : _streamRepo(streamRepo) {}

    /**
     * @brief Execute: Remove client from stream
     */
    Result<void> execute(uint32_t streamId, uint32_t clientId) {
        Logger::getInstance().info(TAG, "Removing client " + String(clientId) +
                                       " from stream " + String(streamId));

        auto result = _streamRepo.removeClient(streamId, clientId);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to remove client: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "Client removed successfully");
        return Result<void>::ok();
    }
};

/**
 * @brief Broadcast frame to stream use case
 */
class BroadcastFrameUseCase {
private:
    static constexpr const char* TAG = "BroadcastFrameUC";
    IStreamRepository& _streamRepo;
    ICameraRepository& _cameraRepo;

public:
    BroadcastFrameUseCase(IStreamRepository& streamRepo, ICameraRepository& cameraRepo)
        : _streamRepo(streamRepo), _cameraRepo(cameraRepo) {}

    /**
     * @brief Execute: Capture and broadcast frame to stream
     */
    Result<void> execute(uint32_t streamId) {
        Logger::getInstance().trace(TAG, "Broadcasting frame to stream " + String(streamId));

        // Check if stream is active
        if (!_streamRepo.isStreamActive(streamId)) {
            return Result<void>::error("Stream not active");
        }

        // Capture frame from camera
        auto captureResult = _cameraRepo.captureFrame();
        if (captureResult.isError()) {
            Logger::getInstance().error(TAG, "Frame capture failed: " + captureResult.getError());
            return Result<void>::error(captureResult.getError());
        }

        Frame frame = captureResult.getValue();

        // Broadcast to all clients
        auto broadcastResult = _streamRepo.broadcastFrame(streamId, frame);

        // Release frame (important!)
        _cameraRepo.releaseFrame(frame);

        if (broadcastResult.isError()) {
            Logger::getInstance().error(TAG, "Broadcast failed: " + broadcastResult.getError());
            return broadcastResult;
        }

        return Result<void>::ok();
    }
};

/**
 * @brief Get stream statistics use case
 */
class GetStreamStatsUseCase {
private:
    static constexpr const char* TAG = "GetStreamStatsUC";
    IStreamRepository& _streamRepo;

public:
    GetStreamStatsUseCase(IStreamRepository& streamRepo)
        : _streamRepo(streamRepo) {}

    /**
     * @brief Execute: Get stream statistics
     */
    Result<StreamStatsDTO> execute(uint32_t streamId) {
        Logger::getInstance().trace(TAG, "Getting stats for stream " + String(streamId));

        auto result = _streamRepo.getStreamStats(streamId);
        if (result.isError()) {
            return Result<StreamStatsDTO>::error(result.getError());
        }

        Stream stream = result.getValue();

        // Convert to DTO
        StreamStatsDTO dto;
        dto.streamId = streamId;
        dto.active = _streamRepo.isStreamActive(streamId);
        dto.clientCount = _streamRepo.getActiveClientCount(streamId);
        dto.targetFPS = stream.getTargetFPS();
        dto.averageFPS = stream.getAverageFPS();
        // dto.uptime would need to be calculated

        return Result<StreamStatsDTO>::ok(dto);
    }
};

/**
 * @brief Set stream frame rate use case
 */
class SetStreamFrameRateUseCase {
private:
    static constexpr const char* TAG = "SetFrameRateUC";
    IStreamRepository& _streamRepo;

public:
    SetStreamFrameRateUseCase(IStreamRepository& streamRepo)
        : _streamRepo(streamRepo) {}

    /**
     * @brief Execute: Set target frame rate (FPS)
     */
    Result<void> execute(uint32_t streamId, float targetFPS) {
        if (targetFPS <= 0 || targetFPS > 60) {
            return Result<void>::error("FPS must be between 0 and 60");
        }

        Logger::getInstance().info(TAG, "Setting stream " + String(streamId) +
                                       " to " + String(targetFPS, 1) + " FPS");

        // Convert FPS to minimum frame time (milliseconds)
        uint16_t minFrameTime = (uint16_t)(1000.0f / targetFPS);

        auto result = _streamRepo.setMinFrameTime(streamId, minFrameTime);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to set frame rate: " + result.getError());
            return result;
        }

        Logger::getInstance().info(TAG, "Frame rate set successfully");
        return Result<void>::ok();
    }
};

/**
 * @brief Pause stream use case
 */
class PauseStreamUseCase {
private:
    static constexpr const char* TAG = "PauseStreamUC";
    IStreamRepository& _streamRepo;

public:
    PauseStreamUseCase(IStreamRepository& streamRepo)
        : _streamRepo(streamRepo) {}

    /**
     * @brief Execute: Pause streaming
     */
    Result<void> execute(uint32_t streamId) {
        Logger::getInstance().info(TAG, "Pausing stream " + String(streamId));

        auto result = _streamRepo.pauseStream(streamId);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to pause stream: " + result.getError());
            return result;
        }

        return Result<void>::ok();
    }
};

/**
 * @brief Resume stream use case
 */
class ResumeStreamUseCase {
private:
    static constexpr const char* TAG = "ResumeStreamUC";
    IStreamRepository& _streamRepo;

public:
    ResumeStreamUseCase(IStreamRepository& streamRepo)
        : _streamRepo(streamRepo) {}

    /**
     * @brief Execute: Resume streaming
     */
    Result<void> execute(uint32_t streamId) {
        Logger::getInstance().info(TAG, "Resuming stream " + String(streamId));

        auto result = _streamRepo.resumeStream(streamId);
        if (result.isError()) {
            Logger::getInstance().error(TAG, "Failed to resume stream: " + result.getError());
            return result;
        }

        return Result<void>::ok();
    }
};

#endif // STREAM_USE_CASES_H
