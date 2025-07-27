#include "ocpp_gateway/common/service_manager.h"
#include "ocpp_gateway/common/logger.h"
#include <algorithm>
#include <unordered_set>
#include <queue>

namespace ocpp_gateway {
namespace common {

ServiceManager::ServiceManager() = default;

ServiceManager::~ServiceManager() {
    finalizeAll();
}

bool ServiceManager::initializeAll() {
    if (all_initialized_) {
        LOG_WARN("Services are already initialized");
        return true;
    }

    try {
        // 依存関係を考慮した初期化順序を計算
        calculated_order_ = calculateInitializationOrder();
        
        LOG_INFO("Initializing {} services in dependency order", calculated_order_.size());
        
        // 計算した順序でサービスを初期化
        for (const auto& type : calculated_order_) {
            auto& info = service_info_[type];
            
            if (info.initializer) {
                LOG_DEBUG("Initializing service: {}", info.name);
                info.initializer();
                info.initialized = true;
                LOG_INFO("Service initialized: {}", info.name);
            } else {
                // 初期化関数がない場合はそのまま初期化済みとする
                info.initialized = true;
                LOG_DEBUG("Service registered without initializer: {}", info.name);
            }
        }
        
        all_initialized_ = true;
        LOG_INFO("All services initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Service initialization failed: {}", e.what());
        return false;
    }
}

void ServiceManager::finalizeAll() {
    if (!all_initialized_) {
        return; // Not initialized, nothing to finalize
    }
    
    LOG_INFO("Finalizing services in reverse order");
    
    // 初期化と逆順で終了処理を実行
    for (auto it = calculated_order_.rbegin(); it != calculated_order_.rend(); ++it) {
        const auto& type = *it;
        auto& info = service_info_[type];
        
        if (info.initialized && !info.finalized) {
            if (info.finalizer) {
                try {
                    LOG_DEBUG("Finalizing service: {}", info.name);
                    info.finalizer();
                    info.finalized = true;
                    LOG_INFO("Service finalized: {}", info.name);
                } catch (const std::exception& e) {
                    LOG_ERROR("Error finalizing service {}: {}", info.name, e.what());
                }
            } else {
                info.finalized = true;
                LOG_DEBUG("Service finalized (no finalizer): {}", info.name);
            }
        }
    }
    
    all_initialized_ = false;
    LOG_INFO("All services finalized");
}

std::vector<std::string> ServiceManager::getServiceNames() const {
    std::vector<std::string> names;
    names.reserve(service_info_.size());
    
    for (const auto& [type, info] : service_info_) {
        names.push_back(info.name);
    }
    
    return names;
}

bool ServiceManager::isServiceInitialized(const std::string& name) const {
    for (const auto& [type, info] : service_info_) {
        if (info.name == name) {
            return info.initialized;
        }
    }
    return false;
}

std::vector<std::type_index> ServiceManager::calculateInitializationOrder() const {
    // トポロジカルソート（Kahn's algorithm）を使用して依存関係を解決
    
    // 依存関係グラフの構築
    std::unordered_map<std::type_index, std::vector<std::type_index>> graph;
    std::unordered_map<std::type_index, int> in_degree;
    
    // すべてのノードを初期化
    for (const auto& type : initialization_order_) {
        graph[type] = {};
        in_degree[type] = 0;
    }
    
    // エッジを追加し、入次数を計算
    for (const auto& type : initialization_order_) {
        const auto& info = service_info_.at(type);
        for (const auto& dep_type : info.dependencies) {
            // 依存先が登録されているかチェック
            if (service_info_.find(dep_type) == service_info_.end()) {
                throw ServiceManagerException(
                    "Dependency not found for service " + info.name + ": " + 
                    std::string(dep_type.name())
                );
            }
            
            graph[dep_type].push_back(type);
            in_degree[type]++;
        }
    }
    
    // 循環依存をチェック
    if (hasCircularDependencies(graph)) {
        throw ServiceManagerException("Circular dependency detected in services");
    }
    
    // トポロジカルソート
    std::queue<std::type_index> zero_in_degree;
    for (const auto& [type, degree] : in_degree) {
        if (degree == 0) {
            zero_in_degree.push(type);
        }
    }
    
    std::vector<std::type_index> result;
    result.reserve(initialization_order_.size());
    
    while (!zero_in_degree.empty()) {
        auto current = zero_in_degree.front();
        zero_in_degree.pop();
        result.push_back(current);
        
        for (const auto& neighbor : graph[current]) {
            in_degree[neighbor]--;
            if (in_degree[neighbor] == 0) {
                zero_in_degree.push(neighbor);
            }
        }
    }
    
    if (result.size() != initialization_order_.size()) {
        throw ServiceManagerException("Failed to resolve service dependencies");
    }
    
    return result;
}

bool ServiceManager::hasCircularDependencies(
    const std::unordered_map<std::type_index, std::vector<std::type_index>>& dependencies
) const {
    std::unordered_set<std::type_index> visited;
    std::unordered_set<std::type_index> rec_stack;
    
    std::function<bool(const std::type_index&)> has_cycle = 
        [&](const std::type_index& node) -> bool {
        visited.insert(node);
        rec_stack.insert(node);
        
        auto it = dependencies.find(node);
        if (it != dependencies.end()) {
            for (const auto& neighbor : it->second) {
                if (rec_stack.find(neighbor) != rec_stack.end()) {
                    return true; // 循環依存発見
                }
                if (visited.find(neighbor) == visited.end() && has_cycle(neighbor)) {
                    return true;
                }
            }
        }
        
        rec_stack.erase(node);
        return false;
    };
    
    for (const auto& type : initialization_order_) {
        if (visited.find(type) == visited.end()) {
            if (has_cycle(type)) {
                return true;
            }
        }
    }
    
    return false;
}

std::string ServiceManager::findServiceName(const std::type_index& type) const {
    auto it = service_info_.find(type);
    if (it != service_info_.end()) {
        return it->second.name;
    }
    return std::string(type.name());
}

} // namespace common
} // namespace ocpp_gateway 