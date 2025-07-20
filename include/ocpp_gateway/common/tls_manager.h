#pragma once

#include "ocpp_gateway/common/config_types.h"
#include <string>
#include <memory>
#include <vector>
#include <optional>

namespace ocpp_gateway {
namespace common {

/**
 * @brief TLS certificate information
 */
struct CertificateInfo {
    std::string subject;
    std::string issuer;
    std::string serial_number;
    std::string not_before;
    std::string not_after;
    std::vector<std::string> dns_names;
    std::vector<std::string> ip_addresses;
    bool is_valid;
    std::string error_message;
};

/**
 * @brief TLS configuration context
 */
struct TlsContext {
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
    std::string version;
    bool verify_server;
    bool verify_client;
    std::string cipher_suites;
};

/**
 * @brief Exception for TLS-related errors
 */
class TlsError : public std::runtime_error {
public:
    explicit TlsError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief TLS Manager for handling certificates and TLS configuration
 */
class TlsManager {
public:
    /**
     * @brief Construct a new TLS Manager
     */
    TlsManager();

    /**
     * @brief Construct a new TLS Manager with configuration
     * 
     * @param config Security configuration
     */
    explicit TlsManager(const config::SecurityConfig& config);

    /**
     * @brief Destructor
     */
    ~TlsManager();

    /**
     * @brief Load TLS configuration
     * 
     * @param config Security configuration
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadConfiguration(const config::SecurityConfig& config);

    /**
     * @brief Validate TLS configuration
     * 
     * @return true if configuration is valid
     * @return false if configuration is invalid
     */
    bool validateConfiguration() const;

    /**
     * @brief Check if TLS is enabled
     * 
     * @return true if TLS is enabled
     * @return false if TLS is disabled
     */
    bool isTlsEnabled() const;

    /**
     * @brief Get certificate information
     * 
     * @param cert_path Path to certificate file
     * @return CertificateInfo Certificate information
     */
    CertificateInfo getCertificateInfo(const std::string& cert_path) const;

    /**
     * @brief Validate certificate file
     * 
     * @param cert_path Path to certificate file
     * @return true if certificate is valid
     * @return false if certificate is invalid
     */
    bool validateCertificate(const std::string& cert_path) const;

    /**
     * @brief Validate private key file
     * 
     * @param key_path Path to private key file
     * @return true if private key is valid
     * @return false if private key is invalid
     */
    bool validatePrivateKey(const std::string& key_path) const;

    /**
     * @brief Check certificate expiration
     * 
     * @param cert_path Path to certificate file
     * @param days_warning Days before expiration to warn
     * @return std::optional<int> Days until expiration, or empty if invalid
     */
    std::optional<int> checkCertificateExpiration(const std::string& cert_path, int days_warning = 30) const;

    /**
     * @brief Generate self-signed certificate for testing
     * 
     * @param cert_path Path to save certificate
     * @param key_path Path to save private key
     * @param common_name Common name for certificate
     * @param days_valid Days the certificate should be valid
     * @return true if generation was successful
     * @return false if generation failed
     */
    bool generateSelfSignedCertificate(
        const std::string& cert_path,
        const std::string& key_path,
        const std::string& common_name,
        int days_valid = 365
    ) const;

    /**
     * @brief Get TLS context for Boost.Beast or other libraries
     * 
     * @return TlsContext TLS context information
     */
    TlsContext getTlsContext() const;

    /**
     * @brief Get supported cipher suites
     * 
     * @return std::vector<std::string> List of supported cipher suites
     */
    std::vector<std::string> getSupportedCipherSuites() const;

    /**
     * @brief Set custom cipher suites
     * 
     * @param cipher_suites Comma-separated list of cipher suites
     */
    void setCustomCipherSuites(const std::string& cipher_suites);

private:
    config::SecurityConfig config_;
    bool tls_enabled_;
    std::string custom_cipher_suites_;

    /**
     * @brief Check if file exists and is readable
     * 
     * @param file_path Path to file
     * @return true if file exists and is readable
     * @return false otherwise
     */
    bool checkFileAccess(const std::string& file_path) const;

    /**
     * @brief Parse certificate dates
     * 
     * @param date_string Date string from certificate
     * @return std::string Formatted date string
     */
    std::string parseCertificateDate(const std::string& date_string) const;
};

} // namespace common
} // namespace ocpp_gateway 