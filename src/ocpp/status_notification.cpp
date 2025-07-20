#include "ocpp_gateway/ocpp/status_notification.h"
#include <spdlog/spdlog.h>

namespace ocpp_gateway {
namespace ocpp {

// Helper function declarations (defined in boot_notification.cpp)
std::string timePointToIso8601(const std::chrono::system_clock::time_point& tp);
std::chrono::system_clock::time_point iso8601ToTimePoint(const std::string& iso8601);

std::string connectorStatusToString(ConnectorStatus status) {
    switch (status) {
        case ConnectorStatus::Available: return "Available";
        case ConnectorStatus::Occupied: return "Occupied";
        case ConnectorStatus::Reserved: return "Reserved";
        case ConnectorStatus::Unavailable: return "Unavailable";
        case ConnectorStatus::Faulted: return "Faulted";
        default: return "Unknown";
    }
}

ConnectorStatus stringToConnectorStatus(const std::string& statusStr) {
    if (statusStr == "Available") return ConnectorStatus::Available;
    if (statusStr == "Occupied") return ConnectorStatus::Occupied;
    if (statusStr == "Reserved") return ConnectorStatus::Reserved;
    if (statusStr == "Unavailable") return ConnectorStatus::Unavailable;
    if (statusStr == "Faulted") return ConnectorStatus::Faulted;
    
    spdlog::error("Unknown connector status: {}", statusStr);
    return ConnectorStatus::Faulted; // Default to faulted for unknown status
}

StatusNotificationRequest::StatusNotificationRequest(
    const std::string& messageId,
    const std::chrono::system_clock::time_point& timestamp,
    int connectorId,
    ConnectorStatus connectorStatus,
    int evseId)
    : Call(messageId, MessageAction::StatusNotification),
      timestamp_(timestamp),
      connectorId_(connectorId),
      connectorStatus_(connectorStatus),
      evseId_(evseId) {}

std::chrono::system_clock::time_point StatusNotificationRequest::getTimestamp() const {
    return timestamp_;
}

int StatusNotificationRequest::getConnectorId() const {
    return connectorId_;
}

ConnectorStatus StatusNotificationRequest::getConnectorStatus() const {
    return connectorStatus_;
}

std::string StatusNotificationRequest::getConnectorStatusString() const {
    return connectorStatusToString(connectorStatus_);
}

int StatusNotificationRequest::getEvseId() const {
    return evseId_;
}

nlohmann::json StatusNotificationRequest::getPayloadJson() const {
    nlohmann::json j;
    
    j["timestamp"] = timePointToIso8601(timestamp_);
    j["connectorId"] = connectorId_;
    j["connectorStatus"] = connectorStatusToString(connectorStatus_);
    j["evseId"] = evseId_;
    
    return j;
}

bool StatusNotificationRequest::setPayloadFromJson(const nlohmann::json& json) {
    try {
        if (!json.contains("timestamp") || !json.contains("connectorId") || 
            !json.contains("connectorStatus") || !json.contains("evseId")) {
            spdlog::error("Missing required fields in StatusNotificationRequest");
            return false;
        }
        
        timestamp_ = iso8601ToTimePoint(json["timestamp"].get<std::string>());
        connectorId_ = json["connectorId"].get<int>();
        connectorStatus_ = stringToConnectorStatus(json["connectorStatus"].get<std::string>());
        evseId_ = json["evseId"].get<int>();
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing StatusNotificationRequest: {}", e.what());
        return false;
    }
}

StatusNotificationResponse::StatusNotificationResponse(const std::string& messageId)
    : CallResult(messageId) {}

nlohmann::json StatusNotificationResponse::getPayloadJson() const {
    // StatusNotification response has an empty payload
    return nlohmann::json::object();
}

bool StatusNotificationResponse::setPayloadFromJson(const nlohmann::json& json) {
    // StatusNotification response has an empty payload, so nothing to parse
    return true;
}

} // namespace ocpp
} // namespace ocpp_gateway