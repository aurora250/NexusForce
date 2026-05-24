#include <NeForce/db/database_pool.hpp>
#include <NeForce/db/sql_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/time/click.hpp>
#include <NeForce/db/mysql/mysql_connect.hpp>
#include <NeForce/db/pgsql/pgsql_connect.hpp>
#include <NeForce/db/redis/redis_connect.hpp>
#include <NeForce/db/redis/redis_result.hpp>
using namespace neforce;

void test_mysql() {
#ifdef NEFORCE_SUPPORT_MYSQL
    db_config mysql_config = db_config::for_mysql("book");
    mysql_config.password = "147258hu";
    database_pool pool(db_type::MYSQL, mysql_config, database_pool::pool_config{5, 5, 64, seconds{2}});

    const auto sql = sql_builder().select({"ISBN", "BookName"}).from("book").where("CollectNumber = ?").build();
    auto pstmt = dynamic_pointer_cast<mysql_connect>(pool.get_tb_connect())->prepare_statement(sql);
    pstmt->bind_param(0, 10);
    auto res = pstmt->execute_query();
    if (res) {
        println(res->column_names());
        while (res->next()) {
            for (int i = 0; i < res->column_count(); ++i) {
                print(res->get(i), " ");
            }
            println();
        }
    }
#endif
}

void test_redis() {
#ifdef NEFORCE_SUPPORT_HIREDIS
    db_config redis_config = db_config::for_redis("0");
    database_pool pool(db_type::REDIS, redis_config, database_pool::pool_config{5, 5, 64, seconds{2}});

    auto conn = dynamic_pointer_cast<redis_connect>(pool.get_kv_connect());
    println(conn->is_valid());
    println(conn->update("SET age 20"));
    auto res = dynamic_pointer_cast<redis_result, default_deleter<redis_result>>(conn->get("age"));
    if (res) {
        println(res->empty());
        while (res->next()) {
            println(res->value());
        }
    }
#endif
}

void test_pgsql() {
#ifdef NEFORCE_SUPPORT_POSTGRESQL
    db_config postgre_config = db_config::for_postgresql();
    postgre_config.password = "483674";
    database_pool pool(db_type::POSTGRESQL, postgre_config, database_pool::pool_config{5, 5, 64, seconds{2}});

    const auto sql = sql_builder().select({"username", "email"}).from("dbuser").where_le("age", "$1").build();
    auto pstmt = dynamic_pointer_cast<pgsql_connect>(pool.get_tb_connect())->prepare_statement(sql);
    pstmt->bind_param(1, 30);
    auto res = pstmt->execute_query();
    if (res) {
        println(res->column_names());
        println(res->column_count());
        while (res->next()) {
            for (int i = 0; i < res->column_count(); ++i) {
                print(res->get(i), " ");
            }
            println();
        }
    }
#endif
}

void test_dbpool() {
#ifdef NEFORCE_SUPPORT_MYSQL
    db_config mysql_config = db_config::for_mysql("book");
    mysql_config.password = "147258hu";

    database_pool pool(db_type::MYSQL, mysql_config, database_pool::pool_config{20, 10, 64, seconds{2}});

    click c;
    c.start();

    {
        vector<thread> threads;
        threads.reserve(20);
        for (int t = 0; t < 20; t++) {
            threads.emplace_back([&pool] {
                for (int i = 0; i < 25; i++) {
                    auto conn = pool.get_connect();
                    ignore = conn->update("SELECT SLEEP(0.01)");
                }
            });
        }
        for (auto& t: threads) {
            t.join();
        }
    }

    c.stop();
    println("pool: ", c.during_s().count());

    c.start();

    {
        vector<thread> threads;
        for (int t = 0; t < 20; t++) {
            threads.emplace_back([&mysql_config] {
                for (int i = 0; i < 25; i++) {
                    auto* conn = new mysql_connect();
                    ignore = conn->connect(mysql_config);
                    ignore = conn->update("SELECT SLEEP(0.01)");
                    delete conn;
                }
            });
        }
        for (auto& t: threads) {
            t.join();
        }
    }

    c.stop();
    println("raw: ", c.during_s().count());
#endif
}
