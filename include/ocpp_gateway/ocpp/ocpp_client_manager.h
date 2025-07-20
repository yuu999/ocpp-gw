#pragma once

#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <boost/asio.hpp>
#include "ocpp_gateway/ocpp/websocket_client.h"
#include "ocpp_gateway/ocpp/ocpp_message_processor.h"
#include "ocpp_gateway/ocpp/ocpp_message_handlers.h"
#include "ocpp_gateway/ocpp/evse_state_machine.h"
#include "ocpp_gateway/common/logger.h"

namespace ocpp_gateway {
namespace ocpp {

/**
 * @struct OcppClientConfig
 * @brief Configuration for OCPP client manager
 */
struct OcppClientConfig {
    std::string csms_url;                     // CSMS WebSocket URL
    std::string ca_cert_path;                 // Path to CA certificate file
    std::string client_cert_path;             // Path to client certificate file (optional)
    std::string client_key_path;              // Path to client private key file (optional)
    bool verify_peer = true;                  // Verify server certificate
    std::chrono::seconds connect_timeout{10}; // Connection timeout
    std::chrono::seconds reconnect_interval{5}; // Initial reconnect interval
    std::chrono::seconds max_reconnect_interval{300}; // Maximum reconnect interval
    int max_reconnect_attempts = 0;           // Maximum reconnect attempts (0 = infinite)
    std::chrono::seconds heartbeat_interval{300}; // Heartbeat interval
    std::string charge_point_model = "OCPP Gateway";  // Charge point model
    std::string charge_point_vendor = "OCPP Gateway"; // Charge point vendor
    std::string firmware_version = "1.0.0";   // Firmware version
};

/**
 * @class OcppClientManager
 * @brief Manages OCPP client connections to CSMS
 */
class OcppClientManager : public std::enable_shared_from_this<OcppClientManager> {
public:
    /**
     * @brief Create an OcppClientManager instance
     * @param io_context Boost IO context
     * @param config OCPP client configuration
     * @return Shared pointer to OcppClientManager
     */
    static std::shared_ptr<OcppClientManager> create(
        boost::asio::io_context& io_context,
        const OcppClientConfig& config);
    
    /**
     * @brief Start the OCPP client manager
     * @return true if started successfully, false otherwise
     */
    bool start();
    
    /**
     * @brief Stop the OCPP client manager
     */
    void stop();
    
    /**
     * @brief Check if the client is connected to the CSMS
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Send an OCPP message to the CSMS
     * @param message JSON message to send
     * @return true if message was sent or queued, false if connection is closed
     */
    bool sendMessage(const std::string& message);
    
    /**
     * @brief Send an OCPP message to the CSMS
     * @param message OCPP message
     * @return true if message was sent or queued, false if connection is closed
     */
    bool sendMessage(const OcppMessage& message);
    
    /**
     * @brief Get the connection state
     * @return ConnectionState enum
     */
    ConnectionState getConnectionState() const;
    
    /**
     * @brief Get the number of queued messages
     * @return Number of queued messages
     */
    size_t getQueueSize() const;
    
    /**
     * @brief Send a BootNotification message
     * @return true if message was sent or queued, false if connection is closed
     */
    bool sendBootNotification();
    
    /**
     * @brief Send a Heartbeat message
     * @return true if message was sent or queued, false if connection is closed
     */
    bool sendHeartbeat();
    
    /**
     * @brief Send a StatusNotification message
     * @param connectorId Connector ID
     * @param errorCode Error code
     * @param status Connector status
     * @return true if message was sent or queued, false if connection is closed
     */
    bool sendStatusNotification(
        int connectorId,
        const std::string& errorCode,
        const std::string& status);
    
    /**
     * @brief Send a MeterValues message
     * @param evseId EVSE ID
     * @param meterValue Meter value
     * @return true if message was sent or queued, false if connection is closed
     */
    bool sendMeterValues(int evseId, double meterValue);
    
