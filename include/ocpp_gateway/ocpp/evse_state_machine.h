#pragma once

#include <string>
#include <memory>
#include <map>
#include <optional>
#include <chrono>
#include <functional>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/error.h"

namespace ocpp_gateway {
namespace ocpp {

/**
 * @enum ConnectorStatus
 * @brief OCPP connector status values
 */
enum class ConnectorStatus {
    AVAILABLE,
    OCCUPIED,
    RESERVED,
    UNAVAILABLE,
    FAULTED
};

/**
 * @enum EvseState
 * @brief EVSE state machine states
 */
enum class EvseState {
    AVAILABLE,      // EVSE is available for charging
    PREPARING,      // EVSE is preparing for charging (e.g., user authentication)
    CHARGING,       // EVSE is actively charging a vehicle
    SUSPENDED_EV,   // Charging suspended by EV
    SUSPENDED_EVSE, // Charging suspended by EVSE
    FINISHING,      // Charging is finishing
    RESERVED,       // EVSE is reserved
    UNAVAILABLE,    // EVSE is unavailable
    FAULTED         // EVSE is in fault state
};

/**
 * @enum EvseEvent
 * @brief Events that can trigger state transitions
 */
enum class EvseEvent {
    PLUG_IN,                // Vehicle plugged in
    PLUG_OUT,               // Vehicle unplugged
    AUTHORIZE_START,        // Authorization to start charging
    AUTHORIZE_STOP,         // Authorization to stop charging
    START_CHARGING,         // Start charging
    STOP_CHARGING,          // Stop charging
    SUSPEND_CHARGING_EV,    // Suspend charging by EV
    SUSPEND_CHARGING_EVSE,  // Suspend charging by EVSE
    RESUME_CHARGING,        // Resume charging
    RESERVE,                // Reserve EVSE
    CANCEL_RESERVATION,     // Cancel reservation
    SET_UNAVAILABLE,        // Set EVSE unavailable
    SET_AVAILABLE,          // Set EVSE available
    FAULT_DETECTED,         // Fault detected
    FAULT_CLEARED           // Fault cleared
};

/**
 * @struct SampledValue
 * @brief Represents a sampled value in a meter value
 */
struct SampledValue {
    std::string value;
    std::string context;
    std::string format;
    std::string measurand;
    std::string phase;
    std::string location;
    std::string unit;
};

/**
 * @struct MeterValue
 * @brief Represents a meter value with timestamp and sampled values
 */
struct MeterValue {
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::vector<SampledValue> sampledValues;
};

/**
 * @enum TransactionStatus
 * @brief Status of a charging transaction
 */
enum class TransactionStatus {
    ACTIVE,
    COMPLETED,
    EXPIRED,
    REJECTED
};

/**
 * @struct Transaction
 * @brief Represents a charging transaction
 */
struct Transaction {
    std::string id;
    std::string idTag;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::optional<std::chrono::time_point<std::chrono::system_clock>> stopTime;
    std::vector<MeterValue> meterValues;
    TransactionStatus status;
};

/**
 * @struct Variable
 * @brief Represents an OCPP variable
 */
struct Variable {
    std::string name;
    std::string value;
    std::string dataType;
    std::optional<double> scale;
    std::optional<std::string> unit;
    std::optional<std::map<std::string, std::string>> enumMapping;
};

/**
 * @class EvseStateMachine
 * @brief State machine for EVSE status tracking and transaction management
 */
class EvseStateMachine : public std::enable_shared_from_this<EvseStateMachine> {
public:
    using StatusChangeCallback = std::function<void(int, const std::string&, const std::string&)>;
    using MeterValueCallback = std::function<void(int, double)>;
    using TransactionEventCallback = std::function<void(const std::string&, const std::string&, 
                                                       const std::string&, int, const std::string&, 
                                                       int, double)>;

    /**
     * @brief Create an EvseStateMachine instance
     * @param io_context Boost IO context
     * @param evseId EVSE ID
     * @param connectorId Connector ID
     * @return Shared pointer to EvseStateMachine
     */
    static std::shared_ptr<EvseStateMachine> create(
        boost::asio::io_context& io_context,
        int evseId,
        int connectorId);
    
    /**
     * @brief Process an event
     * @param event Event to process
     * @param data Additional data for the event (optional)
     * @return true if event was processed successfully, false otherwise
     */
    bool processEvent(EvseEvent event, const nlohmann::json& data = nlohmann::json());
    
    /**
     * @brief Get the current state
     * @return Current state
     */
    EvseState getCurrentState() const;
    
    /**
     * @brief Get the connector status
     * @return Connector status
     */
    ConnectorStatus getConnectorStatus() const;
    
    /**
     * @brief Get the current transaction
     * @return Current transaction or nullopt if no transaction is active
     */
    std::optional<Transaction> getCurrentTransaction() const;
    
