/**
 * @example connection_pool_example.cpp
 * @brief 数据库连接池示例
 *
 * 演示 database_pool 的连接池管理：
 * - 创建连接池与配置参数
 * - get_connect / get_tb_connect 获取连接
 * - RAII 自动归还连接
 * - 连接池状态查询
 * - 优雅停止
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/db/database_pool.hpp>
#include <NeForce/db/db_config.hpp>

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_connect.hpp>
#endif

using namespace neforce;

int main() {
#ifdef NEFORCE_SUPPORT_SQLITE3
    db_config config = db_config::for_sqlite(":memory:");

    // 配置连接池参数
    database_pool::pool_config pool_cfg;
    pool_cfg.min_size = 2;
    pool_cfg.init_size = 2;
    pool_cfg.max_size = 8;
    pool_cfg.max_idle_time = seconds{60};
    pool_cfg.acquire_timeout = milliseconds{5000};

    // 创建连接池
    database_pool pool{db_type::SQLITE3, config, pool_cfg};
    println("连接池已创建");
    printfln("  运行状态: {}", pool.is_running());
    printfln("  总连接数: {}", pool.total_count());
    printfln("  空闲连接数: {}\n", pool.idle_count());

    // 获取连接并使用
    println("=== 获取连接执行操作 ===");
    {
        auto conn = pool.get_tb_connect();
        if (conn != nullptr) {
            conn->update("CREATE TABLE pool_test (id INTEGER PRIMARY KEY, value TEXT)");
            conn->update("INSERT INTO pool_test (value) VALUES ('pool_example')");

            auto result = conn->query("SELECT value FROM pool_test");
            if (result != nullptr && result->next()) {
                printfln("查询结果: {}", result->get(0));
            }
        }
        // conn 离开作用域时自动归还到池中
    }
    println("连接已归还到池中\n");

    // 通过不同接口获取连接
    println("=== 同时获取多个连接 ===");
    {
        auto conn1 = pool.get_connect();    // 返回 idb_connect*
        auto conn2 = pool.get_tb_connect(); // 返回 idb_tb_connect*
        println("同时持有 2 个连接");
    }

    // 停止连接池
    pool.stop();
    println("\n连接池已停止");
    printfln("  运行状态: {}", pool.is_running());
    printfln("  总连接数: {}", pool.total_count());
#else
    println("SQLite3 支持未启用");
#endif
    return 0;
}
