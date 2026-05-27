/**
 * @example db_config_example.cpp
 * @brief 数据库连接配置示例
 *
 * 演示 db_config 的多后端配置创建：
 * - SQLite / MySQL / PostgreSQL / Redis 各后端配置
 * - 配置复制与赋值
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/db/db_config.hpp>

using namespace neforce;

int main() {
    printcln(color::cyan(), "=== db_config 数据库连接配置 ===\n");

    // SQLite 配置
    db_config sqlite_cfg = db_config::for_sqlite("example.db");
    printfln("SQLite: database={}", sqlite_cfg.database);

    // MySQL 配置
    db_config mysql_cfg = db_config::for_mysql("mydb");
    mysql_cfg.host = "192.168.1.100";
    mysql_cfg.username = "admin";
    mysql_cfg.password = "secret";
    mysql_cfg.charset = "utf8mb4";
    printfln("MySQL: host={}, database={}, user={}, charset={}", mysql_cfg.host, mysql_cfg.database, mysql_cfg.username,
             mysql_cfg.charset);

    // PostgreSQL 配置
    db_config pg_cfg = db_config::for_postgresql("mydb");
    pg_cfg.host = "192.168.1.100";
    pg_cfg.username = "postgres";
    pg_cfg.password = "secret";
    printfln("PgSQL: host={}, database={}, user={}", pg_cfg.host, pg_cfg.database, pg_cfg.username);

    // Redis 配置
    db_config redis_cfg = db_config::for_redis("0");
    redis_cfg.host = "192.168.1.100";
    redis_cfg.password = "secret";
    printfln("Redis: host={}, database={}", redis_cfg.host, redis_cfg.database);

    // 配置复制
    db_config copy_cfg{sqlite_cfg};
    printfln("复制: database={}", copy_cfg.database);

    // 配置赋值
    db_config assign_cfg;
    assign_cfg = sqlite_cfg;
    printfln("赋值: database={}", assign_cfg.database);

    return 0;
}
