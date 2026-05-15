#ifndef NEFORCE_DATABASE_REDIS_RESULT_HPP__
#define NEFORCE_DATABASE_REDIS_RESULT_HPP__

/**
 * @file redis_result.hpp
 * @brief Redis结果集实现
 *
 * 此文件提供了Redis查询结果集的实现。
 */

#ifdef NEFORCE_SUPPORT_HIREDIS
#    include <hiredis/hiredis.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @defgroup Redis Redis
 * @brief Redis数据库相关功能
 * @{
 */

/**
 * @struct redis_result
 * @brief Redis结果集类
 *
 * 实现idb_kv_result接口，
 * 自动解析不同类型的Redis回复，提供统一的访问接口。
 *
 * 主要功能：
 * - 支持多种Redis回复类型
 * - 数组回复自动解析为键值对
 * - 单值回复的统一访问
 * - 类型安全的值获取
 */
struct NEFORCE_API redis_result final : idb_kv_result {
private:
    ::redisReply* result_ = nullptr; ///< hiredis回复对象
    size_type cursor_ = 0;           ///< 当前游标位置
    size_type rows_ = 0;             ///< 行数（键值对数量）

    unique_ptr<vector<string>> column_names_;           ///< 列名列表（兼容性保留）
    unique_ptr<vector<pair<string, string>>> kv_pairs_; ///< 键值对列表

    size_type kv_cursor_ = 0; ///< 键值对游标
    bool is_array_ = false;   ///< 是否为数组回复

    NEFORCE_NODISCARD string get_string() const;

public:
    /**
     * @brief 默认构造函数
     *
     * 创建空结果集。
     */
    redis_result();

    /**
     * @brief 构造函数
     * @param reply hiredis回复对象
     *
     * 解析Redis回复，构建结果集。
     * - 数组回复：如果元素数为偶数，解析为键值对
     * - 其他类型：作为单值处理
     */
    explicit redis_result(::redisReply* reply);

    /**
     * @brief 析构函数
     *
     * 释放redisReply对象资源。
     */
    ~redis_result() override;

    /**
     * @brief 检查结果集是否为空
     * @return 空结果集返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept override {
        return result_ == nullptr || (rows_ == 0 && kv_pairs_->empty());
    }

    /**
     * @brief 移动到下一个键值对
     * @return 成功移动返回true，到达末尾返回false
     *
     * 用于遍历哈希表结果集（HGETALL）的键值对。
     * 对于单值结果，首次调用返回true。
     */
    NEFORCE_NODISCARD bool next() noexcept override;

    /**
     * @brief 获取当前键
     * @return 键的字符串视图
     *
     * 对于哈希表结果集（HGETALL），返回当前键；
     * 对于单值结果，返回空字符串。
     */
    NEFORCE_NODISCARD string_view key() const noexcept override;

    /**
     * @brief 获取当前值
     * @return 值的字符串视图
     *
     * 返回当前结果的值部分。
     */
    NEFORCE_NODISCARD string_view value() const noexcept override;

    /**
     * @brief 获取布尔值
     * @return 布尔值
     *
     * 将字符串值解析为布尔值。
     */
    NEFORCE_NODISCARD bool value_bool() const override;

    /**
     * @brief 获取64位整数值
     * @return 整数值
     *
     * 优先使用Redis整型回复，否则解析字符串。
     */
    NEFORCE_NODISCARD int64_t value_int64() const override;

    /**
     * @brief 获取浮点值
     * @return 浮点值
     *
     * 将字符串值解析为浮点数。
     */
    NEFORCE_NODISCARD double value_double() const override;

    /**
     * @brief 获取数组值
     * @return 字符串向量
     *
     * 对于Redis数组回复，返回所有元素；
     * 对于其他类型，返回包含单个元素的向量。
     */
    NEFORCE_NODISCARD vector<string> value_array() const override;

    /**
     * @brief 获取哈希表值
     * @return 键值对向量的常量引用
     *
     * 返回解析后的键值对列表（用于HGETALL命令）。
     */
    NEFORCE_NODISCARD const vector<pair<string, string>>& value_hash() const override { return *kv_pairs_; }

    /**
     * @brief 获取Redis回复类型
     * @return redisReplyType枚举值
     */
    NEFORCE_NODISCARD int type() const noexcept { return result_ != nullptr ? result_->type : -1; }

    /**
     * @brief 检查是否为空值回复
     * @return 空值回复返回true
     */
    NEFORCE_NODISCARD bool is_nil() const noexcept { return result_ != nullptr && result_->type == REDIS_REPLY_NIL; }
};

/** @} */ // Redis

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_REDIS_RESULT_HPP__
