#include <iostream>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include "ocpp_gateway/ocpp/websocket_client.h"
#include "ocpp_gateway/common/logger.h"

using namespace ocpp_gateway;

int main(int argc, char* argv[]) {
    // Initialize logger
    common::Logger::initialize("debug", "websocket_example.log");
    
    // Parse command line arguments
    std::string url = "wss://echo.websocket.org";
    std::string ca_cert_path;
    bool verify_peer = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--url" && i + 1 < argc) {
            url = argv[++i];
        } else if (arg == "--ca-cert" && i + 1 < argc) {
            ca_cert_path = argv[++i];
        } else if (arg == "--no-verify") {
            verify_peer = false;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --url <url>       WebSocket URL (default: wss://echo.websocket.org)\n"
                      << "  --ca-cert <path>  Path to CA certificate file\n"
                      << "  --no-verify       Disable server certificate verification\n"
                      << "  --help            Show this help message\n";
            return 0;
        }
    }
    
    try {
        // Create IO context
        boost::asio::io_context io_context;
        
        // Create WebSocket configuration
        ocpp::WebSocketConfig config;
        config.url = url;
        config.ca_cert_path = ca_cert_path;
        config.verify_peer = verify_peer;
        config.connect_timeout = std::chrono::seconds(10);
        config.reconnect_interval = std::chrono::seconds(5);
        config.max_reconnect_interval = std::chrono::seconds(60);
        config.max_reconnect_attempts = 5;
        
        // Create WebSocket client
        auto client = ocpp::WebSocketClient::create(io_context, config);
        
        // Set message handler
        client->setMessageHandler([](const std::string& message) {
            std::cout << "Received: " << message << std::endl;
        });
        
        // Set close handler
        client->setCloseHandler([](const std::string& reason) {
            std::cout << "Connection closed: " << reason << std::endl;
        });
        
        // Set error handler
        client->setErrorHandler([](const std::string& message, const std::error_code& ec) {
            std::cout << "Error: " << message << " (" << ec.message() << ")" << std::endl;
        });
        
        // Connect to server
        std::cout << "Connecting to " << url << "..." << std::endl;
        client->connect([&client](bool connected) {
            if (connected) {
                std::cout << "Connected!" << std::endl;
                
                // Send a test message
                std::string message = "Hello, WebSocket!";
                std::cout << "Sending: " << message << std::endl;
                client->send(message);
            } else {
                std::cout << "Connection failed!" << std::endl;
            }
        });
        
        // Create a thread to run the IO context
        std::thread io_thread([&io_context]() {
            io_context.run();
        });
        
        // Wait for user input
        std::cout << "Press Enter to close the connection..." << std::endl;
        std::cin.get();
        
        // Close the connection
        client->close("User requested");
        
        // Wait for IO thread to finish
        io_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}