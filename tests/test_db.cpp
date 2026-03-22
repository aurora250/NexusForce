#include "test.h"

void test_sql(){
    auto sql1 = sql_builder()
        .select({"id", "name", "email"})
        .from("users")
        .where_eq("status", "'active'")
        .where_gt("age", "18")
        .order_by_desc("created_at")
        .limit(10)
        .build();
    println(sql1);

    auto sql2 = sql_builder()
        .select({"u.name", "u.email", "o.order_no", "o.amount"})
        .from("users", "u")
        .left_join("orders o", "u.id = o.user_id")
        .where_ge("o.amount", "100")
        .order_by_desc("o.created_at")
        .build();
    println(sql2);

    auto sql3 = sql_builder()
        .select_count("*", "total")
        .select_sum("amount", "total_amount")
        .select_avg("amount", "avg_amount")
        .from("orders")
        .group_by("user_id")
        .having("SUM(amount) > 1000")
        .build();
    println(sql3);

    auto sql4 = sql_builder()
        .select_all()
        .from("products")
        .where_like("name", "'%phone%'")
        .where_between("price", "100", "500")
        .page(2, 20)
        .build();
    println(sql4);

    auto sql5 = sql_builder()
        .insert_into("users", {"name", "email", "age"})
        .values({"'John'", "'john@example.com'", "25"})
        .build();
    println(sql5);

    auto sql6 = sql_builder()
        .update("users")
        .set("status", "'inactive'")
        .set_increment("login_count")
        .where_eq("id", "123")
        .build();
    println(sql6);

    auto sql7 = sql_builder()
        .delete_from("users")
        .where_eq("status", "'deleted'")
        .where_lt("last_login", "'2020-01-01'")
        .build();
    println(sql7);

    auto sql8 = sql_builder()
        .select({"category", "COUNT(*) as cnt", "AVG(price) as avg_price"})
        .from("products")
        .where_in("status", {"'active'", "'pending'"})
        .where_is_not_null("description")
        .group_by("category")
        .having("COUNT(*) > 10")
        .order_by_asc("category")
        .build();
    println(sql8);

    auto sql9 = sql_builder()
        .select({"u.name", "total_orders"})
        .from_subquery(
            "SELECT user_id, COUNT(*) as total_orders FROM orders GROUP BY user_id",
            "o"
        )
        .inner_join("users u", "u.id = o.user_id")
        .where_gt("o.total_orders", "5")
        .build();
    println(sql9);

    auto sql10 = sql_builder()
        .distinct()
        .select({"city", "country"})
        .from("users")
        .order_by_asc("country")
        .build();
    println(sql10);

    sql_builder builder;
    builder.select({"id", "name"})
           .from("users");

    auto active_users = builder.where_eq("status", "'active'").build();
    builder.reset();
}

void test_mysql() {
#ifdef NEFORCE_SUPPORT_MYSQL
    db_config mysql_config = db_config::for_mysql("book");
    mysql_config.password = "147258hu";
    database_pool pool(db_type::MYSQL, mysql_config, 10, 20, seconds{2});

    const auto sql = sql_builder()
        .select({"ISBN", "BookName"})
        .from("book")
        .where("CollectNumber = ?")
        .build();
    auto pstmt = dynamic_pointer_cast<mysql_connect>(
        pool.get_tb_connect())->prepare_statement(sql);
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
    database_pool pool(db_type::REDIS, redis_config, 10, 20, seconds{2});
    auto conn = dynamic_pointer_cast<redis_connect>(pool.get_kv_connect());
    println(conn->is_valid());
    println(conn->update("SET age 20"));
    auto res = dynamic_pointer_cast<redis_result>(conn->get("age"));
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
    database_pool pool(db_type::POSTGRESQL, postgre_config, 10, 20, seconds{2});

    const auto sql = sql_builder()
        .select({"username", "email"})
        .from("dbuser")
        .where_le("age", "$1")
        .build();
    auto pstmt = dynamic_pointer_cast<pgsql_connect>(
        pool.get_tb_connect())->prepare_statement(sql);
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
    auto begin = timestamp::now();
    db_config mysql_config = db_config::for_mysql("book");
    mysql_config.password = "147258hu";

    {
        database_pool pool(db_type::MYSQL, mysql_config);
        for (int i = 0; i < 5000; i++) {
            bool fin = pool.get_connect()->update("SELECT 1");
        }
        println(timestamp::now() - begin);

        auto result = pool.get_tb_connect()->query("SELECT * FROM book");
        while (result->next()) {
            for (int i = 0; i < result->column_count(); i++) {
                if (i == 2) {
                    int count = result->get_int16(i);
                    print("collected :", count, ", ");
                } else if (i == 3) {
                    float count = result->get_float32(i);
                    print("usable :", count, ", ");
                } else if (i == 5) {
                    _NEFORCE datetime dt = result->get_datetime(i);
                    print("date: ", dt, ", ");
                } else {
                    print(result->get(i), ", ");
                }
            }
            println();
        }
        println(result->row_count(), ", ", result->column_count());
    }

    begin = timestamp::now();
    for (int i = 0; i < 5000; i++) {
        char sql[power(2, 10)] = {};
        _NEFORCE sprintf(sql, "SELECT 1");
        auto* conn = new mysql_connect();
        if(conn->connect(mysql_config)) {
            (void) conn->update(sql);
        }
        delete conn;
    }
    println(timestamp::now() - begin);
#endif
}