    /**
     * @brief Set a variable value
     * @param name Variable name
     * @param value Variable value
     * @param dataType Data type
     * @param scale Scale factor (optional)
     * @param unit Unit (optional)
     * @param enumMapping Enumeration mapping (optional)
     */
    void setVariable(const std::string& name, const std::string& value, 
                    const std::string& dataType, 
                    std::optional<double> scale = std::nullopt,
                    std::optional<std::string> unit = std::nullopt,
                    std::optional<std::map<std::string, std::string>> enumMapping = std::nullopt);
    
    /**
     * @brief Get a variable value
     * @param name Variable name
     * @return Variable value or empty string if not found
     */
    std::string getVariableValue(const std::string& name) const;
    
    /**
     * @brief Get a variable
     * @param name Variable name
     * @return Variable or nullopt if not found
     */
    std::optional<Variable> getVariable(const std::string& name) const;
    
    /**
     * @brief Set the status change callback
     * @param callback Function to call when status changes
     */
    void setStatusChangeCallback(StatusChangeCallback callback);
    
    /**
     * @brief Set the meter value callback
     * @param callback Function to call when meter values are available
     */
    void setMeterValueCallback(MeterValueCallback callback);
    
    /**
     * @brief Set the transaction event callback
     * @param callback Function to call when transaction events occur
     */
    void setTransactionEventCallback(TransactionEventCallback callback);
    
    /**
     * @brief Start the heartbeat timer
     * @param interval Heartbeat interval
     */
    void startHeartbeat(std::chrono::seconds interval);
    
    /**
     * @brief Stop the heartbeat timer
     */
    void stopHeartbeat();
    
    /**
     * @brief Start the meter value timer
     * @param interval Meter value interval
     */
    void startMeterValueTimer(std::chrono::seconds interval);
    
    /**
     * @brief Stop the meter value timer
     */
    void stopMeterValueTimer();
    
    /**
     * @brief Get the EVSE ID
     * @return EVSE ID
     */
    int getEvseId() const;
    
    /**
     * @brief Get the connector ID
     * @return Connector ID
     */
    int getConnectorId() const;
    
    /**
     * @brief Convert state to string
     * @param state State to convert
     * @return String representation of state
     */
    static std::string stateToString(EvseState state);
    
    /**
     * @brief Convert connector status to string
     * @param status Connector status to convert
     * @return String representation of connector status
     */
    static std::string connectorStatusToString(ConnectorStatus status);
    
    /**
     * @brief Convert string to connector status
     * @param status String representation of connector status
     * @return Connector status
     */
    static ConnectorStatus stringToConnectorStatus(const std::string& status);

private:
    EvseStateMachine(boost::asio::io_context& io_context, int evseId, int connectorId);
    
    /**
     * @brief Initialize the state machine
     */
    void initialize();
    
    /**
     * @brief Handle state transition
     * @param newState New state
     */
    void handleStateTransition(EvseState newState);
    
    /**
     * @brief Update connector status based on current state
     */
    void updateConnectorStatus();
    
    /**
     * @brief Start a transaction
     * @param idTag ID tag
     * @return true if transaction was started successfully, false otherwise
     */
    bool startTransaction(const std::string& idTag);
    
    /**
     * @brief Stop the current transaction
     * @param reason Reason for stopping
     * @return true if transaction was stopped successfully, false otherwise
     */
    bool stopTransaction(const std::string& reason);
    
    /**
     * @brief Add a meter value to the current transaction
     * @param value Meter value
     */
    void addMeterValue(double value);
    
    /**
     * @brief Generate a transaction ID
     * @return Transaction ID
     */
    std::string generateTransactionId() const;
    
    /**
     * @brief Handle heartbeat timer events
     * @param ec Error code
     */
    void onHeartbeatTimer(const boost::system::error_code& ec);
    
    /**
     * @brief Handle meter value timer events
     * @param ec Error code
     */
    void onMeterValueTimer(const boost::system::error_code& ec);
    
    /**
     * @brief Get the current timestamp as ISO8601 string
     * @return Current timestamp
     */
    std::string getCurrentTimestamp() const;
    
    /**
     * @brief Convert time point to ISO8601 string
     * @param tp Time point
     * @return ISO8601 string
     */
    std::string timePointToIso8601(const std::chrono::time_point<std::chrono::system_clock>& tp) const;

    // Boost ASIO components
    boost::asio::io_context& io_context_;
    boost::asio::steady_timer heartbeat_timer_;
    boost::asio::steady_timer meter_value_timer_;
    
    // EVSE properties
    int evse_id_;
    int connector_id_;
    EvseState current_state_;
    ConnectorStatus connector_status_;
    std::optional<Transaction> current_transaction_;
    std::map<std::string, Variable> variables_;
    
    // Callbacks
    StatusChangeCallback status_change_callback_;
    MeterValueCallback meter_value_callback_;
    TransactionEventCallback transaction_event_callback_;
    
    // Timers
    std::chrono::seconds heartbeat_interval_;
    std::chrono::seconds meter_value_interval_;
    
    // Transaction sequence number
    int transaction_seq_no_;
};

} // namespace ocpp
} // namespace ocpp_gateway