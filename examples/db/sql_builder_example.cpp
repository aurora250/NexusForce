/**
 * @example sql_builder_example.cpp
 * @brief SQL构建器示例
 *
 * 演示 sql_builder 的编程式SQL构建功能：
 * - SELECT/INSERT/UPDATE/DELETE 基本语句
 * - 聚合函数（COUNT/SUM/AVG/MAX/MIN）
 * - 分页查询
 * - 子查询
 * - 多表JOIN
 * - 复杂WHERE条件
 * - 方言感知占位符（Generic `?` / PG `$N` / Oracle `:N`）
 * - 方言感知建表（AUTO_INCREMENT 适配）
 */

#include <NeForce/core/system/console.hpp>
#include <NeForce/db/sql_builder.hpp>

using namespace neforce;

int main() {
    printcln(color::cyan(), "=== sql_builder 编程式SQL构建 ===\n");

    // SELECT 查询
    sql_builder sel;
    sel.select("id")
            .select("name")
            .select("email")
            .select_count("id", "total")
            .from("users", "u")
            .left_join("orders", "u.id = orders.user_id")
            .where_eq("u.status", "'active'")
            .where_gt("u.age", "18")
            .group_by(vector<string>{"u.id", "u.name"})
            .order_by_desc("total")
            .limit(10);
    printfln("SELECT: {}", sel.build());

    // INSERT 语句
    sql_builder ins;
    ins.insert_into("users", {"name", "email", "age"});
    printfln("INSERT: {}", ins.build());

    // UPDATE 语句
    sql_builder upd;
    upd.update("users").set("name = 'NewName'").set("age = 25").where_eq("id", "1");
    printfln("UPDATE: {}", upd.build());

    // DELETE 语句
    sql_builder del;
    del.delete_from("users").where_lt("age", "18");
    printfln("DELETE: {}", del.build());

    // 聚合函数
    sql_builder agg;
    agg.select_sum("amount", "total_amount")
            .select_avg("amount", "avg_amount")
            .select_max("amount", "max_amount")
            .select_min("amount", "min_amount")
            .from("orders");
    printfln("聚合:   {}", agg.build());

    // 分页
    sql_builder paged;
    paged.select_all().from("users").order_by_asc("id").page(3, 10);
    printfln("分页:   {}", paged.build());

    // 子查询
    sql_builder sub;
    sub.select_subquery("SELECT MAX(amount) FROM orders WHERE orders.user_id = users.id", "max_order").from("users");
    printfln("子查询: {}", sub.build());

    // 多表JOIN
    sql_builder joins;
    joins.select_all()
            .from("t1")
            .inner_join("t2", "t1.id = t2.id")
            .left_join("t3", "t2.id = t3.id")
            .right_join("t4", "t3.id = t4.id");
    printfln("多JOIN: {}", joins.build());

    // 复杂条件
    sql_builder cond;
    cond.select_all()
            .from("users")
            .where_eq("status", "'active'")
            .where_between("age", "18", "65")
            .where_like("name", "'%John%'")
            .where_is_not_null("email")
            .or_where("role = 'admin'");
    printfln("条件:   {}", cond.build());

    // 方言感知占位符
    printcln(color::cyan(), "\n=== 方言感知占位符 ===\n");

    // Generic / MySQL / SQLite 使用 ? 占位符
    {
        sql_builder b;
        b.set_dialect(sql_dialect::GENERIC);
        b.update("users");
        b.set_param("name");
        b.set_param("email");
        b.where_eq("id", b.next_placeholder());
        printfln("Generic: {} (param_count={})", b.build(), b.param_count());
    }

    // PostgreSQL 使用 $1, $2, $3 占位符
    {
        sql_builder b;
        b.set_dialect(sql_dialect::POSTGRESQL);
        b.update("users");
        b.set_param("name");
        b.set_param("email");
        b.where_eq("id", b.next_placeholder());
        printfln("PgSQL:   {} (param_count={})", b.build(), b.param_count());
    }

    // Oracle 使用 :1, :2, :3 占位符
    {
        sql_builder b;
        b.set_dialect(sql_dialect::ORACLE);
        b.update("users");
        b.set_param("name");
        b.set_param("email");
        b.where_eq("id", b.next_placeholder());
        printfln("Oracle:  {} (param_count={})", b.build(), b.param_count());
    }

    // INSERT 自动填充方言占位符
    {
        sql_builder b;
        b.set_dialect(sql_dialect::POSTGRESQL);
        b.insert_into("users", {"name", "email", "age"});
        printfln("PgSQL INSERT: {} (param_count={})", b.build(), b.param_count());
    }

    // 删除语句 + dialect
    {
        sql_builder b;
        b.set_dialect(sql_dialect::POSTGRESQL);
        b.delete_from("users").where_eq("id", b.placeholder(1));
        printfln("PgSQL DELETE: {}", b.build());
    }

    // 方言感知建表
    printcln(color::cyan(), "\n=== 方言感知建表（AUTO_INCREMENT适配）===\n");

    auto print_ddl = [](sql_dialect dialect, const char* label) {
        sql_builder b;
        b.set_dialect(dialect);
        b.create_table("products").column_auto_increment("id", "INTEGER").column("name", "TEXT");
        printfln("{}:\n{}\n", label, b.build());
    };

    print_ddl(sql_dialect::GENERIC, "Generic (AUTO_INCREMENT)");
    print_ddl(sql_dialect::POSTGRESQL, "PgSQL    (SERIAL)");
    print_ddl(sql_dialect::MSSQL, "MSSQL    (IDENTITY)");
    print_ddl(sql_dialect::ORACLE, "Oracle   (GENERATED AS IDENTITY)");

    // 方言感知分页
    printcln(color::cyan(), "=== 方言感知分页 ===\n");

    {
        sql_builder b;
        b.set_dialect(sql_dialect::GENERIC);
        b.select_all().from("users").limit(10).offset(20);
        printfln("Generic: {} (LIMIT/OFFSET)", b.build());
    }
    {
        sql_builder b;
        b.set_dialect(sql_dialect::ORACLE);
        b.select_all().from("users").limit(10).offset(20);
        printfln("Oracle:  {} (FETCH FIRST)", b.build());
    }
    {
        sql_builder b;
        b.set_dialect(sql_dialect::ORACLE);
        b.select_all().from("users").page(3, 10);
        printfln("Oracle page(3,10): {}", b.build());
    }

    return 0;
}