    /**
     * @brief Add an EVSE
     * @param evseId EVSE ID
     * @param connectorId Connector ID
     * @return true if EVSE was added successfully, false otherwise
     */
    bool addEvse(int evseId, int connectorId);
    
    /**
     * @brief Remove an EVSE
     * @param evseId EVSE ID
     * @param connectorId Connector ID
     * @return true if EVSE was removed successfully, false otherwise
     */
    bool removeEvse(int evseId, int connectorId);
    
    /**
     * @brief Get an EVSE state machine
     * @param evseId EVSE ID
     * @param connectorId Connector ID
     * @return Shared pointer to EvseStateMachine or nullptr if not found
     */
    std::shared_ptr<EvseStateMachine> getEvseStateMachine(int evseId, int connectorId);
    
    /**
     * @brief Process an EVSE event
     * @param evseId EVSE ID
     * @param connectorId Connector ID
     * @param event Event to process
     * @param data Additional data for the event (optional)
     * @return true if event was processed successfully, false otherwise
     */
    bool processEvseEvent(int evseId, int connectorId, EvseEvent event, 
                         const nlohmann::json& data = nlohmann::json());
    
    /**
     * @brief Handle EVSE status change
     * @param connectorId Connector ID
     * @param errorCode Error code
     * @param status Connector status
     */
    void onEvseStatusChange(int connectorId, const std::string& errorCode, const std::string& status);
    
    /**
     * @brief Handle EVSE meter value
     * @param evseId EVSE ID
     * @param meterValue Meter value
     */
    void onEvseMeterValue(int evseId, double meterValue);
    
    /**
     * @brief Handle EVSE transaction event
     * @param eventType Event type (Started, Updated, Ended)
     * @param timestamp Timestamp
     * @param triggerReason Trigger reason
     * @param seqNo Sequence number
     * @param transactionId Transaction ID
     * @param evseId EVSE ID
     * @param meterValue Meter value
     */
    void onEvseTransactionEvent(const std::string& eventType, const std::string& timestamp, 
                               const std::string& triggerReason, int seqNo, const std::string& transactionId, 
                               int evseId, double meterValue);

private:
    OcppClientManager(boost::asio::io_context& io_context, const OcppClientConfig& config);
    
    /**
     * @brief Initialize the WebSocket client
     * @return true if successful, false otherwise
     */
    bool initWebSocketClient();
    
    /**
     * @brief Initialize the message processor
     * @return true if successful, false otherwise
     */
    bool initMessageProcessor();
    
    /**
     * @brief Register message handlers
     */
    void registerMessageHandlers();
    
    /**
     * @brief Handle WebSocket connection events
     * @param connected true if connected, false if connection failed
     */
    void onWebSocketConnect(bool connected);
    
    /**
     * @brief Handle WebSocket messages
     * @param message Received message
     */
    void onWebSocketMessage(const std::string& message);
    
    /**
     * @brief Handle WebSocket close events
     * @param reason Close reason
     */
    void onWebSocketClose(const std::string& reason);
    
    /**
     * @brief Handle WebSocket errors
     * @param message Error message
     * @param ec Error code
     */
    void onWebSocketError(const std::string& message, const std::error_code& ec);
    
    /**
     * @brief Start the heartbeat timer
     */
    void startHeartbeatTimer();
    
    /**
     * @brief Handle heartbeat timer events
     * @param ec Error code
     */
    void onHeartbeatTimer(const boost::system::error_code& ec);

    // Boost ASIO components
    boost::asio::io_context& io_context_;
    boost::asio::steady_timer heartbeat_timer_;
    
    // WebSocket client
    std::shared_ptr<WebSocketClient> ws_client_;
    
    // Message processor
    std::shared_ptr<OcppMessageProcessor> message_processor_;
    
    // Configuration
    OcppClientConfig config_;
    
    // State
    std::atomic<bool> running_;
    
    // EVSE state machines
    std::vector<std::shared_ptr<EvseStateMachine>> evse_state_machines_;
    std::mutex evse_mutex_;
};

} // namespace ocpp
} // namespace ocpp_gateway