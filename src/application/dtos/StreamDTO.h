/**
 * @file StreamDTO.h
 * @brief Data Transfer Objects for streaming information
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef STREAM_DTO_H
#define STREAM_DTO_H

#include <Arduino.h>
#include <IPAddress.h>
#include <vector>

/**
 * @brief Stream client DTO
 */
struct StreamClientDTO {
    uint32_t clientId;
    String ipAddress;
    uint16_t port;
    uint32_t framesSent;
    uint32_t framesDropped;
    float currentFPS;
    uint32_t connectedDuration; // seconds

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"clientId\":" + String(clientId) + ",";
        json += "\"ipAddress\":\"" + ipAddress + "\",";
        json += "\"port\":" + String(port) + ",";
        json += "\"framesSent\":" + String(framesSent) + ",";
        json += "\"framesDropped\":" + String(framesDropped) + ",";
        json += "\"currentFPS\":" + String(currentFPS, 2) + ",";
        json += "\"connectedDuration\":" + String(connectedDuration);
        json += "}";
        return json;
    }
};

/**
 * @brief Stream statistics DTO
 */
struct StreamStatsDTO {
    uint32_t streamId;
    bool active;
    size_t clientCount;
    std::vector<StreamClientDTO> clients;
    float targetFPS;
    float averageFPS;
    uint32_t uptime; // seconds

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"streamId\":" + String(streamId) + ",";
        json += "\"active\":" + String(active ? "true" : "false") + ",";
        json += "\"clientCount\":" + String(clientCount) + ",";
        json += "\"targetFPS\":" + String(targetFPS, 2) + ",";
        json += "\"averageFPS\":" + String(averageFPS, 2) + ",";
        json += "\"uptime\":" + String(uptime) + ",";
        json += "\"clients\":[";

        for (size_t i = 0; i < clients.size(); i++) {
            json += clients[i].toJson();
            if (i < clients.size() - 1) {
                json += ",";
            }
        }

        json += "]}";
        return json;
    }
};

/**
 * @brief Stream configuration DTO
 */
struct StreamConfigDTO {
    uint16_t minFrameTime; // milliseconds
    uint8_t maxClients;
    String format;

    /**
     * @brief Convert to JSON string
     */
    String toJson() const {
        String json = "{";
        json += "\"minFrameTime\":" + String(minFrameTime) + ",";
        json += "\"maxClients\":" + String(maxClients) + ",";
        json += "\"format\":\"" + format + "\"";
        json += "}";
        return json;
    }
};

#endif // STREAM_DTO_H
