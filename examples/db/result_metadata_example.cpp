/**
 * @example result_metadata_example.cpp
 * @brief 结果集元数据与类型安全访问示例
 *
 * 演示 idb_tb_result 的高级查询功能：
 * - 结果集元数据获取（列数、列名、列类型）
 * - 类型安全的 getter（get_int32 / get_float64 等）
 * - NULL 值检测
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
    conn.update("CREATE TABLE employees ("
                "id INTEGER PRIMARY KEY, "
                "name TEXT NOT NULL, "
                "age INTEGER, "
                "salary REAL, "
                "department TEXT)");
    conn.update("INSERT INTO employees VALUES (1, 'Alice', 30, 75000.0, 'Engineering')");

    auto result = conn.query("SELECT * FROM employees");
    if (result == nullptr) {
        eprintln("查询失败!");
        return 1;
    }

    // 结果集元数据
    printfln("列数: {}", result->column_count());
    println("列元数据:");

    const auto& names = result->column_names();
    for (size_t i = 0; i < names.size(); ++i) {
        auto meta = result->column_metadata(i);
        printfln("  列[{}]: name={}, type={}, max_length={}, nullable={}", i, meta.name, meta.type, meta.max_length,
                 meta.nullable);
    }

    // 类型安全的数据访问
    println("\n=== 类型安全访问 ===");
    if (result->next()) {
        printfln("  get(1)        → {}", result->get(1));          // 字符串
        printfln("  get_int32(2)  → {}", result->get_int32(2));    // 32位整数
        printfln("  get_float64(3) → {}", result->get_float64(3)); // 64位浮点
        printfln("  get(4)        → {}", result->get(4));          // 部门
    }
#else
    println("SQLite3 支持未启用");
#endif
    return 0;
}
