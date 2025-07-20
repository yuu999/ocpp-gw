#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <stdexcept>

namespace ocpp_gateway {
namespace config {

/**
 * @brief Exception thrown for configuration validation errors
 */
class ConfigValidationError : public std::runtime_error {
public:
    explicit ConfigValidationError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Log rotation configuration
 */
struct LogRotationConfig {
    int max_size_mb = 10;
    int max_files = 5;

    bool validate() const {
        if (max_size_mb <= 0) {
            throw ConfigValidationError("Log rotation max_size_mb must be positive");
        }
        if (max_files <= 0) {
            throw ConfigValidationError("Log rotation max_files must be positive");
        }
        return true;
    }
};

/**
 * @brief Metrics configuration
 */
struct MetricsConfig {
    int prometheus_port = 9090;

    bool validate() const {
        if (prometheus_port <= 0 || prometheus_port > 65535) {
            throw ConfigValidationError("Prometheus port must be between 1 and 65535");
        }
        return true;
    }
};

/**
 * @brief Security configuration
 */
struct SecurityConfig {
    std::string tls_cert_path;
    std::string tls_key_path;
    std::string ca_cert_path;
    bool client_cert_required = false;

    bool validate() const {
        // TLS paths can be empty if TLS is not used
        return true;
    }
};

/**
 * @brief Connection configuration for Modbus TCP
 */
struct ModbusTcpConnectionConfig {
    std::string ip;
    int port = 502;
    int unit_id = 1;

    bool validate() const {
        if (ip.empty()) {
            throw ConfigValidationError("Modbus TCP IP address cannot be empty");
        }
        if (port <= 0 || port > 65535) {
            throw ConfigValidationError("Modbus TCP port must be between 1 and 65535");
        }
        if (unit_id < 0 || unit_id > 247) {
            throw ConfigValidationError("Modbus unit ID must be between 0 and 247");
        }
        return true;
    }
};

/**
 * @brief Connection configuration for Modbus RTU
 */
struct ModbusRtuConnectionConfig {
    std::string port;
    int baud_rate = 9600;
    int data_bits = 8;
    int stop_bits = 1;
    std::string parity = "none";
    int unit_id = 1;

    bool validate() const {
        if (port.empty()) {
            throw ConfigValidationError("Modbus RTU port cannot be empty");
        }
        if (baud_rate <= 0) {
            throw ConfigValidationError("Modbus RTU baud_rate must be positive");
        }
        if (data_bits != 7 && data_bits != 8) {
            throw ConfigValidationError("Modbus RTU data_bits must be 7 or 8");
        }
        if (stop_bits != 1 && stop_bits != 2) {
            throw ConfigValidationError("Modbus RTU stop_bits must be 1 or 2");
        }
        if (parity != "none" && parity != "even" && parity != "odd") {
            throw ConfigValidationError("Modbus RTU parity must be 'none', 'even', or 'odd'");
        }
        if (unit_id < 0 || unit_id > 247) {
            throw ConfigValidationError("Modbus unit ID must be between 0 and 247");
        }
        return true;
    }
};

/**
 * @brief Connection configuration for ECHONET Lite
 */
struct EchonetLiteConnectionConfig {
    std::string ip;

    bool validate() const {
        if (ip.empty()) {
            throw ConfigValidationError("ECHONET Lite IP address cannot be empty");
        }
        return true;
    }
};

/**
 * @brief Connection configuration variant for different protocols
 */
using ConnectionConfig = std::variant<
    ModbusTcpConnectionConfig,
    ModbusRtuConnectionConfig,
    EchonetLiteConnectionConfig
>;

/**
 * @brief Enumeration for device protocol types
 */
enum class ProtocolType {
    MODBUS_TCP,
    MODBUS_RTU,
    ECHONET_LITE,
    UNKNOWN
};

/**
 * @brief Convert string to protocol type
 * 
 * @param protocol Protocol string
 * @return ProtocolType Corresponding protocol type
 */
inline ProtocolType protocolFromString(const std::string& protocol) {
    if (protocol == "modbus_tcp") {
        return ProtocolType::MODBUS_TCP;
    } else if (protocol == "modbus_rtu") {
        return ProtocolType::MODBUS_RTU;
    } else if (protocol == "echonet_lite") {
        return ProtocolType::ECHONET_LITE;
    }
    return ProtocolType::UNKNOWN;
}

/**
 * @brief Convert protocol type to string
 * 
 * @param protocol Protocol type
 * @return std::string Corresponding protocol string
 */
inline std::string protocolToString(ProtocolType protocol) {
    switch (protocol) {
        case ProtocolType::MODBUS_TCP:
            return "modbus_tcp";
        case ProtocolType::MODBUS_RTU:
            return "modbus_rtu";
        case ProtocolType::ECHONET_LITE:
            return "echonet_lite";
        default:
            return "unknown";
    }
}

/**
 * @brief Enumeration for log levels
 */
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @brief Convert string to log level
 * 
 * @param level Log level string
 * @return LogLevel Corresponding log level
 */
inline LogLevel logLevelFromString(const std::string& level) {
    if (level == "TRACE") {
        return LogLevel::TRACE;
    } else if (level == "DEBUG") {
        return LogLevel::DEBUG;
    } else if (level == "INFO") {
        return LogLevel::INFO;
    } else if (level == "WARNING") {
        return LogLevel::WARNING;
    } else if (level == "ERROR") {
        return LogLevel::ERROR;
    } else if (level == "CRITICAL") {
        return LogLevel::CRITICAL;
    }
    return LogLevel::INFO; // Default
}

/**
 * @brief Convert log level to string
 * 
 * @param level Log level
 * @return std::string Corresponding log level string
 */
inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:
            return "TRACE";
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "INFO";
    }
}

} // namespace config
} // namespace ocpp_gateway