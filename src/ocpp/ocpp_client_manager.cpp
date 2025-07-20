#include "ocpp_gateway/ocpp/ocpp_client_manager.h"
#include "ocpp_gateway/common/error.h"

namespace ocpp_gateway {
namespace ocpp {

// Helper struct to store EVSE state machine with its IDs
struct EvseEntry {
    int evseId;
    int connectorId;
    std::shared_ptr<EvseStateMachine> stateMachine;
};

std::shared_ptr<OcppClientManager> OcppClientManager::create(
    boost::asio::io_context& io_context,
    const OcppClientConfig& config) {
    return std::shared_ptr<OcppClientManager>(new OcppClientManager(io_context, config));
}

OcppClientManager::OcppClientManager(boost::asio::io_context& io_context, const OcppClientConfig& config)
    : io_context_(io_context),
      heartbeat_timer_(io_context),
      config_(config),
      running_(false) {
}

bool OcppClientManager::start() {
    if (running_) {
        LOG_WARN("OCPP client manager already running");
        return true;
    }
    
    LOG_INFO("Starting OCPP client manager");
    
    // Initialize message processor
    if (!initMessageProcessor()) {
        LOG_ERROR("Failed to initialize message processor");
        return false;
    }
    
    // Initialize WebSocket client
    if (!initWebSocketClient()) {
        LOG_ERROR("Failed to initialize WebSocket client");
        return false;
    }
    
    running_ = true;
    
    // Connect to CSMS
    ws_client_->connect([this](bool connected) {
        onWebSocketConnect(connected);
    });
    
    return true;
}

void OcppClientManager::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping OCPP client manager");
    
    // Stop heartbeat timer
    heartbeat_timer_.cancel();
    
    // Stop all EVSE state machines
    {
        std::lock_guard<std::mutex> lock(evse_mutex_);
        for (auto& evse : evse_state_machines_) {
            evse->stopHeartbeat();
            evse->stopMeterValueTimer();
        }
        evse_state_machines_.clear();
    }
    
    // Close WebSocket connection
    if (ws_client_) {
        ws_client_->close("Client shutdown");
    }
    
    running_ = false;
}

bool OcppClientManager::initWebSocketClient() {
    try {
        // Create WebSocket configuration
        WebSocketConfig ws_config;
        ws_config.url = config_.csms_url;
        ws_config.ca_cert_path = config_.ca_cert_path;
        ws_config.client_cert_path = config_.client_cert_path;
        ws_config.client_key_path = config_.client_key_path;
        ws_config.verify_peer = config_.verify_peer;
        ws_config.connect_timeout = config_.connect_timeout;
        ws_config.reconnect_interval = config_.reconnect_interval;
        ws_config.max_reconnect_interval = config_.max_reconnect_interval;
        ws_config.max_reconnect_attempts = config_.max_reconnect_attempts;
        ws_config.subprotocol = "ocpp2.0.1";
        
        // Create WebSocket client
        ws_client_ = WebSocketClient::create(io_context_, ws_config);
        
        // Set handlers
        ws_client_->setMessageHandler([this](const std::string& message) {
            onWebSocketMessage(message);
        });
        
        ws_client_->setCloseHandler([this](const std::string& reason) {
            onWebSocketClose(reason);
        });
        
        ws_client_->setErrorHandler([this](const std::string& message, const std::error_code& ec) {
            onWebSocketError(message, ec);
        });
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize WebSocket client: {}", e.what());
        return false;
    }
}

bool OcppClientManager::initMessageProcessor() {
    try {
        // Create message processor
        message_processor_ = OcppMessageProcessor::create(io_context_);
        
        // Set message callback
        message_processor_->setMessageCallback([this](const std::string& message) {
            return ws_client_->send(message);
        });
        
        // Register message handlers
        registerMessageHandlers();
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize message processor: {}", e.what());
        return false;
    }
}

void OcppClientManager::registerMessageHandlers() {
    // Register handlers for incoming messages from CSMS
    message_processor_->registerHandler(OcppMessageAction::REMOTE_START_TRANSACTION, 
                                       RemoteStartTransactionHandler::create());
    
    message_processor_->registerHandler(OcppMessageAction::REMOTE_STOP_TRANSACTION, 
                                       RemoteStopTransactionHandler::create());
    
    message_processor_->registerHandler(OcppMessageAction::UNLOCK_CONNECTOR, 
                                       UnlockConnectorHandler::create());
    
    message_processor_->registerHandler(OcppMessageAction::TRIGGER_MESSAGE, 
                                       TriggerMessageHandler::create());
    
    message_processor_->registerHandler(OcppMessageAction::SET_CHARGING_PROFILE, 
                                       SetChargingProfileHandler::create());
    
    message_processor_->registerHandler(OcppMessageAction::DATA_TRANSFER, 
                                       DataTransferHandler::create());
    
    LOG_INFO("Registered OCPP message handlers");
}

void OcppClientManager::onWebSocketConnect(bool connected) {
    if (connected) {
        LOG_INFO("Connected to CSMS at {}", config_.csms_url);
        
        // Update message processor connection state
        message_processor_->setConnected(true);
        
        // Start heartbeat timer
        startHeartbeatTimer();
        
        // Send BootNotification message
        sendBootNotification();
    } else {
        LOG_ERROR("Failed to connect to CSMS at {}", config_.csms_url);
        
        // Update message processor connection state
        message_processor_->setConnected(false);
    }
}

void OcppClientManager::onWebSocketMessage(const std::string& message) {
    LOG_DEBUG("Received OCPP message: {}", message);
    
    // Process the message
    if (!message_processor_->processIncomingMessage(message)) {
        LOG_ERROR("Failed to process incoming OCPP message");
    }
}

void OcppClientManager::onWebSocketClose(const std::string& reason) {
    LOG_INFO("WebSocket connection closed: {}", reason);
    
    // Update message processor connection state
    message_processor_->setConnected(false);
    
    // Stop heartbeat timer
    heartbeat_timer_.cancel();
}

void OcppClientManager::onWebSocketError(const std::string& message, const std::error_code& ec) {
    LOG_ERROR("WebSocket error: {} ({})", message, ec.message());
    
    // Update message processor connection state
    message_processor_->setConnected(false);
}

void OcppClientManager::startHeartbeatTimer() {
    heartbeat_timer_.expires_after(config_.heartbeat_interval);
    heartbeat_timer_.async_wait(
        std::bind(&OcppClientManager::onHeartbeatTimer, shared_from_this(),
                 std::placeholders::_1));
}

void OcppClientManager::onHeartbeatTimer(const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted) {
        // Timer was cancelled
        return;
    }
    
