#include "ocpp_gateway/ocpp/evse_state_machine.h"
#include <iomanip>
#include <sstream>
#include <uuid/uuid.h>

namespace ocpp_gateway {
namespace ocpp {

std::shared_ptr<EvseStateMachine> EvseStateMachine::create(
    boost::asio::io_context& io_context,
    int evseId,
    int connectorId) {
    return std::shared_ptr<EvseStateMachine>(new EvseStateMachine(io_context, evseId, connectorId));
}

EvseStateMachine::EvseStateMachine(boost::asio::io_context& io_context, int evseId, int connectorId)
    : io_context_(io_context),
      heartbeat_timer_(io_context),
      meter_value_timer_(io_context),
      evse_id_(evseId),
      connector_id_(connectorId),
      current_state_(EvseState::AVAILABLE),
      connector_status_(ConnectorStatus::AVAILABLE),
      heartbeat_interval_(300),
      meter_value_interval_(60),
      transaction_seq_no_(0) {
    initialize();
}

void EvseStateMachine::initialize() {
    LOG_INFO("Initializing EVSE state machine for EVSE {} Connector {}", evse_id_, connector_id_);
    updateConnectorStatus();
}

bool EvseStateMachine::processEvent(EvseEvent event, const nlohmann::json& data) {
    LOG_DEBUG("Processing event for EVSE {} Connector {}: {}", evse_id_, connector_id_, static_cast<int>(event));
    
    // State transition logic based on current state and event
    switch (current_state_) {
        case EvseState::AVAILABLE:
            switch (event) {
                case EvseEvent::PLUG_IN:
                    handleStateTransition(EvseState::PREPARING);
                    return true;
                case EvseEvent::RESERVE:
                    handleStateTransition(EvseState::RESERVED);
                    return true;
                case EvseEvent::SET_UNAVAILABLE:
                    handleStateTransition(EvseState::UNAVAILABLE);
                    return true;
                case EvseEvent::FAULT_DETECTED:
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state AVAILABLE", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::PREPARING:
            switch (event) {
                case EvseEvent::PLUG_OUT:
                    handleStateTransition(EvseState::AVAILABLE);
                    return true;
                case EvseEvent::AUTHORIZE_START:
                    if (data.contains("idTag")) {
                        if (startTransaction(data["idTag"])) {
                            handleStateTransition(EvseState::CHARGING);
                            return true;
                        }
                    } else {
                        LOG_ERROR("Missing idTag in AUTHORIZE_START event data");
                    }
                    return false;
                case EvseEvent::SET_UNAVAILABLE:
                    handleStateTransition(EvseState::UNAVAILABLE);
                    return true;
                case EvseEvent::FAULT_DETECTED:
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state PREPARING", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::CHARGING:
            switch (event) {
                case EvseEvent::STOP_CHARGING:
                    if (stopTransaction("Local")) {
                        handleStateTransition(EvseState::FINISHING);
                        return true;
                    }
                    return false;
                case EvseEvent::AUTHORIZE_STOP:
                    if (data.contains("idTag")) {
                        if (stopTransaction("DeAuthorized")) {
                            handleStateTransition(EvseState::FINISHING);
                            return true;
                        }
                    } else {
                        LOG_ERROR("Missing idTag in AUTHORIZE_STOP event data");
                    }
                    return false;
                case EvseEvent::SUSPEND_CHARGING_EV:
                    handleStateTransition(EvseState::SUSPENDED_EV);
                    return true;
                case EvseEvent::SUSPEND_CHARGING_EVSE:
                    handleStateTransition(EvseState::SUSPENDED_EVSE);
                    return true;
                case EvseEvent::FAULT_DETECTED:
                    stopTransaction("Faulted");
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state CHARGING", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::SUSPENDED_EV:
            switch (event) {
                case EvseEvent::RESUME_CHARGING:
                    handleStateTransition(EvseState::CHARGING);
                    return true;
                case EvseEvent::STOP_CHARGING:
                    if (stopTransaction("Local")) {
                        handleStateTransition(EvseState::FINISHING);
                        return true;
                    }
                    return false;
                case EvseEvent::AUTHORIZE_STOP:
                    if (data.contains("idTag")) {
                        if (stopTransaction("DeAuthorized")) {
                            handleStateTransition(EvseState::FINISHING);
                            return true;
                        }
                    } else {
                        LOG_ERROR("Missing idTag in AUTHORIZE_STOP event data");
                    }
                    return false;
                case EvseEvent::FAULT_DETECTED:
                    stopTransaction("Faulted");
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state SUSPENDED_EV", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::SUSPENDED_EVSE:
            switch (event) {
                case EvseEvent::RESUME_CHARGING:
                    handleStateTransition(EvseState::CHARGING);
                    return true;
                case EvseEvent::STOP_CHARGING:
                    if (stopTransaction("Local")) {
                        handleStateTransition(EvseState::FINISHING);
                        return true;
                    }
                    return false;
                case EvseEvent::AUTHORIZE_STOP:
                    if (data.contains("idTag")) {
                        if (stopTransaction("DeAuthorized")) {
                            handleStateTransition(EvseState::FINISHING);
                            return true;
                        }
                    } else {
                        LOG_ERROR("Missing idTag in AUTHORIZE_STOP event data");
                    }
                    return false;
                case EvseEvent::FAULT_DETECTED:
                    stopTransaction("Faulted");
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state SUSPENDED_EVSE", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::FINISHING:
            switch (event) {
                case EvseEvent::PLUG_OUT:
                    handleStateTransition(EvseState::AVAILABLE);
                    return true;
                case EvseEvent::FAULT_DETECTED:
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state FINISHING", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::RESERVED:
            switch (event) {
                case EvseEvent::PLUG_IN:
                    handleStateTransition(EvseState::PREPARING);
                    return true;
                case EvseEvent::CANCEL_RESERVATION:
                    handleStateTransition(EvseState::AVAILABLE);
                    return true;
                case EvseEvent::SET_UNAVAILABLE:
                    handleStateTransition(EvseState::UNAVAILABLE);
                    return true;
                case EvseEvent::FAULT_DETECTED:
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state RESERVED", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::UNAVAILABLE:
            switch (event) {
                case EvseEvent::SET_AVAILABLE:
                    handleStateTransition(EvseState::AVAILABLE);
                    return true;
                case EvseEvent::FAULT_DETECTED:
                    handleStateTransition(EvseState::FAULTED);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state UNAVAILABLE", static_cast<int>(event));
                    return false;
            }
            break;
            
        case EvseState::FAULTED:
            switch (event) {
                case EvseEvent::FAULT_CLEARED:
                    handleStateTransition(EvseState::AVAILABLE);
                    return true;
                default:
                    LOG_WARNING("Invalid event {} for state FAULTED", static_cast<int>(event));
                    return false;
            }
            break;
    }
    
    return false;
}

void EvseStateMachine::handleStateTransition(EvseState newState) {
    LOG_INFO("EVSE {} Connector {} state transition: {} -> {}", 
             evse_id_, connector_id_, 
             stateToString(current_state_), stateToString(newState));
    
    EvseState oldState = current_state_;
    current_state_ = newState;
    
    // Update connector status based on new state
    updateConnectorStatus();
    
    // Notify status change
    if (status_change_callback_) {
        status_change_callback_(connector_id_, 
                               "NoError", 
                               connectorStatusToString(connector_status_));
    }
    
    // Start or stop meter value timer based on state
    if (newState == EvseState::CHARGING) {
        startMeterValueTimer(meter_value_interval_);
    } else if (oldState == EvseState::CHARGING) {
        stopMeterValueTimer();
    }
}

void EvseStateMachine::updateConnectorStatus() {
    switch (current_state_) {
        case EvseState::AVAILABLE:
            connector_status_ = ConnectorStatus::AVAILABLE;
            break;
        case EvseState::PREPARING:
        case EvseState::CHARGING:
        case EvseState::SUSPENDED_EV:
        case EvseState::SUSPENDED_EVSE:
        case EvseState::FINISHING:
            connector_status_ = ConnectorStatus::OCCUPIED;
            break;
        case EvseState::RESERVED:
            connector_status_ = ConnectorStatus::RESERVED;
            break;
        case EvseState::UNAVAILABLE:
            connector_status_ = ConnectorStatus::UNAVAILABLE;
            break;
        case EvseState::FAULTED:
            connector_status_ = ConnectorStatus::FAULTED;
            break;
    }
}

bool EvseStateMachine::startTransaction(const std::string& idTag) {
    LOG_INFO("Starting transaction for EVSE {} Connector {} with idTag {}", 
             evse_id_, connector_id_, idTag);
    
    if (current_transaction_) {
        LOG_ERROR("Transaction already in progress for EVSE {} Connector {}", 
                 evse_id_, connector_id_);
        return false;
    }
    
    Transaction transaction;
    transaction.id = generateTransactionId();
    transaction.idTag = idTag;
    transaction.startTime = std::chrono::system_clock::now();
    transaction.status = TransactionStatus::ACTIVE;
    
    current_transaction_ = transaction;
    transaction_seq_no_ = 0;
    
    // Notify transaction event
    if (transaction_event_callback_) {
        double meterValue = 0.0;
        try {
            // Try to get initial meter value
            auto meterValueVar = getVariable("MeterValue.Energy.Active.Import.Register");
            if (meterValueVar && !meterValueVar->value.empty()) {
                meterValue = std::stod(meterValueVar->value);
                if (meterValueVar->scale) {
                    meterValue *= *meterValueVar->scale;
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting initial meter value: {}", e.what());
        }
        
        transaction_event_callback_(
            "Started",
            getCurrentTimestamp(),
            "Authorized",
            transaction_seq_no_++,
            transaction.id,
            evse_id_,
            meterValue
        );
    }
    
    return true;
}

bool EvseStateMachine::stopTransaction(const std::string& reason) {
    LOG_INFO("Stopping transaction for EVSE {} Connector {} with reason {}", 
             evse_id_, connector_id_, reason);
    
    if (!current_transaction_) {
        LOG_ERROR("No transaction in progress for EVSE {} Connector {}", 
                 evse_id_, connector_id_);
        return false;
    }
    
    current_transaction_->stopTime = std::chrono::system_clock::now();
    current_transaction_->status = TransactionStatus::COMPLETED;
    
    // Notify transaction event
    if (transaction_event_callback_) {
        double meterValue = 0.0;
        try {
            // Try to get final meter value
            auto meterValueVar = getVariable("MeterValue.Energy.Active.Import.Register");
            if (meterValueVar && !meterValueVar->value.empty()) {
                meterValue = std::stod(meterValueVar->value);
                if (meterValueVar->scale) {
                    meterValue *= *meterValueVar->scale;
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting final meter value: {}", e.what());
        }
        
        transaction_event_callback_(
            "Ended",
            getCurrentTimestamp(),
            reason,
            transaction_seq_no_++,
            current_transaction_->id,
            evse_id_,
            meterValue
        );
    }
    
    // Clear current transaction
    current_transaction_ = std::nullopt;
    
    return true;
}

void EvseStateMachine::addMeterValue(double value) {
    if (!current_transaction_) {
        LOG_WARNING("Cannot add meter value: no transaction in progress for EVSE {} Connector {}", 
                   evse_id_, connector_id_);
        return;
    }
    
    MeterValue meterValue;
    meterValue.timestamp = std::chrono::system_clock::now();
    
    SampledValue sampledValue;
    sampledValue.value = std::to_string(value);
    sampledValue.context = "Sample.Periodic";
    sampledValue.format = "Raw";
    sampledValue.measurand = "Energy.Active.Import.Register";
    sampledValue.unit = "Wh";
    
    meterValue.sampledValues.push_back(sampledValue);
    current_transaction_->meterValues.push_back(meterValue);
    
    // Notify meter value
    if (meter_value_callback_) {
        meter_value_callback_(evse_id_, value);
    }
    
    // Notify transaction event (Updated)
    if (transaction_event_callback_ && current_transaction_) {
        transaction_event_callback_(
            "Updated",
            getCurrentTimestamp(),
            "MeterValue",
            transaction_seq_no_++,
            current_transaction_->id,
            evse_id_,
            value
        );
    }
}

std::string EvseStateMachine::generateTransactionId() const {
    uuid_t uuid;
    char uuid_str[37];
    
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, uuid_str);
    
    return uuid_str;
}

void EvseStateMachine::startHeartbeat(std::chrono::seconds interval) {
    heartbeat_interval_ = interval;
    heartbeat_timer_.expires_after(heartbeat_interval_);
    heartbeat_timer_.async_wait(
        std::bind(&EvseStateMachine::onHeartbeatTimer, shared_from_this(), 
                 std::placeholders::_1));
    
    LOG_DEBUG("Started heartbeat timer for EVSE {} Connector {} with interval {}s", 
             evse_id_, connector_id_, interval.count());
}

void EvseStateMachine::stopHeartbeat() {
    heartbeat_timer_.cancel();
    LOG_DEBUG("Stopped heartbeat timer for EVSE {} Connector {}", evse_id_, connector_id_);
}

void EvseStateMachine::startMeterValueTimer(std::chrono::seconds interval) {
    meter_value_interval_ = interval;
    meter_value_timer_.expires_after(meter_value_interval_);
    meter_value_timer_.async_wait(
        std::bind(&EvseStateMachine::onMeterValueTimer, shared_from_this(), 
                 std::placeholders::_1));
    
    LOG_DEBUG("Started meter value timer for EVSE {} Connector {} with interval {}s", 
             evse_id_, connector_id_, interval.count());
}

void EvseStateMachine::stopMeterValueTimer() {
    meter_value_timer_.cancel();
    LOG_DEBUG("Stopped meter value timer for EVSE {} Connector {}", evse_id_, connector_id_);
}

void EvseStateMachine::onHeartbeatTimer(const boost::system::error_code& ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            LOG_ERROR("Heartbeat timer error for EVSE {} Connector {}: {}", 
                     evse_id_, connector_id_, ec.message());
        }
        return;
    }
    
    LOG_DEBUG("Heartbeat for EVSE {} Connector {}", evse_id_, connector_id_);
    
    // Schedule next heartbeat
    heartbeat_timer_.expires_after(heartbeat_interval_);
    heartbeat_timer_.async_wait(
        std::bind(&EvseStateMachine::onHeartbeatTimer, shared_from_this(), 
                 std::placeholders::_1));
}

void EvseStateMachine::onMeterValueTimer(const boost::system::error_code& ec) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            LOG_ERROR("Meter value timer error for EVSE {} Connector {}: {}", 
                     evse_id_, connector_id_, ec.message());
        }
        return;
    }
    
    LOG_DEBUG("Meter value timer for EVSE {} Connector {}", evse_id_, connector_id_);
    
    // Only send meter values if in charging state
    if (current_state_ == EvseState::CHARGING && current_transaction_) {
        try {
            // Try to get meter value
            auto meterValueVar = getVariable("MeterValue.Energy.Active.Import.Register");
            if (meterValueVar && !meterValueVar->value.empty()) {
                double meterValue = std::stod(meterValueVar->value);
                if (meterValueVar->scale) {
                    meterValue *= *meterValueVar->scale;
                }
                addMeterValue(meterValue);
            } else {
                LOG_WARNING("Meter value variable not found or empty");
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error getting meter value: {}", e.what());
        }
    }
    
    // Schedule next meter value
    if (current_state_ == EvseState::CHARGING) {
        meter_value_timer_.expires_after(meter_value_interval_);
        meter_value_timer_.async_wait(
            std::bind(&EvseStateMachine::onMeterValueTimer, shared_from_this(), 
                     std::placeholders::_1));
    }
}

void EvseStateMachine::setVariable(const std::string& name, const std::string& value, 
                                  const std::string& dataType, 
                                  std::optional<double> scale,
                                  std::optional<std::string> unit,
                                  std::optional<std::map<std::string, std::string>> enumMapping) {
    Variable var;
    var.name = name;
    var.value = value;
    var.dataType = dataType;
    var.scale = scale;
    var.unit = unit;
    var.enumMapping = enumMapping;
    
    variables_[name] = var;
}

std::string EvseStateMachine::getVariableValue(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return it->second.value;
    }
    return "";
}

std::optional<Variable> EvseStateMachine::getVariable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void EvseStateMachine::setStatusChangeCallback(StatusChangeCallback callback) {
    status_change_callback_ = callback;
}

void EvseStateMachine::setMeterValueCallback(MeterValueCallback callback) {
    meter_value_callback_ = callback;
}

void EvseStateMachine::setTransactionEventCallback(TransactionEventCallback callback) {
    transaction_event_callback_ = callback;
}

EvseState EvseStateMachine::getCurrentState() const {
    return current_state_;
}

ConnectorStatus EvseStateMachine::getConnectorStatus() const {
    return connector_status_;
}

std::optional<Transaction> EvseStateMachine::getCurrentTransaction() const {
    return current_transaction_;
}

int EvseStateMachine::getEvseId() const {
    return evse_id_;
}

int EvseStateMachine::getConnectorId() const {
    return connector_id_;
}

std::string EvseStateMachine::stateToString(EvseState state) {
    switch (state) {
        case EvseState::AVAILABLE: return "Available";
        case EvseState::PREPARING: return "Preparing";
        case EvseState::CHARGING: return "Charging";
        case EvseState::SUSPENDED_EV: return "SuspendedEV";
        case EvseState::SUSPENDED_EVSE: return "SuspendedEVSE";
        case EvseState::FINISHING: return "Finishing";
        case EvseState::RESERVED: return "Reserved";
        case EvseState::UNAVAILABLE: return "Unavailable";
        case EvseState::FAULTED: return "Faulted";
        default: return "Unknown";
    }
}

std::string EvseStateMachine::connectorStatusToString(ConnectorStatus status) {
    switch (status) {
        case ConnectorStatus::AVAILABLE: return "Available";
        case ConnectorStatus::OCCUPIED: return "Occupied";
        case ConnectorStatus::RESERVED: return "Reserved";
        case ConnectorStatus::UNAVAILABLE: return "Unavailable";
        case ConnectorStatus::FAULTED: return "Faulted";
        default: return "Unknown";
    }
}

ConnectorStatus EvseStateMachine::stringToConnectorStatus(const std::string& status) {
    if (status == "Available") return ConnectorStatus::AVAILABLE;
    if (status == "Occupied") return ConnectorStatus::OCCUPIED;
    if (status == "Reserved") return ConnectorStatus::RESERVED;
    if (status == "Unavailable") return ConnectorStatus::UNAVAILABLE;
    if (status == "Faulted") return ConnectorStatus::FAULTED;
    return ConnectorStatus::AVAILABLE;
}

std::string EvseStateMachine::getCurrentTimestamp() const {
    return timePointToIso8601(std::chrono::system_clock::now());
}

std::string EvseStateMachine::timePointToIso8601(const std::chrono::time_point<std::chrono::system_clock>& tp) const {
    auto time = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%FT%T") << '.' << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return ss.str();
}

} // namespace ocpp
} // namespace ocpp_gateway