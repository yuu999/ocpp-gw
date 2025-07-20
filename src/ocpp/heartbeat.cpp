#include "ocpp_gateway/ocpp/heartbeat.h"
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>

namespace ocpp_gateway {
namespace ocpp {

// Helper function declarations (defined in boot_notification.cpp)
std::string timePointToIso8601(const std::chrono::system_clock::time_point& tp);
std::chrono::system_clock::time_point iso8601ToTimePoint(const std::string& iso8601);

HeartbeatRequest::HeartbeatRequest(const std::string& messageId)
    : Call(messageId, MessageAction::Heartbeat) {}

nlohmann::json HeartbeatRequest::getPayloadJson() const {
    // Heartbeat request has an empty payload
    return nlohmann::json::object();
}

bool HeartbeatRequest::setPayloadFromJson(const nlohmann::json& json) {
    // Heartbeat request has an empty payload, so nothing to parse
    return true;
}

HeartbeatResponse::HeartbeatResponse(
    const std::string& messageId,
    const std::chrono::system_clock::time_point& currentTime)
    : CallResult(messageId),
      currentTime_(currentTime) {}

std::chrono::system_clock::time_point HeartbeatResponse::getCurrentTime() const {
    return currentTime_;
}

nlohmann::json HeartbeatResponse::getPayloadJson() const {
    nlohmann::json j;
    j["currentTime"] = timePointToIso8601(currentTime_);
    return j;
}

bool HeartbeatResponse::setPayloadFromJson(const nlohmann::json& json) {
    try {
        if (!json.contains("currentTime")) {
            spdlog::error("Missing required field 'currentTime' in HeartbeatResponse");
            return false;
        }
        
        currentTime_ = iso8601ToTimePoint(json["currentTime"].get<std::string>());
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing HeartbeatResponse: {}", e.what());
        return false;
    }
}

} // namespace ocpp
} // namespace ocpp_gateway