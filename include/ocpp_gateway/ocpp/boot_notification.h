#pragma once

#include "ocpp_gateway/ocpp/message.h"
#include <string>
#include <optional>
#include <chrono>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @enum RegistrationStatus
 * @brief Registration status as defined in OCPP 2.0.1
 */
enum class RegistrationStatus {
    Accepted,
    Pending,
    Rejected
};

/**
 * @brief Convert RegistrationStatus to string
 * @param status RegistrationStatus enum value
 * @return String representation of the status
 */
std::string registrationStatusToString(RegistrationStatus status);

/**
 * @brief Convert string to RegistrationStatus
 * @param statusStr String representation of the status
 * @return RegistrationStatus enum value
 */
RegistrationStatus stringToRegistrationStatus(const std::string& statusStr);

/**
 * @class BootNotificationRequest
 * @brief OCPP BootNotification request message
 */
class BootNotificationRequest : public Call {
public:
    /**
     * @enum Reason
     * @brief Boot reason as defined in OCPP 2.0.1
     */
    enum class Reason {
        ApplicationReset,
        FirmwareUpdate,
        LocalReset,
        PowerUp,
        RemoteReset,
        ScheduledReset,
        Triggered,
        Unknown,
        Watchdog
    };

    /**
     * @struct ChargingStation
     * @brief Charging station information
     */
    struct ChargingStation {
        std::string model;
        std::string vendorName;
        std::optional<std::string> firmwareVersion;
        std::optional<std::string> serialNumber;
        std::optional<std::string> modem;
    };

    /**
     * @brief Constructor
     * @param messageId Unique message identifier
     * @param reason Boot reason
     * @param chargingStation Charging station information
     */
    BootNotificationRequest(const std::string& messageId, 
                           Reason reason,
                           const ChargingStation& chargingStation);
    
    /**
     * @brief Get the boot reason
     * @return Boot reason
     */
    Reason getReason() const;
    
    /**
     * @brief Get the boot reason as string
     * @return String representation of the boot reason
     */
    std::string getReasonString() const;
    
    /**
     * @brief Get the charging station information
     * @return ChargingStation struct
     */
    const ChargingStation& getChargingStation() const;
    
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
    
    /**
     * @brief Convert Reason to string
     * @param reason Reason enum value
     * @return String representation of the reason
     */
    static std::string reasonToString(Reason reason);
    
    /**
     * @brief Convert string to Reason
     * @param reasonStr String representation of the reason
     * @return Reason enum value
     */
    static Reason stringToReason(const std::string& reasonStr);

private:
    Reason reason_;
    ChargingStation chargingStation_;
};

/**
 * @class BootNotificationResponse
 * @brief OCPP BootNotification response message
 */
class BootNotificationResponse : public CallResult {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier (must match the request ID)
     * @param currentTime Current time
     * @param interval Heartbeat interval in seconds
     * @param status Registration status
     */
    BootNotificationResponse(const std::string& messageId,
                            const std::chrono::system_clock::time_point& currentTime,
                            int interval,
                            RegistrationStatus status);
    
    /**
     * @brief Get the current time
     * @return Current time
     */
    std::chrono::system_clock::time_point getCurrentTime() const;
    
    /**
     * @brief Get the heartbeat interval
     * @return Interval in seconds
     */
    int getInterval() const;
    
    /**
     * @brief Get the registration status
     * @return RegistrationStatus
     */
    RegistrationStatus getStatus() const;
    
    /**
     * @brief Get the registration status as string
     * @return String representation of the status
     */
    std::string getStatusString() const;
    
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
    int interval_;
    RegistrationStatus status_;
};

} // namespace ocpp
} // namespace ocpp_gateway