/**
 * @file IStreamRepository.h
 * @brief Interface for stream repository
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef I_STREAM_REPOSITORY_H
#define I_STREAM_REPOSITORY_H

#include "../entities/Stream.h"
#include "../entities/Camera.h"
#include "../../core/Result.h"
#include <IPAddress.h>

/**
 * @brief Stream repository interface
 *
 * Manages video streaming sessions and clients.
 */
class IStreamRepository {
public:
    virtual ~IStreamRepository() = default;

    /**
     * @brief Start a new stream
     *
     * @return Result<uint32_t> Stream ID or error
     */
    virtual Result<uint32_t> startStream() = 0;

    /**
     * @brief Stop active stream
     *
     * @param streamId Stream ID to stop
     * @return Result<void> Success or error
     */
    virtual Result<void> stopStream(uint32_t streamId) = 0;

    /**
     * @brief Stop all active streams
     *
     * @return Result<void> Success or error
     */
    virtual Result<void> stopAllStreams() = 0;

    /**
     * @brief Add client to stream
     *
     * @param streamId Stream ID
     * @param clientIp Client IP address
     * @param clientPort Client port
     * @return Result<uint32_t> Client ID or error
     */
    virtual Result<uint32_t> addClient(uint32_t streamId, IPAddress clientIp, uint16_t clientPort) = 0;

    /**
     * @brief Remove client from stream
     *
     * @param streamId Stream ID
     * @param clientId Client ID
     * @return Result<void> Success or error
     */
    virtual Result<void> removeClient(uint32_t streamId, uint32_t clientId) = 0;

    /**
     * @brief Send frame to all clients in stream
     *
     * @param streamId Stream ID
     * @param frame Frame to send
     * @return Result<void> Success or error
     */
    virtual Result<void> broadcastFrame(uint32_t streamId, const Frame& frame) = 0;

    /**
     * @brief Send frame to specific client
     *
     * @param streamId Stream ID
     * @param clientId Client ID
     * @param frame Frame to send
     * @return Result<void> Success or error
     */
    virtual Result<void> sendFrameToClient(uint32_t streamId, uint32_t clientId, const Frame& frame) = 0;

    /**
     * @brief Get stream by ID
     *
     * @param streamId Stream ID
     * @return Result<Stream> Stream entity or error
     */
    virtual Result<Stream> getStream(uint32_t streamId) const = 0;

    /**
     * @brief Get all active streams
     *
     * @return Result<std::vector<Stream>> Vector of active streams or error
     */
    virtual Result<void> getAllStreams(Stream* streams, size_t& count) const = 0;

    /**
     * @brief Check if stream is active
     *
     * @param streamId Stream ID
     * @return bool True if active, false otherwise
     */
    virtual bool isStreamActive(uint32_t streamId) const = 0;

    /**
     * @brief Get number of active streams
     *
     * @return size_t Number of active streams
     */
    virtual size_t getActiveStreamCount() const = 0;

    /**
     * @brief Get number of clients in stream
     *
     * @param streamId Stream ID
     * @return size_t Number of clients
     */
    virtual size_t getClientCount(uint32_t streamId) const = 0;

    /**
     * @brief Set minimum frame time for stream
     *
     * @param streamId Stream ID
     * @param minFrameTime Minimum time between frames in milliseconds
     * @return Result<void> Success or error
     */
    virtual Result<void> setMinFrameTime(uint32_t streamId, uint16_t minFrameTime) = 0;
};

#endif // I_STREAM_REPOSITORY_H
