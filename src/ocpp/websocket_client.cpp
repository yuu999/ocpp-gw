#include "ocpp_gateway/ocpp/websocket_client.h"
#include <regex>
#include <random>

namespace ocpp_gateway {
namespace ocpp {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

std::shared_ptr<WebSocketClient> WebSocketClient::create(
    boost::asio::io_context& io_context,
    const WebSocketConfig& config) {
    return std::shared_ptr<WebSocketClient>(new WebSocketClient(io_context, config));
}

WebSocketClient::WebSocketClient(boost::asio::io_context& io_context, const WebSocketConfig& config)
    : io_context_(io_context),
      resolver_(io_context),
      reconnect_timer_(io_context),
      connect_timeout_timer_(io_context),
      state_(ConnectionState::DISCONNECTED),
      reconnect_attempts_(0),
      write_in_progress_(false),
      config_(config) {
    
    // Parse URL
    if (!parseUrl(config.url)) {
        LOG_ERROR("Invalid WebSocket URL: {}", config.url);
        state_ = ConnectionState::ERROR;
        return;
    }
    
    // Initialize SSL context
    if (!initSslContext()) {
        LOG_ERROR("Failed to initialize SSL context");
        state_ = ConnectionState::ERROR;
        return;
    }
}

bool WebSocketClient::parseUrl(const std::string& url) {
    // Parse WebSocket URL (wss://example.com:443/ocpp)
    std::regex url_regex("(wss?)://([^:/]+)(?::(\\d+))?(/.*)?");
    std::smatch matches;
    
    if (!std::regex_match(url, matches, url_regex)) {
        return false;
    }
    
    std::string scheme = matches[1].str();
    host_ = matches[2].str();
    port_ = matches[3].matched ? matches[3].str() : (scheme == "wss" ? "443" : "80");
    target_ = matches[4].matched ? matches[4].str() : "/";
    
    // Ensure we're using secure WebSocket for OCPP
    if (scheme != "wss") {
        LOG_WARN("Non-secure WebSocket (ws://) is not recommended for OCPP");
    }
    
    return true;
}

bool WebSocketClient::initSslContext() {
    try {
        // Create SSL context
        ssl_context_ = std::make_unique<ssl::context>(ssl::context::tlsv12_client);
        
        // Set SSL options
        ssl_context_->set_options(
            ssl::context::default_workarounds |
            ssl::context::no_sslv2 |
            ssl::context::no_sslv3 |
            ssl::context::no_tlsv1 |
            ssl::context::no_tlsv1_1);
        
        // Set verification mode
        if (config_.verify_peer) {
            ssl_context_->set_verify_mode(ssl::verify_peer);
        } else {
            LOG_WARN("Server certificate verification is disabled");
            ssl_context_->set_verify_mode(ssl::verify_none);
        }
        
        // Load CA certificate
        if (!config_.ca_cert_path.empty()) {
            ssl_context_->load_verify_file(config_.ca_cert_path);
        } else if (config_.verify_peer) {
            // Use default verification paths if no CA certificate is specified
            ssl_context_->set_default_verify_paths();
        }
        
        // Load client certificate and key if provided
        if (!config_.client_cert_path.empty() && !config_.client_key_path.empty()) {
            ssl_context_->use_certificate_file(
                config_.client_cert_path, ssl::context::pem);
            ssl_context_->use_private_key_file(
                config_.client_key_path, ssl::context::pem);
            LOG_INFO("Client certificate loaded for mutual authentication");
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("SSL context initialization failed: {}", e.what());
        return false;
    }
}

void WebSocketClient::connect(ConnectHandler on_connect) {
    if (state_ == ConnectionState::CONNECTED || 
        state_ == ConnectionState::CONNECTING) {
        LOG_WARN("WebSocket already connected or connecting");
        if (on_connect) {
            on_connect(state_ == ConnectionState::CONNECTED);
        }
        return;
    }
    
    // Store connect handler
    connect_handler_ = on_connect;
    
    // Reset state
    state_ = ConnectionState::CONNECTING;
    
    // Start connection process
    doConnect();
}

void WebSocketClient::doConnect() {
    LOG_INFO("Connecting to WebSocket server: {}:{}{}", host_, port_, target_);
    
    try {
        // Create WebSocket stream
        ws_ = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(
            io_context_, *ssl_context_);
        
        // Set SNI hostname (Server Name Indication)
        if (!SSL_set_tlsext_host_name(ws_->next_layer().native_handle(), host_.c_str())) {
            throw boost::system::system_error(
                static_cast<int>(::ERR_get_error()),
                boost::asio::error::get_ssl_category());
        }
        
        // Set WebSocket options
        ws_->set_option(websocket::stream_base::timeout::suggested(
            beast::role_type::client));
        ws_->set_option(websocket::stream_base::decorator(
            [this](websocket::request_type& req) {
                req.set(http::field::user_agent, "OCPP Gateway/1.0");
                // Set subprotocol if specified
                if (!config_.subprotocol.empty()) {
                    req.set(http::field::sec_websocket_protocol, config_.subprotocol);
                }
            }));
        
        // Start connection timeout timer
        connect_timeout_timer_.expires_after(config_.connect_timeout);
        connect_timeout_timer_.async_wait(
            std::bind(&WebSocketClient::onConnectTimeout, shared_from_this(),
                     std::placeholders::_1));
        
        // Resolve hostname
        resolver_.async_resolve(
            host_, port_,
            std::bind(&WebSocketClient::onResolve, shared_from_this(),
                     std::placeholders::_1, std::placeholders::_2));
    } catch (const std::exception& e) {
        handleError("Connection setup failed", boost::system::error_code());
    }
}

void WebSocketClient::onResolve(
    const boost::system::error_code& ec,
    tcp::resolver::results_type results) {
    
    if (ec) {
        handleError("DNS resolution failed", ec);
        return;
    }
    
    // Connect to the IP address we get from a lookup
    beast::get_lowest_layer(*ws_).async_connect(
        results,
        std::bind(&WebSocketClient::onConnect, shared_from_this(),
                 std::placeholders::_1, std::placeholders::_2));
}

void WebSocketClient::onConnect(
    const boost::system::error_code& ec,
    const tcp::endpoint& endpoint) {
    
    if (ec) {
        handleError("TCP connection failed", ec);
        return;
    }
    
    LOG_DEBUG("TCP connected to {}:{}", endpoint.address().to_string(), endpoint.port());
    
    // Perform SSL handshake
    ws_->next_layer().async_handshake(
        ssl::stream_base::client,
        std::bind(&WebSocketClient::onSslHandshake, shared_from_this(),
                 std::placeholders::_1));
}

void WebSocketClient::onSslHandshake(const boost::system::error_code& ec) {
    if (ec) {
        handleError("SSL handshake failed", ec);
        return;
    }
    
    LOG_DEBUG("SSL handshake completed");
    
    // Perform WebSocket handshake
    ws_->async_handshake(
        host_, target_,
        std::bind(&WebSocketClient::onHandshake, shared_from_this(),
                 std::placeholders::_1));
}

void WebSocketClient::onHandshake(const boost::system::error_code& ec) {
    // Cancel the timeout timer
    connect_timeout_timer_.cancel();
    
    if (ec) {
        handleError("WebSocket handshake failed", ec);
        return;
    }
    
    LOG_INFO("WebSocket connection established to {}:{}{}", host_, port_, target_);
    
    // Connection successful
    state_ = ConnectionState::CONNECTED;
    reconnect_attempts_ = 0;
    
    // Start reading messages
    doRead();
    
    // Send any queued messages
    if (!message_queue_.empty()) {
        doWrite();
    }
    
    // Notify connection handler
    if (connect_handler_) {
        connect_handler_(true);
    }
}

void WebSocketClient::onConnectTimeout(const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted) {
        // Timer was cancelled, connection succeeded
        return;
    }
    
    if (state_ == ConnectionState::CONNECTING) {
        LOG_ERROR("Connection timeout after {} seconds", config_.connect_timeout.count());
        
        // Close any pending connection
        try {
            if (ws_ && beast::get_lowest_layer(*ws_).socket().is_open()) {
                beast::get_lowest_layer(*ws_).socket().close();
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error closing socket on timeout: {}", e.what());
        }
        
        // Handle the timeout error
        handleError("Connection timeout", boost::asio::error::timed_out);
    }
}

void WebSocketClient::doRead() {
    if (state_ != ConnectionState::CONNECTED) {
        return;
    }
    
    // Read a message
    ws_->async_read(
        read_buffer_,
        std::bind(&WebSocketClient::onRead, shared_from_this(),
                 std::placeholders::_1, std::placeholders::_2));
}

void WebSocketClient::onRead(
    const boost::system::error_code& ec,
    std::size_t bytes_transferred) {
    
    if (ec) {
        if (ec == websocket::error::closed) {
            LOG_INFO("WebSocket connection closed by server");
            state_ = ConnectionState::CLOSED;
            
            if (close_handler_) {
                close_handler_("Connection closed by server");
            }
            
            // Try to reconnect if configured
            if (config_.max_reconnect_attempts == 0 || 
                reconnect_attempts_ < config_.max_reconnect_attempts) {
                scheduleReconnect();
            }
        } else {
            handleError("Read error", ec);
        }
        return;
    }
    
    // Extract the message
    std::string message = beast::buffers_to_string(read_buffer_.data());
    read_buffer_.consume(read_buffer_.size());
    
    LOG_DEBUG("Received WebSocket message: {}", message);
    
    // Notify message handler
    if (message_handler_) {
        message_handler_(message);
    }
    
    // Continue reading
    doRead();
}

bool WebSocketClient::send(const std::string& message) {
    if (state_ == ConnectionState::CLOSED || state_ == ConnectionState::ERROR) {
        LOG_ERROR("Cannot send message, connection is closed or in error state");
        return false;
    }
    
    LOG_DEBUG("Sending WebSocket message: {}", message);
    
    // Add message to queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        message_queue_.push(message);
    }
    
    // If connected and not writing, start write operation
    if (state_ == ConnectionState::CONNECTED && !write_in_progress_) {
        doWrite();
    }
    
    return true;
}

void WebSocketClient::doWrite() {
    if (state_ != ConnectionState::CONNECTED) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (message_queue_.empty() || write_in_progress_) {
        return;
    }
    
    write_in_progress_ = true;
    
    // Get the next message
    std::string message = message_queue_.front();
    message_queue_.pop();
    
    // Send the message
    ws_->async_write(
        net::buffer(message),
        std::bind(&WebSocketClient::onWrite, shared_from_this(),
                 std::placeholders::_1, std::placeholders::_2));
}

void WebSocketClient::onWrite(
    const boost::system::error_code& ec,
    std::size_t bytes_transferred) {
    
    if (ec) {
        write_in_progress_ = false;
        handleError("Write error", ec);
        return;
    }
    
    LOG_DEBUG("Sent {} bytes", bytes_transferred);
    
    // Mark write as complete
    write_in_progress_ = false;
    
    // Send next message if any
    doWrite();
}

void WebSocketClient::close(const std::string& reason) {
    if (state_ == ConnectionState::CLOSED || state_ == ConnectionState::CLOSING) {
        return;
    }
    
    LOG_INFO("Closing WebSocket connection: {}", reason);
    
    state_ = ConnectionState::CLOSING;
    
    // Cancel any pending reconnect
    reconnect_timer_.cancel();
    
    // Close the WebSocket connection if it's open
    if (ws_ && ws_->is_open()) {
        try {
            ws_->async_close(
                websocket::close_code::normal,
                std::bind(&WebSocketClient::onClose, shared_from_this(),
                         std::placeholders::_1));
        } catch (const std::exception& e) {
            LOG_ERROR("Error during WebSocket close: {}", e.what());
            state_ = ConnectionState::CLOSED;
        }
    } else {
        state_ = ConnectionState::CLOSED;
    }
}

void WebSocketClient::onClose(const boost::system::error_code& ec) {
    if (ec) {
        LOG_WARN("Error during WebSocket close: {}", ec.message());
    }
    
    state_ = ConnectionState::CLOSED;
    
    if (close_handler_) {
        close_handler_("Connection closed by client");
    }
}

void WebSocketClient::scheduleReconnect() {
    if (state_ == ConnectionState::RECONNECTING) {
        return;
    }
    
    state_ = ConnectionState::RECONNECTING;
    reconnect_attempts_++;
    
    // Calculate next reconnect interval with exponential backoff
    auto interval = calculateReconnectInterval();
    
    LOG_INFO("Scheduling reconnect attempt {} in {} seconds", 
             reconnect_attempts_, interval.count());
    
    reconnect_timer_.expires_after(interval);
    reconnect_timer_.async_wait(
        std::bind(&WebSocketClient::onReconnectTimer, shared_from_this(),
                 std::placeholders::_1));
}

void WebSocketClient::onReconnectTimer(const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted) {
        // Timer was cancelled
        return;
    }
    
