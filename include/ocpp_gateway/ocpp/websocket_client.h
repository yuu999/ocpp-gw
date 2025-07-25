#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <queue>
#include <mutex>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include "ocpp_gateway/common/logger.h"
#include "ocpp_gateway/common/error.h"

namespace ocpp_gateway {
namespace ocpp {

/**
 * @enum ConnectionState
 * @brief Enumeration of WebSocket connection states
 */
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    CLOSING,
    CLOSED,
    ERROR
};

/**
 * @struct WebSocketConfig
 * @brief Configuration for WebSocket client
 */
struct WebSocketConfig {
    std::string url;                          // WebSocket URL (wss://example.com/ocpp)
    std::string ca_cert_path;                 // Path to CA certificate file
    std::string client_cert_path;             // Path to client certificate file (optional)
    std::string client_key_path;              // Path to client private key file (optional)
    bool verify_peer = true;                  // Verify server certificate
    std::chrono::seconds connect_timeout{10}; // Connection timeout
    std::chrono::seconds reconnect_interval{5}; // Initial reconnect interval
    std::chrono::seconds max_reconnect_interval{300}; // Maximum reconnect interval
    int max_reconnect_attempts = 0;           // Maximum reconnect attempts (0 = infinite)
    std::string subprotocol = "ocpp2.0.1";    // WebSocket subprotocol
};

/**
 * @class WebSocketClient
 * @brief Secure WebSocket client with TLS support and automatic reconnection
 */
class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
    using MessageHandler = std::function<void(const std::string&)>;
    using ConnectHandler = std::function<void(bool)>;
    using CloseHandler = std::function<void(const std::string&)>;
    using ErrorHandler = std::function<void(const std::string&, const std::error_code&)>;

    /**
     * @brief Create a WebSocketClient instance
     * @param io_context Boost IO context
     * @param config WebSocket configuration
     * @return Shared pointer to WebSocketClient
     */
    static std::shared_ptr<WebSocketClient> create(
        boost::asio::io_context& io_context,
        const WebSocketConfig& config);

    /**
     * @brief Connect to the WebSocket server
     * @param on_connect Callback when connection is established or fails
     */
    void connect(ConnectHandler on_connect = nullptr);

    /**
     * @brief Send a message to the server
     * @param message Message to send
     * @return true if message was sent or queued, false if connection is closed
     */
    bool send(const std::string& message);

    /**
     * @brief Close the connection
     * @param reason Close reason
     */
    void close(const std::string& reason = "Normal closure");

    /**
     * @brief Get the current connection state
     * @return ConnectionState enum
     */
    ConnectionState getState() const;

    /**
     * @brief Set the message handler
     * @param handler Function to handle incoming messages
     */
    void setMessageHandler(MessageHandler handler);

    /**
     * @brief Set the close handler
     * @param handler Function to handle connection closure
     */
    void setCloseHandler(CloseHandler handler);

    /**
     * @brief Set the error handler
     * @param handler Function to handle errors
     */
    void setErrorHandler(ErrorHandler handler);

    /**
     * @brief Check if the connection is active
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * @brief Get the number of reconnect attempts
     * @return Number of reconnect attempts
     */
    int getReconnectAttempts() const;

    /**
     * @brief Reset the reconnect attempts counter
     */
    void resetReconnectAttempts();

    /**
     * @brief Get the URL
     * @return WebSocket URL
     */
    std::string getUrl() const;

private:
    WebSocketClient(boost::asio::io_context& io_context, const WebSocketConfig& config);
    
    /**
     * @brief Parse the WebSocket URL
     * @param url WebSocket URL (wss://example.com:443/ocpp)
     * @return true if URL is valid, false otherwise
     */
    bool parseUrl(const std::string& url);
    
    /**
     * @brief Initialize the SSL context
     * @return true if successful, false otherwise
     */
    bool initSslContext();
    
    /**
     * @brief Start the connection process
     */
    void doConnect();
    
    /**
     * @brief Handle DNS resolution
     * @param ec Error code
     * @param results Resolver results
     */
    void onResolve(const boost::system::error_code& ec, 
                  boost::asio::ip::tcp::resolver::results_type results);
    
    /**
     * @brief Handle TCP connection
     * @param ec Error code
     */
    void onConnect(const boost::system::error_code& ec);
    
    /**
     * @brief Handle SSL handshake
     * @param ec Error code
     */
    void onSslHandshake(const boost::system::error_code& ec);
    
    /**
     * @brief Handle WebSocket handshake
     * @param ec Error code
     */
    void onHandshake(const boost::system::error_code& ec);
    
    /**
     * @brief Start reading messages
     */
    void doRead();
    
    /**
     * @brief Handle read completion
     * @param ec Error code
     * @param bytes_transferred Number of bytes read
     */
    void onRead(const boost::system::error_code& ec, std::size_t bytes_transferred);
    
    /**
     * @brief Send a queued message
     */
    void doWrite();
    
    /**
     * @brief Handle write completion
     * @param ec Error code
     * @param bytes_transferred Number of bytes written
     */
    void onWrite(const boost::system::error_code& ec, std::size_t bytes_transferred);
    
    /**
     * @brief Handle connection closure
     * @param ec Error code
     */
    void onClose(const boost::system::error_code& ec);
    
    /**
     * @brief Schedule reconnection
     */
    void scheduleReconnect();
    
    /**
     * @brief Handle reconnection timer
     * @param ec Error code
     */
    void onReconnectTimer(const boost::system::error_code& ec);
    
    /**
     * @brief Handle connection timeout
     * @param ec Error code
     */
    void onConnectTimeout(const boost::system::error_code& ec);
    
    /**
     * @brief Calculate next reconnect interval using exponential backoff
     * @return Next reconnect interval
     */
    std::chrono::seconds calculateReconnectInterval();
    
    /**
     * @brief Handle error
     * @param message Error message
     * @param ec Error code
     */
    void handleError(const std::string& message, const boost::system::error_code& ec);

    // Boost ASIO components
    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::resolver resolver_;
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;
    std::unique_ptr<boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>> ws_;
    boost::beast::flat_buffer read_buffer_;
    boost::asio::steady_timer reconnect_timer_;
    boost::asio::steady_timer connect_timeout_timer_;
    
    // Connection state
    std::atomic<ConnectionState> state_;
    std::atomic<int> reconnect_attempts_;
    std::string host_;
    std::string port_;
    std::string target_;
    
    // Message queue
    std::queue<std::string> message_queue_;
    std::mutex queue_mutex_;
    bool write_in_progress_;
    
    // Callbacks
    MessageHandler message_handler_;
    ConnectHandler connect_handler_;
    CloseHandler close_handler_;
    ErrorHandler error_handler_;
    
    // Configuration
    WebSocketConfig config_;
};

} // namespace ocpp
} // namespace ocpp_gateway