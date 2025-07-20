#pragma once

#include "ocpp_gateway/ocpp/message.h"
#include <chrono>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @class HeartbeatRequest
 * @brief OCPP Heartbeat request message
 */
class HeartbeatRequest : public Call {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier
     */
    explicit HeartbeatRequest(const std::string& messageId);
    
    /**
     * @brief Get the payload as JSON
     * @return JSON object containing the payload
     */
    nlohmann::json getPayloadJson() const override;
    
    /**
     * @brief Set the payload from JSON
     * @param json JSON object containing the payload
     * @return true if successful, false otherwise
     */
    bool setPayloadFromJson(const nlohmann::json& json) override;
};

/**
 * @class HeartbeatResponse
 * @brief OCPP Heartbeat response message
 */
class HeartbeatResponse : public CallResult {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier (must match the request ID)
     * @param currentTime Current time
     */
    HeartbeatResponse(const std::string& messageId,
                     const std::chrono::system_clock::time_point& currentTime);
    
    /**
     * @brief Get the current time
     * @return Current time
     */
    std::chrono::system_clock::time_point getCurrentTime() const;
    
    /**
     * @brief Get the payload as JSON
     * @return JSON object containing the payload
     */
    nlohmann::json getPayloadJson() const override;
    
    /**
     * @brief Set the payload from JSON
     * @param json JSON object containing the payload
     * @return true if successful, false otherwise
     */
    bool setPayloadFromJson(const nlohmann::json& json) override;

private:
    std::chrono::system_clock::time_point currentTime_;
};

} // namespace ocpp
} // namespace ocpp_gateway