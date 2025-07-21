#include "ocpp_gateway/common/rbac_manager.h"
#include "ocpp_gateway/common/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>
#include <algorithm>
#include <cstring>

namespace ocpp_gateway {
namespace common {

RbacManager::RbacManager() {
    initializeRolePermissions();
}

RbacManager::~RbacManager() = default;

bool RbacManager::initialize() {
    // Create default admin user if no users exist
    if (users_.empty()) {
        UserInfo admin_user;
        admin_user.username = "admin";
        admin_user.password_hash = hashPassword("admin");
        admin_user.role = UserRole::ADMIN;
        admin_user.email = "admin@ocpp-gateway.local";
        admin_user.full_name = "System Administrator";
        admin_user.is_active = true;
        admin_user.created_at = getCurrentTimestamp();
        admin_user.last_login = "";
        
        users_["admin"] = admin_user;
        
        LOG_INFO("Created default admin user");
    }
    
    return true;
}

bool RbacManager::loadUsers(const std::string& config_file) {
    // This is a simplified implementation
    // In a real implementation, you would parse YAML/JSON configuration
    
    std::ifstream file(config_file);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open user configuration file: {}", config_file);
        return false;
    }
    
    // For now, just log that we're loading users
    LOG_INFO("Loading users from configuration file: {}", config_file);
    
    // In a real implementation, you would parse the file and populate users_
    
    return true;
}

bool RbacManager::saveUsers(const std::string& config_file) const {
    // This is a simplified implementation
    // In a real implementation, you would serialize to YAML/JSON
    
    std::ofstream file(config_file);
    if (!file.is_open()) {
        LOG_ERROR("Cannot create user configuration file: {}", config_file);
        return false;
    }
    
    LOG_INFO("Saving users to configuration file: {}", config_file);
    
    // In a real implementation, you would serialize users_ to the file
    
    return true;
}

AuthResult RbacManager::authenticate(const std::string& username, const std::string& password) const {
    AuthResult result;
    result.success = false;
    result.role = UserRole::GUEST; // Initialize with default role
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        result.error_message = "User not found";
        return result;
    }
    
    const UserInfo& user = it->second;
    
    if (!user.is_active) {
        result.error_message = "User account is disabled";
        return result;
    }
    
    if (!verifyPassword(password, user.password_hash)) {
        result.error_message = "Invalid password";
        return result;
    }
    
    // Generate token
    std::string token = generateToken(username);
    
    result.success = true;
    result.username = username;
    result.role = user.role;
    result.token = token;
    result.permissions = getPermissionsForRole(user.role);
    
    LOG_INFO("User authenticated successfully: {}", username);
    
    return result;
}

AuthResult RbacManager::authenticateToken(const std::string& token) const {
    AuthResult result;
    result.success = false;
    result.role = UserRole::GUEST; // Initialize with default role
    
    auto it = tokens_.find(token);
    if (it == tokens_.end()) {
        result.error_message = "Invalid token";
        return result;
    }
    
    std::string username = it->second;
    auto user_it = users_.find(username);
    if (user_it == users_.end()) {
        result.error_message = "User not found";
        return result;
    }
    
    const UserInfo& user = user_it->second;
    
    if (!user.is_active) {
        result.error_message = "User account is disabled";
        return result;
    }
    
    result.success = true;
    result.username = username;
    result.role = user.role;
    result.token = token;
    result.permissions = getPermissionsForRole(user.role);
    
    return result;
}

bool RbacManager::hasPermission(const std::string& username, Permission permission) const {
    auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }
    
    const UserInfo& user = it->second;
    if (!user.is_active) {
        return false;
    }
    
    auto role_it = role_permissions_.find(user.role);
    if (role_it == role_permissions_.end()) {
        return false;
    }
    
    return role_it->second.find(permission) != role_it->second.end();
}

bool RbacManager::hasAnyPermission(const std::string& username, const std::vector<Permission>& permissions) const {
    for (const auto& permission : permissions) {
        if (hasPermission(username, permission)) {
            return true;
        }
    }
    return false;
}

bool RbacManager::hasAllPermissions(const std::string& username, const std::vector<Permission>& permissions) const {
    for (const auto& permission : permissions) {
        if (!hasPermission(username, permission)) {
            return false;
        }
    }
    return true;
}