    if (!isConnected()) {
        // Not connected, don't send heartbeat
        return;
    }
    
    LOG_DEBUG("Sending heartbeat");
    
    // Send Heartbeat message
    sendHeartbeat();
    
    // Restart timer
    startHeartbeatTimer();
}

bool OcppClientManager::sendMessage(const std::string& message) {
    if (!ws_client_ || !isConnected()) {
        LOG_ERROR("Cannot send message, not connected to CSMS");
        return false;
    }
    
    return ws_client_->send(message);
}

bool OcppClientManager::sendMessage(const OcppMessage& message) {
    if (!message_processor_) {
        LOG_ERROR("Cannot send message, message processor not initialized");
        return false;
    }
    
    return message_processor_->sendMessage(message);
}

bool OcppClientManager::isConnected() const {
    return ws_client_ && ws_client_->isConnected();
}

ConnectionState OcppClientManager::getConnectionState() const {
    return ws_client_ ? ws_client_->getState() : ConnectionState::DISCONNECTED;
}

size_t OcppClientManager::getQueueSize() const {
    return message_processor_ ? message_processor_->getQueueSize() : 0;
}

bool OcppClientManager::sendBootNotification() {
    LOG_INFO("Sending BootNotification");
    
    OcppMessage message = BootNotificationHandler::createRequest(
        config_.charge_point_model,
        config_.charge_point_vendor,
        config_.firmware_version);
    
    return sendMessage(message);
}

bool OcppClientManager::sendHeartbeat() {
    LOG_DEBUG("Sending Heartbeat");
    
    OcppMessage message = HeartbeatHandler::createRequest();
    
    return sendMessage(message);
}

bool OcppClientManager::sendStatusNotification(
    int connectorId,
    const std::string& errorCode,
    const std::string& status) {
    
    LOG_INFO("Sending StatusNotification for connector {}: {}", connectorId, status);
    
    OcppMessage message = StatusNotificationHandler::createRequest(
        connectorId,
        errorCode,
        status);
    
    return sendMessage(message);
}

bool OcppClientManager::sendMeterValues(int evseId, double meterValue) {
    LOG_DEBUG("Sending MeterValues for EVSE {}: {} kWh", evseId, meterValue);
    
    OcppMessage message = MeterValuesHandler::createRequest(
        evseId,
        meterValue);
    
    return sendMessage(message);
}


