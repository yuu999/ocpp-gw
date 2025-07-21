#pragma once

#include <stdexcept>
#include <string>
#include <system_error>

namespace ocpp_gateway {
namespace common {

/**
 * @brief Base exception class for all OCPP Gateway exceptions
 */
class OcppGatewayException : public std::runtime_error {
public:
    explicit OcppGatewayException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown for configuration validation errors
 */
class ConfigValidationError : public OcppGatewayException {
public:
    explicit ConfigValidationError(const std::string& message)
        : OcppGatewayException(message) {}
};

/**
 * @brief Exception thrown for network communication errors
 */
class NetworkError : public OcppGatewayException {
public:
    explicit NetworkError(const std::string& message)
        : OcppGatewayException(message) {}
    
    NetworkError(const std::string& message, const std::error_code& ec)
        : OcppGatewayException(message + ": " + ec.message()),
          error_code_(ec) {}
    
    const std::error_code& error_code() const { return error_code_; }

private:
    std::error_code error_code_;
};

/**
 * @brief Exception thrown for protocol errors
 */
class ProtocolError : public OcppGatewayException {
public:
    explicit ProtocolError(const std::string& message)
        : OcppGatewayException(message) {}
};

/**
 * @brief Exception thrown for device communication errors
 */
class DeviceError : public OcppGatewayException {
public:
    explicit DeviceError(const std::string& message)
        : OcppGatewayException(message) {}
    
    DeviceError(const std::string& message, int device_error_code)
        : OcppGatewayException(message + " (code: " + std::to_string(device_error_code) + ")"),
          device_error_code_(device_error_code) {}
    
    int device_error_code() const __attribute__((unused)) { return device_error_code_; }

private:
    int device_error_code_ = 0;
};

/**
 * @brief Exception thrown for timeout errors
 */
class TimeoutError : public OcppGatewayException {
public:
    explicit TimeoutError(const std::string& message)
        : OcppGatewayException(message) {}
};

/**
 * @brief Exception thrown for security-related errors
 */
class SecurityError : public OcppGatewayException {
public:
    explicit SecurityError(const std::string& message)
        : OcppGatewayException(message) {}
};

/**
 * @brief Exception thrown for internal errors
 */
class InternalError : public OcppGatewayException {
public:
    explicit InternalError(const std::string& message)
        : OcppGatewayException(message) {}
};

/**
 * @brief Exception thrown for runtime errors
 */
class RuntimeError : public OcppGatewayException {
public:
    explicit RuntimeError(const std::string& message)
        : OcppGatewayException(message) {}
};

/**
 * @brief Utility class for error handling
 */
class ErrorUtils {
public:
    /**
     * @brief Convert a Boost error code to a NetworkError exception
     * @param ec Error code
     * @param message Error message prefix
     * @return NetworkError exception
     */
    static NetworkError makeNetworkError(const std::error_code& ec, const std::string& message) __attribute__((unused)) {
        return NetworkError(message, ec);
    }
    
    /**
     * @brief Convert a device error code to a DeviceError exception
     * @param error_code Device error code
     * @param message Error message prefix
     * @return DeviceError exception
     */
    static DeviceError makeDeviceError(int error_code, const std::string& message) __attribute__((unused)) {
        return DeviceError(message, error_code);
    }
    
    /**
     * @brief Check if an operation succeeded, throw an exception if not
     * @param success Success flag
     * @param message Error message if operation failed
     * @throws OcppGatewayException if success is false
     */
    static void checkOperation(bool success, const std::string& message) __attribute__((unused)) {
        if (!success) {
            throw OcppGatewayException(message);
        }
    }
    
    /**
     * @brief Check if a pointer is valid, throw an exception if not
     * @param ptr Pointer to check
     * @param message Error message if pointer is null
     * @throws OcppGatewayException if ptr is null
     */
    template<typename T>
    static void checkNotNull(const T* ptr, const std::string& message) __attribute__((unused)) {
        if (ptr == nullptr) {
            throw OcppGatewayException(message);
        }
    }
};

} // namespace common
} // namespace ocpp_gateway