    LOG_INFO("Attempting to reconnect (attempt {})", reconnect_attempts_);
    
    // Start connection process
    state_ = ConnectionState::CONNECTING;
    doConnect();
}

std::chrono::seconds WebSocketClient::calculateReconnectInterval() {
    // Exponential backoff with jitter
    std::chrono::seconds base_interval = config_.reconnect_interval;
    
    if (reconnect_attempts_ > 1) {
        // Calculate exponential backoff: base * 2^(attempts-1)
        int exp_factor = 1 << (reconnect_attempts_ - 1);
        base_interval = std::min(
            config_.reconnect_interval * exp_factor,
            config_.max_reconnect_interval);
    }
    
    // Add jitter (±20%)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> jitter(-20, 20);
    
    int jitter_percent = jitter(gen);
    auto interval = base_interval;
    interval += std::chrono::seconds(
        (interval.count() * jitter_percent) / 100);
    
    return interval;
}

void WebSocketClient::handleError(const std::string& message, const boost::system::error_code& ec) {
    std::string error_message = message;
    if (ec) {
        error_message += ": " + ec.message();
    }
    
    LOG_ERROR("WebSocket error: {}", error_message);
    
    // Update state
    state_ = ConnectionState::ERROR;
    
    // Cancel timers
    connect_timeout_timer_.cancel();
    
    // Notify error handler
    if (error_handler_) {
        error_handler_(message, ec);
    }
    
    // Try to reconnect if configured
    if (config_.max_reconnect_attempts == 0 || 
        reconnect_attempts_ < config_.max_reconnect_attempts) {
        scheduleReconnect();
    } else {
        LOG_ERROR("Maximum reconnect attempts ({}) reached, giving up", 
                 config_.max_reconnect_attempts);
    }
}

ConnectionState WebSocketClient::getState() const {
    return state_;
}

bool WebSocketClient::isConnected() const {
    return state_ == ConnectionState::CONNECTED;
}

int WebSocketClient::getReconnectAttempts() const {
    return reconnect_attempts_;
}

void WebSocketClient::resetReconnectAttempts() {
    reconnect_attempts_ = 0;
}

std::string WebSocketClient::getUrl() const {
    return config_.url;
}

void WebSocketClient::setMessageHandler(MessageHandler handler) {
    message_handler_ = handler;
}

void WebSocketClient::setCloseHandler(CloseHandler handler) {
    close_handler_ = handler;
}

void WebSocketClient::setErrorHandler(ErrorHandler handler) {
    error_handler_ = handler;
}

} // namespace ocpp
} // namespace ocpp_gateway