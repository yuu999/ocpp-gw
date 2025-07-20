#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <vector>

namespace ocpp_gateway {
namespace common {

/**
 * @brief Simple file system watcher that monitors files for changes
 */
class FileWatcher {
public:
    /**
     * @brief Callback function type for file change events
     * @param file_path Path to the changed file
     */
    using FileChangeCallback = std::function<void(const std::string&)>;

    /**
     * @brief Construct a new File Watcher object
     * 
     * @param poll_interval_ms Polling interval in milliseconds
     */
    explicit FileWatcher(std::chrono::milliseconds poll_interval_ms = std::chrono::milliseconds(1000));

    /**
     * @brief Destroy the File Watcher object
     */
    ~FileWatcher();

    /**
     * @brief Add a file to watch
     * 
     * @param file_path Path to the file to watch
     * @param callback Callback function to call when the file changes
     * @return true if the file was added successfully
     * @return false if the file does not exist or could not be added
     */
    bool addWatch(const std::string& file_path, FileChangeCallback callback);

    /**
     * @brief Add a directory to watch (watches all files in the directory)
     * 
     * @param directory_path Path to the directory to watch
     * @param callback Callback function to call when any file in the directory changes
     * @param file_extension Optional file extension filter (e.g., ".yaml", ".json")
     * @param recursive Whether to watch subdirectories recursively
     * @return true if the directory was added successfully
     * @return false if the directory does not exist or could not be added
     */
    bool addDirectoryWatch(const std::string& directory_path, FileChangeCallback callback, 
                          const std::string& file_extension = "", bool recursive = false);

    /**
     * @brief Remove a file or directory from the watch list
     * 
     * @param path Path to the file or directory to remove
     * @return true if the path was removed
     * @return false if the path was not being watched
     */
    bool removeWatch(const std::string& path);

    /**
     * @brief Start watching for file changes
     */
    void start();

    /**
     * @brief Stop watching for file changes
     */
    void stop();

    /**
     * @brief Check if the watcher is running
     * 
     * @return true if the watcher is running
     * @return false if the watcher is stopped
     */
    bool isRunning() const { return running_; }

private:
    struct WatchInfo {
        std::filesystem::file_time_type last_write_time;
        FileChangeCallback callback;
        bool is_directory;
        std::string file_extension;
        bool recursive;
    };

    std::chrono::milliseconds poll_interval_;
    std::map<std::string, WatchInfo> watches_;
    std::thread watch_thread_;
    std::mutex watches_mutex_;
    std::atomic<bool> running_{false};

    /**
     * @brief Watch thread function
     */
    void watchThread();

    /**
     * @brief Check if a file has changed
     * 
     * @param path Path to the file
     * @param watch_info Watch information
     * @return true if the file has changed
     * @return false if the file has not changed
     */
    bool hasFileChanged(const std::string& path, WatchInfo& watch_info);

    /**
     * @brief Check if any files in a directory have changed
     * 
     * @param directory_path Path to the directory
     * @param watch_info Watch information
     * @param changed_files Vector to store paths of changed files
     */
    void checkDirectoryChanges(const std::string& directory_path, WatchInfo& watch_info, 
                              std::vector<std::string>& changed_files);
};

} // namespace common
} // namespace ocpp_gateway