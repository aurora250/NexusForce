#ifndef NEFORCE_CORE_REFLECT_REGISTRY_HPP__
#define NEFORCE_CORE_REFLECT_REGISTRY_HPP__

/**
 * @file registry.hpp
 * @brief 反射类型注册表
 *
 * 此文件提供了反射类型的全局注册表，
 * 用于存储和管理所有已注册的反射类型。
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/reflect/type.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @class registry
 * @brief 反射类型注册表
 *
 * 全局唯一的类型注册表，存储所有已注册的反射类型元数据。
 * 提供类型查找、遍历和基类解析功能。
 */
class registry {
private:
    mutex mutex_;                                         ///< 保护类型映射的互斥锁
    unordered_map<type_id, unique_ptr<meta_type>> types_; ///< 类型ID到元数据的映射

    registry() = default;

public:
    /**
     * @brief 获取单例实例
     * @return 注册表实例引用
     */
    static registry& instance() {
        static registry inst;
        return inst;
    }

    /**
     * @brief 注册类型
     * @tparam T 要注册的类型
     * @param name 类型名称
     * @return 类型元数据引用
     *
     * 如果类型已注册，返回现有元数据；否则创建新的元数据。
     */
    template <typename T> meta_type& register_type(string_view name) {
        type_id id = name.to_hash();

        lock<mutex> lk(mutex_);
        auto it = types_.find(id);
        if (it != types_.end()) {
            return *it->second;
        }

        auto new_it = types_.emplace(id, make_unique<meta_type>(name, id, sizeof(T))).first;
        return *new_it->second;
    }

    /**
     * @brief 查找类型
     * @param name 类型名称
     * @return 类型元数据指针，不存在返回nullptr
     *
     * @note 调用前需已持有互斥锁
     */
    meta_type* find_unlocked(string_view name) {
        auto it = types_.find(name.to_hash());
        return it != types_.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 查找类型
     * @param name 类型名称
     * @return 类型元数据指针，不存在返回nullptr
     */
    meta_type* find(string_view name) {
        lock<mutex> lock(mutex_);
        return find_unlocked(name);
    }

    /**
     * @brief 查找类型
     * @param id 类型ID
     * @return 类型元数据指针，不存在返回nullptr
     */
    meta_type* find(type_id id) {
        lock<mutex> lock(mutex_);
        auto it = types_.find(id);
        return it != types_.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 遍历所有注册的类型
     * @tparam Func 回调函数类型
     * @param func 回调函数，接收meta_type&参数
     *
     * 对每个注册的类型调用回调函数。
     */
    template <typename Func> void for_each(Func&& func) {
        lock<mutex> lock(mutex_);
        for (auto& type: types_) {
            func(*type.second);
        }
    }

    /**
     * @brief 解析所有类型的基类关系
     *
     * 遍历所有类型，解析其待解析的基类名称。
     * 应在所有类型注册完成后调用。
     */
    void resolve_all_bases() {
        lock<mutex> lk(mutex_);
        for (auto& type: types_) {
            type.second->resolve_bases_unlocked(this);
        }
    }
};

/** @} */ // Reflection

/// @cond

inline void meta_type::resolve_bases(registry* registry) {
    for (auto& base_name: pending_base_names_) {
        if (registry) {
            auto* base = registry->find(base_name.view());
            if (base) {
                base_types_.push_back(base);
            }
        }
    }
    pending_base_names_.clear();
}

inline void meta_type::resolve_bases_unlocked(registry* registry) {
    for (auto& base_name: pending_base_names_) {
        if (registry) {
            auto* base = registry->find_unlocked(base_name.view());
            if (base) {
                base_types_.push_back(base);
            }
        }
    }
    pending_base_names_.clear();
}

/// @endcond

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_REGISTRY_HPP__
