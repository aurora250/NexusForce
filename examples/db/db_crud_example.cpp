/**
 * @example db_crud_example.cpp
 * @brief 数据库CRUD与事务示例
 *
 * 演示通过 idb_tb_connect 接口进行数据库操作：
 * - 建表与插入
 * - 查询与结果集遍历
 * - 更新与删除
 * - 表存在性检查
 * - 事务：Begin/Commit/Rollback
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/db/db_config.hpp>

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_connect.hpp>
#endif

using namespace neforce;

int main() {
#ifdef NEFORCE_SUPPORT_SQLITE3
    sqlite_connect conn;
    db_config config = db_config::for_sqlite(":memory:");

    if (!conn.connect(config)) {
        eprintln("连接失败!");
        return 1;
    }
    println("已连接到 SQLite 内存数据库\n");

    // 建表
    conn.update("CREATE TABLE users ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT NOT NULL, "
                "age INTEGER, "
                "email TEXT, "
                "salary REAL)");
    println("users 表已创建");

    // 插入数据
    conn.update("INSERT INTO users (name, age, email, salary) VALUES ('Alice', 30, 'alice@test.com', 75000.5)");
    conn.update("INSERT INTO users (name, age, email, salary) VALUES ('Bob', 25, 'bob@test.com', 62000.0)");
    conn.update("INSERT INTO users (name, age, email, salary) VALUES ('Charlie', 35, 'charlie@test.com', 88000.0)");
    println("已插入 3 行数据\n");

    // 查询数据
    println("=== 查询所有用户 ===");
    auto result = conn.query("SELECT id, name, age, email, salary FROM users ORDER BY id");
    if (result != nullptr) {
        printfln("列数: {}, 列名: {}", result->column_count(), string::join(result->column_names(), ", "));
        while (result->next()) {
            printfln("  Row: id={}, name={}, age={}, email={}, salary={:.1f}", result->get(0), result->get(1),
                     result->get_int32(2), result->get(3), result->get_float64(4));
        }
    }

    // 更新数据
    println("\n=== 更新 Alice 的薪资 ===");
    conn.update("UPDATE users SET age = 31, salary = 80000.0 WHERE name = 'Alice'");
    auto result2 = conn.query("SELECT name, age, salary FROM users WHERE name = 'Alice'");
    if (result2 != nullptr && result2->next()) {
        printfln("Alice 更新后: age={}, salary={:.1f}", result2->get_int32(1), result2->get_float64(2));
    }

    // 删除数据
    println("\n=== 删除 Charlie ===");
    conn.update("DELETE FROM users WHERE name = 'Charlie'");
    auto result3 = conn.query("SELECT COUNT(*) AS cnt FROM users");
    if (result3 != nullptr && result3->next()) {
        printfln("删除后剩余行数: {}", result3->get_int32(0));
    }

    // table_exists
    println("\n=== table_exists ===");
    printfln("users 表存在: {}", conn.table_exists("users"));
    printfln("nonexistent 表存在: {}", conn.table_exists("nonexistent"));

    // 事务：Begin → Commit
    println("\n=== 事务：Commit ===");
    conn.begin();
    conn.update("INSERT INTO users (name, age, email) VALUES ('TxUser', 50, 'tx@test.com')");
    conn.commit();
    auto tx_result = conn.query("SELECT name FROM users WHERE name = 'TxUser'");
    if (tx_result != nullptr && tx_result->next()) {
        printfln("Commit 后查到: name={}", tx_result->get(0));
    }

    // 事务：Begin → Rollback
    println("\n=== 事务：Rollback ===");
    conn.begin();
    conn.update("INSERT INTO users (name, age, email) VALUES ('RbUser', 50, 'rb@test.com')");
    conn.rollback();
    auto rb_result = conn.query("SELECT name FROM users WHERE name = 'RbUser'");
    bool found = rb_result != nullptr && rb_result->next();
    printfln("Rollback 后查到: {} (预期 false)", found);

#else
    println("SQLite3 支持未启用");
#endif
    return 0;
}