std::optional<UserInfo> RbacManager::getUserInfo(const std::string& username) const {
    auto it = users_.find(username);
    if (it == users_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool RbacManager::addUser(const UserInfo& user_info) {
    if (users_.find(user_info.username) != users_.end()) {
        LOG_ERROR("User already exists: {}", user_info.username);
        return false;
    }
    
    users_[user_info.username] = user_info;
    LOG_INFO("Added user: {}", user_info.username);
    return true;
}

bool RbacManager::updateUser(const std::string& username, const UserInfo& user_info) {
    auto it = users_.find(username);
    if (it == users_.end()) {
        LOG_ERROR("User not found: {}", username);
        return false;
    }
    
    users_[username] = user_info;
    LOG_INFO("Updated user: {}", username);
    return true;
}

bool RbacManager::deleteUser(const std::string& username) {
    auto it = users_.find(username);
    if (it == users_.end()) {
        LOG_ERROR("User not found: {}", username);
        return false;
    }
    
    users_.erase(it);
    LOG_INFO("Deleted user: {}", username);
    return true;
}

bool RbacManager::changePassword(const std::string& username, const std::string& old_password, const std::string& new_password) {
    auto it = users_.find(username);
    if (it == users_.end()) {
        LOG_ERROR("User not found: {}", username);
        return false;
    }
    
    UserInfo& user = it->second;
    
    if (!verifyPassword(old_password, user.password_hash)) {
        LOG_ERROR("Invalid old password for user: {}", username);
        return false;
    }
    
    user.password_hash = hashPassword(new_password);
    LOG_INFO("Changed password for user: {}", username);
    return true;
}

std::string RbacManager::generateToken(const std::string& username, int duration_hours) const {
    // Generate a simple token (in a real implementation, you would use JWT)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    ss << username << "_" << std::hex;
    for (int i = 0; i < 16; ++i) {
        ss << std::setw(2) << std::setfill('0') << dis(gen);
    }
    
    return ss.str();
}

bool RbacManager::validateToken(const std::string& token) const {
    return tokens_.find(token) != tokens_.end();
}

bool RbacManager::revokeToken(const std::string& token) {
    auto it = tokens_.find(token);
    if (it == tokens_.end()) {
        return false;
    }
    
    std::string username = it->second; // Store username before erasing
    tokens_.erase(it);
    LOG_INFO("Revoked token for user: {}", username);
    return true;
}

std::vector<UserInfo> RbacManager::getAllUsers() const {
    std::vector<UserInfo> result;
    for (const auto& pair : users_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<Permission> RbacManager::getPermissionsForRole(UserRole role) const {
    auto it = role_permissions_.find(role);
    if (it == role_permissions_.end()) {
        return {};
    }
    
    std::vector<Permission> result;
    for (const auto& permission : it->second) {
        result.push_back(permission);
    }
    return result;
}

std::string RbacManager::roleToString(UserRole role) {
    switch (role) {
        case UserRole::ADMIN:
            return "admin";
        case UserRole::OPERATOR:
            return "operator";
        case UserRole::VIEWER:
            return "viewer";
        case UserRole::GUEST:
            return "guest";
        default:
            return "unknown";
    }
}

UserRole RbacManager::roleFromString(const std::string& role_string) {
    if (role_string == "admin") {
        return UserRole::ADMIN;
    } else if (role_string == "operator") {
        return UserRole::OPERATOR;
    } else if (role_string == "viewer") {
        return UserRole::VIEWER;
    } else if (role_string == "guest") {
        return UserRole::GUEST;
    }
    return UserRole::GUEST; // Default
}

std::string RbacManager::permissionToString(Permission permission) {
    switch (permission) {
        case Permission::SYSTEM_READ:
            return "system_read";
        case Permission::SYSTEM_WRITE:
            return "system_write";
        case Permission::SYSTEM_RESTART:
            return "system_restart";
        case Permission::DEVICE_READ:
            return "device_read";
        case Permission::DEVICE_WRITE:
            return "device_write";
        case Permission::DEVICE_ADD:
            return "device_add";
        case Permission::DEVICE_DELETE:
            return "device_delete";
        case Permission::DEVICE_CONFIGURE:
            return "device_configure";
        case Permission::CONFIG_READ:
            return "config_read";
        case Permission::CONFIG_WRITE:
            return "config_write";
        case Permission::CONFIG_RELOAD:
            return "config_reload";
        case Permission::METRICS_READ:
            return "metrics_read";
        case Permission::LOGS_READ:
            return "logs_read";
        case Permission::SECURITY_READ:
            return "security_read";
        case Permission::SECURITY_WRITE:
            return "security_write";
        case Permission::USER_MANAGE:
            return "user_manage";
        case Permission::OCPP_READ:
            return "ocpp_read";
        case Permission::OCPP_WRITE:
            return "ocpp_write";
        case Permission::OCPP_CONFIGURE:
            return "ocpp_configure";
        default:
            return "unknown";
    }
}

Permission RbacManager::permissionFromString(const std::string& permission_string) {
    if (permission_string == "system_read") {
        return Permission::SYSTEM_READ;
    } else if (permission_string == "system_write") {
        return Permission::SYSTEM_WRITE;
    } else if (permission_string == "system_restart") {
        return Permission::SYSTEM_RESTART;
    } else if (permission_string == "device_read") {
        return Permission::DEVICE_READ;
    } else if (permission_string == "device_write") {
        return Permission::DEVICE_WRITE;
    } else if (permission_string == "device_add") {
        return Permission::DEVICE_ADD;
    } else if (permission_string == "device_delete") {
        return Permission::DEVICE_DELETE;
    } else if (permission_string == "device_configure") {
        return Permission::DEVICE_CONFIGURE;
    } else if (permission_string == "config_read") {
        return Permission::CONFIG_READ;
    } else if (permission_string == "config_write") {
        return Permission::CONFIG_WRITE;
    } else if (permission_string == "config_reload") {
        return Permission::CONFIG_RELOAD;
    } else if (permission_string == "metrics_read") {
        return Permission::METRICS_READ;
    } else if (permission_string == "logs_read") {
        return Permission::LOGS_READ;
    } else if (permission_string == "security_read") {
        return Permission::SECURITY_READ;
    } else if (permission_string == "security_write") {
        return Permission::SECURITY_WRITE;
    } else if (permission_string == "user_manage") {
        return Permission::USER_MANAGE;
    } else if (permission_string == "ocpp_read") {
        return Permission::OCPP_READ;
    } else if (permission_string == "ocpp_write") {
        return Permission::OCPP_WRITE;
    } else if (permission_string == "ocpp_configure") {
        return Permission::OCPP_CONFIGURE;
    }
    return Permission::SYSTEM_READ; // Default
}

std::string RbacManager::hashPassword(const std::string& password) const {
    // This is a simplified implementation
    // In a real implementation, you would use a proper hashing library like bcrypt
    
    // Simple hash for demonstration
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool RbacManager::verifyPassword(const std::string& password, const std::string& hash) const {
    std::string password_hash = hashPassword(password);
    return password_hash == hash;
}

void RbacManager::initializeRolePermissions() {
    // Admin role - full access
    role_permissions_[UserRole::ADMIN] = {
        Permission::SYSTEM_READ, Permission::SYSTEM_WRITE, Permission::SYSTEM_RESTART,
        Permission::DEVICE_READ, Permission::DEVICE_WRITE, Permission::DEVICE_ADD,
        Permission::DEVICE_DELETE, Permission::DEVICE_CONFIGURE,
        Permission::CONFIG_READ, Permission::CONFIG_WRITE, Permission::CONFIG_RELOAD,
        Permission::METRICS_READ, Permission::LOGS_READ,
        Permission::SECURITY_READ, Permission::SECURITY_WRITE, Permission::USER_MANAGE,
        Permission::OCPP_READ, Permission::OCPP_WRITE, Permission::OCPP_CONFIGURE
    };
    
    // Operator role - device management and monitoring
    role_permissions_[UserRole::OPERATOR] = {
        Permission::SYSTEM_READ,
        Permission::DEVICE_READ, Permission::DEVICE_WRITE, Permission::DEVICE_CONFIGURE,
        Permission::CONFIG_READ,
        Permission::METRICS_READ, Permission::LOGS_READ,
        Permission::OCPP_READ, Permission::OCPP_WRITE
    };
    
    // Viewer role - read-only access
    role_permissions_[UserRole::VIEWER] = {
        Permission::SYSTEM_READ,
        Permission::DEVICE_READ,
        Permission::CONFIG_READ,
        Permission::METRICS_READ, Permission::LOGS_READ,
        Permission::OCPP_READ
    };
    
    // Guest role - limited access
    role_permissions_[UserRole::GUEST] = {
        Permission::SYSTEM_READ,
        Permission::DEVICE_READ,
        Permission::METRICS_READ
    };
}

std::string RbacManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

} // namespace common
} // namespace ocpp_gateway 