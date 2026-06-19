#ifndef NEFORCE_DATABASE_SQL_CONNECT_BASE_HPP__
#define NEFORCE_DATABASE_SQL_CONNECT_BASE_HPP__

/**
 * @file sql_connect_base.hpp
 * @brief SQL连接CRTP基类
 *
 * 使用CRTP模式为SQL后端提供 begin/commit/rollback/native_handle/table_exists 的共享实现。
 * 派生类只需提供 begin_sql/commit_sql/rollback_sql/table_exists_query 四个方言方法，
 * 并将原生连接句柄 link_ 设为 protected。
 */

#include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @struct sql_connect_base
 * @brief SQL连接CRTP基类
 *
 * 通过CRTP静态多态为MySQL/PostgreSQL/SQLite提供事务管理和表存在性检查的共享实现。
 *
 * @tparam Derived 派生类类型
 *
 * 派生类需要提供：
 * - link_ （protected成员，原生连接句柄）
 * - begin_sql() → 返回开始事务的SQL
 * - commit_sql() → 返回提交事务的SQL
 * - rollback_sql() → 返回回滚事务的SQL
 * - table_exists_query(table) → 返回检查表是否存在的SQL
 */
template <typename Derived>
struct sql_connect_base : idb_tb_connect {
private:
    Derived& derived() noexcept { return static_cast<Derived&>(*this); }
    const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }

public:
    bool begin() final { return derived().update(derived().begin_sql()); }
    bool commit() final { return derived().update(derived().commit_sql()); }
    bool rollback() final { return derived().update(derived().rollback_sql()); }

    NEFORCE_NODISCARD void* native_handle() noexcept final { return static_cast<Derived*>(this)->link_; }

    NEFORCE_NODISCARD bool table_exists(const string& table) const override {
        auto result = derived().query(derived().table_exists_query(table));
        return result != nullptr && result->row_count() > 0;
    }
};

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_SQL_CONNECT_BASE_HPP__
