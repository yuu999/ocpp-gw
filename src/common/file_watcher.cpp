#include "ocpp_gateway/common/file_watcher.h"
#include "ocpp_gateway/common/logger.h"
#include <algorithm>

namespace ocpp_gateway {
namespace common {

namespace fs = std::filesystem;

FileWatcher::FileWatcher(std::chrono::milliseconds poll_interval_ms)
    : poll_interval_(poll_interval_ms) {
}

FileWatcher::~FileWatcher() {
    stop();
}

bool FileWatcher::addWatch(const std::string& file_path, FileChangeCallback callback) {
    try {
        if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
            LOG_ERROR("Cannot watch file {}: file does not exist or is not a regular file", file_path);
            return false;
        }

        std::lock_guard<std::mutex> lock(watches_mutex_);
        WatchInfo info;
        info.last_write_time = fs::last_write_time(file_path);
        info.callback = callback;
        info.is_directory = false;
        info.file_extension = "";
        info.recursive = false;
        watches_[file_path] = info;
        
        LOG_DEBUG("Added watch for file: {}", file_path);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error adding watch for file {}: {}", file_path, e.what());
        return false;
    }
}

bool FileWatcher::addDirectoryWatch(const std::string& directory_path, FileChangeCallback callback,
                                   const std::string& file_extension, bool recursive) {
    try {
        if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
            LOG_ERROR("Cannot watch directory {}: directory does not exist", directory_path);
            return false;
        }

        std::lock_guard<std::mutex> lock(watches_mutex_);
        WatchInfo info;
        info.last_write_time = fs::last_write_time(directory_path);
        info.callback = callback;
        info.is_directory = true;
        info.file_extension = file_extension;
        info.recursive = recursive;
        watches_[directory_path] = info;
        
        LOG_DEBUG("Added watch for directory: {} (extension: {}, recursive: {})", 
                 directory_path, file_extension.empty() ? "all" : file_extension, recursive);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error adding watch for directory {}: {}", directory_path, e.what());
        return false;
    }
}

bool FileWatcher::removeWatch(const std::string& path) {
    std::lock_guard<std::mutex> lock(watches_mutex_);
    auto it = watches_.find(path);
    if (it != watches_.end()) {
        watches_.erase(it);
        LOG_DEBUG("Removed watch for path: {}", path);
        return true;
    }
    return false;
}

void FileWatcher::start() {
    if (running_) {
        return;
    }

    running_ = true;
    watch_thread_ = std::thread(&FileWatcher::watchThread, this);
    LOG_INFO("File watcher started");
}

void FileWatcher::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }
    LOG_INFO("File watcher stopped");
}

void FileWatcher::watchThread() {
    while (running_) {
        std::this_thread::sleep_for(poll_interval_);

        std::vector<std::string> changed_files;
        {
            std::lock_guard<std::mutex> lock(watches_mutex_);
            
            for (auto& [path, watch_info] : watches_) {
                try {
                    if (!fs::exists(path)) {
                        LOG_WARN("Watched path no longer exists: {}", path);
                        continue;
                    }

                    if (watch_info.is_directory) {
                        checkDirectoryChanges(path, watch_info, changed_files);
                    } else if (hasFileChanged(path, watch_info)) {
                        changed_files.push_back(path);
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("Error checking changes for {}: {}", path, e.what());
                }
            }
        }

        // Process callbacks outside the lock to avoid deadlocks
        for (const auto& file_path : changed_files) {
            std::lock_guard<std::mutex> lock(watches_mutex_);
            auto it = watches_.find(file_path);
            if (it != watches_.end()) {
                try {
                    LOG_DEBUG("File changed: {}", file_path);
                    it->second.callback(file_path);
                } catch (const std::exception& e) {
                    LOG_ERROR("Error in file change callback for {}: {}", file_path, e.what());
                }
            }
        }
    }
}

bool FileWatcher::hasFileChanged(const std::string& path, WatchInfo& watch_info) {
    try {
        auto current_write_time = fs::last_write_time(path);
        if (current_write_time != watch_info.last_write_time) {
            watch_info.last_write_time = current_write_time;
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error checking file modification time for {}: {}", path, e.what());
    }
    return false;
}

void FileWatcher::checkDirectoryChanges(const std::string& directory_path, WatchInfo& watch_info,
                                       std::vector<std::string>& changed_files) {
    try {
        // Update directory's last write time
        auto current_dir_time = fs::last_write_time(directory_path);
        bool dir_changed = current_dir_time != watch_info.last_write_time;
        watch_info.last_write_time = current_dir_time;

        // If directory hasn't changed and we're not doing recursive checks, we can skip
        if (!dir_changed && !watch_info.recursive) {
            return;
        }

        // Check all files in the directory
        fs::directory_iterator end_iter;
        fs::directory_iterator dir_iter(directory_path);
        
        for (; dir_iter != end_iter; ++dir_iter) {
            const auto& entry = *dir_iter;
            const auto& path = entry.path().string();
            
            // Skip if not matching extension
            if (!watch_info.file_extension.empty() && 
                entry.is_regular_file() && 
                entry.path().extension().string() != watch_info.file_extension) {
                continue;
            }
            
            if (entry.is_regular_file()) {
                // Check if this specific file has changed
                auto current_write_time = fs::last_write_time(entry.path());
                
                // We need to check if we've seen this file before
                auto file_it = watches_.find(path);
                if (file_it == watches_.end()) {
                    // New file, add it to our tracking
                    WatchInfo file_info;
                    file_info.last_write_time = current_write_time;
                    file_info.is_directory = false;
                    watches_[path] = file_info;
                    
                    // New file counts as a change
                    changed_files.push_back(path);
                } else if (current_write_time != file_it->second.last_write_time) {
                    // Existing file with new modification time
                    file_it->second.last_write_time = current_write_time;
                    changed_files.push_back(path);
                }
            } else if (watch_info.recursive && entry.is_directory()) {
                // Recursively check subdirectories
                auto subdir_it = watches_.find(path);
                if (subdir_it == watches_.end()) {
                    // New subdirectory, add it to our tracking
                    WatchInfo subdir_info;
                    subdir_info.last_write_time = fs::last_write_time(entry.path());
                    subdir_info.is_directory = true;
                    subdir_info.file_extension = watch_info.file_extension;
                    subdir_info.recursive = true;
                    watches_[path] = subdir_info;
                }
                
                // Check files in the subdirectory
                checkDirectoryChanges(path, watches_[path], changed_files);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error checking directory changes for {}: {}", directory_path, e.what());
    }
}

} // namespace common
} // namespace ocpp_gateway