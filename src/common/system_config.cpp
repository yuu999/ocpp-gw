#include "ocpp_gateway/common/system_config.h"
#include "ocpp_gateway/common/logger.h"
#include <yaml-cpp/yaml.h>
#include <json/json.h>
#include <fstream>
#include <sstream>

namespace ocpp_gateway {
namespace config {

SystemConfig::SystemConfig()
    : log_level_(LogLevel::INFO) {
    // Default values are set in the header
}

SystemConfig::SystemConfig(const std::string& yaml_file) 
    : SystemConfig() {
    loadFromYaml(yaml_file);
}

bool SystemConfig::loadFromYaml(const std::string& yaml_file) {
    try {
        LOG_INFO("YAMLファイルを読み込み中: {}", yaml_file);
        YAML::Node config = YAML::LoadFile(yaml_file);
        LOG_INFO("YAMLファイルの読み込み完了");
        
        if (!config["system"]) {
            LOG_ERROR("YAMLファイルに'system'セクションがありません");
            return false;
        }

        YAML::Node system = config["system"];
        LOG_INFO("システム設定セクションを処理中...");
        
        // Load log level
        if (system["log_level"]) {
            log_level_ = logLevelFromString(system["log_level"].as<std::string>());
            LOG_INFO("ログレベルを設定: {}", system["log_level"].as<std::string>());
        }
        
        // Load log rotation
        if (system["log_rotation"]) {
            if (system["log_rotation"]["max_size_mb"]) {
                log_rotation_.max_size_mb = system["log_rotation"]["max_size_mb"].as<int>();
            }
            if (system["log_rotation"]["max_files"]) {
                log_rotation_.max_files = system["log_rotation"]["max_files"].as<int>();
            }
        }
        
        // Load metrics
        if (system["metrics"]) {
            if (system["metrics"]["prometheus_port"]) {
                metrics_.prometheus_port = system["metrics"]["prometheus_port"].as<int>();
            }
        }
        
        // Load security
        if (system["security"]) {
            if (system["security"]["tls_cert_path"]) {
                security_.tls_cert_path = system["security"]["tls_cert_path"].as<std::string>();
            }
            if (system["security"]["tls_key_path"]) {
                security_.tls_key_path = system["security"]["tls_key_path"].as<std::string>();
            }
            if (system["security"]["ca_cert_path"]) {
                security_.ca_cert_path = system["security"]["ca_cert_path"].as<std::string>();
            }
            if (system["security"]["client_cert_required"]) {
                security_.client_cert_required = system["security"]["client_cert_required"].as<bool>();
            }
        }
        
        LOG_INFO("システム設定の読み込み完了");
        return validate();
    } catch (const YAML::Exception& e) {
        LOG_ERROR("YAML例外: {}", e.what());
        return false;
    } catch (const ConfigValidationError& e) {
        LOG_ERROR("設定検証エラー: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("一般例外: {}", e.what());
        return false;
    }
}

bool SystemConfig::loadFromYamlString(const std::string& yaml_content) {
    try {
        YAML::Node config = YAML::Load(yaml_content);
        
        if (!config["system"]) {
            return false;
        }

        YAML::Node system = config["system"];
        
        // Load log level
        if (system["log_level"]) {
            log_level_ = logLevelFromString(system["log_level"].as<std::string>());
        }
        
        // Load log rotation
        if (system["log_rotation"]) {
            if (system["log_rotation"]["max_size_mb"]) {
                log_rotation_.max_size_mb = system["log_rotation"]["max_size_mb"].as<int>();
            }
            if (system["log_rotation"]["max_files"]) {
                log_rotation_.max_files = system["log_rotation"]["max_files"].as<int>();
            }
        }
        
        // Load metrics
        if (system["metrics"]) {
            if (system["metrics"]["prometheus_port"]) {
                metrics_.prometheus_port = system["metrics"]["prometheus_port"].as<int>();
            }
        }
        
        // Load security
        if (system["security"]) {
            if (system["security"]["tls_cert_path"]) {
                security_.tls_cert_path = system["security"]["tls_cert_path"].as<std::string>();
            }
            if (system["security"]["tls_key_path"]) {
                security_.tls_key_path = system["security"]["tls_key_path"].as<std::string>();
            }
            if (system["security"]["ca_cert_path"]) {
                security_.ca_cert_path = system["security"]["ca_cert_path"].as<std::string>();
            }
            if (system["security"]["client_cert_required"]) {
                security_.client_cert_required = system["security"]["client_cert_required"].as<bool>();
            }
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

bool SystemConfig::loadFromJson(const std::string& json_file) {
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
        
        if (!root.isMember("system")) {
            return false;
        }
        
        const Json::Value& system = root["system"];
        
        // Load log level
        if (system.isMember("log_level")) {
            log_level_ = logLevelFromString(system["log_level"].asString());
        }
        
        // Load log rotation
        if (system.isMember("log_rotation")) {
            const Json::Value& log_rotation = system["log_rotation"];
            if (log_rotation.isMember("max_size_mb")) {
                log_rotation_.max_size_mb = log_rotation["max_size_mb"].asInt();
            }
            if (log_rotation.isMember("max_files")) {
                log_rotation_.max_files = log_rotation["max_files"].asInt();
            }
        }
        
        // Load metrics
        if (system.isMember("metrics")) {
            const Json::Value& metrics = system["metrics"];
            if (metrics.isMember("prometheus_port")) {
                metrics_.prometheus_port = metrics["prometheus_port"].asInt();
            }
        }
        
        // Load security
        if (system.isMember("security")) {
            const Json::Value& security = system["security"];
            if (security.isMember("tls_cert_path")) {
                security_.tls_cert_path = security["tls_cert_path"].asString();
            }
            if (security.isMember("tls_key_path")) {
                security_.tls_key_path = security["tls_key_path"].asString();
            }
            if (security.isMember("ca_cert_path")) {
                security_.ca_cert_path = security["ca_cert_path"].asString();
            }
            if (security.isMember("client_cert_required")) {
                security_.client_cert_required = security["client_cert_required"].asBool();
            }
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

bool SystemConfig::loadFromJsonString(const std::string& json_content) {
    try {
        Json::Value root;
        Json::CharReaderBuilder builder;
        JSONCPP_STRING errs;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(json_content.c_str(), json_content.c_str() + json_content.length(), &root, &errs)) {
            return false;
        }
        
        if (!root.isMember("system")) {
            return false;
        }
        
        const Json::Value& system = root["system"];
        
        // Load log level
        if (system.isMember("log_level")) {
            log_level_ = logLevelFromString(system["log_level"].asString());
        }
        
        // Load log rotation
        if (system.isMember("log_rotation")) {
            const Json::Value& log_rotation = system["log_rotation"];
            if (log_rotation.isMember("max_size_mb")) {
                log_rotation_.max_size_mb = log_rotation["max_size_mb"].asInt();
            }
            if (log_rotation.isMember("max_files")) {
                log_rotation_.max_files = log_rotation["max_files"].asInt();
            }
        }
        
        // Load metrics
        if (system.isMember("metrics")) {
            const Json::Value& metrics = system["metrics"];
            if (metrics.isMember("prometheus_port")) {
                metrics_.prometheus_port = metrics["prometheus_port"].asInt();
            }
        }
        
        // Load security
        if (system.isMember("security")) {
            const Json::Value& security = system["security"];
            if (security.isMember("tls_cert_path")) {
                security_.tls_cert_path = security["tls_cert_path"].asString();
            }
            if (security.isMember("tls_key_path")) {
                security_.tls_key_path = security["tls_key_path"].asString();
            }
            if (security.isMember("ca_cert_path")) {
                security_.ca_cert_path = security["ca_cert_path"].asString();
            }
            if (security.isMember("client_cert_required")) {
                security_.client_cert_required = security["client_cert_required"].asBool();
            }
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

bool SystemConfig::saveToYaml(const std::string& yaml_file) const {
    try {
        YAML::Node system;
        
        // Save log level
        system["log_level"] = logLevelToString(log_level_);
        
        // Save log rotation
        YAML::Node log_rotation;
        log_rotation["max_size_mb"] = log_rotation_.max_size_mb;
        log_rotation["max_files"] = log_rotation_.max_files;
        system["log_rotation"] = log_rotation;
        
        // Save metrics
        YAML::Node metrics;
        metrics["prometheus_port"] = metrics_.prometheus_port;
        system["metrics"] = metrics;
        
        // Save security
        YAML::Node security;
        security["tls_cert_path"] = security_.tls_cert_path;
        security["tls_key_path"] = security_.tls_key_path;
        security["ca_cert_path"] = security_.ca_cert_path;
        security["client_cert_required"] = security_.client_cert_required;
        system["security"] = security;
        
        YAML::Node root;
        root["system"] = system;
        
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

bool SystemConfig::saveToJson(const std::string& json_file) const {
    try {
        Json::Value system;
        
        // Save log level
        system["log_level"] = logLevelToString(log_level_);
        
        // Save log rotation
        Json::Value log_rotation;
        log_rotation["max_size_mb"] = log_rotation_.max_size_mb;
        log_rotation["max_files"] = log_rotation_.max_files;
        system["log_rotation"] = log_rotation;
        
        // Save metrics
        Json::Value metrics;
        metrics["prometheus_port"] = metrics_.prometheus_port;
        system["metrics"] = metrics;
        
        // Save security
        Json::Value security;
        security["tls_cert_path"] = security_.tls_cert_path;
        security["tls_key_path"] = security_.tls_key_path;
        security["ca_cert_path"] = security_.ca_cert_path;
        security["client_cert_required"] = security_.client_cert_required;
        system["security"] = security;
        
        Json::Value root;
        root["system"] = system;
        
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

bool SystemConfig::validate() const {
    try {
        // Validate log rotation
        log_rotation_.validate();
        
        // Validate metrics
        metrics_.validate();
        
        // Validate security
        security_.validate();
        
        return true;
    } catch (const ConfigValidationError& e) {
        throw;
    } catch (const std::exception& e) {
        throw ConfigValidationError(std::string("Unexpected error during validation: ") + e.what());
    }
}

} // namespace config
} // namespace ocpp_gateway