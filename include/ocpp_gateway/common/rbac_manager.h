#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <optional>

namespace ocpp_gateway {
namespace common {

/**
 * @brief User roles enumeration
 */
enum class UserRole {
    ADMIN,      // Full access to all functions
    OPERATOR,   // Can monitor and configure devices
    VIEWER,     // Read-only access to monitoring data
    GUEST       // Limited access to basic information
};

/**
 * @brief API permissions enumeration
 */
enum class Permission {
    // System management
    SYSTEM_READ,
    SYSTEM_WRITE,
    SYSTEM_RESTART,
    
    // Device management
    DEVICE_READ,
    DEVICE_WRITE,
    DEVICE_ADD,
    DEVICE_DELETE,
    DEVICE_CONFIGURE,
    
    // Configuration management
    CONFIG_READ,
    CONFIG_WRITE,
    CONFIG_RELOAD,
    
    // Monitoring and metrics
    METRICS_READ,
    LOGS_READ,
    
    // Security management
    SECURITY_READ,
    SECURITY_WRITE,
    USER_MANAGE,
    
    // OCPP management
    OCPP_READ,
    OCPP_WRITE,
    OCPP_CONFIGURE
};

/**
 * @brief User information
 */
struct UserInfo {
    std::string username;
    std::string password_hash;
    UserRole role;
    std::string email;
    std::string full_name;
    bool is_active;
    std::string created_at;
    std::string last_login;
    std::vector<std::string> groups;
};

/**
 * @brief Authentication result
 */
struct AuthResult {
    bool success;
    std::string username;
    UserRole role;
    std::string token;
    std::string error_message;
    std::vector<Permission> permissions;
};

/**
 * @brief Exception for RBAC-related errors
 */
class RbacError : public std::runtime_error {
public:
    explicit RbacError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief RBAC Manager for handling authentication and authorization
 */
class RbacManager {
public:
    /**
     * @brief Construct a new RBAC Manager
     */
    RbacManager();

    /**
     * @brief Destructor
     */
    ~RbacManager();

    /**
     * @brief Initialize RBAC system
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();

    /**
     * @brief Load users from configuration file
     * 
     * @param config_file Path to user configuration file
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool loadUsers(const std::string& config_file);

    /**
     * @brief Save users to configuration file
     * 
     * @param config_file Path to user configuration file
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool saveUsers(const std::string& config_file) const;

    /**
     * @brief Authenticate user with username and password
     * 
     * @param username Username
     * @param password Password
     * @return AuthResult Authentication result
     */
    AuthResult authenticate(const std::string& username, const std::string& password) const;

    /**
     * @brief Authenticate user with token
     * 
     * @param token Authentication token
     * @return AuthResult Authentication result
     */
    AuthResult authenticateToken(const std::string& token) const;

    /**
     * @brief Check if user has permission
     * 
     * @param username Username
     * @param permission Permission to check
     * @return true if user has permission
     * @return false if user does not have permission
     */
    bool hasPermission(const std::string& username, Permission permission) const;

    /**
     * @brief Check if user has any of the permissions
     * 
     * @param username Username
     * @param permissions List of permissions to check
     * @return true if user has at least one permission
     * @return false if user has none of the permissions
     */
    bool hasAnyPermission(const std::string& username, const std::vector<Permission>& permissions) const;

    /**
     * @brief Check if user has all of the permissions
     * 
     * @param username Username
     * @param permissions List of permissions to check
     * @return true if user has all permissions
     * @return false if user is missing any permission
     */
    bool hasAllPermissions(const std::string& username, const std::vector<Permission>& permissions) const;

    /**
     * @brief Get user information
     * 
     * @param username Username
     * @return std::optional<UserInfo> User information if found
     */
    std::optional<UserInfo> getUserInfo(const std::string& username) const;

    /**
     * @brief Add new user
     * 
     * @param user_info User information
     * @return true if user was added successfully
     * @return false if user addition failed
     */
    bool addUser(const UserInfo& user_info);

    /**
     * @brief Update user information
     * 
     * @param username Username
     * @param user_info Updated user information
     * @return true if user was updated successfully
     * @return false if user update failed
     */
    bool updateUser(const std::string& username, const UserInfo& user_info);

    /**
     * @brief Delete user
     * 
     * @param username Username
     * @return true if user was deleted successfully
     * @return false if user deletion failed
     */
    bool deleteUser(const std::string& username);

    /**
     * @brief Change user password
     * 
     * @param username Username
     * @param old_password Old password
     * @param new_password New password
     * @return true if password was changed successfully
     * @return false if password change failed
     */
    bool changePassword(const std::string& username, const std::string& old_password, const std::string& new_password);

    /**
     * @brief Generate authentication token
     * 
     * @param username Username
     * @param duration_hours Token duration in hours
     * @return std::string Generated token
     */
    std::string generateToken(const std::string& username, int duration_hours = 24) const;

    /**
     * @brief Validate token
     * 
     * @param token Authentication token
     * @return true if token is valid
     * @return false if token is invalid
     */
    bool validateToken(const std::string& token) const;

    /**
     * @brief Revoke token
     * 
     * @param token Authentication token
     * @return true if token was revoked successfully
     * @return false if token revocation failed
     */
    bool revokeToken(const std::string& token);

    /**
     * @brief Get all users
     * 
     * @return std::vector<UserInfo> List of all users
     */
    std::vector<UserInfo> getAllUsers() const;

    /**
     * @brief Get permissions for role
     * 
     * @param role User role
     * @return std::vector<Permission> List of permissions for role
     */
    std::vector<Permission> getPermissionsForRole(UserRole role) const;

    /**
     * @brief Convert role to string
     * 
     * @param role User role
     * @return std::string Role string
     */
    static std::string roleToString(UserRole role);

    /**
     * @brief Convert string to role
     * 
     * @param role_string Role string
     * @return UserRole User role
     */
    static UserRole roleFromString(const std::string& role_string);

    /**
     * @brief Convert permission to string
     * 
     * @param permission Permission
     * @return std::string Permission string
     */
    static std::string permissionToString(Permission permission);

    /**
     * @brief Convert string to permission
     * 
     * @param permission_string Permission string
     * @return Permission Permission
     */
    static Permission permissionFromString(const std::string& permission_string);

private:
    std::map<std::string, UserInfo> users_;
    std::map<std::string, std::string> tokens_; // token -> username
    std::map<UserRole, std::set<Permission>> role_permissions_;
    
    /**
     * @brief Hash password
     * 
     * @param password Plain text password
     * @return std::string Hashed password
     */
    std::string hashPassword(const std::string& password) const;

    /**
     * @brief Verify password hash
     * 
     * @param password Plain text password
     * @param hash Password hash
     * @return true if password matches hash
     * @return false if password does not match hash
     */
    bool verifyPassword(const std::string& password, const std::string& hash) const;

    /**
     * @brief Initialize default role permissions
     */
    void initializeRolePermissions();

    /**
     * @brief Get current timestamp
     * 
     * @return std::string Current timestamp string
     */
    std::string getCurrentTimestamp() const;
};

} // namespace common
} // namespace ocpp_gateway 