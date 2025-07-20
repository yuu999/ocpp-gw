#include "ocpp_gateway/ocpp/boot_notification.h"
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>

namespace ocpp_gateway {
namespace ocpp {

std::string registrationStatusToString(RegistrationStatus status) {
    switch (status) {
        case RegistrationStatus::Accepted: return "Accepted";
        case RegistrationStatus::Pending: return "Pending";
        case RegistrationStatus::Rejected: return "Rejected";
        default: return "Unknown";
    }
}

RegistrationStatus stringToRegistrationStatus(const std::string& statusStr) {
    if (statusStr == "Accepted") return RegistrationStatus::Accepted;
    if (statusStr == "Pending") return RegistrationStatus::Pending;
    if (statusStr == "Rejected") return RegistrationStatus::Rejected;
    
    spdlog::error("Unknown registration status: {}", statusStr);
    return RegistrationStatus::Rejected; // Default to rejected for unknown status
}

std::string BootNotificationRequest::reasonToString(BootNotificationRequest::Reason reason) {
    switch (reason) {
        case Reason::ApplicationReset: return "ApplicationReset";
        case Reason::FirmwareUpdate: return "FirmwareUpdate";
        case Reason::LocalReset: return "LocalReset";
        case Reason::PowerUp: return "PowerUp";
        case Reason::RemoteReset: return "RemoteReset";
        case Reason::ScheduledReset: return "ScheduledReset";
        case Reason::Triggered: return "Triggered";
        case Reason::Unknown: return "Unknown";
        case Reason::Watchdog: return "Watchdog";
        default: return "Unknown";
    }
}

BootNotificationRequest::Reason BootNotificationRequest::stringToReason(const std::string& reasonStr) {
    if (reasonStr == "ApplicationReset") return Reason::ApplicationReset;
    if (reasonStr == "FirmwareUpdate") return Reason::FirmwareUpdate;
    if (reasonStr == "LocalReset") return Reason::LocalReset;
    if (reasonStr == "PowerUp") return Reason::PowerUp;
    if (reasonStr == "RemoteReset") return Reason::RemoteReset;
    if (reasonStr == "ScheduledReset") return Reason::ScheduledReset;
    if (reasonStr == "Triggered") return Reason::Triggered;
    if (reasonStr == "Unknown") return Reason::Unknown;
    if (reasonStr == "Watchdog") return Reason::Watchdog;
    
    spdlog::error("Unknown boot reason: {}", reasonStr);
    return Reason::Unknown;
}

BootNotificationRequest::BootNotificationRequest(
    const std::string& messageId,
    Reason reason,
    const ChargingStation& chargingStation)
    : Call(messageId, MessageAction::BootNotification),
      reason_(reason),
      chargingStation_(chargingStation) {}

BootNotificationRequest::Reason BootNotificationRequest::getReason() const {
    return reason_;
}

std::string BootNotificationRequest::getReasonString() const {
    return reasonToString(reason_);
}

const BootNotificationRequest::ChargingStation& BootNotificationRequest::getChargingStation() const {
    return chargingStation_;
}

nlohmann::json BootNotificationRequest::getPayloadJson() const {
    nlohmann::json j;
    
    j["reason"] = reasonToString(reason_);
    
    nlohmann::json cs;
    cs["model"] = chargingStation_.model;
    cs["vendorName"] = chargingStation_.vendorName;
    
    if (chargingStation_.firmwareVersion) {
        cs["firmwareVersion"] = *chargingStation_.firmwareVersion;
    }
    
    if (chargingStation_.serialNumber) {
        cs["serialNumber"] = *chargingStation_.serialNumber;
    }
    
    if (chargingStation_.modem) {
        cs["modem"] = *chargingStation_.modem;
    }
    
    j["chargingStation"] = cs;
    
    return j;
}

bool BootNotificationRequest::setPayloadFromJson(const nlohmann::json& json) {
    try {
        if (!json.contains("reason") || !json.contains("chargingStation")) {
            spdlog::error("Missing required fields in BootNotificationRequest");
            return false;
        }
        
        reason_ = stringToReason(json["reason"].get<std::string>());
        
        const auto& cs = json["chargingStation"];
        
        if (!cs.contains("model") || !cs.contains("vendorName")) {
            spdlog::error("Missing required fields in chargingStation");
            return false;
        }
        
        chargingStation_.model = cs["model"].get<std::string>();
        chargingStation_.vendorName = cs["vendorName"].get<std::string>();
        
        if (cs.contains("firmwareVersion")) {
            chargingStation_.firmwareVersion = cs["firmwareVersion"].get<std::string>();
        } else {
            chargingStation_.firmwareVersion = std::nullopt;
        }
        
        if (cs.contains("serialNumber")) {
            chargingStation_.serialNumber = cs["serialNumber"].get<std::string>();
        } else {
            chargingStation_.serialNumber = std::nullopt;
        }
        
        if (cs.contains("modem")) {
            chargingStation_.modem = cs["modem"].get<std::string>();
        } else {
            chargingStation_.modem = std::nullopt;
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing BootNotificationRequest: {}", e.what());
        return false;
    }
}

// Helper function to convert time_point to ISO8601 string
std::string timePointToIso8601(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%FT%T");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    
    return ss.str();
}

// Helper function to parse ISO8601 string to time_point
std::chrono::system_clock::time_point iso8601ToTimePoint(const std::string& iso8601) {
    std::tm tm = {};
    std::stringstream ss(iso8601);
    
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse ISO8601 date: " + iso8601);
    }
    
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    // Handle milliseconds if present
    if (ss.peek() == '.') {
        ss.ignore(); // Skip the dot
        int ms;
        ss >> ms;
        tp += std::chrono::milliseconds(ms);
    }
    
    // Handle timezone (just 'Z' for now, could be extended)
    if (ss.peek() == 'Z') {
        // UTC time, no adjustment needed
    }
    
    return tp;
}

BootNotificationResponse::BootNotificationResponse(
    const std::string& messageId,
    const std::chrono::system_clock::time_point& currentTime,
    int interval,
    RegistrationStatus status)
    : CallResult(messageId),
      currentTime_(currentTime),
      interval_(interval),
      status_(status) {}

std::chrono::system_clock::time_point BootNotificationResponse::getCurrentTime() const {
    return currentTime_;
}

int BootNotificationResponse::getInterval() const {
    return interval_;
}

RegistrationStatus BootNotificationResponse::getStatus() const {
    return status_;
}

std::string BootNotificationResponse::getStatusString() const {
    return registrationStatusToString(status_);
}

nlohmann::json BootNotificationResponse::getPayloadJson() const {
    nlohmann::json j;
    
    j["currentTime"] = timePointToIso8601(currentTime_);
    j["interval"] = interval_;
    j["status"] = registrationStatusToString(status_);
    
    return j;
}

bool BootNotificationResponse::setPayloadFromJson(const nlohmann::json& json) {
    try {
        if (!json.contains("currentTime") || !json.contains("interval") || !json.contains("status")) {
            spdlog::error("Missing required fields in BootNotificationResponse");
            return false;
        }
        
        currentTime_ = iso8601ToTimePoint(json["currentTime"].get<std::string>());
        interval_ = json["interval"].get<int>();
        status_ = stringToRegistrationStatus(json["status"].get<std::string>());
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error parsing BootNotificationResponse: {}", e.what());
        return false;
    }
}

} // namespace ocpp
} // namespace ocpp_gateway