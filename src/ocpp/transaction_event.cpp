#include "ocpp_gateway/ocpp/transaction_event.h"
#include <spdlog/spdlog.h>

namespace ocpp_gateway {
namespace ocpp {

// Helper function declarations (defined in boot_notification.cpp)
std::string timePointToIso8601(const std::chrono::system_clock::time_point& tp);
std::chrono::system_clock::time_point iso8601ToTimePoint(const std::string& iso8601);

std::string transactionEventTypeToString(TransactionEventType type) {
    switch (type) {
        case TransactionEventType::Started: return "Started";
        case TransactionEventType::Updated: return "Updated";
        case TransactionEventType::Ended: return "Ended";
        default: return "Unknown";
    }
}

TransactionEventType stringToTransactionEventType(const std::string& typeStr) {
    if (typeStr == "Started") return TransactionEventType::Started;
    if (typeStr == "Updated") return TransactionEventType::Updated;
    if (typeStr == "Ended") return TransactionEventType::Ended;
    
    spdlog::error("Unknown transaction event type: {}", typeStr);
    return TransactionEventType::Updated; // Default to updated for unknown type
}

std::string triggerReasonToString(TriggerReason reason) {
    switch (reason) {
        case TriggerReason::Authorized: return "Authorized";
        case TriggerReason::CablePluggedIn: return "CablePluggedIn";
        case TriggerReason::ChargingRateChanged: return "ChargingRateChanged";
        case TriggerReason::ChargingStateChanged: return "ChargingStateChanged";
        case TriggerReason::Deauthorized: return "Deauthorized";
        case TriggerReason::EnergyLimitReached: return "EnergyLimitReached";
        case TriggerReason::EVCommunicationLost: return "EVCommunicationLost";
        case TriggerReason::EVConnectTimeout: return "EVConnectTimeout";
        case TriggerReason::MeterValueClock: return "MeterValueClock";
        case TriggerReason::MeterValuePeriodic: return "MeterValuePeriodic";
        case TriggerReason::TimeLimitReached: return "TimeLimitReached";
        case TriggerReason::Trigger: return "Trigger";
        case TriggerReason::UnlockCommand: return "UnlockCommand";
        case TriggerReason::StopAuthorized: return "StopAuthorized";
        case TriggerReason::EVDeparted: return "EVDeparted";
        case TriggerReason::EVDetected: return "EVDetected";
        case TriggerReason::RemoteStop: return "RemoteStop";
        case TriggerReason::RemoteStart: return "RemoteStart";
        case TriggerReason::AbnormalCondition: return "AbnormalCondition";
        case TriggerReason::SignedDataReceived: return "SignedDataReceived";
        case TriggerReason::ResetCommand: return "ResetCommand";
        default: return "Unknown";
    }
}

TriggerReason stringToTriggerReason(const std::string& reasonStr) {
    if (reasonStr == "Authorized") return TriggerReason::Authorized;
    if (reasonStr == "CablePluggedIn") return TriggerReason::CablePluggedIn;
    if (reasonStr == "ChargingRateChanged") return TriggerReason::ChargingRateChanged;
    if (reasonStr == "ChargingStateChanged") return TriggerReason::ChargingStateChanged;
    if (reasonStr == "Deauthorized") return TriggerReason::Deauthorized;
    if (reasonStr == "EnergyLimitReached") return TriggerReason::EnergyLimitReached;
    if (reasonStr == "EVCommunicationLost") return TriggerReason::EVCommunicationLost;
    if (reasonStr == "EVConnectTimeout") return TriggerReason::EVConnectTimeout;
    if (reasonStr == "MeterValueClock") return TriggerReason::MeterValueClock;
    if (reasonStr == "MeterValuePeriodic") return TriggerReason::MeterValuePeriodic;
    if (reasonStr == "TimeLimitReached") return TriggerReason::TimeLimitReached;
    if (reasonStr == "Trigger") return TriggerReason::Trigger;
    if (reasonStr == "UnlockCommand") return TriggerReason::UnlockCommand;
    if (reasonStr == "StopAuthorized") return TriggerReason::StopAuthorized;
    if (reasonStr == "EVDeparted") return TriggerReason::EVDeparted;
    if (reasonStr == "EVDetected") return TriggerReason::EVDetected;
    if (reasonStr == "RemoteStop") return TriggerReason::RemoteStop;
    if (reasonStr == "RemoteStart") return TriggerReason::RemoteStart;
    if (reasonStr == "AbnormalCondition") return TriggerReason::AbnormalCondition;
    if (reasonStr == "SignedDataReceived") return TriggerReason::SignedDataReceived;
    if (reasonStr == "ResetCommand") return TriggerReason::ResetCommand;
    
    spdlog::error("Unknown trigger reason: {}", reasonStr);
    return TriggerReason::Trigger; // Default to Trigger for unknown reason
}

std::string chargingStateToString(ChargingState state) {
    switch (state) {
        case ChargingState::Charging: return "Charging";
        case ChargingState::EVConnected: return "EVConnected";
        case ChargingState::SuspendedEV: return "SuspendedEV";
        case ChargingState::SuspendedEVSE: return "SuspendedEVSE";
        case ChargingState::Idle: return "Idle";
        default: return "Unknown";
    }
}

ChargingState stringToChargingState(const std::string& stateStr) {
    if (stateStr == "Charging") return ChargingState::Charging;
    if (stateStr == "EVConnected") return ChargingState::EVConnected;
    if (stateStr == "SuspendedEV") return ChargingState::SuspendedEV;
    if (stateStr == "SuspendedEVSE") return ChargingState::SuspendedEVSE;
    if (stateStr == "Idle") return ChargingState::Idle;
    
    spdlog::error("Unknown charging state: {}", stateStr);
    return ChargingState::Idle; // Default to Idle for unknown state
}

TransactionEventRequest::TransactionEventRequest(
    const std::string& messageId,
    TransactionEventType eventType,
    const std::chrono::system_clock::time_point& timestamp,
    TriggerReason triggerReason,
    int seqNo,
    const Transaction& transactionInfo,
    const EVSE& evse,
    const std::optional<IdToken>& idToken,
    const std::optional<std::vector<MeterValue>>& meterValues)
    : Call(messageId, MessageAction::TransactionEvent),
      eventType_(eventType),
      timestamp_(timestamp),
      triggerReason_(triggerReason),
      seqNo_(seqNo),
      transactionInfo_(transactionInfo),
      evse_(evse),
      idToken_(idToken),
      meterValues_(meterValues) {}

TransactionEventType TransactionEventRequest::getEventType() const {
    return eventType_;
}

std::string TransactionEventRequest::getEventTypeString() const {
    return transactionEventTypeToString(eventType_);
}

std::chrono::system_clock::time_point TransactionEventRequest::getTimestamp() const {
    return timestamp_;
}

TriggerReason TransactionEventRequest::getTriggerReason() const {
    return triggerReason_;
}

std::string TransactionEventRequest::getTriggerReasonString() const {
    return triggerReasonToString(triggerReason_);
}

int TransactionEventRequest::getSeqNo() const {
    return seqNo_;
}

// cppcheck-suppress unusedFunction
const Transaction& TransactionEventRequest::getTransactionInfo() const {
    return transactionInfo_;
}

// cppcheck-suppress unusedFunction
const EVSE& TransactionEventRequest::getEvse() const {
    return evse_;
}

// cppcheck-suppress unusedFunction
const std::optional<IdToken>& TransactionEventRequest::getIdToken() const {
    return idToken_;
}

// cppcheck-suppress unusedFunction
const std::optional<std::vector<MeterValue>>& TransactionEventRequest::getMeterValues() const {
    return meterValues_;
}

nlohmann::json TransactionEventRequest::getPayloadJson() const {
    nlohmann::json j;
    
    j["eventType"] = transactionEventTypeToString(eventType_);
    j["timestamp"] = timePointToIso8601(timestamp_);
    j["triggerReason"] = triggerReasonToString(triggerReason_);
    j["seqNo"] = seqNo_;
    
    // Transaction info
    nlohmann::json transactionJson;
    transactionJson["transactionId"] = transactionInfo_.transactionId;
    
    if (transactionInfo_.chargingState) {
        transactionJson["chargingState"] = *transactionInfo_.chargingState;
    }
    
    if (transactionInfo_.timeSpentCharging) {
        transactionJson["timeSpentCharging"] = *transactionInfo_.timeSpentCharging;
    }
    
    if (transactionInfo_.stoppedReason) {
        transactionJson["stoppedReason"] = *transactionInfo_.stoppedReason;
    }
    
    if (transactionInfo_.remoteStartId) {
        transactionJson["remoteStartId"] = *transactionInfo_.remoteStartId;
    }
    
    j["transactionInfo"] = transactionJson;
    
    // EVSE info
    nlohmann::json evseJson;
    evseJson["id"] = evse_.id;
    
    if (evse_.connectorId) {
        evseJson["connectorId"] = *evse_.connectorId;
    }
    
    j["evse"] = evseJson;
    
    // Optional fields
    if (idToken_) {
        nlohmann::json idTokenJson;
        idTokenJson["idToken"] = idToken_->idToken;
        idTokenJson["type"] = idToken_->type;
        j["idToken"] = idTokenJson;
    }
    
    if (meterValues_) {
        nlohmann::json meterValuesJson = nlohmann::json::array();
        
        for (const auto& meterValue : *meterValues_) {
            nlohmann::json meterValueJson;
            meterValueJson["timestamp"] = timePointToIso8601(meterValue.timestamp);
            
            nlohmann::json sampledValuesJson = nlohmann::json::array();
            
            for (const auto& sampledValue : meterValue.sampledValues) {
                nlohmann::json sampledValueJson;
                sampledValueJson["value"] = sampledValue.value;
                
                if (!sampledValue.context.empty()) {
                    sampledValueJson["context"] = sampledValue.context;
                }
                
                if (!sampledValue.measurand.empty()) {
                    sampledValueJson["measurand"] = sampledValue.measurand;
                }
                
                if (!sampledValue.phase.empty()) {
                    sampledValueJson["phase"] = sampledValue.phase;
                }
                
                if (!sampledValue.location.empty()) {
                    sampledValueJson["location"] = sampledValue.location;
                }
                
                if (!sampledValue.unitOfMeasure.empty()) {
                    sampledValueJson["unitOfMeasure"] = sampledValue.unitOfMeasure;
                }
                
                sampledValuesJson.push_back(sampledValueJson);
            }
            
            meterValueJson["sampledValue"] = sampledValuesJson;
            meterValuesJson.push_back(meterValueJson);
        }
        
        j["meterValues"] = meterValuesJson;
    }
    
    return j;
}

bool TransactionEventRequest::setPayloadFromJson(const nlohmann::json& json) {
    try {
        if (!json.contains("eventType") || !json.contains("timestamp") || 
            !json.contains("triggerReason") || !json.contains("seqNo") || 
            !json.contains("transactionInfo") || !json.contains("evse")) {
            spdlog::error("Missing required fields in TransactionEventRequest");
            return false;
        }
        
        eventType_ = stringToTransactionEventType(json["eventType"].get<std::string>());
        timestamp_ = iso8601ToTimePoint(json["timestamp"].get<std::string>());
        triggerReason_ = stringToTriggerReason(json["triggerReason"].get<std::string>());
        seqNo_ = json["seqNo"].get<int>();
        
        // Parse transaction info
        const auto& transactionJson = json["transactionInfo"];
        
        if (!transactionJson.contains("transactionId")) {
            spdlog::error("Missing transactionId in transactionInfo");
            return false;
        }
        
        transactionInfo_.transactionId = transactionJson["transactionId"].get<std::string>();
        
        if (transactionJson.contains("chargingState")) {
            transactionInfo_.chargingState = transactionJson["chargingState"].get<std::string>();
        } else {
            transactionInfo_.chargingState = std::nullopt;
        }
        
        if (transactionJson.contains("timeSpentCharging")) {
            transactionInfo_.timeSpentCharging = transactionJson["timeSpentCharging"].get<int>();
        } else {
            transactionInfo_.timeSpentCharging = std::nullopt;
        }
        
        if (transactionJson.contains("stoppedReason")) {
            transactionInfo_.stoppedReason = transactionJson["stoppedReason"].get<std::string>();
        } else {
            transactionInfo_.stoppedReason = std::nullopt;
        }
        
        if (transactionJson.contains("remoteStartId")) {
            transactionInfo_.remoteStartId = transactionJson["remoteStartId"].get<int>();
        } else {
            transactionInfo_.remoteStartId = std::nullopt;
        }
        
        // Parse EVSE info
        const auto& evseJson = json["evse"];
        
        if (!evseJson.contains("id")) {
            spdlog::error("Missing id in evse");
            return false;
        }
        
        evse_.id = evseJson["id"].get<int>();
        
        if (evseJson.contains("connectorId")) {
            evse_.connectorId = evseJson["connectorId"].get<int>();
        } else {
            evse_.connectorId = std::nullopt;
        }
        
        // Parse optional fields
        if (json.contains("idToken")) {
            const auto& idTokenJson = json["idToken"];
            
            if (!idTokenJson.contains("idToken") || !idTokenJson.contains("type")) {
                spdlog::error("Invalid idToken format");
                return false;
            }
            
            IdToken idToken;
            idToken.idToken = idTokenJson["idToken"].get<std::string>();
            idToken.type = idTokenJson["type"].get<std::string>();
            
            idToken_ = idToken;
        } else {
            idToken_ = std::nullopt;
        }
        
        if (json.contains("meterValues")) {
            const auto& meterValuesJson = json["meterValues"];
            std::vector<MeterValue> meterValues;
            
            for (const auto& meterValueJson : meterValuesJson) {
                if (!meterValueJson.contains("timestamp") || !meterValueJson.contains("sampledValue")) {
                    spdlog::error("Invalid meterValue format");
                    return false;
                }
                
                MeterValue meterValue;
                meterValue.timestamp = iso8601ToTimePoint(meterValueJson["timestamp"].get<std::string>());
                
                const auto& sampledValuesJson = meterValueJson["sampledValue"];
                
                for (const auto& sampledValueJson : sampledValuesJson) {
                    if (!sampledValueJson.contains("value")) {
                        spdlog::error("Invalid sampledValue format");
                        return false;
                    }
                    
                    SampledValue sampledValue;
                    sampledValue.value = sampledValueJson["value"].get<std::string>();
                    
                    if (sampledValueJson.contains("context")) {
                        sampledValue.context = sampledValueJson["context"].get<std::string>();
                    }
                    
                    if (sampledValueJson.contains("measurand")) {
                        sampledValue.measurand = sampledValueJson["measurand"].get<std::string>();
                    }
                    
                    if (sampledValueJson.contains("phase")) {
                        sampledValue.phase = sampledValueJson["phase"].get<std::string>();
                    }
                    
                    if (sampledValueJson.contains("location")) {
                        sampledValue.location = sampledValueJson["location"].get<std::string>();
                    }
                    
                    if (sampledValueJson.contains("unitOfMeasure")) {
                        sampledValue.unitOfMeasure = sampledValueJson["unitOfMeasure"].get<std::string>();
                    }
                    
                    meterValue.sampledValues.push_back(sampledValue);
                }
                
                meterValues.push_back(meterValue);
            }
            
            meterValues_ = meterValues;
        } else {
            meterValues_ = std::nullopt;
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing TransactionEventRequest: {}", e.what());
        return false;
    }
}

TransactionEventResponse::TransactionEventResponse(
    const std::string& messageId,
    const std::optional<nlohmann::json>& idTokenInfo,
    const std::optional<int>& chargingPriority)
    : CallResult(messageId),
      idTokenInfo_(idTokenInfo),
      chargingPriority_(chargingPriority) {}

// cppcheck-suppress unusedFunction
const std::optional<nlohmann::json>& TransactionEventResponse::getIdTokenInfo() const {
    return idTokenInfo_;
}

// cppcheck-suppress unusedFunction
const std::optional<int>& TransactionEventResponse::getChargingPriority() const {
    return chargingPriority_;
}

nlohmann::json TransactionEventResponse::getPayloadJson() const {
    nlohmann::json j = nlohmann::json::object();
    
    if (idTokenInfo_) {
        j["idTokenInfo"] = *idTokenInfo_;
    }
    
    if (chargingPriority_) {
        j["chargingPriority"] = *chargingPriority_;
    }
    
    return j;
}

bool TransactionEventResponse::setPayloadFromJson(const nlohmann::json& json) {
    try {
        if (json.contains("idTokenInfo")) {
            idTokenInfo_ = json["idTokenInfo"];
        } else {
            idTokenInfo_ = std::nullopt;
        }
        
        if (json.contains("chargingPriority")) {
            chargingPriority_ = json["chargingPriority"].get<int>();
        } else {
            chargingPriority_ = std::nullopt;
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing TransactionEventResponse: {}", e.what());
        return false;
    }
}

} // namespace ocpp
} // namespace ocpp_gateway