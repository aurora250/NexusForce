/**
 * @example prepared_statement_example.cpp
 * @brief 预处理语句示例
 *
 * 演示 idb_prepared_statement 的参数化查询：
 * - 准备语句与参数绑定（字符串/整数/浮点）
 * - 执行型预处理语句（INSERT）
 * - 查询型预处理语句（SELECT + execute_query）
 * - 参数数量获取
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
    conn.update("CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT NOT NULL, age INTEGER, email TEXT)");

    // 准备 INSERT 语句
    auto stmt = conn.prepare_statement("INSERT INTO users (name, age, email) VALUES (?, ?, ?)");
    if (stmt == nullptr) {
        eprintln("预处理语句创建失败!");
        return 1;
    }
    printfln("参数数量: {}\n", stmt->param_count());

    // 绑定多种类型并执行
    const char* names[] = {"Alice", "Bob", "Charlie"};
    int32_t ages[] = {30, 25, 35};
    const char* emails[] = {"alice@test.com", "bob@test.com", "charlie@test.com"};

    for (int i = 0; i < 3; ++i) {
        stmt->bind_param(1, string(names[i]));
        stmt->bind_param(2, ages[i]);
        stmt->bind_param(3, string_view(emails[i]));
        if (stmt->execute()) {
            printfln("插入成功: {}", names[i]);
        }
    }

    // 查询型预处理语句
    println("\n=== 年龄 > 20 的用户 ===");
    auto query_stmt = conn.prepare_statement("SELECT name, age, email FROM users WHERE age > ? ORDER BY name");
    if (query_stmt != nullptr) {
        query_stmt->bind_param(1, int32_t(20));
        auto result = query_stmt->execute_query();
        if (result != nullptr) {
            while (result->next()) {
                printfln("  name={}, age={}, email={}", result->get(0), result->get_int32(1), result->get(2));
            }
        }
    }
#else
    println("SQLite3 支持未启用");
#endif
    return 0;
}
