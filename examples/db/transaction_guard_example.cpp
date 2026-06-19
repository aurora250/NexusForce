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
#include <NeForce/db/sql_builder.hpp>

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_connect.hpp>
#endif

using namespace neforce;

int main() {
#ifdef NEFORCE_SUPPORT_SQLITE3
    sqlite_connect conn;
    ignore = conn.connect(db_config::for_sqlite(":memory:"));

    ignore = conn.update("CREATE TABLE accounts (id INTEGER PRIMARY KEY, name TEXT, balance REAL)");

    // 初始化数据
    {
        sql_builder ins;
        ins.insert_into("accounts", {"name", "balance"}).values({"'Alice'", "1000.0"});
        ignore = conn.update(ins.build());

        ins.reset();
        ins.insert_into("accounts", {"name", "balance"}).values({"'Bob'", "500.0"});
        ignore = conn.update(ins.build());
    }

    // 转账：成功场景（显式 commit）
    println("=== 转账成功（Commit） ===");
    {
        scope_transaction tx{conn};
        // UPDATE
        {
            sql_builder upd;
            upd.update("accounts").set("balance = balance - 200").where_eq("name", "'Alice'");
            ignore = conn.update(upd.build());
        }
        {
            sql_builder upd;
            upd.update("accounts").set("balance = balance + 200").where_eq("name", "'Bob'");
            ignore = conn.update(upd.build());
        }
        tx.commit();
        println("事务已提交");
    }

    // SELECT
    {
        sql_builder sel;
        sel.select({"name", "balance"}).from("accounts").order_by_asc("id");
        auto result = conn.query(sel.build());
        while (result != nullptr && result->next()) {
            printfln("  {}: balance={:.1f}", result->get(0), result->get_float64(1));
        }
    }

    // 转账：失败场景（离开作用域自动 rollback）
    println("\n=== 转账失败（自动回滚） ===");
    {
        scope_transaction tx{conn};
        {
            sql_builder upd;
            upd.update("accounts").set("balance = balance - 9999").where_eq("name", "'Alice'");
            ignore = conn.update(upd.build());
        }
        {
            sql_builder upd;
            upd.update("accounts").set("balance = balance + 9999").where_eq("name", "'Bob'");
            ignore = conn.update(upd.build());
        }
        println("未调用 commit，离开作用域时将自动 rollback");
    }

    {
        sql_builder sel;
        sel.select({"name", "balance"}).from("accounts").order_by_asc("id");
        auto result2 = conn.query(sel.build());
        while (result2 != nullptr && result2->next()) {
            printfln("  {}: balance={:.1f} (应为转账前的值)", result2->get(0), result2->get_float64(1));
        }
    }

    // make_transaction 工厂函数
    println("\n=== make_transaction 工厂函数 ===");
    {
        auto tx = make_transaction(conn);
        sql_builder upd;
        upd.update("accounts").set("balance", "999").where_eq("name", "'Alice'");
        ignore = conn.update(upd.build());
        tx.commit();
        println("使用工厂函数创建事务并提交");
    }
#else
    println("SQLite3 支持未启用");
#endif
    return 0;
}
