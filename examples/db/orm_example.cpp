/**
 * @example orm_example.cpp
 * @brief ORM 对象-关系映射示例
 *
 * 演示 sql_mapper + repository 的反射驱动 ORM 功能：
 * - 实体定义与反射注册（含 DB 注解）
 * - sql_mapper 自动生成 DDL/DML SQL
 * - 方言感知占位符（Generic/PG/Oracle）
 * - repository 泛型 CRUD 操作
 * - 分页查询与聚合
 */

#include <NeForce/core/reflect/reflect.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/db/db_config.hpp>
#include <NeForce/db/sql_builder.hpp>
#include <NeForce/db/sql_mapper.hpp>
#include <NeForce/db/repository.hpp>

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_connect.hpp>
#endif

using namespace neforce;
using namespace neforce::reflect;

// 实体定义（实际项目中配合 NFRS 使用宏标记即可自动生成注册代码）
struct User {
    int id = 0;
    string name;
    string email;
    int age = 0;
};

int main() {
    // 注册反射元数据（生产项目中由 NFRS 根据 NEFORCE_REFLECT_OBJ 宏自动生成）
    reflect::reflect<User>("User")
            .table_name("tbl_users")
            .property("id", &User::id, PROP_PRIMARY_KEY | PROP_AUTO_INC)
            .property("name", &User::name, PROP_REQUIRED)
            .property("email", &User::email, PROP_UNIQUE)
            .property("age", &User::age)
            .constructor();
    NEFORCE_REFLECT_RESOLVE_BASES();

    printcln(color::cyan(), "=== sql_mapper 自动生成 SQL ===\n");

    // DDL: 建表 / 删表
    printfln("CREATE TABLE:\n{}\n", sql_mapper<User>::create_table_sql());
    printfln("DROP TABLE:\n{}\n", sql_mapper<User>::drop_table_sql());

    // DML: INSERT 方言差异
    printcln(color::yellow(), "--- INSERT 方言感知占位符 ---");
    printfln("Generic:  {}", sql_mapper<User>::insert_sql());
    printfln("PgSQL:    {}", sql_mapper<User>::insert_sql("", sql_dialect::POSTGRESQL));
    printfln("Oracle:   {}", sql_mapper<User>::insert_sql("", sql_dialect::ORACLE));

    // DML: UPDATE / DELETE
    printcln(color::yellow(), "\n--- UPDATE / DELETE ---");
    printfln("UPDATE: {}", sql_mapper<User>::update_sql());
    printfln("DELETE: {}", sql_mapper<User>::delete_sql());

    // SELECT
    printcln(color::yellow(), "\n--- SELECT ---");
    printfln("全部字段: {}", sql_mapper<User>::select_sql());
    printfln("按主键查: {}", sql_mapper<User>::select_by_pk_sql());

    printcln(color::cyan(), "\n=== repository 泛型 CRUD ===\n");

#ifdef NEFORCE_SUPPORT_SQLITE3
    sqlite_connect conn;
    conn.connect(db_config::for_sqlite(":memory:"));

    repository<User, idb_tb_connect> repo{conn};

    // 建表
    if (!repo.create_table()) {
        eprintln("建表失败!");
        return 1;
    }
    println("表已创建");

    // 插入
    User u1;
    u1.name = "Alice";
    u1.email = "alice@test.com";
    u1.age = 30;
    repo.insert(u1);

    User u2;
    u2.name = "Bob";
    u2.email = "bob@test.com";
    u2.age = 25;
    repo.insert(u2);

    User u3;
    u3.name = "Charlie";
    u3.email = "charlie@test.com";
    u3.age = 35;
    repo.insert(u3);
    printfln("已插入 {} 条记录\n", repo.count());

    // 查询全部
    println("=== 全部用户 ===");
    for (auto& user: repo.find_all()) {
        printfln("  id={}, name={}, email={}, age={}", user.id, user.name, user.email, user.age);
    }

    // 条件查询
    println("\n=== 年龄 > 25 ===");
    for (auto& user: repo.find_where("age > 25")) {
        printfln("  id={}, name={}, age={}", user.id, user.name, user.age);
    }

    // 更新
    println("\n=== 更新 Alice ===");
    u1.age = 31;
    repo.update(u1);
    printfln("Alice 更新后: age={}", u1.age);

    // 分页
    println("\n=== 分页查询（第1页，每页2条）===");
    for (auto& user: repo.find_page(1, 2, "id")) {
        printfln("  id={}, name={}", user.id, user.name);
    }

    println("\n=== 分页查询（第2页，每页2条）===");
    for (auto& user: repo.find_page(2, 2, "id")) {
        printfln("  id={}, name={}", user.id, user.name);
    }

    // 表存在性检查
    printfln("\ntable_exists: {}", repo.table_exists());

    // 删除
    println("\n=== 删除 Alice ===");
    repo.remove(u1);
    printfln("剩余记录数: {}", repo.count());

    // 删表
    repo.drop_table();
    printfln("删表后 table_exists: {}", repo.table_exists());

#else
    println("SQLite3 支持未启用，跳过 repository 实际操作演示");
    println("上述 sql_mapper SQL 生成结果不依赖数据库连接");
#endif
    return 0;
}