bool OcppClientManager::addEvse(int evseId, int connectorId) {
    LOG_INFO("Adding EVSE {} Connector {}", evseId, connectorId);
    
    std::lock_guard<std::mutex> lock(evse_mutex_);
    
    // Check if EVSE already exists
    for (const auto& evse : evse_state_machines_) {
        if (evse->getEvseId() == evseId && evse->getConnectorId() == connectorId) {
            LOG_WARN("EVSE {} Connector {} already exists", evseId, connectorId);
            return false;
        }
    }
    
    // Create new EVSE state machine
    auto evse_state_machine = EvseStateMachine::create(io_context_, evseId, connectorId);
    
    // Set callbacks
    evse_state_machine->setStatusChangeCallback(
        std::bind(&OcppClientManager::onEvseStatusChange, shared_from_this(),
                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    
    evse_state_machine->setMeterValueCallback(
        std::bind(&OcppClientManager::onEvseMeterValue, shared_from_this(),
                 std::placeholders::_1, std::placeholders::_2));
    
    evse_state_machine->setTransactionEventCallback(
        std::bind(&OcppClientManager::onEvseTransactionEvent, shared_from_this(),
                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                 std::placeholders::_4, std::placeholders::_5, std::placeholders::_6,
                 std::placeholders::_7));
    
    // Start heartbeat
    evse_state_machine->startHeartbeat(config_.heartbeat_interval);
    
    // Add to list
    evse_state_machines_.push_back(evse_state_machine);
    
    // Send initial status notification
    sendStatusNotification(
        connectorId,
        "NoError",
        EvseStateMachine::connectorStatusToString(evse_state_machine->getConnectorStatus()));
    
    return true;
}

bool OcppClientManager::removeEvse(int evseId, int connectorId) {
    LOG_INFO("Removing EVSE {} Connector {}", evseId, connectorId);
    
    std::lock_guard<std::mutex> lock(evse_mutex_);
    
    // Find EVSE
    auto it = std::find_if(evse_state_machines_.begin(), evse_state_machines_.end(),
                          [evseId, connectorId](const std::shared_ptr<EvseStateMachine>& evse) {
                              return evse->getEvseId() == evseId && evse->getConnectorId() == connectorId;
                          });
    
    if (it == evse_state_machines_.end()) {
        LOG_WARN("EVSE {} Connector {} not found", evseId, connectorId);
        return false;
    }
    
    // Stop heartbeat
    (*it)->stopHeartbeat();
    
    // Remove from list
    evse_state_machines_.erase(it);
    
    return true;
}

std::shared_ptr<EvseStateMachine> OcppClientManager::getEvseStateMachine(int evseId, int connectorId) {
    std::lock_guard<std::mutex> lock(evse_mutex_);
    
    // Find EVSE
    auto it = std::find_if(evse_state_machines_.begin(), evse_state_machines_.end(),
                          [evseId, connectorId](const std::shared_ptr<EvseStateMachine>& evse) {
                              return evse->getEvseId() == evseId && evse->getConnectorId() == connectorId;
                          });
    
    if (it == evse_state_machines_.end()) {
        return nullptr;
    }
    
    return *it;
}

bool OcppClientManager::processEvseEvent(int evseId, int connectorId, EvseEvent event, 
                                       const nlohmann::json& data) {
    LOG_INFO("Processing event for EVSE {} Connector {}: {}", 
             evseId, connectorId, static_cast<int>(event));
    
    auto evse = getEvseStateMachine(evseId, connectorId);
    if (!evse) {
        LOG_ERROR("EVSE {} Connector {} not found", evseId, connectorId);
        return false;
    }
    
    return evse->processEvent(event, data);
}

void OcppClientManager::onEvseStatusChange(int connectorId, const std::string& errorCode, 
                                         const std::string& status) {
    LOG_INFO("EVSE status change: Connector {} status {} error {}", 
             connectorId, status, errorCode);
    
    // Send StatusNotification to CSMS
    sendStatusNotification(connectorId, errorCode, status);
}

void OcppClientManager::onEvseMeterValue(int evseId, double meterValue) {
    LOG_DEBUG("EVSE meter value: EVSE {} value {}", evseId, meterValue);
    
    // Send MeterValues to CSMS
    sendMeterValues(evseId, meterValue);
}

void OcppClientManager::onEvseTransactionEvent(const std::string& eventType, const std::string& timestamp, 
                                             const std::string& triggerReason, int seqNo, 
                                             const std::string& transactionId, int evseId, double meterValue) {
    LOG_INFO("EVSE transaction event: EVSE {} type {} transaction {} reason {}", 
             evseId, eventType, transactionId, triggerReason);
    
    // Create TransactionEvent message
    OcppMessage message = TransactionEventHandler::createRequest(
        eventType,
        timestamp,
        triggerReason,
        seqNo,
        transactionId,
        evseId,
        meterValue);
    
    // Send message to CSMS
    sendMessage(message);
}