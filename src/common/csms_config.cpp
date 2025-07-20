#include "ocpp_gateway/common/csms_config.h"
#include "ocpp_gateway/common/config_types.h"
#include <yaml-cpp/yaml.h>
#include <json/json.h>
#include <fstream>
#include <sstream>

namespace ocpp_gateway {
namespace config {

CsmsConfig::CsmsConfig()
    : url_("wss://csms.example.com/ocpp"),
      reconnect_interval_sec_(30),
      max_reconnect_attempts_(10),
      heartbeat_interval_sec_(300) {
    // Default values are set in the initializer list
}

CsmsConfig::CsmsConfig(const std::string& yaml_file)
    : CsmsConfig() {
    loadFromYaml(yaml_file);
}

bool CsmsConfig::loadFromYaml(const std::string& yaml_file) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_file);
        
        if (!config["csms"]) {
            return false;
        }

        YAML::Node csms = config["csms"];
        
        // Load URL
        if (csms["url"]) {
            url_ = csms["url"].as<std::string>();
        }
        
        // Load reconnect interval
        if (csms["reconnect_interval_sec"]) {
            reconnect_interval_sec_ = csms["reconnect_interval_sec"].as<int>();
        }
        
        // Load max reconnect attempts
        if (csms["max_reconnect_attempts"]) {
            max_reconnect_attempts_ = csms["max_reconnect_attempts"].as<int>();
        }
        
        // Load heartbeat interval
        if (csms["heartbeat_interval_sec"]) {
            heartbeat_interval_sec_ = csms["heartbeat_interval_sec"].as<int>();
        }
        
        return validate();
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool CsmsConfig::loadFromYamlString(const std::string& yaml_content) {
    try {
        YAML::Node config = YAML::Load(yaml_content);
        
        if (!config["csms"]) {
            return false;
        }

        YAML::Node csms = config["csms"];
        
        // Load URL
        if (csms["url"]) {
            url_ = csms["url"].as<std::string>();
        }
        
        // Load reconnect interval
        if (csms["reconnect_interval_sec"]) {
            reconnect_interval_sec_ = csms["reconnect_interval_sec"].as<int>();
        }
        
        // Load max reconnect attempts
        if (csms["max_reconnect_attempts"]) {
            max_reconnect_attempts_ = csms["max_reconnect_attempts"].as<int>();
        }
        
        // Load heartbeat interval
        if (csms["heartbeat_interval_sec"]) {
            heartbeat_interval_sec_ = csms["heartbeat_interval_sec"].as<int>();
        }
        
        return validate();
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool CsmsConfig::loadFromJson(const std::string& json_file) {
    try {
        std::ifstream file(json_file);
        if (!file.is_open()) {
            return false;
        }
        
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        if (!Json::parseFromStream(builder, file, &root, &errs)) {
            return false;
        }
        
        if (!root.isMember("csms")) {
            return false;
        }
        
        const Json::Value& csms = root["csms"];
        
        // Load URL
        if (csms.isMember("url")) {
            url_ = csms["url"].asString();
        }
        
        // Load reconnect interval
        if (csms.isMember("reconnect_interval_sec")) {
            reconnect_interval_sec_ = csms["reconnect_interval_sec"].asInt();
        }
        
        // Load max reconnect attempts
        if (csms.isMember("max_reconnect_attempts")) {
            max_reconnect_attempts_ = csms["max_reconnect_attempts"].asInt();
        }
        
        // Load heartbeat interval
        if (csms.isMember("heartbeat_interval_sec")) {
            heartbeat_interval_sec_ = csms["heartbeat_interval_sec"].asInt();
        }
        
        return validate();
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool CsmsConfig::loadFromJsonString(const std::string& json_content) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(json_content.c_str(), json_content.c_str() + json_content.length(), &root, &errs)) {
            return false;
        }
        
        if (!root.isMember("csms")) {
            return false;
        }
        
        const Json::Value& csms = root["csms"];
        
        // Load URL
        if (csms.isMember("url")) {
            url_ = csms["url"].asString();
        }
        
        // Load reconnect interval
        if (csms.isMember("reconnect_interval_sec")) {
            reconnect_interval_sec_ = csms["reconnect_interval_sec"].asInt();
        }
        
        // Load max reconnect attempts
        if (csms.isMember("max_reconnect_attempts")) {
            max_reconnect_attempts_ = csms["max_reconnect_attempts"].asInt();
        }
        
        // Load heartbeat interval
        if (csms.isMember("heartbeat_interval_sec")) {
            heartbeat_interval_sec_ = csms["heartbeat_interval_sec"].asInt();
        }
        
        return validate();
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const ConfigValidationError& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool CsmsConfig::saveToYaml(const std::string& yaml_file) const {
    try {
        YAML::Node csms;
        
        // Save URL
        csms["url"] = url_;
        
        // Save reconnect interval
        csms["reconnect_interval_sec"] = reconnect_interval_sec_;
        
        // Save max reconnect attempts
        csms["max_reconnect_attempts"] = max_reconnect_attempts_;
        
        // Save heartbeat interval
        csms["heartbeat_interval_sec"] = heartbeat_interval_sec_;
        
        YAML::Node root;
        root["csms"] = csms;
        
        std::ofstream file(yaml_file);
        if (!file.is_open()) {
            return false;
        }
        
        file << root;
        return true;
    } catch (const YAML::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool CsmsConfig::saveToJson(const std::string& json_file) const {
    try {
        Json::Value csms;
        
        // Save URL
        csms["url"] = url_;
        
        // Save reconnect interval
        csms["reconnect_interval_sec"] = reconnect_interval_sec_;
        
        // Save max reconnect attempts
        csms["max_reconnect_attempts"] = max_reconnect_attempts_;
        
        // Save heartbeat interval
        csms["heartbeat_interval_sec"] = heartbeat_interval_sec_;
        
        Json::Value root;
        root["csms"] = csms;
        
        std::ofstream file(json_file);
        if (!file.is_open()) {
            return false;
        }
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(root, &file);
        
        return true;
    } catch (const Json::Exception& e) {
        // Log error
        return false;
    } catch (const std::exception& e) {
        // Log error
        return false;
    }
}

bool CsmsConfig::validate() const {
    try {
        if (url_.empty()) {
            throw ConfigValidationError("CSMS URL cannot be empty");
        }
        
        if (reconnect_interval_sec_ <= 0) {
            throw ConfigValidationError("CSMS reconnect interval must be positive");
        }
        
        if (max_reconnect_attempts_ <= 0) {
            throw ConfigValidationError("CSMS max reconnect attempts must be positive");
        }
        
        if (heartbeat_interval_sec_ <= 0) {
            throw ConfigValidationError("CSMS heartbeat interval must be positive");
        }
        
        return true;
    } catch (const ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw ConfigValidationError(std::string("Unexpected error during validation: ") + e.what());
    }
}

} // namespace config
} // namespace ocpp_gateway