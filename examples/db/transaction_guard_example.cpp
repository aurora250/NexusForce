/**
 * @example transaction_guard_example.cpp
 * @brief RAII事务管理示例
 *
 * 演示 transaction_guard 的自动事务管理：
 * - 构造时自动 BEGIN
 * - commit() 显式提交
 * - 析构时自动 ROLLBACK（未提交时）
 * - make_transaction 工厂函数
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/db/db_config.hpp>
#include <NeForce/db/transaction_guard.hpp>

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_connect.hpp>
#endif

using namespace neforce;

int main() {
#ifdef NEFORCE_SUPPORT_SQLITE3
    sqlite_connect conn;
    conn.connect(db_config::for_sqlite(":memory:"));
    conn.update("CREATE TABLE accounts (id INTEGER PRIMARY KEY, name TEXT, balance REAL)");
    conn.update("INSERT INTO accounts (name, balance) VALUES ('Alice', 1000.0)");
    conn.update("INSERT INTO accounts (name, balance) VALUES ('Bob', 500.0)");

    // 转账：成功场景（显式 commit）
    println("=== 转账成功（Commit） ===");
    {
        transaction_guard tx{conn};
        conn.update("UPDATE accounts SET balance = balance - 200 WHERE name = 'Alice'");
        conn.update("UPDATE accounts SET balance = balance + 200 WHERE name = 'Bob'");
        tx.commit();
        println("事务已提交");
    }

    auto result = conn.query("SELECT name, balance FROM accounts ORDER BY id");
    while (result != nullptr && result->next()) {
        printfln("  {}: balance={:.1f}", result->get(0), result->get_float64(1));
    }

    // 转账：失败场景（离开作用域自动 rollback）
    println("\n=== 转账失败（自动回滚） ===");
    {
        transaction_guard tx{conn};
        conn.update("UPDATE accounts SET balance = balance - 9999 WHERE name = 'Alice'");
        conn.update("UPDATE accounts SET balance = balance + 9999 WHERE name = 'Bob'");
        println("未调用 commit，离开作用域时将自动 rollback");
    }

    auto result2 = conn.query("SELECT name, balance FROM accounts ORDER BY id");
    while (result2 != nullptr && result2->next()) {
        printfln("  {}: balance={:.1f} (应为转账前的值)", result2->get(0), result2->get_float64(1));
    }

    // make_transaction 工厂函数
    println("\n=== make_transaction 工厂函数 ===");
    {
        auto tx = make_transaction(conn);
        conn.update("UPDATE accounts SET balance = 999 WHERE name = 'Alice'");
        tx.commit();
        println("使用工厂函数创建事务并提交");
    }
#else
    println("SQLite3 支持未启用");
#endif
    return 0;
}
