#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <variant>

namespace ocpp_gateway {
namespace common {

/**
 * @enum ConnectorStatus
 * @brief Status of a connector as defined in OCPP 2.0.1
 */
enum class ConnectorStatus {
    Available,
    Occupied,
    Reserved,
    Unavailable,
    Faulted
};

/**
 * @enum AvailabilityStatus
 * @brief Availability status of an EVSE
 */
enum class AvailabilityStatus {
    Operative,
    Inoperative
};

/**
 * @enum TransactionStatus
 * @brief Status of a charging transaction
 */
enum class TransactionStatus {
    Started,
    Updated,
    Ended
};

/**
 * @struct SampledValue
 * @brief Represents a sampled value in a meter reading
 */
struct SampledValue {
    std::string value;
    std::string context;
    std::string format;
    std::string measurand;
    std::string phase;
    std::string location;
    std::string unit;
};

/**
 * @struct MeterValue
 * @brief Represents a meter reading with timestamp
 */
struct MeterValue {
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::vector<SampledValue> sampledValues;
};

/**
 * @struct Transaction
 * @brief Represents a charging transaction
 */
struct Transaction {
    std::string id;
    std::string idTag;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::optional<std::chrono::time_point<std::chrono::system_clock>> stopTime;
    std::vector<MeterValue> meterValues;
    TransactionStatus status;
};

/**
 * @struct EvseState
 * @brief Represents the state of an EVSE
 */
struct EvseState {
    std::string id;
    ConnectorStatus status;
    AvailabilityStatus availability;
    std::optional<Transaction> currentTransaction;
    std::map<std::string, std::string> variables;
    std::chrono::time_point<std::chrono::system_clock> lastHeartbeat;
    bool connected;
};

/**
 * @struct DeviceValue
 * @brief Represents a device value with type information
 */
struct DeviceValue {
    enum class Type {
        Boolean,
        Integer,
        Float,
        String,
        Enum
    };

    Type type;
    std::variant<bool, int64_t, double, std::string> value;
    std::map<std::string, std::string> enumMapping;
};

} // namespace common
} // namespace ocpp_gateway