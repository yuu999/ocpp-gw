#include "ocpp_gateway/ocpp/mapping_config.h"
#include "ocpp_gateway/common/logger.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

namespace fs = std::filesystem;

// Global flag for program termination
std::atomic<bool> running(true);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Initialize logger
    ocpp_gateway::common::LogConfig log_config;
    log_config.console_output = true;
    log_config.file_output = false;
    log_config.log_level = "info";
    ocpp_gateway::common::Logger::initialize(log_config);

    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mapping_templates_directory>" << std::endl;
        return 1;
    }

    std::string templates_dir = argv[1];
    if (!fs::exists(templates_dir) || !fs::is_directory(templates_dir)) {
        std::cerr << "Error: " << templates_dir << " is not a valid directory" << std::endl;
        return 1;
    }

    try {
        // Create mapping template collection
        ocpp_gateway::ocpp::MappingTemplateCollection collection;

        // Load templates from directory
        std::cout << "Loading templates from " << templates_dir << std::endl;
        if (!collection.loadFromDirectory(templates_dir)) {
            std::cerr << "Error: Failed to load templates from directory" << std::endl;
            return 1;
        }

        // Print loaded templates
        std::cout << "Loaded " << collection.getTemplates().size() << " templates:" << std::endl;
        for (const auto& tmpl : collection.getTemplates()) {
            std::cout << "  - " << tmpl.getId() << ": " << tmpl.getDescription() << std::endl;
            std::cout << "    Variables: " << tmpl.getVariables().size() << std::endl;
        }

        // Resolve template inheritance
        std::cout << "Resolving template inheritance..." << std::endl;
        if (!collection.resolveInheritance()) {
            std::cerr << "Error: Failed to resolve template inheritance" << std::endl;
            return 1;
        }

        // Enable hot reload with callback
        std::cout << "Enabling hot reload for directory: " << templates_dir << std::endl;
        if (!collection.enableHotReload(templates_dir, [&collection](const std::string& file_path) {
            std::cout << "Template file changed: " << file_path << std::endl;
            std::cout << "Updated templates:" << std::endl;
            for (const auto& tmpl : collection.getTemplates()) {
                std::cout << "  - " << tmpl.getId() << ": " << tmpl.getDescription() << std::endl;
                std::cout << "    Variables: " << tmpl.getVariables().size() << std::endl;
            }
        })) {
            std::cerr << "Error: Failed to enable hot reload" << std::endl;
            return 1;
        }

        // Main loop - wait for changes or termination
        std::cout << "Watching for template changes. Press Ctrl+C to exit." << std::endl;
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Disable hot reload before exiting
        std::cout << "Disabling hot reload..." << std::endl;
        collection.disableHotReload();

        std::cout << "Exiting gracefully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}