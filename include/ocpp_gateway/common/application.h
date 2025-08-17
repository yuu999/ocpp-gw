#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <boost/asio/io_context.hpp>

namespace ocpp_gateway {

// Forward declarations
namespace api {
    class AdminApi;
    class CliManager;
    class WebUI;
}

namespace common {
    class PrometheusExporter;
    class ConfigManager;
    class MetricsCollector;
}

namespace ocpp {
    class OcppClientManager;
}

namespace mapping {
    class MappingEngine;
}

// namespace device {
//     class DeviceManager;
// }

namespace config {
    class ConfigManager;
}

/**
 * @brief Main application class for OCPP Gateway
 */
class Application {
public:
    /**
     * @brief Application configuration
     */
    struct Config {
        std::string config_path;
        bool verbose = false;
        bool daemon_mode = false;
        bool cli_mode = false;
        std::vector<std::string> cli_args;
    };

    /**
     * @brief Application execution result
     */
    enum class ExitCode {
        SUCCESS = 0,
        INITIALIZATION_FAILED = 1,
        CONFIG_ERROR = 2,
        RUNTIME_ERROR = 3
    };

    Application();
    ~Application();

    /**
     * @brief Initialize the application
     * @param config Application configuration
     * @return true if initialization was successful
     */
    bool initialize(const Config& config);

    /**
     * @brief Run the application
     * @return Exit code
     */
    ExitCode run();

    /**
     * @brief Request application stop
     */
    void requestStop();

    /**
     * @brief Check if application is running
     * @return true if running
     */
    bool isRunning() const;

    /**
     * @brief Execute CLI command
     * @param args Command arguments
     * @return true if execution was successful
     */
    bool executeCli(const std::vector<std::string>& args);

private:
    /**
     * @brief Setup signal handlers
     */
    void setupSignalHandlers();

    /**
     * @brief Initialize system components
     * @return true if initialization was successful
     */
    bool initializeComponents();

    /**
     * @brief Run in daemon mode
     * @return Exit code
     */
    ExitCode runDaemon();

    /**
     * @brief Run in CLI mode
     * @return Exit code
     */
    ExitCode runCli();

    /**
     * @brief Run interactive CLI
     * @return Exit code
     */
    ExitCode runInteractiveCli();

    /**
     * @brief Run single CLI command
     * @return Exit code
     */
    ExitCode runSingleCli();

    /**
     * @brief Run main loop
     */
    void runMainLoop();

    /**
     * @brief Cleanup system resources
     */
    void cleanup();

    /**
     * @brief Reload configuration
     */
    void reloadConfiguration();

    // Configuration
    Config config_;
    
    // Runtime state
    std::atomic<bool> running_;
    std::atomic<bool> reload_requested_;

    // IO Context for async operations
    boost::asio::io_context io_context_;

    // Service components  
    std::unique_ptr<api::AdminApi> admin_api_;
    std::unique_ptr<api::CliManager> cli_manager_;
    std::unique_ptr<api::WebUI> web_ui_;
    std::unique_ptr<common::PrometheusExporter> prometheus_exporter_;
    
    // Business logic components
    std::unique_ptr<mapping::MappingEngine> mapping_engine_;
    // std::unique_ptr<device::DeviceManager> device_manager_;
    std::shared_ptr<ocpp::OcppClientManager> ocpp_client_manager_;

    // Signal handling
    static Application* instance_;
    static void signalHandler(int signal);
};

} // namespace ocpp_gateway 