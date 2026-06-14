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

class signal_base;

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
    mutex mutex_;                                          ///< 保护类型映射的互斥锁
    unordered_map<type_id, unique_ptr<meta_type>> types_;  ///< 类型ID到元数据的映射
    unordered_map<type_id, type_id> name_hash_to_type_id_; ///< 名称hash到类型ID的辅助映射

    registry() = default;

public:
    /**
     * @brief 获取单例实例
     * @return 注册表实例引用
     */
    NEFORCE_API static registry& instance();

    /**
     * @brief 注册类型
     * @tparam T 要注册的类型
     * @param name 类型名称
     * @return 类型元数据引用
     *
     * 如果类型已注册，返回现有元数据；否则创建新的元数据。
     * 使用 type_id_for<T>() 作为主键以与 meta_any 的类型 ID 保持一致。
     */
    template <typename T>
    meta_type& register_type(string_view name) {
        type_id id = type_id_for<T>();

        lock<mutex> lk(mutex_);
        auto it = types_.find(id);
        if (it != types_.end()) {
            return *it->second;
        }

        auto name_hash = name.to_hash();
        auto new_it = types_.emplace(id, make_unique<meta_type>(name, id, sizeof(T))).first;
        name_hash_to_type_id_.emplace(name_hash, id);
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
        auto name_hash = name.to_hash();
        auto map_it = name_hash_to_type_id_.find(name_hash);
        if (map_it == name_hash_to_type_id_.end()) {
            return nullptr;
        }
        auto it = types_.find(map_it->second);
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
        const auto it = types_.find(id);
        return it != types_.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief 遍历所有注册的类型
     * @tparam Func 回调函数类型
     * @param func 回调函数，接收meta_type&参数
     *
     * 对每个注册的类型调用回调函数。
     */
    template <typename Func>
    void for_each(Func&& func) {
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
        for (const auto& type: types_) {
            type.second->resolve_bases_unlocked(this);
        }
    }

    /**
     * @brief 查找信号并连接到槽函数
     * @param sender_obj 发送者对象指针
     * @param sender_type 发送者类型名称
     * @param signal_name 信号名称
     * @param signal_offset 信号成员在类中的字节偏移
     * @param receiver_obj 接收者对象指针
     * @param receiver_type 接收者类型名称
     * @param slot_name 槽函数名称
     * @return 连接成功返回 true
     *
     * 通过类型和名称在注册表中查找对应的信号和槽，
     * 利用信号基类 signal_base 和 meta_function 实现运行时动态连接。
     *
     * @note signal_offset 可通过 type_builder::signal_with_offset() 设置
     */
    static bool dynamic_connect(void* sender_obj, string_view sender_type, string_view signal_name,
                                size_t signal_offset, void* receiver_obj, string_view receiver_type,
                                string_view slot_name) {
        const auto* s_meta = instance().find(sender_type);
        const auto* r_meta = instance().find(receiver_type);
        if ((s_meta == nullptr) || (r_meta == nullptr)) {
            return false;
        }

        const auto* slot = r_meta->get_function(slot_name);
        if (slot == nullptr) {
            return false;
        }

        auto* sig_base = reinterpret_cast<signal_base*>(static_cast<char*>(sender_obj) + signal_offset);
        if (sig_base == nullptr) {
            return false;
        }

        return connect_signal_to_slot(sig_base, slot, receiver_obj);
    }

    /**
     * @brief 将信号连接至反射槽函数
     * @param sig 信号基类指针
     * @param slot 槽函数元数据
     * @param receiver 接收者对象指针
     * @return 总是返回 true（连接通过 signal::connect 维持）
     *
     * 通过 emit_dynamic 在信号触发时将参数转发至槽函数。
     */
    static bool connect_signal_to_slot(signal_base* sig, const meta_function* slot, void* receiver);
};

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_REGISTRY_HPP__
