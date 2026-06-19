#ifndef NEFORCE_DATABASE_TRANSACTION_GUARD_HPP__
#define NEFORCE_DATABASE_TRANSACTION_GUARD_HPP__

/**
 * @file scope_transaction.hpp
 * @brief RAII事务作用域守卫
 *
 * 提供异常安全的事务管理，构造时自动开始事务，析构时根据提交状态决定提交或回滚。
 *
 * 使用示例：
 * @code
 * void do_work(auto& conn) {
 *     scope_transaction tx{conn};     // BEGIN
 *     conn.update("INSERT INTO ...");
 *     conn.update("UPDATE ...");
 *     tx.commit();                     // COMMIT
 *     // 如果未调用 commit() 或发生异常，析构时自动 ROLLBACK
 * }
 * @endcode
 */

#include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @class scope_transaction
 * @brief RAII事务作用域守卫
 *
 * 管理数据库事务的生命周期：
 * - 构造时调用 begin()
 * - commit() 显式提交事务（仅可调用一次）
 * - 析构时若未提交则调用 rollback()
 *
 * 不可复制，支持移动语义。
 *
 * @tparam Connect 连接类型，需满足 idb_connect 接口（提供 begin/commit/rollback）
 */
template <typename Connect>
class scope_transaction {
private:
    Connect* conn_ = nullptr; ///< 数据库连接指针
    bool committed_ = false;  ///< 是否已提交

public:
    /**
     * @brief 构造函数，开始事务
     * @param conn 数据库连接引用
     *
     * 立即调用 conn.begin() 开始事务。
     */
    explicit scope_transaction(Connect& conn) :
    conn_(&conn) {
        conn_->begin();
    }

    /**
     * @brief 析构函数
     *
     * 如果事务未提交，自动调用 rollback() 回滚。
     * 如果事务已提交，不做任何操作。
     */
    ~scope_transaction() noexcept {
        if (conn_ != nullptr && !committed_) {
            try {
                conn_->rollback();
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
    }

    scope_transaction(const scope_transaction&) = delete;
    scope_transaction& operator=(const scope_transaction&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     *
     * 移动后源对象变为无效状态，不再持有连接引用。
     */
    scope_transaction(scope_transaction&& other) noexcept :
    conn_(other.conn_),
    committed_(other.committed_) {
        other.conn_ = nullptr;
        other.committed_ = true;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     *
     * 如果当前守卫未提交，先回滚当前事务。
     */
    scope_transaction& operator=(scope_transaction&& other) noexcept {
        if (addressof(other) != this) {
            if (conn_ != nullptr && !committed_) {
                try {
                    conn_->rollback();
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (...) {
                    // ignore
                }
            }
            conn_ = other.conn_;
            committed_ = other.committed_;
            other.conn_ = nullptr;
            other.committed_ = true;
        }
        return *this;
    }

    /**
     * @brief 提交事务
     *
     * 调用 conn.commit() 提交事务，并将守卫标记为已提交。
     * 已提交的守卫在析构时不会回滚。
     * 重复调用无效。
     */
    void commit() {
        if (conn_ != nullptr && !committed_) {
            conn_->commit();
            committed_ = true;
        }
    }

    /**
     * @brief 检查事务是否已提交
     * @return 已提交返回true
     */
    NEFORCE_NODISCARD bool committed() const noexcept { return committed_; }
};

/**
 * @brief 事务守卫的便捷构造函数
 * @param conn 数据库连接引用
 * @return scope_transaction 对象
 */
template <typename Connect>
NEFORCE_NODISCARD scope_transaction<Connect> make_transaction(Connect& conn) {
    return scope_transaction<Connect>(conn);
}

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_TRANSACTION_GUARD_HPP__
