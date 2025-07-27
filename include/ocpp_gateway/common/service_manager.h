#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <vector>
#include <stdexcept>

namespace ocpp_gateway {
namespace common {

/**
 * @brief サービス管理例外クラス
 */
class ServiceManagerException : public std::runtime_error {
public:
    explicit ServiceManagerException(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief サービス登録情報
 */
struct ServiceInfo {
    std::string name;
    std::function<void()> initializer;
    std::function<void()> finalizer;
    std::vector<std::type_index> dependencies;
    bool initialized = false;
    bool finalized = false;
};

/**
 * @brief サービス管理クラス
 * 
 * 依存性注入パターンを使用してシステム内のサービスの
 * ライフサイクル管理と依存関係解決を行います。
 */
class ServiceManager {
public:
    ServiceManager();
    ~ServiceManager();

    /**
     * @brief サービスの登録
     * @tparam T サービス型
     * @param name サービス名
     * @param instance サービスインスタンス
     * @param initializer 初期化関数
     * @param finalizer 終了処理関数
     * @param dependencies 依存するサービス型のリスト
     */
    template<typename T>
    void registerService(
        const std::string& name,
        std::unique_ptr<T> instance,
        std::function<void()> initializer = nullptr,
        std::function<void()> finalizer = nullptr,
        const std::vector<std::type_index>& dependencies = {}
    ) {
        auto type = std::type_index(typeid(T));
        
        if (services_.find(type) != services_.end()) {
            throw ServiceManagerException("Service already registered: " + name);
        }

        // Store the instance with proper type erasure
        void* raw_ptr = instance.release();
        auto deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
        services_[type] = std::unique_ptr<void, void(*)(void*)>(raw_ptr, deleter);
        
        // Store service information
        ServiceInfo info;
        info.name = name;
        info.initializer = initializer;
        info.finalizer = finalizer;
        info.dependencies = dependencies;
        
        service_info_[type] = std::move(info);
        initialization_order_.push_back(type);
    }

    /**
     * @brief ファクトリー関数を使用したサービス登録
     * @tparam T サービス型
     * @param name サービス名
     * @param factory サービス作成ファクトリー関数
     * @param initializer 初期化関数
     * @param finalizer 終了処理関数
     * @param dependencies 依存するサービス型のリスト
     */
    template<typename T>
    void registerService(
        const std::string& name,
        std::function<std::unique_ptr<T>()> factory,
        std::function<void()> initializer = nullptr,
        std::function<void()> finalizer = nullptr,
        const std::vector<std::type_index>& dependencies = {}
    ) {
        auto instance = factory();
        registerService<T>(name, std::move(instance), initializer, finalizer, dependencies);
    }

    /**
     * @brief サービスの取得
     * @tparam T サービス型
     * @return サービスインスタンス
     */
    template<typename T>
    T* getService() {
        auto type = std::type_index(typeid(T));
        auto it = services_.find(type);
        
        if (it == services_.end()) {
            return nullptr;
        }
        
        return static_cast<T*>(it->second.get());
    }

    /**
     * @brief サービスの取得（const版）
     * @tparam T サービス型
     * @return サービスインスタンス
     */
    template<typename T>
    const T* getService() const {
        auto type = std::type_index(typeid(T));
        auto it = services_.find(type);
        
        if (it == services_.end()) {
            return nullptr;
        }
        
        return static_cast<const T*>(it->second.get());
    }

    /**
     * @brief 必須サービスの取得
     * @tparam T サービス型
     * @return サービスインスタンス
     * @throws ServiceManagerException サービスが見つからない場合
     */
    template<typename T>
    T& requireService() {
        auto* service = getService<T>();
        if (!service) {
            throw ServiceManagerException("Required service not found: " + 
                std::string(typeid(T).name()));
        }
        return *service;
    }

    /**
     * @brief 必須サービスの取得（const版）
     * @tparam T サービス型
     * @return サービスインスタンス
     * @throws ServiceManagerException サービスが見つからない場合
     */
    template<typename T>
    const T& requireService() const {
        auto* service = getService<T>();
        if (!service) {
            throw ServiceManagerException("Required service not found: " + 
                std::string(typeid(T).name()));
        }
        return *service;
    }

    /**
     * @brief サービスの存在確認
     * @tparam T サービス型
     * @return 存在する場合true
     */
    template<typename T>
    bool hasService() const {
        auto type = std::type_index(typeid(T));
        return services_.find(type) != services_.end();
    }

    /**
     * @brief 全サービスの初期化
     * 依存関係を考慮して適切な順序で初期化を実行
     * @return 初期化成功時true
     */
    bool initializeAll();

    /**
     * @brief 全サービスの終了処理
     * 初期化と逆順で終了処理を実行
     */
    void finalizeAll();

    /**
     * @brief サービス情報の取得
     * @return 登録されているサービスの情報
     */
    std::vector<std::string> getServiceNames() const;

    /**
     * @brief サービス状態の確認
     * @param name サービス名
     * @return 初期化済みの場合true
     */
    bool isServiceInitialized(const std::string& name) const;

private:
    /**
     * @brief 依存関係を考慮した初期化順序の計算
     * @return 初期化順序のサービス型リスト
     */
    std::vector<std::type_index> calculateInitializationOrder() const;

    /**
     * @brief 循環依存のチェック
     * @param dependencies 依存関係マップ
     * @return 循環依存がある場合true
     */
    bool hasCircularDependencies(
        const std::unordered_map<std::type_index, std::vector<std::type_index>>& dependencies
    ) const;

    /**
     * @brief サービス名の検索
     * @param type サービス型
     * @return サービス名
     */
    std::string findServiceName(const std::type_index& type) const;

    // サービスインスタンス（型 -> インスタンス）
    std::unordered_map<std::type_index, std::unique_ptr<void, void(*)(void*)>> services_;
    
    // サービス情報（型 -> 情報）
    std::unordered_map<std::type_index, ServiceInfo> service_info_;
    
    // 登録順序（依存関係解決前）
    std::vector<std::type_index> initialization_order_;
    
    // 計算済み初期化順序
    std::vector<std::type_index> calculated_order_;
    
    // 初期化状態
    bool all_initialized_ = false;
};

} // namespace common
} // namespace ocpp_gateway 