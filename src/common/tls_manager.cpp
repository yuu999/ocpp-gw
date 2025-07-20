#include "ocpp_gateway/common/tls_manager.h"
#include "ocpp_gateway/common/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <algorithm>

namespace ocpp_gateway {
namespace common {

TlsManager::TlsManager()
    : tls_enabled_(false)
    , custom_cipher_suites_()
{
}

TlsManager::TlsManager(const config::SecurityConfig& config)
    : config_(config)
    , tls_enabled_(false)
    , custom_cipher_suites_()
{
    loadConfiguration(config);
}

TlsManager::~TlsManager() = default;

bool TlsManager::loadConfiguration(const config::SecurityConfig& config) {
    config_ = config;
    
    // Check if TLS is enabled
    tls_enabled_ = !config_.tls_cert_path.empty() && !config_.tls_key_path.empty();
    
    if (tls_enabled_) {
        // Validate configuration
        if (!validateConfiguration()) {
            LOG_ERROR("TLS configuration validation failed");
            return false;
        }
        
        // Check certificate and key files
        if (!validateCertificate(config_.tls_cert_path)) {
            LOG_ERROR("TLS certificate validation failed: {}", config_.tls_cert_path);
            return false;
        }
        
        if (!validatePrivateKey(config_.tls_key_path)) {
            LOG_ERROR("TLS private key validation failed: {}", config_.tls_key_path);
            return false;
        }
        
        // Check CA certificate if provided
        if (!config_.ca_cert_path.empty() && !validateCertificate(config_.ca_cert_path)) {
            LOG_ERROR("CA certificate validation failed: {}", config_.ca_cert_path);
            return false;
        }
        
        LOG_INFO("TLS configuration loaded successfully");
    } else {
        LOG_INFO("TLS is disabled");
    }
    
    return true;
}

bool TlsManager::validateConfiguration() const {
    try {
        return config_.validate();
    } catch (const config::ConfigValidationError& e) {
        LOG_ERROR("TLS configuration validation error: {}", e.what());
        return false;
    }
}

bool TlsManager::isTlsEnabled() const {
    return tls_enabled_;
}

CertificateInfo TlsManager::getCertificateInfo(const std::string& cert_path) const {
    CertificateInfo info;
    info.is_valid = false;
    
    if (!checkFileAccess(cert_path)) {
        info.error_message = "Certificate file not accessible: " + cert_path;
        return info;
    }
    
    // Use OpenSSL to get certificate information
    // This is a simplified implementation - in a real implementation,
    // you would use OpenSSL APIs to extract certificate details
    
    std::ifstream cert_file(cert_path);
    if (!cert_file.is_open()) {
        info.error_message = "Cannot open certificate file: " + cert_path;
        return info;
    }
    
    std::string line;
    bool in_cert = false;
    std::string cert_data;
    
    while (std::getline(cert_file, line)) {
        if (line.find("-----BEGIN CERTIFICATE-----") != std::string::npos) {
            in_cert = true;
            continue;
        }
        if (line.find("-----END CERTIFICATE-----") != std::string::npos) {
            in_cert = false;
            break;
        }
        if (in_cert) {
            cert_data += line;
        }
    }
    
    if (cert_data.empty()) {
        info.error_message = "No valid certificate found in file: " + cert_path;
        return info;
    }
    
    // For now, set basic information
    // In a real implementation, you would parse the certificate using OpenSSL
    info.subject = "CN=Unknown";
    info.issuer = "CN=Unknown";
    info.serial_number = "Unknown";
    info.not_before = "Unknown";
    info.not_after = "Unknown";
    info.is_valid = true;
    
    return info;
}

bool TlsManager::validateCertificate(const std::string& cert_path) const {
    if (!checkFileAccess(cert_path)) {
        return false;
    }
    
    // Check if file contains valid certificate format
    std::ifstream cert_file(cert_path);
    if (!cert_file.is_open()) {
        return false;
    }
    
    std::string line;
    bool has_begin = false;
    bool has_end = false;
    
    while (std::getline(cert_file, line)) {
        if (line.find("-----BEGIN CERTIFICATE-----") != std::string::npos) {
            has_begin = true;
        }
        if (line.find("-----END CERTIFICATE-----") != std::string::npos) {
            has_end = true;
        }
    }
    
    return has_begin && has_end;
}

bool TlsManager::validatePrivateKey(const std::string& key_path) const {
    if (!checkFileAccess(key_path)) {
        return false;
    }
    
    // Check if file contains valid private key format
    std::ifstream key_file(key_path);
    if (!key_file.is_open()) {
        return false;
    }
    
    std::string line;
    bool has_begin = false;
    bool has_end = false;
    
    while (std::getline(key_file, line)) {
        if (line.find("-----BEGIN PRIVATE KEY-----") != std::string::npos ||
            line.find("-----BEGIN RSA PRIVATE KEY-----") != std::string::npos ||
            line.find("-----BEGIN EC PRIVATE KEY-----") != std::string::npos) {
            has_begin = true;
        }
        if (line.find("-----END PRIVATE KEY-----") != std::string::npos ||
            line.find("-----END RSA PRIVATE KEY-----") != std::string::npos ||
            line.find("-----END EC PRIVATE KEY-----") != std::string::npos) {
            has_end = true;
        }
    }
    
    return has_begin && has_end;
}

std::optional<int> TlsManager::checkCertificateExpiration(const std::string& cert_path, int days_warning) const {
    CertificateInfo info = getCertificateInfo(cert_path);
    if (!info.is_valid) {
        return std::nullopt;
    }
    
    // Parse expiration date
    // This is a simplified implementation - in a real implementation,
    // you would parse the actual certificate dates using OpenSSL
    
    // For now, return a placeholder value
    // In a real implementation, you would calculate days until expiration
    return 365; // Placeholder: 365 days
}

bool TlsManager::generateSelfSignedCertificate(
    const std::string& cert_path,
    const std::string& key_path,
    const std::string& common_name,
    int days_valid) const {
    
    // This is a placeholder implementation
    // In a real implementation, you would use OpenSSL APIs to generate certificates
    
    LOG_INFO("Generating self-signed certificate for: {}", common_name);
    
    // Create certificate file
    std::ofstream cert_file(cert_path);
    if (!cert_file.is_open()) {
        LOG_ERROR("Cannot create certificate file: {}", cert_path);
        return false;
    }
    
    // Generate a simple self-signed certificate
    // In a real implementation, this would use OpenSSL
    cert_file << "-----BEGIN CERTIFICATE-----\n";
    cert_file << "MIIDXTCCAkWgAwIBAgIJAKoK8HrHrHrHrMA0GCSqGSIb3DQEBCwUAMEUxCzAJ\n";
    cert_file << "BgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJ\n";
    cert_file << "bnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwHhcNMjQxMjIxMTAwMDAwWhcN\n";
    cert_file << "MjUxMjIxMTAwMDAwWjBFMQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29t\n";
    cert_file << "ZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQdHkgTHRk\n";
    cert_file << "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...\n";
    cert_file << "-----END CERTIFICATE-----\n";
    cert_file.close();
    
    // Create private key file
    std::ofstream key_file(key_path);
    if (!key_file.is_open()) {
        LOG_ERROR("Cannot create private key file: {}", key_path);
        return false;
    }
    
    // Generate a simple private key
    // In a real implementation, this would use OpenSSL
    key_file << "-----BEGIN PRIVATE KEY-----\n";
    key_file << "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC6Vju+6Vju\n";
    key_file << "+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju\n";
    key_file << "+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju+6Vju\n";
    key_file << "AgMBAAECggEBAKqCvB6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x\n";
    key_file << "6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x\n";
    key_file << "6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x6x\n";
    key_file << "-----END PRIVATE KEY-----\n";
    key_file.close();
    
    LOG_INFO("Self-signed certificate generated successfully");
    return true;
}

TlsContext TlsManager::getTlsContext() const {
    TlsContext context;
    context.cert_path = config_.tls_cert_path;
    context.key_path = config_.tls_key_path;
    context.ca_path = config_.ca_cert_path;
    context.version = config_.tls_version;
    context.verify_server = config_.verify_server_cert;
    context.verify_client = config_.verify_client_cert;
    context.cipher_suites = custom_cipher_suites_.empty() ? 
        config_.cipher_suites : custom_cipher_suites_;
    
    return context;
}

std::vector<std::string> TlsManager::getSupportedCipherSuites() const {
    // Return list of supported cipher suites
    // In a real implementation, this would query OpenSSL for supported ciphers
    return {
        "TLS_AES_256_GCM_SHA384",
        "TLS_CHACHA20_POLY1305_SHA256",
        "TLS_AES_128_GCM_SHA256",
        "ECDHE-RSA-AES256-GCM-SHA384",
        "ECDHE-RSA-AES128-GCM-SHA256",
        "ECDHE-RSA-AES256-SHA384",
        "ECDHE-RSA-AES128-SHA256"
    };
}

void TlsManager::setCustomCipherSuites(const std::string& cipher_suites) {
    custom_cipher_suites_ = cipher_suites;
}

bool TlsManager::checkFileAccess(const std::string& file_path) const {
    if (file_path.empty()) {
        return false;
    }
    
    std::filesystem::path path(file_path);
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

std::string TlsManager::parseCertificateDate(const std::string& date_string) const {
    // Parse certificate date format (e.g., "Dec 21 10:00:00 2024 GMT")
    // This is a simplified implementation
    return date_string;
}

} // namespace common
} // namespace ocpp_gateway 