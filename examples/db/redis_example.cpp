/**
 * @example redis_example.cpp
 * @brief Redis键值操作示例
 *
 * 演示 redis_connect 的 KV 操作：
 * - SET/GET/EXISTS/DEL 基本操作
 * - 带过期时间的 SETEX
 * - Hash 操作（HSET/HGETALL）
 * - List 操作（LPUSH/LRANGE）
 * - Set 操作（SADD/SMEMBERS）
 * - Redis 事务（MULTI/EXEC）
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/db/db_config.hpp>

#ifdef NEFORCE_SUPPORT_HIREDIS
#    include <NeForce/db/redis/redis_connect.hpp>
#endif

using namespace neforce;

int main() {
#ifdef NEFORCE_SUPPORT_HIREDIS
    db_config config = db_config::for_redis("0");
    config.password = "483674";
    redis_connect conn;

    if (!conn.connect(config)) {
        println("Redis 服务不可用");
        return 0;
    }
    conn.update("FLUSHDB");

    // 基本 SET/GET
    println("=== SET/GET ===");
    conn.set("greeting", "Hello from NexusForce!");
    auto result = conn.get("greeting");
    if (result != nullptr && result->next()) {
        printfln("GET greeting → {}", result->value());
    }

    // SETEX 带过期时间
    conn.setex("temp_key", "expires_in_600s", 600);
    println("SETEX temp_key (600s TTL)");

    // EXISTS / DEL
    println("\n=== EXISTS / DEL ===");
    conn.set("del_test", "value");
    printfln("EXISTS del_test: {}", conn.exists("del_test"));
    conn.del("del_test");
    printfln("EXISTS del_test (after DEL): {}", conn.exists("del_test"));

    // Hash 操作
    println("\n=== Hash 操作 ===");
    conn.hset("user:1", "name", "Alice");
    conn.hset("user:1", "age", "30");
    conn.hset("user:1", "email", "alice@test.com");
    auto hgetall = conn.hgetall("user:1");
    if (hgetall != nullptr) {
        println("HGETALL user:1:");
        while (hgetall->next()) {
            printfln("  {} → {}", hgetall->key(), hgetall->value());
        }
    }

    // List 操作
    println("\n=== List 操作 ===");
    conn.lpush("tasks", "task3");
    conn.lpush("tasks", "task2");
    conn.lpush("tasks", "task1");
    auto lrange = conn.lrange("tasks", 0, -1);
    if (lrange != nullptr) {
        auto arr = lrange->value_array();
        println("LRANGE tasks 0 -1:");
        for (size_t i = 0; i < arr.size(); ++i) {
            printfln("  [{}] {}", i, arr[i]);
        }
    }

    // Set 操作
    println("\n=== Set 操作 ===");
    conn.sadd("tags", "cpp");
    conn.sadd("tags", "database");
    conn.sadd("tags", "network");
    auto smembers = conn.smembers("tags");
    if (smembers != nullptr) {
        auto arr = smembers->value_array();
        println("SMEMBERS tags:");
        for (size_t i = 0; i < arr.size(); ++i) {
            printfln("  - {}", arr[i]);
        }
    }

    // Redis 事务 (MULTI/EXEC)
    println("\n=== Redis 事务 ===");
    conn.begin();
    conn.set("tx_key", "tx_value");
    conn.set("tx_counter", "1");
    conn.commit();

    auto query_result = conn.query("GET tx_key");
    if (query_result != nullptr && query_result->next()) {
        printfln("GET tx_key → {}", query_result->value());
    }
#else
    println("Hiredis 支持未启用");
#endif
    return 0;
}
