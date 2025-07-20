#pragma once

#include "ocpp_gateway/ocpp/message.h"
#include <chrono>
#include <optional>
#include <vector>

namespace ocpp_gateway {
namespace ocpp {

/**
 * @enum TransactionEventType
 * @brief Transaction event types as defined in OCPP 2.0.1
 */
enum class TransactionEventType {
    Started,
    Updated,
    Ended
};

/**
 * @brief Convert TransactionEventType to string
 * @param type TransactionEventType enum value
 * @return String representation of the type
 */
std::string transactionEventTypeToString(TransactionEventType type);

/**
 * @brief Convert string to TransactionEventType
 * @param typeStr String representation of the type
 * @return TransactionEventType enum value
 */
TransactionEventType stringToTransactionEventType(const std::string& typeStr);

/**
 * @enum TriggerReason
 * @brief Trigger reasons as defined in OCPP 2.0.1
 */
enum class TriggerReason {
    Authorized,
    CablePluggedIn,
    ChargingRateChanged,
    ChargingStateChanged,
    Deauthorized,
    EnergyLimitReached,
    EVCommunicationLost,
    EVConnectTimeout,
    MeterValueClock,
    MeterValuePeriodic,
    TimeLimitReached,
    Trigger,
    UnlockCommand,
    StopAuthorized,
    EVDeparted,
    EVDetected,
    RemoteStop,
    RemoteStart,
    AbnormalCondition,
    SignedDataReceived,
    ResetCommand
};

/**
 * @brief Convert TriggerReason to string
 * @param reason TriggerReason enum value
 * @return String representation of the reason
 */
std::string triggerReasonToString(TriggerReason reason);

/**
 * @brief Convert string to TriggerReason
 * @param reasonStr String representation of the reason
 * @return TriggerReason enum value
 */
TriggerReason stringToTriggerReason(const std::string& reasonStr);

/**
 * @enum ChargingState
 * @brief Charging states as defined in OCPP 2.0.1
 */
enum class ChargingState {
    Charging,
    EVConnected,
    SuspendedEV,
    SuspendedEVSE,
    Idle
};

/**
 * @brief Convert ChargingState to string
 * @param state ChargingState enum value
 * @return String representation of the state
 */
std::string chargingStateToString(ChargingState state);

/**
 * @brief Convert string to ChargingState
 * @param stateStr String representation of the state
 * @return ChargingState enum value
 */
ChargingState stringToChargingState(const std::string& stateStr);

/**
 * @struct IdToken
 * @brief Identification token
 */
struct IdToken {
    std::string idToken;
    std::string type;
    // Additional fields could be added as needed
};

/**
 * @struct SampledValue
 * @brief Sampled value for meter readings
 */
struct SampledValue {
    std::string value;
    std::string context;
    std::string measurand;
    std::string phase;
    std::string location;
    std::string unitOfMeasure;
};

/**
 * @struct MeterValue
 * @brief Meter reading with timestamp
 */
struct MeterValue {
    std::chrono::system_clock::time_point timestamp;
    std::vector<SampledValue> sampledValues;
};

/**
 * @struct Transaction
 * @brief Transaction details
 */
struct Transaction {
    std::string transactionId;
    std::optional<std::string> chargingState;
    std::optional<int> timeSpentCharging;
    std::optional<std::string> stoppedReason;
    std::optional<int> remoteStartId;
};

/**
 * @struct EVSE
 * @brief EVSE details
 */
struct EVSE {
    int id;
    std::optional<int> connectorId;
};

/**
 * @class TransactionEventRequest
 * @brief OCPP TransactionEvent request message
 */
class TransactionEventRequest : public Call {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier
     * @param eventType Type of transaction event
     * @param timestamp Time of event
     * @param triggerReason Reason for the event
     * @param seqNo Sequence number of the event
     * @param transactionInfo Transaction information
     * @param evse EVSE information
     * @param idToken Identification token (optional)
     * @param meterValues Meter values (optional)
     */
    TransactionEventRequest(const std::string& messageId,
                           TransactionEventType eventType,
                           const std::chrono::system_clock::time_point& timestamp,
                           TriggerReason triggerReason,
                           int seqNo,
                           const Transaction& transactionInfo,
                           const EVSE& evse,
                           const std::optional<IdToken>& idToken = std::nullopt,
                           const std::optional<std::vector<MeterValue>>& meterValues = std::nullopt);
    
    /**
     * @brief Get the event type
     * @return TransactionEventType
     */
    TransactionEventType getEventType() const;
    
    /**
     * @brief Get the event type as string
     * @return String representation of the event type
     */
    std::string getEventTypeString() const;
    
    /**
     * @brief Get the timestamp
     * @return Timestamp of the event
     */
    std::chrono::system_clock::time_point getTimestamp() const;
    
    /**
     * @brief Get the trigger reason
     * @return TriggerReason
     */
    TriggerReason getTriggerReason() const;
    
    /**
     * @brief Get the trigger reason as string
     * @return String representation of the trigger reason
     */
    std::string getTriggerReasonString() const;
    
    /**
     * @brief Get the sequence number
     * @return Sequence number
     */
    int getSeqNo() const;
    
    /**
     * @brief Get the transaction information
     * @return Transaction struct
     */
    const Transaction& getTransactionInfo() const;
    
    /**
     * @brief Get the EVSE information
     * @return EVSE struct
     */
    const EVSE& getEvse() const;
    
    /**
     * @brief Get the identification token
     * @return Optional IdToken struct
     */
    const std::optional<IdToken>& getIdToken() const;
    
    /**
     * @brief Get the meter values
     * @return Optional vector of MeterValue structs
     */
    const std::optional<std::vector<MeterValue>>& getMeterValues() const;
    
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
    TransactionEventType eventType_;
    std::chrono::system_clock::time_point timestamp_;
    TriggerReason triggerReason_;
    int seqNo_;
    Transaction transactionInfo_;
    EVSE evse_;
    std::optional<IdToken> idToken_;
    std::optional<std::vector<MeterValue>> meterValues_;
};

/**
 * @class TransactionEventResponse
 * @brief OCPP TransactionEvent response message
 */
class TransactionEventResponse : public CallResult {
public:
    /**
     * @brief Constructor
     * @param messageId Unique message identifier (must match the request ID)
     * @param idTokenInfo ID token info (optional)
     * @param chargingPriority Charging priority (optional)
     */
    TransactionEventResponse(const std::string& messageId,
                            const std::optional<nlohmann::json>& idTokenInfo = std::nullopt,
                            const std::optional<int>& chargingPriority = std::nullopt);
    
    /**
     * @brief Get the ID token info
     * @return Optional JSON object containing ID token info
     */
    const std::optional<nlohmann::json>& getIdTokenInfo() const;
    
    /**
     * @brief Get the charging priority
     * @return Optional charging priority
     */
    const std::optional<int>& getChargingPriority() const;
    
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
    std::optional<nlohmann::json> idTokenInfo_;
    std::optional<int> chargingPriority_;
};

} // namespace ocpp
} // namespace ocpp_gateway