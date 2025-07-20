#pragma once

#include "ocpp_gateway/ocpp/message.h"
#include <chrono>
#include <optional>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @enum ConnectorStatus
 * @brief Connector status as defined in OCPP 2.0.1
 */
enum class ConnectorStatus {
    Available,
    Occupied,
    Reserved,
    Unavailable,
    Faulted
};

/**
 * @brief Convert ConnectorStatus to string
 * @param status ConnectorStatus enum value
 * @return String representation of the status
 */
std::string connectorStatusToString(ConnectorStatus status);

/**
 * @brief Convert string to ConnectorStatus
 * @param statusStr String representation of the status
 * @return ConnectorStatus enum value
 */
ConnectorStatus stringToConnectorStatus(const std::string& statusStr);

/**
 * @class StatusNotificationRequest
 * @brief OCPP StatusNotification request message
 */
class StatusNotificationRequest : public Call {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier
     * @param timestamp Time of status change
     * @param connectorId Connector ID (0 = Charging Station)
     * @param connectorStatus Status of the connector
     * @param evseId EVSE ID
     */
    StatusNotificationRequest(const std::string& messageId,
                             const std::chrono::system_clock::time_point& timestamp,
                             int connectorId,
                             ConnectorStatus connectorStatus,
                             int evseId);
    
    /**
     * @brief Get the timestamp
     * @return Timestamp of status change
     */
    std::chrono::system_clock::time_point getTimestamp() const;
    
    /**
     * @brief Get the connector ID
     * @return Connector ID
     */
    int getConnectorId() const;
    
    /**
     * @brief Get the connector status
     * @return ConnectorStatus
     */
    ConnectorStatus getConnectorStatus() const;
    
    /**
     * @brief Get the connector status as string
     * @return String representation of the status
     */
    std::string getConnectorStatusString() const;
    
    /**
     * @brief Get the EVSE ID
     * @return EVSE ID
     */
    int getEvseId() const;
    
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
    std::chrono::system_clock::time_point timestamp_;
    int connectorId_;
    ConnectorStatus connectorStatus_;
    int evseId_;
};

/**
 * @class StatusNotificationResponse
 * @brief OCPP StatusNotification response message
 */
class StatusNotificationResponse : public CallResult {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier (must match the request ID)
     */
    explicit StatusNotificationResponse(const std::string& messageId);
    
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

} // namespace ocpp
} // namespace ocpp_gateway