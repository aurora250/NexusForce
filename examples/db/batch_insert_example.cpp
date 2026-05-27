/**
 * @example batch_insert_example.cpp
 * @brief 批量插入示例
 *
 * 演示 batch_insert 的高效批量数据写入：
 * - 单次调用插入多行数据
 * - 自动构建参数化 INSERT 语句
 * - 边界情况（空数据、空列）
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
    conn.connect(db_config::for_sqlite(":memory:"));
    conn.update("CREATE TABLE products (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT NOT NULL, price TEXT, category TEXT)");

    // 批量插入
    println("=== 批量插入 ===");
    size_t count = conn.batch_insert("products", {"name", "price", "category"},
                                     {{"Laptop", "5999.00", "Electronics"},
                                      {"Mouse", "299.00", "Electronics"},
                                      {"Keyboard", "899.00", "Electronics"},
                                      {"Monitor", "2999.00", "Electronics"},
                                      {"Desk", "1599.00", "Furniture"}});
    printfln("批量插入了 {} 行\n", count);

    // 验证
    println("=== 查询结果 ===");
    auto result = conn.query("SELECT name, price, category FROM products ORDER BY id");
    while (result != nullptr && result->next()) {
        printfln("  {} | {} | {}", result->get(0), result->get(1), result->get(2));
    }

    // 边界情况
    println("\n=== 边界情况 ===");
    size_t zero = conn.batch_insert("products", {"name"}, {});
    printfln("空数据返回: {} (预期 0)", zero);

#else
    println("SQLite3 支持未启用");
#endif
    return 0;
}
