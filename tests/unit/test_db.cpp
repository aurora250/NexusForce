#include <NeForce/db/db_config.hpp>
#include <NeForce/db/db_interface.hpp>
#include <NeForce/db/sql_builder.hpp>
#include <NeForce/db/sql_connect_base.hpp>
#include <NeForce/db/transaction_guard.hpp>
#include <gtest/gtest.h>
using namespace neforce;

class SqlBuilderTest : public ::testing::Test {
protected:
    sql_builder builder;

    void SetUp() override { builder.reset(); }
};

TEST_F(SqlBuilderTest, DefaultConstructor) {
    sql_builder b;
    EXPECT_EQ(b.type(), sql_operate::SELECT);
    EXPECT_TRUE(b.is_empty());
}

TEST_F(SqlBuilderTest, Destructor) {
    {
        sql_builder b;
        b.from("test");
    }
    SUCCEED();
}

TEST_F(SqlBuilderTest, CopyConstructor) {
    builder.select("id").from("users");
    sql_builder copy(builder);
    EXPECT_EQ(copy.build(), "SELECT id FROM users;");
}

TEST_F(SqlBuilderTest, CopyAssignment) {
    builder.select("id").from("users");
    sql_builder copy;
    copy = builder;
    EXPECT_EQ(copy.build(), "SELECT id FROM users;");
}

TEST_F(SqlBuilderTest, CopyAssignmentSelf) {
    builder.select("id").from("users");
    builder = builder;
    EXPECT_EQ(builder.build(), "SELECT id FROM users;");
}

TEST_F(SqlBuilderTest, MoveConstructor) {
    builder.select("id").from("users");
    sql_builder moved(move(builder));
    EXPECT_EQ(moved.build(), "SELECT id FROM users;");
}

TEST_F(SqlBuilderTest, MoveAssignment) {
    builder.select("id").from("users");
    sql_builder moved;
    moved = move(builder);
    EXPECT_EQ(moved.build(), "SELECT id FROM users;");
}

TEST_F(SqlBuilderTest, SelectVectorFields) {
    vector<string> fields = {"id", "name", "email"};
    builder.select(fields).from("users");
    EXPECT_EQ(builder.build(), "SELECT id, name, email FROM users;");
}

TEST_F(SqlBuilderTest, SelectInitializerList) {
    builder.select({"id", "name", "email"}).from("users");
    EXPECT_EQ(builder.build(), "SELECT id, name, email FROM users;");
}

TEST_F(SqlBuilderTest, SelectSingleField) {
    builder.select("id").from("users");
    EXPECT_EQ(builder.build(), "SELECT id FROM users;");
}

TEST_F(SqlBuilderTest, SelectMultipleFieldsChained) {
    builder.select("id").select("name").select("email").from("users");
    EXPECT_EQ(builder.build(), "SELECT id, name, email FROM users;");
}

TEST_F(SqlBuilderTest, SelectAll) {
    builder.select_all().from("users");
    EXPECT_EQ(builder.build(), "SELECT * FROM users;");
}

TEST_F(SqlBuilderTest, SelectAllWithPreviousFields) {
    builder.select("id").from("users");
    builder.select_all().from("users");
    EXPECT_EQ(builder.build(), "SELECT * FROM users;");
}

TEST_F(SqlBuilderTest, Distinct) {
    builder.select("name").distinct().from("users");
    EXPECT_EQ(builder.build(), "SELECT DISTINCT name FROM users;");
}

TEST_F(SqlBuilderTest, FromTable) {
    builder.select_all().from("users");
    EXPECT_EQ(builder.build(), "SELECT * FROM users;");
    EXPECT_EQ(builder.table(), "users");
}

TEST_F(SqlBuilderTest, FromTableWithAlias) {
    builder.select_all().from("users", "u");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u;");
}

TEST_F(SqlBuilderTest, JoinEnumType) {
    builder.select_all().from("users", "u").join(sql_join::LEFT, "orders", "u.id = orders.user_id");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u LEFT JOIN orders ON u.id = orders.user_id;");
}

TEST_F(SqlBuilderTest, JoinStringShorthand) {
    builder.select_all().from("users", "u").join("orders", "u.id = orders.user_id");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u INNER JOIN orders ON u.id = orders.user_id;");
}

TEST_F(SqlBuilderTest, LeftJoin) {
    builder.select_all().from("users", "u").left_join("orders", "u.id = orders.user_id");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u LEFT JOIN orders ON u.id = orders.user_id;");
}

TEST_F(SqlBuilderTest, RightJoin) {
    builder.select_all().from("users", "u").right_join("orders", "u.id = orders.user_id");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u RIGHT JOIN orders ON u.id = orders.user_id;");
}

TEST_F(SqlBuilderTest, InnerJoin) {
    builder.select_all().from("users", "u").inner_join("orders", "u.id = orders.user_id");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u INNER JOIN orders ON u.id = orders.user_id;");
}

TEST_F(SqlBuilderTest, FullJoin) {
    builder.select_all().from("users", "u").full_join("orders", "u.id = orders.user_id");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u FULL JOIN orders ON u.id = orders.user_id;");
}

TEST_F(SqlBuilderTest, MultipleJoins) {
    builder.select_all()
            .from("users", "u")
            .inner_join("orders", "u.id = orders.user_id")
            .left_join("products", "orders.product_id = products.id");
    EXPECT_EQ(builder.build(), "SELECT * FROM users u "
                               "INNER JOIN orders ON u.id = orders.user_id "
                               "LEFT JOIN products ON orders.product_id = products.id;");
}

TEST_F(SqlBuilderTest, WhereCondition) {
    builder.select_all().from("users").where("id > 10");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE id > 10;");
}

TEST_F(SqlBuilderTest, WhereMultipleConditions) {
    builder.select_all().from("users").where("id > 10").where("name IS NOT NULL");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE id > 10 AND name IS NOT NULL;");
}

TEST_F(SqlBuilderTest, WhereEq) {
    builder.select_all().from("users").where_eq("name", "'John'");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE name = 'John';");
}

TEST_F(SqlBuilderTest, WhereNe) {
    builder.select_all().from("users").where_ne("status", "'inactive'");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE status != 'inactive';");
}

TEST_F(SqlBuilderTest, WhereGt) {
    builder.select_all().from("users").where_gt("age", "18");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE age > 18;");
}

TEST_F(SqlBuilderTest, WhereGe) {
    builder.select_all().from("users").where_ge("age", "18");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE age >= 18;");
}

TEST_F(SqlBuilderTest, WhereLt) {
    builder.select_all().from("users").where_lt("age", "65");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE age < 65;");
}

TEST_F(SqlBuilderTest, WhereLe) {
    builder.select_all().from("users").where_le("age", "65");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE age <= 65;");
}

TEST_F(SqlBuilderTest, WhereLike) {
    builder.select_all().from("users").where_like("name", "'%John%'");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE name LIKE '%John%';");
}

TEST_F(SqlBuilderTest, WhereNotLike) {
    builder.select_all().from("users").where_not_like("name", "'%John%'");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE name NOT LIKE '%John%';");
}

TEST_F(SqlBuilderTest, WhereIn) {
    vector<string> values = {"'a'", "'b'", "'c'"};
    builder.select_all().from("users").where_in("status", values);
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE status IN ('a', 'b', 'c');");
}

TEST_F(SqlBuilderTest, WhereInSingleValue) {
    vector<string> values = {"'active'"};
    builder.select_all().from("users").where_in("status", values);
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE status IN ('active');");
}

TEST_F(SqlBuilderTest, WhereInEmpty) {
    vector<string> values;
    builder.select_all().from("users").where_in("status", values);
    EXPECT_EQ(builder.build(), "SELECT * FROM users;");
}

TEST_F(SqlBuilderTest, WhereNotIn) {
    vector<string> values = {"'deleted'", "'banned'"};
    builder.select_all().from("users").where_not_in("status", values);
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE status NOT IN ('deleted', 'banned');");
}

TEST_F(SqlBuilderTest, WhereNotInEmpty) {
    vector<string> values;
    builder.select_all().from("users").where_not_in("status", values);
    EXPECT_EQ(builder.build(), "SELECT * FROM users;");
}

TEST_F(SqlBuilderTest, WhereBetween) {
    builder.select_all().from("users").where_between("age", "18", "65");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE age BETWEEN 18 AND 65;");
}

TEST_F(SqlBuilderTest, WhereNotBetween) {
    builder.select_all().from("users").where_not_between("age", "0", "17");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE age NOT BETWEEN 0 AND 17;");
}

TEST_F(SqlBuilderTest, WhereIsNull) {
    builder.select_all().from("users").where_is_null("deleted_at");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE deleted_at IS NULL;");
}

TEST_F(SqlBuilderTest, WhereIsNotNull) {
    builder.select_all().from("users").where_is_not_null("email");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE email IS NOT NULL;");
}

TEST_F(SqlBuilderTest, WhereExists) {
    builder.select_all().from("users").where_exists("SELECT 1 FROM orders WHERE orders.user_id = users.id");
    EXPECT_EQ(builder.build(),
              "SELECT * FROM users WHERE EXISTS (SELECT 1 FROM orders WHERE orders.user_id = users.id);");
}

TEST_F(SqlBuilderTest, WhereNotExists) {
    builder.select_all().from("users").where_not_exists("SELECT 1 FROM orders WHERE orders.user_id = users.id");
    EXPECT_EQ(builder.build(),
              "SELECT * FROM users WHERE NOT EXISTS (SELECT 1 FROM orders WHERE orders.user_id = users.id);");
}

TEST_F(SqlBuilderTest, OrWhere) {
    builder.select_all().from("users").where_eq("status", "'active'").or_where("role = 'admin'");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE (status = 'active' OR role = 'admin');");
}

TEST_F(SqlBuilderTest, OrWhereWithoutPreviousCondition) {
    builder.select_all().from("users").or_where("status = 'active'");
    EXPECT_EQ(builder.build(), "SELECT * FROM users WHERE status = 'active';");
}

TEST_F(SqlBuilderTest, GroupBySingleField) {
    builder.select("status").select_count().from("users").group_by("status");
    EXPECT_EQ(builder.build(), "SELECT status, COUNT(*) FROM users GROUP BY status;");
}

TEST_F(SqlBuilderTest, GroupByMultipleFieldsChained) {
    builder.select("country").select("city").select_count().from("users").group_by("country").group_by("city");
    EXPECT_EQ(builder.build(), "SELECT country, city, COUNT(*) FROM users GROUP BY country, city;");
}

TEST_F(SqlBuilderTest, GroupByVector) {
    vector<string> fields = {"country", "city"};
    builder.select("country").select("city").select_count().from("users").group_by(fields);
    EXPECT_EQ(builder.build(), "SELECT country, city, COUNT(*) FROM users GROUP BY country, city;");
}

TEST_F(SqlBuilderTest, Having) {
    builder.select("status").select_count().from("users").group_by("status").having("COUNT(*) > 5");
    EXPECT_EQ(builder.build(), "SELECT status, COUNT(*) FROM users GROUP BY status HAVING COUNT(*) > 5;");
}

TEST_F(SqlBuilderTest, HavingMultipleConditions) {
    builder.select("status")
            .select_count()
            .select_avg("age")
            .from("users")
            .group_by("status")
            .having("COUNT(*) > 5")
            .having("AVG(age) > 30");
    EXPECT_EQ(builder.build(), "SELECT status, COUNT(*), AVG(age) FROM users "
                               "GROUP BY status HAVING COUNT(*) > 5 AND AVG(age) > 30;");
}

TEST_F(SqlBuilderTest, OrderByAsc) {
    builder.select_all().from("users").order_by("name", sql_order::ASC);
    EXPECT_EQ(builder.build(), "SELECT * FROM users ORDER BY name ASC;");
}

TEST_F(SqlBuilderTest, OrderByDesc) {
    builder.select_all().from("users").order_by("name", sql_order::DESC);
    EXPECT_EQ(builder.build(), "SELECT * FROM users ORDER BY name DESC;");
}

TEST_F(SqlBuilderTest, OrderByAscShorthand) {
    builder.select_all().from("users").order_by_asc("name");
    EXPECT_EQ(builder.build(), "SELECT * FROM users ORDER BY name ASC;");
}

TEST_F(SqlBuilderTest, OrderByDescShorthand) {
    builder.select_all().from("users").order_by_desc("name");
    EXPECT_EQ(builder.build(), "SELECT * FROM users ORDER BY name DESC;");
}

TEST_F(SqlBuilderTest, OrderByMultipleFields) {
    builder.select_all().from("users").order_by_asc("last_name").order_by_desc("first_name");
    EXPECT_EQ(builder.build(), "SELECT * FROM users ORDER BY last_name ASC, first_name DESC;");
}

TEST_F(SqlBuilderTest, OrderByDefaultAsc) {
    builder.select_all().from("users").order_by("name");
    EXPECT_EQ(builder.build(), "SELECT * FROM users ORDER BY name ASC;");
}

TEST_F(SqlBuilderTest, Limit) {
    builder.select_all().from("users").limit(10);
    EXPECT_EQ(builder.build(), "SELECT * FROM users LIMIT 10;");
}

TEST_F(SqlBuilderTest, LimitZero) {
    builder.select_all().from("users").limit(0);
    EXPECT_EQ(builder.build(), "SELECT * FROM users;");
}

TEST_F(SqlBuilderTest, Offset) {
    builder.select_all().from("users").offset(20);
    EXPECT_EQ(builder.build(), "SELECT * FROM users OFFSET 20;");
}

TEST_F(SqlBuilderTest, OffsetZero) {
    builder.select_all().from("users").offset(0);
    EXPECT_EQ(builder.build(), "SELECT * FROM users;");
}

TEST_F(SqlBuilderTest, LimitAndOffset) {
    builder.select_all().from("users").limit(10).offset(20);
    EXPECT_EQ(builder.build(), "SELECT * FROM users LIMIT 10 OFFSET 20;");
}

TEST_F(SqlBuilderTest, Page) {
    builder.select_all().from("users").page(3, 10);
    EXPECT_EQ(builder.build(), "SELECT * FROM users LIMIT 10 OFFSET 20;");
}

TEST_F(SqlBuilderTest, PageFirstPage) {
    builder.select_all().from("users").page(1, 10);
    EXPECT_EQ(builder.build(), "SELECT * FROM users LIMIT 10;");
}

TEST_F(SqlBuilderTest, InsertIntoWithFields) {
    vector<string> fields = {"name", "email"};
    builder.insert_into("users", fields);
    EXPECT_EQ(builder.build(), "INSERT INTO users (name, email) VALUES (?, ?);");
}

TEST_F(SqlBuilderTest, InsertIntoTableOnly) {
    builder.insert_into("users");
    EXPECT_EQ(builder.type(), sql_operate::INSERT);
}

TEST_F(SqlBuilderTest, InsertWithColumnsAndValues) {
    builder.insert_into("users").columns({"name", "email", "age"}).values({"?", "?", "18"});
    EXPECT_EQ(builder.build(), "INSERT INTO users (name, email, age) VALUES (?, ?, 18);");
}

TEST_F(SqlBuilderTest, InsertColumnsAutoPlaceholders) {
    builder.insert_into("users").columns({"name", "email"});
    EXPECT_EQ(builder.build(), "INSERT INTO users (name, email) VALUES (?, ?);");
}

TEST_F(SqlBuilderTest, InsertValues) {
    builder.insert_into("users", {"name", "email"});
    builder.values({"'John'", "'john@test.com'"});
    EXPECT_EQ(builder.build(), "INSERT INTO users (name, email) VALUES ('John', 'john@test.com');");
}

TEST_F(SqlBuilderTest, InsertThrowsWithoutFields) {
    builder.insert_into("users");
    EXPECT_THROW(ignore = builder.build(), value_exception);
}

TEST_F(SqlBuilderTest, Update) {
    builder.update("users").set("name = 'John'").where_eq("id", "1");
    EXPECT_EQ(builder.build(), "UPDATE users SET name = 'John' WHERE id = 1;");
}

TEST_F(SqlBuilderTest, UpdateMultipleSets) {
    builder.update("users").set("name = 'John'").set("email", "'john@test.com'").where_eq("id", "1");
    EXPECT_EQ(builder.build(), "UPDATE users SET name = 'John', email = 'john@test.com' WHERE id = 1;");
}

TEST_F(SqlBuilderTest, UpdateWithoutWhere) {
    builder.update("users").set("active = 1");
    EXPECT_EQ(builder.build(), "UPDATE users SET active = 1;");
}

TEST_F(SqlBuilderTest, UpdateThrowsWithoutAssignments) {
    builder.update("users");
    EXPECT_THROW(ignore = builder.build(), value_exception);
}

TEST_F(SqlBuilderTest, SetIncrement) {
    builder.update("users").set_increment("age", 5).where_eq("id", "1");
    EXPECT_EQ(builder.build(), "UPDATE users SET age = age + 5 WHERE id = 1;");
}

TEST_F(SqlBuilderTest, SetIncrementDefault) {
    builder.update("users").set_increment("count").where_eq("id", "1");
    EXPECT_EQ(builder.build(), "UPDATE users SET count = count + 1 WHERE id = 1;");
}

TEST_F(SqlBuilderTest, SetDecrement) {
    builder.update("users").set_decrement("stock", 3).where_eq("id", "1");
    EXPECT_EQ(builder.build(), "UPDATE users SET stock = stock - 3 WHERE id = 1;");
}

TEST_F(SqlBuilderTest, SetDecrementDefault) {
    builder.update("users").set_decrement("count").where_eq("id", "1");
    EXPECT_EQ(builder.build(), "UPDATE users SET count = count - 1 WHERE id = 1;");
}

TEST_F(SqlBuilderTest, Remove) {
    builder.remove().delete_from("users");
    EXPECT_EQ(builder.type(), sql_operate::DELETE);
}

TEST_F(SqlBuilderTest, DeleteFrom) {
    builder.delete_from("users").where_eq("id", "1");
    EXPECT_EQ(builder.build(), "DELETE FROM users WHERE id = 1;");
}

TEST_F(SqlBuilderTest, DeleteFromWithoutWhere) {
    builder.delete_from("users");
    EXPECT_EQ(builder.build(), "DELETE FROM users;");
}

TEST_F(SqlBuilderTest, SelectCountWithAlias) {
    builder.select_count("id", "total").from("users");
    EXPECT_EQ(builder.build(), "SELECT COUNT(id) AS total FROM users;");
}

TEST_F(SqlBuilderTest, SelectCountWithoutAlias) {
    builder.select_count("id").from("users");
    EXPECT_EQ(builder.build(), "SELECT COUNT(id) FROM users;");
}

TEST_F(SqlBuilderTest, SelectCountStar) {
    builder.select_count().from("users");
    EXPECT_EQ(builder.build(), "SELECT COUNT(*) FROM users;");
}

TEST_F(SqlBuilderTest, SelectSumWithAlias) {
    builder.select_sum("amount", "total_amount").from("orders");
    EXPECT_EQ(builder.build(), "SELECT SUM(amount) AS total_amount FROM orders;");
}

TEST_F(SqlBuilderTest, SelectSumWithoutAlias) {
    builder.select_sum("amount").from("orders");
    EXPECT_EQ(builder.build(), "SELECT SUM(amount) FROM orders;");
}

TEST_F(SqlBuilderTest, SelectAvgWithAlias) {
    builder.select_avg("age", "avg_age").from("users");
    EXPECT_EQ(builder.build(), "SELECT AVG(age) AS avg_age FROM users;");
}

TEST_F(SqlBuilderTest, SelectAvgWithoutAlias) {
    builder.select_avg("age").from("users");
    EXPECT_EQ(builder.build(), "SELECT AVG(age) FROM users;");
}

TEST_F(SqlBuilderTest, SelectMaxWithAlias) {
    builder.select_max("price", "max_price").from("products");
    EXPECT_EQ(builder.build(), "SELECT MAX(price) AS max_price FROM products;");
}

TEST_F(SqlBuilderTest, SelectMaxWithoutAlias) {
    builder.select_max("price").from("products");
    EXPECT_EQ(builder.build(), "SELECT MAX(price) FROM products;");
}

TEST_F(SqlBuilderTest, SelectMinWithAlias) {
    builder.select_min("price", "min_price").from("products");
    EXPECT_EQ(builder.build(), "SELECT MIN(price) AS min_price FROM products;");
}

TEST_F(SqlBuilderTest, SelectMinWithoutAlias) {
    builder.select_min("price").from("products");
    EXPECT_EQ(builder.build(), "SELECT MIN(price) FROM products;");
}

TEST_F(SqlBuilderTest, SelectDistinct) {
    builder.select_distinct("name").from("users");
    EXPECT_EQ(builder.build(), "SELECT DISTINCT name FROM users;");
}

TEST_F(SqlBuilderTest, SelectSubqueryWithAlias) {
    builder.select_subquery("SELECT MAX(amount) FROM orders WHERE orders.user_id = users.id", "max_order")
            .from("users");
    EXPECT_EQ(builder.build(),
              "SELECT (SELECT MAX(amount) FROM orders WHERE orders.user_id = users.id) AS max_order FROM users;");
}

TEST_F(SqlBuilderTest, SelectSubqueryWithoutAlias) {
    builder.select_subquery("SELECT COUNT(*) FROM orders WHERE orders.user_id = users.id").from("users");
    EXPECT_EQ(builder.build(), "SELECT (SELECT COUNT(*) FROM orders WHERE orders.user_id = users.id) FROM users;");
}

TEST_F(SqlBuilderTest, FromSubquery) {
    builder.select_all().from_subquery("SELECT id FROM users WHERE active = 1", "active_users");
    EXPECT_EQ(builder.build(), "SELECT * FROM (SELECT id FROM users WHERE active = 1) active_users;");
}

TEST_F(SqlBuilderTest, FromSubqueryWithWhere) {
    builder.select_all()
            .from_subquery("SELECT id, name FROM users WHERE active = 1", "active_users")
            .where_eq("name", "'John'");
    EXPECT_EQ(builder.build(),
              "SELECT * FROM (SELECT id, name FROM users WHERE active = 1) active_users WHERE name = 'John';");
}

TEST_F(SqlBuilderTest, Reset) {
    builder.select("id").from("users").where_eq("status", "'active'");
    builder.reset();
    EXPECT_EQ(builder.type(), sql_operate::SELECT);
    EXPECT_TRUE(builder.is_empty());
}

TEST_F(SqlBuilderTest, IsEmpty) {
    EXPECT_TRUE(builder.is_empty());
    builder.from("users");
    EXPECT_FALSE(builder.is_empty());
}

TEST_F(SqlBuilderTest, Table) {
    builder.from("users", "u");
    EXPECT_EQ(builder.table(), "users");
}

TEST_F(SqlBuilderTest, Type) {
    EXPECT_EQ(builder.type(), sql_operate::SELECT);
    builder.update("users");
    EXPECT_EQ(builder.type(), sql_operate::UPDATE);
    builder.reset();
    builder.insert_into("users", {"name"});
    EXPECT_EQ(builder.type(), sql_operate::INSERT);
    builder.reset();
    builder.delete_from("users");
    EXPECT_EQ(builder.type(), sql_operate::DELETE);
}

TEST_F(SqlBuilderTest, ComplexSelectQuery) {
    builder.select("u.id")
            .select("u.name")
            .select_count("o.id", "order_count")
            .from("users", "u")
            .left_join("orders", "u.id = orders.user_id")
            .where_eq("u.status", "'active'")
            .where_gt("u.age", "18")
            .group_by("u.id")
            .group_by("u.name")
            .having("COUNT(o.id) > 0")
            .order_by_desc("order_count")
            .limit(10)
            .offset(5);
    EXPECT_EQ(builder.build(), "SELECT u.id, u.name, COUNT(o.id) AS order_count "
                               "FROM users u "
                               "LEFT JOIN orders ON u.id = orders.user_id "
                               "WHERE u.status = 'active' AND u.age > 18 "
                               "GROUP BY u.id, u.name "
                               "HAVING COUNT(o.id) > 0 "
                               "ORDER BY order_count DESC "
                               "LIMIT 10 OFFSET 5;");
}

TEST_F(SqlBuilderTest, ChainingMixedCalls) {
    builder.select("id").from("users").where_eq("active", "1").order_by_asc("name").limit(50);
    EXPECT_EQ(builder.build(), "SELECT id FROM users WHERE active = 1 ORDER BY name ASC LIMIT 50;");
}

TEST_F(SqlBuilderTest, EmptyBuildThrows) { EXPECT_THROW(ignore = builder.build(), value_exception); }

TEST_F(SqlBuilderTest, CopyPreservesAllData) {
    builder.insert_into("users", {"name", "email"});
    builder.values({"'a'", "'b'"});
    sql_builder copy(builder);
    EXPECT_EQ(copy.build(), "INSERT INTO users (name, email) VALUES ('a', 'b');");
}

TEST_F(SqlBuilderTest, CopyAssignmentDeepCopy) {
    builder.insert_into("users", {"name", "email"});
    builder.values({"'a'", "'b'"});
    sql_builder copy;
    copy = builder;
    builder.reset();
    EXPECT_EQ(copy.build(), "INSERT INTO users (name, email) VALUES ('a', 'b');");
}

TEST_F(SqlBuilderTest, BuildAfterResetClearsState) {
    builder.select("id").from("users");
    builder.reset();
    EXPECT_TRUE(builder.is_empty());
}

TEST_F(SqlBuilderTest, JoinAllTypes) {
    builder.select_all()
            .from("t1")
            .inner_join("t2", "t1.id = t2.id")
            .left_join("t3", "t2.id = t3.id")
            .right_join("t4", "t3.id = t4.id")
            .full_join("t5", "t4.id = t5.id");
    EXPECT_EQ(builder.build(), "SELECT * FROM t1 "
                               "INNER JOIN t2 ON t1.id = t2.id "
                               "LEFT JOIN t3 ON t2.id = t3.id "
                               "RIGHT JOIN t4 ON t3.id = t4.id "
                               "FULL JOIN t5 ON t4.id = t5.id;");
}

TEST_F(SqlBuilderTest, WhereConditionsCombinedWithAnd) {
    builder.select_all()
            .from("users")
            .where_eq("status", "'active'")
            .where_ne("role", "'banned'")
            .where_gt("age", "18")
            .where_ge("score", "50")
            .where_lt("age", "65")
            .where_le("attempts", "3");
    EXPECT_EQ(builder.build(), "SELECT * FROM users "
                               "WHERE status = 'active' "
                               "AND role != 'banned' "
                               "AND age > 18 "
                               "AND score >= 50 "
                               "AND age < 65 "
                               "AND attempts <= 3;");
}

TEST_F(SqlBuilderTest, UnionBasic) {
    builder.select("id").from("users").union_("SELECT id FROM archived");
    EXPECT_EQ(builder.build(), "SELECT id FROM users\n"
                               "UNION\n"
                               "SELECT id FROM archived;");
}

TEST_F(SqlBuilderTest, UnionAll) {
    builder.select("id").from("active").union_all("SELECT id FROM inactive");
    EXPECT_EQ(builder.build(), "SELECT id FROM active\n"
                               "UNION ALL\n"
                               "SELECT id FROM inactive;");
}

TEST_F(SqlBuilderTest, Intersect) {
    builder.select("email").from("users").intersect("SELECT email FROM subscribers");
    EXPECT_EQ(builder.build(), "SELECT email FROM users\n"
                               "INTERSECT\n"
                               "SELECT email FROM subscribers;");
}

TEST_F(SqlBuilderTest, IntersectAll) {
    builder.select("id").from("t1").intersect_all("SELECT id FROM t2");
    EXPECT_EQ(builder.build(), "SELECT id FROM t1\n"
                               "INTERSECT ALL\n"
                               "SELECT id FROM t2;");
}

TEST_F(SqlBuilderTest, Except) {
    builder.select("id").from("all_users").except_("SELECT id FROM deleted_users");
    EXPECT_EQ(builder.build(), "SELECT id FROM all_users\n"
                               "EXCEPT\n"
                               "SELECT id FROM deleted_users;");
}

TEST_F(SqlBuilderTest, ExceptAll) {
    builder.select("id").from("t1").except_all("SELECT id FROM t2");
    EXPECT_EQ(builder.build(), "SELECT id FROM t1\n"
                               "EXCEPT ALL\n"
                               "SELECT id FROM t2;");
}

TEST_F(SqlBuilderTest, MultipleUnions) {
    builder.select("id").from("active").union_("SELECT id FROM inactive").union_all("SELECT id FROM migrated");
    EXPECT_EQ(builder.build(), "SELECT id FROM active\n"
                               "UNION\n"
                               "SELECT id FROM inactive\n"
                               "UNION ALL\n"
                               "SELECT id FROM migrated;");
}

TEST_F(SqlBuilderTest, UnionWithOrderBy) {
    builder.select("name").from("users").union_("SELECT name FROM archived").order_by_asc("name");
    EXPECT_EQ(builder.build(), "SELECT name FROM users\n"
                               "UNION\n"
                               "SELECT name FROM archived ORDER BY name ASC;");
}

TEST_F(SqlBuilderTest, UnionWithLimit) {
    builder.select("id").from("t1").union_("SELECT id FROM t2").limit(10);
    EXPECT_EQ(builder.build(), "SELECT id FROM t1\n"
                               "UNION\n"
                               "SELECT id FROM t2 LIMIT 10;");
}

TEST_F(SqlBuilderTest, UnionAllWithWhere) {
    builder.select("name")
            .from("users")
            .where_eq("status", "'active'")
            .union_all("SELECT name FROM archived WHERE status = 'active'");
    EXPECT_EQ(builder.build(), "SELECT name FROM users WHERE status = 'active'\n"
                               "UNION ALL\n"
                               "SELECT name FROM archived WHERE status = 'active';");
}

TEST_F(SqlBuilderTest, SetOpPreservesType) {
    builder.select("id").from("users").union_("SELECT id FROM archived");
    EXPECT_EQ(builder.type(), sql_operate::SELECT);
}

TEST_F(SqlBuilderTest, SetOpCopyPreserved) {
    builder.select("id").from("users").union_("SELECT id FROM archived");
    sql_builder copy{builder};
    EXPECT_EQ(copy.build(), "SELECT id FROM users\n"
                            "UNION\n"
                            "SELECT id FROM archived;");
}

TEST_F(SqlBuilderTest, SetOpResetClears) {
    builder.select("id").from("users").union_("SELECT id FROM archived");
    builder.reset();
    EXPECT_TRUE(builder.is_empty());
}

TEST_F(SqlBuilderTest, WithCTE) {
    builder.with_("regional_users", "SELECT id, name FROM users WHERE region = 'US'")
            .select_all()
            .from("regional_users");
    EXPECT_EQ(builder.build(), "WITH regional_users AS (\n"
                               "SELECT id, name FROM users WHERE region = 'US'\n"
                               ")\n"
                               "SELECT * FROM regional_users;");
}

TEST_F(SqlBuilderTest, WithRecursiveCTE) {
    builder.with_recursive("org_tree", "SELECT id, parent_id, name FROM employees WHERE parent_id IS NULL\n"
                                       "UNION ALL\n"
                                       "SELECT e.id, e.parent_id, e.name FROM employees e\n"
                                       "INNER JOIN org_tree ot ON e.parent_id = ot.id")
            .select_all()
            .from("org_tree");
    string sql = builder.build();
    EXPECT_NE(sql.find("WITH RECURSIVE"), string::npos);
    EXPECT_NE(sql.find("org_tree AS ("), string::npos);
}

TEST_F(SqlBuilderTest, WithMultipleCTEs) {
    builder.with_("cte1", "SELECT id FROM table_a").with_("cte2", "SELECT id FROM table_b").select_all().from("cte1");
    EXPECT_EQ(builder.build(), "WITH cte1 AS (\n"
                               "SELECT id FROM table_a\n"
                               "),\n"
                               "cte2 AS (\n"
                               "SELECT id FROM table_b\n"
                               ")\n"
                               "SELECT * FROM cte1;");
}

TEST_F(SqlBuilderTest, CrossJoin) {
    builder.select_all().from("users").cross_join("departments");
    EXPECT_EQ(builder.build(), "SELECT * FROM users CROSS JOIN departments;");
}

TEST_F(SqlBuilderTest, CrossJoinWithWhere) {
    builder.select_all().from("products").cross_join("inventory").where("products.id = inventory.product_id");
    EXPECT_EQ(builder.build(), "SELECT * FROM products CROSS JOIN inventory WHERE products.id = inventory.product_id;");
}

TEST_F(SqlBuilderTest, CTEWithUnion) {
    builder.with_("active", "SELECT id FROM users WHERE active = 1")
            .select("id")
            .from("active")
            .union_("SELECT id FROM archived");
    EXPECT_EQ(builder.build(), "WITH active AS (\n"
                               "SELECT id FROM users WHERE active = 1\n"
                               ")\n"
                               "SELECT id FROM active\n"
                               "UNION\n"
                               "SELECT id FROM archived;");
}

TEST_F(SqlBuilderTest, CTECopyPreserved) {
    builder.with_("cte", "SELECT 1").select_all().from("cte");
    sql_builder copy{builder};
    EXPECT_EQ(copy.build(), "WITH cte AS (\n"
                            "SELECT 1\n"
                            ")\n"
                            "SELECT * FROM cte;");
}

TEST_F(SqlBuilderTest, CTEClearedOnReset) {
    builder.with_("cte", "SELECT 1").select_all().from("cte");
    builder.reset();
    EXPECT_TRUE(builder.is_empty());
    EXPECT_THROW(ignore = builder.build(), value_exception);
}

TEST_F(SqlBuilderTest, SelectRowNumber) {
    builder.select("name").select_row_number("rn", {}, {"salary DESC"}).from("employees");
    string sql = builder.build();
    EXPECT_NE(sql.find("ROW_NUMBER() OVER (ORDER BY salary DESC) AS rn"), string::npos);
}

TEST_F(SqlBuilderTest, SelectRankWithPartition) {
    builder.select("dept_id").select_rank("rk", {"dept_id"}, {"salary DESC"}).from("employees");
    string sql = builder.build();
    EXPECT_NE(sql.find("RANK() OVER (PARTITION BY dept_id ORDER BY salary DESC) AS rk"), string::npos);
}

TEST_F(SqlBuilderTest, SelectDenseRank) {
    builder.select("name").select_dense_rank("dr", {}, {"score DESC"}).from("scores");
    string sql = builder.build();
    EXPECT_NE(sql.find("DENSE_RANK() OVER (ORDER BY score DESC) AS dr"), string::npos);
}

TEST_F(SqlBuilderTest, SelectNtile) {
    builder.select("name").select_ntile(4, "quartile", {}, {"sales DESC"}).from("sales");
    string sql = builder.build();
    EXPECT_NE(sql.find("NTILE(4) OVER (ORDER BY sales DESC) AS quartile"), string::npos);
}

TEST_F(SqlBuilderTest, SelectLead) {
    builder.select_lead("salary", "next_salary", {"dept_id"}, {"id ASC"}).from("employees");
    string sql = builder.build();
    EXPECT_NE(sql.find("LEAD(salary) OVER (PARTITION BY dept_id ORDER BY id ASC) AS next_salary"), string::npos);
}

TEST_F(SqlBuilderTest, SelectLag) {
    builder.select_lag("salary", "prev_salary", {"dept_id"}, {"id ASC"}).from("employees");
    string sql = builder.build();
    EXPECT_NE(sql.find("LAG(salary) OVER (PARTITION BY dept_id ORDER BY id ASC) AS prev_salary"), string::npos);
}

TEST_F(SqlBuilderTest, SelectFirstValue) {
    builder.select("dept_id").select_first_value("salary", "highest", {"dept_id"}, {"salary DESC"}).from("employees");
    string sql = builder.build();
    EXPECT_NE(sql.find("FIRST_VALUE(salary) OVER (PARTITION BY dept_id ORDER BY salary DESC) AS highest"),
              string::npos);
}

TEST_F(SqlBuilderTest, SelectLastValue) {
    builder.select_last_value("salary", "lowest", {"dept_id"}, {"salary DESC"}).from("employees");
    string sql = builder.build();
    EXPECT_NE(sql.find("LAST_VALUE(salary) OVER (PARTITION BY dept_id ORDER BY salary DESC) AS lowest"), string::npos);
}

TEST_F(SqlBuilderTest, CaseSimpleExpression) {
    vector<pair<string, string>> when_then;
    when_then.emplace_back(string("'active'"), string("'Enabled'"));
    when_then.emplace_back(string("'inactive'"), string("'Disabled'"));
    string expr = sql_builder::make_case_simple("status", move(when_then), "'Unknown'");
    builder.select(expr + " AS status_label").from("users");
    string sql = builder.build();
    EXPECT_NE(sql.find("CASE status WHEN 'active' THEN 'Enabled' WHEN 'inactive' THEN 'Disabled' ELSE 'Unknown' END"),
              string::npos);
}

TEST_F(SqlBuilderTest, CaseSearchedExpression) {
    vector<pair<string, string>> when_then;
    when_then.emplace_back(string("age >= 18"), string("'Adult'"));
    when_then.emplace_back(string("age >= 13"), string("'Teen'"));
    string expr = sql_builder::make_case_searched(move(when_then), "'Child'");
    builder.select("name").select(expr + " AS category").from("users");
    string sql = builder.build();
    EXPECT_NE(sql.find("CASE WHEN age >= 18 THEN 'Adult' WHEN age >= 13 THEN 'Teen' ELSE 'Child' END"), string::npos);
}

TEST_F(SqlBuilderTest, CastExpression) {
    string expr = sql_builder::make_cast("price", "INTEGER");
    builder.select(expr + " AS price_int").from("products");
    string sql = builder.build();
    EXPECT_NE(sql.find("CAST(price AS INTEGER) AS price_int"), string::npos);
}

TEST_F(SqlBuilderTest, WindowWithWhereAndOrderBy) {
    builder.select({"name", "department"})
            .select_rank("rnk", {"department"}, {"salary DESC"})
            .from("employees")
            .where_gt("salary", "50000")
            .order_by_asc("department");
    string sql = builder.build();
    EXPECT_NE(sql.find("OVER (PARTITION BY department ORDER BY salary DESC)"), string::npos);
    EXPECT_NE(sql.find("ORDER BY department ASC"), string::npos);
}

TEST_F(SqlBuilderTest, MultipleWindowFunctions) {
    builder.select_row_number("rn", {}, {"id"}).select_rank("rk", {}, {"score DESC"}).from("results");
    string sql = builder.build();
    EXPECT_NE(sql.find("ROW_NUMBER()"), string::npos);
    EXPECT_NE(sql.find("RANK()"), string::npos);
}

TEST_F(SqlBuilderTest, WindowWithoutAlias) {
    builder.select("name").select_rank("", {}).from("players");
    string sql = builder.build();
    EXPECT_NE(sql.find("RANK() OVER ()"), string::npos);
}

TEST_F(SqlBuilderTest, CreateTempTable) {
    builder.create_temp_table("tmp_data").column("id", "INTEGER").column("val", "TEXT");
    EXPECT_EQ(builder.build(), "CREATE TEMP TABLE tmp_data (\n"
                               "    id INTEGER,\n"
                               "    val TEXT\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTempTableIfNotExists) {
    builder.create_temp_table_if_not_exists("tmp_users").column_primary_key("id", "INTEGER").column("name", "TEXT");
    EXPECT_EQ(builder.build(), "CREATE TEMP TABLE IF NOT EXISTS tmp_users (\n"
                               "    id INTEGER PRIMARY KEY,\n"
                               "    name TEXT\n"
                               ");");
}

TEST_F(SqlBuilderTest, TempTableWithConstraints) {
    builder.create_temp_table("tmp_products")
            .column_primary_key("id", "INTEGER")
            .column_not_null("name", "VARCHAR(100)")
            .table_unique({"name"});
    string sql = builder.build();
    EXPECT_NE(sql.find("CREATE TEMP TABLE"), string::npos);
    EXPECT_NE(sql.find("NOT NULL"), string::npos);
    EXPECT_NE(sql.find("UNIQUE (name)"), string::npos);
}

TEST_F(SqlBuilderTest, TempTableCopyPreserved) {
    builder.create_temp_table("tmp").column("a", "INT");
    sql_builder copy{builder};
    EXPECT_EQ(copy.build(), "CREATE TEMP TABLE tmp (\n"
                            "    a INT\n"
                            ");");
}

TEST_F(SqlBuilderTest, InsertMultiRowWithAddValues) {
    builder.insert_into("users", {"name", "email"}).values({"?", "?"}).add_values({"?", "?"}).add_values({"?", "?"});
    EXPECT_EQ(builder.build(), "INSERT INTO users (name, email) VALUES (?, ?), (?, ?), (?, ?);");
}

TEST_F(SqlBuilderTest, InsertBatchValues) {
    vector<vector<string>> rows;
    rows.push_back({"'a'", "'a@x.com'"});
    rows.push_back({"'b'", "'b@x.com'"});
    rows.push_back({"'c'", "'c@x.com'"});
    builder.insert_into("users", {"name", "email"}).values(move(rows));
    EXPECT_EQ(builder.build(),
              "INSERT INTO users (name, email) VALUES ('a', 'a@x.com'), ('b', 'b@x.com'), ('c', 'c@x.com');");
}

TEST_F(SqlBuilderTest, InsertBatchValuesSingleRow) {
    vector<vector<string>> rows;
    rows.push_back({"'only'"});
    builder.insert_into("users", {"name"}).values(move(rows));
    EXPECT_EQ(builder.build(), "INSERT INTO users (name) VALUES ('only');");
}

TEST_F(SqlBuilderTest, InsertBatchValuesEmpty) {
    builder.insert_into("users", {"name", "email"}).values(vector<vector<string>>{});
    EXPECT_EQ(builder.build(), "INSERT INTO users (name, email) VALUES ();");
}

TEST_F(SqlBuilderTest, InsertValuesClearsExtraRows) {
    builder.insert_into("t", {"a", "b"}).values({"x", "y"});
    EXPECT_EQ(builder.build(), "INSERT INTO t (a, b) VALUES (x, y);");
}

TEST_F(SqlBuilderTest, InsertMultiRowCopyPreserved) {
    builder.insert_into("t", {"a", "b"}).values({"?", "?"}).add_values({"?", "?"});
    sql_builder copy{builder};
    EXPECT_EQ(copy.build(), "INSERT INTO t (a, b) VALUES (?, ?), (?, ?);");
}

TEST_F(SqlBuilderTest, GroupByRollup) {
    builder.select({"year", "month"})
            .select_sum("sales", "total")
            .from("sales_data")
            .group_by_rollup({"year", "month"});
    EXPECT_EQ(builder.build(), "SELECT year, month, SUM(sales) AS total FROM sales_data GROUP BY ROLLUP(year, month);");
}

TEST_F(SqlBuilderTest, GroupByCube) {
    builder.select({"region", "product"})
            .select_sum("revenue", "total")
            .from("sales")
            .group_by_cube({"region", "product"});
    EXPECT_EQ(builder.build(),
              "SELECT region, product, SUM(revenue) AS total FROM sales GROUP BY CUBE(region, product);");
}

TEST_F(SqlBuilderTest, GroupByRollupSingleField) {
    builder.select("category").select_count().from("products").group_by_rollup({"category"});
    EXPECT_EQ(builder.build(), "SELECT category, COUNT(*) FROM products GROUP BY ROLLUP(category);");
}

TEST_F(SqlBuilderTest, RollupClearsRegularGroupBy) {
    builder.select({"a", "b"}).select_count().from("t").group_by("a").group_by_rollup({"b"});
    string sql = builder.build();
    EXPECT_NE(sql.find("ROLLUP(b)"), string::npos);
    EXPECT_EQ(sql.find("GROUP BY a"), string::npos);
}

TEST_F(SqlBuilderTest, CubeCopyPreserved) {
    builder.select({"a"}).select_count().from("t").group_by_cube({"a", "b"});
    sql_builder copy{builder};
    EXPECT_NE(copy.build().find("GROUP BY CUBE(a, b)"), string::npos);
}

TEST_F(SqlBuilderTest, FetchFirst) {
    builder.select_all().from("users").fetch_first(10);
    EXPECT_EQ(builder.build(), "SELECT * FROM users FETCH FIRST 10 ROWS ONLY;");
}

TEST_F(SqlBuilderTest, FetchFirstWithOffset) {
    builder.select_all().from("users").offset(20).fetch_first(10);
    EXPECT_EQ(builder.build(), "SELECT * FROM users OFFSET 20 ROWS FETCH FIRST 10 ROWS ONLY;");
}

TEST_F(SqlBuilderTest, FetchFirstOverridesLimit) {
    builder.select_all().from("users").limit(5).fetch_first(10);
    string sql = builder.build();
    EXPECT_NE(sql.find("FETCH FIRST 10 ROWS ONLY"), string::npos);
    EXPECT_EQ(sql.find("LIMIT"), string::npos);
}

TEST_F(SqlBuilderTest, LimitOverridesFetchFirst) {
    builder.select_all().from("users").fetch_first(10).limit(5);
    string sql = builder.build();
    EXPECT_NE(sql.find("LIMIT 5"), string::npos);
    EXPECT_EQ(sql.find("FETCH FIRST"), string::npos);
}

TEST_F(SqlBuilderTest, FetchFirstCopyPreserved) {
    builder.select_all().from("users").offset(10).fetch_first(5);
    sql_builder copy{builder};
    EXPECT_EQ(copy.build(), "SELECT * FROM users OFFSET 10 ROWS FETCH FIRST 5 ROWS ONLY;");
}

TEST_F(SqlBuilderTest, FetchFirstWithoutOffset) {
    builder.select("id").from("users").order_by_asc("name").fetch_first(25);
    EXPECT_EQ(builder.build(), "SELECT id FROM users ORDER BY name ASC FETCH FIRST 25 ROWS ONLY;");
}

TEST_F(SqlBuilderTest, CreateIndex) {
    builder.create_index("idx_users_email", "users", {"email"});
    EXPECT_EQ(builder.build(), "CREATE INDEX idx_users_email ON users (email);");
}

TEST_F(SqlBuilderTest, CreateUniqueIndex) {
    builder.create_unique_index("idx_users_email", "users", {"email"});
    EXPECT_EQ(builder.build(), "CREATE UNIQUE INDEX idx_users_email ON users (email);");
}

TEST_F(SqlBuilderTest, CreateCompositeIndex) {
    builder.create_index("idx_users_name", "users", {"last_name", "first_name"});
    EXPECT_EQ(builder.build(), "CREATE INDEX idx_users_name ON users (last_name, first_name);");
}

TEST_F(SqlBuilderTest, DropIndex) {
    builder.drop_index("idx_users_email");
    EXPECT_EQ(builder.build(), "DROP INDEX idx_users_email;");
}

TEST_F(SqlBuilderTest, DropIndexIfExists) {
    builder.drop_index_if_exists("idx_users_email");
    EXPECT_EQ(builder.build(), "DROP INDEX IF EXISTS idx_users_email;");
}

TEST_F(SqlBuilderTest, TruncateTable) {
    builder.truncate("users");
    EXPECT_EQ(builder.build(), "TRUNCATE TABLE users;");
}

TEST_F(SqlBuilderTest, CreateIndexThrowsWithoutColumns) {
    builder.create_index("idx_empty", "t", {});
    EXPECT_THROW(ignore = builder.build(), value_exception);
}

TEST_F(SqlBuilderTest, CreateIndexCopyPreserved) {
    builder.create_index("idx_test", "users", {"name"});
    sql_builder copy{builder};
    EXPECT_EQ(copy.build(), "CREATE INDEX idx_test ON users (name);");
}

TEST_F(SqlBuilderTest, CreateTableBasic) {
    builder.create_table("users")
            .column("id", "INTEGER")
            .column_not_null("name", "TEXT")
            .column_unique("email", "TEXT");
    EXPECT_EQ(builder.build(), "CREATE TABLE users (\n"
                               "    id INTEGER,\n"
                               "    name TEXT NOT NULL,\n"
                               "    email TEXT UNIQUE\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTableWithPrimaryKey) {
    builder.create_table("users").column_primary_key("id", "INTEGER").column_not_null("name", "VARCHAR(255)");
    EXPECT_EQ(builder.build(), "CREATE TABLE users (\n"
                               "    id INTEGER PRIMARY KEY,\n"
                               "    name VARCHAR(255) NOT NULL\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTableWithAutoIncrement) {
    builder.create_table("users").column_auto_increment("id", "INTEGER").column("name", "TEXT");
    EXPECT_EQ(builder.build(), "CREATE TABLE users (\n"
                               "    id INTEGER AUTO_INCREMENT PRIMARY KEY,\n"
                               "    name TEXT\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTableWithDefault) {
    builder.create_table("users")
            .column_primary_key("id", "INTEGER")
            .column_default("status", "VARCHAR(20)", "'active'");
    EXPECT_EQ(builder.build(), "CREATE TABLE users (\n"
                               "    id INTEGER PRIMARY KEY,\n"
                               "    status VARCHAR(20) DEFAULT 'active'\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTableWithCheck) {
    builder.create_table("products")
            .column_primary_key("id", "INTEGER")
            .column_check("price", "DECIMAL(10,2)", "price > 0");
    EXPECT_EQ(builder.build(), "CREATE TABLE products (\n"
                               "    id INTEGER PRIMARY KEY,\n"
                               "    price DECIMAL(10,2) CHECK (price > 0)\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTableIfNotExists) {
    builder.create_table_if_not_exists("users").column_primary_key("id", "INTEGER").column("name", "TEXT");
    EXPECT_EQ(builder.build(), "CREATE TABLE IF NOT EXISTS users (\n"
                               "    id INTEGER PRIMARY KEY,\n"
                               "    name TEXT\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTableTableLevelConstraints) {
    builder.create_table("orders")
            .column("order_id", "INTEGER")
            .column("user_id", "INTEGER")
            .column("amount", "DECIMAL(10,2)")
            .table_primary_key({"order_id"})
            .table_foreign_key("user_id", "users", "id");
    EXPECT_EQ(builder.build(), "CREATE TABLE orders (\n"
                               "    order_id INTEGER,\n"
                               "    user_id INTEGER,\n"
                               "    amount DECIMAL(10,2),\n"
                               "    PRIMARY KEY (order_id),\n"
                               "    FOREIGN KEY (user_id) REFERENCES users(id)\n"
                               ");");
}

TEST_F(SqlBuilderTest, CreateTableWithUniqueConstraint) {
    builder.create_table("users")
            .column("first_name", "TEXT")
            .column("last_name", "TEXT")
            .table_unique({"first_name", "last_name"});
    EXPECT_NE(builder.build().find("UNIQUE (first_name, last_name)"), string::npos);
}

TEST_F(SqlBuilderTest, CreateTableWithCheckConstraint) {
    builder.create_table("products").column("price", "DECIMAL(10,2)").table_check("price > 0");
    EXPECT_NE(builder.build().find("CHECK (price > 0)"), string::npos);
}

TEST_F(SqlBuilderTest, CreateTableThrowsWithoutColumns) {
    builder.create_table("empty");
    EXPECT_THROW(ignore = builder.build(), value_exception);
}

TEST_F(SqlBuilderTest, CreateView) {
    builder.create_view("active_users", "SELECT id, name FROM users WHERE active = 1");
    EXPECT_EQ(builder.build(), "CREATE VIEW active_users AS\n"
                               "SELECT id, name FROM users WHERE active = 1;");
}

TEST_F(SqlBuilderTest, CreateOrReplaceView) {
    builder.create_or_replace_view("user_count", "SELECT COUNT(*) FROM users");
    EXPECT_EQ(builder.build(), "CREATE OR REPLACE VIEW user_count AS\n"
                               "SELECT COUNT(*) FROM users;");
}

TEST_F(SqlBuilderTest, DropView) {
    builder.drop_view("old_view");
    EXPECT_EQ(builder.build(), "DROP VIEW old_view;");
}

TEST_F(SqlBuilderTest, DropViewIfExists) {
    builder.drop_view_if_exists("maybe_view");
    EXPECT_EQ(builder.build(), "DROP VIEW IF EXISTS maybe_view;");
}

TEST_F(SqlBuilderTest, AlterTableAddColumn) {
    builder.alter_table("users").add_column("age", "INTEGER");
    EXPECT_EQ(builder.build(), "ALTER TABLE users ADD COLUMN age INTEGER;");
}

TEST_F(SqlBuilderTest, AlterTableDropColumn) {
    builder.alter_table("users").drop_column("age");
    EXPECT_EQ(builder.build(), "ALTER TABLE users DROP COLUMN age;");
}

TEST_F(SqlBuilderTest, AlterTableRenameColumn) {
    builder.alter_table("users").rename_column("name", "full_name");
    EXPECT_EQ(builder.build(), "ALTER TABLE users RENAME COLUMN name TO full_name;");
}

TEST_F(SqlBuilderTest, AlterTableSetDefault) {
    builder.alter_table("users").alter_column_set_default("status", "'active'");
    EXPECT_EQ(builder.build(), "ALTER TABLE users ALTER COLUMN status SET DEFAULT 'active';");
}

TEST_F(SqlBuilderTest, AlterTableDropDefault) {
    builder.alter_table("users").alter_column_drop_default("status");
    EXPECT_EQ(builder.build(), "ALTER TABLE users ALTER COLUMN status DROP DEFAULT;");
}

TEST_F(SqlBuilderTest, AlterTableSetNotNull) {
    builder.alter_table("users").alter_column_set_not_null("email");
    EXPECT_EQ(builder.build(), "ALTER TABLE users ALTER COLUMN email SET NOT NULL;");
}

TEST_F(SqlBuilderTest, AlterTableDropNotNull) {
    builder.alter_table("users").alter_column_drop_not_null("email");
    EXPECT_EQ(builder.build(), "ALTER TABLE users ALTER COLUMN email DROP NOT NULL;");
}

TEST_F(SqlBuilderTest, AlterTableMultipleActions) {
    builder.alter_table("users")
            .add_column("age", "INTEGER")
            .alter_column_set_default("status", "'active'")
            .alter_column_set_not_null("name");
    EXPECT_EQ(builder.build(), "ALTER TABLE users ADD COLUMN age INTEGER, ALTER COLUMN status SET DEFAULT 'active', "
                               "ALTER COLUMN name SET NOT NULL;");
}

TEST_F(SqlBuilderTest, AlterTableSetType) {
    builder.alter_table("users").alter_column_set_type("name", "VARCHAR(255)");
    EXPECT_EQ(builder.build(), "ALTER TABLE users ALTER COLUMN name SET DATA TYPE VARCHAR(255);");
}

TEST_F(SqlBuilderTest, AlterTableAddPrimaryKey) {
    builder.alter_table("users").add_primary_key({"id"});
    EXPECT_EQ(builder.build(), "ALTER TABLE users ADD PRIMARY KEY (id);");
}

TEST_F(SqlBuilderTest, AlterTableDropConstraint) {
    builder.alter_table("users").drop_constraint("uq_email");
    EXPECT_EQ(builder.build(), "ALTER TABLE users DROP CONSTRAINT uq_email;");
}

TEST_F(SqlBuilderTest, AlterTableThrowsWithoutActions) {
    builder.alter_table("users");
    EXPECT_THROW(ignore = builder.build(), value_exception);
}

struct MockTransaction {
    int begin_calls = 0;
    int commit_calls = 0;
    int rollback_calls = 0;

    void begin() { ++begin_calls; }
    void commit() { ++commit_calls; }
    void rollback() { ++rollback_calls; }
};

class TransactionGuardTest : public ::testing::Test {
protected:
    MockTransaction mock;
};

TEST_F(TransactionGuardTest, ConstructorCallsBegin) {
    transaction_guard tx{mock};
    EXPECT_EQ(mock.begin_calls, 1);
    EXPECT_EQ(mock.commit_calls, 0);
    EXPECT_EQ(mock.rollback_calls, 0);
}

TEST_F(TransactionGuardTest, DestructorRollsBackWhenNotCommitted) {
    {
        transaction_guard tx{mock};
        EXPECT_EQ(mock.begin_calls, 1);
    }
    EXPECT_EQ(mock.rollback_calls, 1);
    EXPECT_EQ(mock.commit_calls, 0);
}

TEST_F(TransactionGuardTest, CommitCallsCommitAndPreventsRollback) {
    {
        transaction_guard tx{mock};
        tx.commit();
        EXPECT_EQ(mock.commit_calls, 1);
        EXPECT_TRUE(tx.committed());
    }
    EXPECT_EQ(mock.rollback_calls, 0);
    EXPECT_EQ(mock.commit_calls, 1);
}

TEST_F(TransactionGuardTest, DoubleCommitIsIdempotent) {
    transaction_guard tx{mock};
    tx.commit();
    tx.commit();
    EXPECT_EQ(mock.commit_calls, 1);
}

TEST_F(TransactionGuardTest, CommittedReturnsFalseBeforeCommit) {
    transaction_guard tx{mock};
    EXPECT_FALSE(tx.committed());
    tx.commit();
    EXPECT_TRUE(tx.committed());
}

TEST_F(TransactionGuardTest, MoveConstructorTransfersOwnership) {
    {
        transaction_guard tx1{mock};
        EXPECT_EQ(mock.begin_calls, 1);

        transaction_guard tx2{move(tx1)};
        EXPECT_EQ(mock.begin_calls, 1);
        EXPECT_EQ(mock.commit_calls, 0);
        EXPECT_EQ(mock.rollback_calls, 0);
    }
    EXPECT_EQ(mock.rollback_calls, 1);
}

TEST_F(TransactionGuardTest, MoveAssignmentRollsBackCurrentAndTransfers) {
    MockTransaction mock2;
    {
        transaction_guard tx1{mock};
        transaction_guard tx2{mock2};
        EXPECT_EQ(mock.begin_calls, 1);
        EXPECT_EQ(mock2.begin_calls, 1);

        tx2 = move(tx1);
        EXPECT_EQ(mock2.rollback_calls, 1);
        EXPECT_EQ(mock.commit_calls, 0);
    }
    EXPECT_EQ(mock.rollback_calls, 1);
}

TEST_F(TransactionGuardTest, MovedFromDoesNotRollback) {
    transaction_guard tx1{mock};
    transaction_guard tx2{move(tx1)};
    EXPECT_EQ(mock.rollback_calls, 0);
}

TEST_F(TransactionGuardTest, MakeTransactionCreatesGuard) {
    auto tx = make_transaction(mock);
    EXPECT_EQ(mock.begin_calls, 1);
    tx.commit();
    EXPECT_TRUE(tx.committed());
}

TEST_F(TransactionGuardTest, NoBeginOnMovedFrom) {
    transaction_guard tx1{mock};
    EXPECT_EQ(mock.begin_calls, 1);
    transaction_guard tx2{move(tx1)};
    EXPECT_EQ(mock.begin_calls, 1);
}

class DbConfigTest : public ::testing::Test {};

TEST_F(DbConfigTest, DefaultHostIsLocalhost) {
    db_config cfg;
    EXPECT_EQ(cfg.host, "127.0.0.1");
}

TEST_F(DbConfigTest, FieldsAreDefaultInitialized) {
    db_config cfg;
    EXPECT_TRUE(cfg.username.empty());
    EXPECT_TRUE(cfg.password.empty());
    EXPECT_TRUE(cfg.database.empty());
    EXPECT_TRUE(cfg.charset.empty());
}

TEST_F(DbConfigTest, CopyConstructor) {
    db_config cfg;
    cfg.username = "admin";
    cfg.password = "secret";
    cfg.database = "testdb";
    cfg.host = "10.0.0.1";

    db_config copy{cfg};
    EXPECT_EQ(copy.username, "admin");
    EXPECT_EQ(copy.password, "secret");
    EXPECT_EQ(copy.database, "testdb");
    EXPECT_EQ(copy.host, "10.0.0.1");
}

TEST_F(DbConfigTest, CopyAssignment) {
    db_config cfg;
    cfg.username = "admin";
    cfg.database = "testdb";

    db_config assigned;
    assigned = cfg;
    EXPECT_EQ(assigned.username, "admin");
    EXPECT_EQ(assigned.database, "testdb");
}

TEST_F(DbConfigTest, MoveConstructor) {
    db_config cfg;
    cfg.username = "admin";
    cfg.database = "testdb";

    db_config moved{move(cfg)};
    EXPECT_EQ(moved.username, "admin");
    EXPECT_EQ(moved.database, "testdb");
}

TEST_F(DbConfigTest, MoveAssignment) {
    db_config cfg;
    cfg.username = "admin";
    cfg.database = "testdb";

    db_config assigned;
    assigned = move(cfg);
    EXPECT_EQ(assigned.username, "admin");
    EXPECT_EQ(assigned.database, "testdb");
}

#ifdef NEFORCE_SUPPORT_SQLITE3
TEST_F(DbConfigTest, ForSqliteSetsCorrectDefaults) {
    db_config cfg = db_config::for_sqlite("test.db");
    EXPECT_EQ(cfg.database, "test.db");
    EXPECT_EQ(cfg.host, "127.0.0.1");
}

TEST_F(DbConfigTest, ForSqliteEmptyFile) {
    db_config cfg = db_config::for_sqlite("");
    EXPECT_TRUE(cfg.database.empty());
}
#endif

#ifdef NEFORCE_SUPPORT_MYSQL
TEST_F(DbConfigTest, ForMysqlSetsCorrectDefaults) {
    db_config cfg = db_config::for_mysql("mydb");
    EXPECT_EQ(cfg.database, "mydb");
    EXPECT_EQ(cfg.username, "root");
    EXPECT_EQ(cfg.host, "127.0.0.1");
}

TEST_F(DbConfigTest, ForMysqlDefaultPort) {
    db_config cfg = db_config::for_mysql("mydb");
    EXPECT_EQ(cfg.port.value(), 3306);
}
#endif

#ifdef NEFORCE_SUPPORT_POSTGRESQL
TEST_F(DbConfigTest, ForPostgresqlSetsCorrectDefaults) {
    db_config cfg = db_config::for_postgresql("mydb");
    EXPECT_EQ(cfg.database, "mydb");
    EXPECT_EQ(cfg.username, "postgres");
    EXPECT_EQ(cfg.host, "127.0.0.1");
}

TEST_F(DbConfigTest, ForPostgresqlDefaultDatabase) {
    db_config cfg = db_config::for_postgresql();
    EXPECT_EQ(cfg.database, "postgres");
}
#endif

#ifdef NEFORCE_SUPPORT_HIREDIS
TEST_F(DbConfigTest, ForRedisSetsCorrectDefaults) {
    db_config cfg = db_config::for_redis("0");
    EXPECT_EQ(cfg.database, "0");
    EXPECT_EQ(cfg.port.value(), 6379);
    EXPECT_EQ(cfg.host, "127.0.0.1");
}
#endif

class ColumnMetaTest : public ::testing::Test {};

TEST_F(ColumnMetaTest, DefaultValues) {
    column_meta meta;
    EXPECT_TRUE(meta.name.empty());
    EXPECT_EQ(meta.type, 0);
    EXPECT_EQ(meta.max_length, 0);
    EXPECT_TRUE(meta.nullable);
}

TEST_F(ColumnMetaTest, CanSetName) {
    column_meta meta;
    meta.name = "user_id";
    EXPECT_EQ(meta.name, "user_id");
}

TEST_F(ColumnMetaTest, CanSetType) {
    column_meta meta;
    meta.type = 3;
    EXPECT_EQ(meta.type, 3);
}

TEST_F(ColumnMetaTest, CanSetMaxLength) {
    column_meta meta;
    meta.max_length = 255;
    EXPECT_EQ(meta.max_length, 255);
}

TEST_F(ColumnMetaTest, CanSetNullable) {
    column_meta meta;
    meta.nullable = false;
    EXPECT_FALSE(meta.nullable);
}

TEST_F(ColumnMetaTest, CopyConstruction) {
    column_meta meta;
    meta.name = "email";
    meta.type = 5;
    meta.max_length = 128;
    meta.nullable = false;

    column_meta copy{meta};
    EXPECT_EQ(copy.name, "email");
    EXPECT_EQ(copy.type, 5);
    EXPECT_EQ(copy.max_length, 128);
    EXPECT_FALSE(copy.nullable);
}

#ifdef NEFORCE_SUPPORT_MYSQL
#    include <NeForce/db/mysql/mysql_connect.hpp>
#    include <NeForce/db/mysql/mysql_result.hpp>
#    include <NeForce/db/mysql/mysql_prepared_result.hpp>
#    include <NeForce/db/mysql/mysql_prepared_statement.hpp>

class MysqlConnectTest : public ::testing::Test {
protected:
    mysql_connect conn;
};

TEST_F(MysqlConnectTest, DefaultConstructedHasValidHandle) { EXPECT_TRUE(conn.connected()); }

TEST_F(MysqlConnectTest, NativeHandleReturnsLink) { EXPECT_NE(conn.native_handle(), nullptr); }

TEST_F(MysqlConnectTest, GetCharacterSetBeforeConnectReturnsNonNull) {
    auto cs = conn.get_character_set();
    SUCCEED();
}

TEST_F(MysqlConnectTest, SetOptionsBeforeConnectReturnsTrue) {
    EXPECT_TRUE(conn.set_options(MYSQL_OPT_CONNECT_TIMEOUT, "10"));
}

TEST_F(MysqlConnectTest, IsValidWithoutConnectReturnsFalse) { EXPECT_FALSE(conn.is_valid()); }

TEST_F(MysqlConnectTest, UpdateWithoutConnectReturnsFalse) { EXPECT_FALSE(conn.update("SELECT 1")); }

TEST_F(MysqlConnectTest, QueryWithoutConnectReturnsNull) {
    auto result = conn.query("SELECT 1");
    EXPECT_EQ(result, nullptr);
}

class MysqlResultTest : public ::testing::Test {};

TEST_F(MysqlResultTest, DefaultConstructorCreatesEmpty) {
    mysql_result result;
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.row_count(), 0);
    EXPECT_EQ(result.column_count(), 0);
}

class MysqlFactoryTest : public ::testing::Test {
protected:
    db_config config{db_config::for_mysql("test")};
};

TEST_F(MysqlFactoryTest, FactoryConstruction) {
    mysql_factory factory{config};
    SUCCEED();
}

TEST_F(MysqlFactoryTest, FactoryMoveConstruction) {
    mysql_factory factory1{config};
    mysql_factory factory2{move(factory1)};
    SUCCEED();
}

TEST_F(MysqlFactoryTest, FactoryDestructionIsSafe) {
    mysql_factory factory{config};
    SUCCEED();
}
#endif

#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <NeForce/db/pgsql/pgsql_connect.hpp>
#    include <NeForce/db/pgsql/pgsql_result.hpp>
#    include <NeForce/db/pgsql/pgsql_prepared_statement.hpp>

class PgsqlConnectTest : public ::testing::Test {
protected:
    pgsql_connect conn;
};

TEST_F(PgsqlConnectTest, DefaultConstructedIsNotConnected) { EXPECT_FALSE(conn.connected()); }

TEST_F(PgsqlConnectTest, CloseOnUnconnectedIsSafe) {
    EXPECT_NO_THROW(conn.close());
    EXPECT_FALSE(conn.connected());
}

TEST_F(PgsqlConnectTest, NativeHandleIsNullBeforeConnect) { EXPECT_EQ(conn.native_handle(), nullptr); }

TEST_F(PgsqlConnectTest, GetErrorBeforeConnectReturnsEmpty) { EXPECT_TRUE(conn.get_error().empty()); }

TEST_F(PgsqlConnectTest, GetErrnoBeforeConnectIsZero) { EXPECT_EQ(conn.get_errno(), 0); }

TEST_F(PgsqlConnectTest, IsValidBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.connected()); }

TEST_F(PgsqlConnectTest, GetCharacterSetBeforeConnectReturnsEmpty) { EXPECT_TRUE(conn.get_character_set().empty()); }

TEST_F(PgsqlConnectTest, SetCharacterSetBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.set_character_set("UTF8")); }

class PgsqlResultTest : public ::testing::Test {};

TEST_F(PgsqlResultTest, NullResultIsEmpty) {
    pgsql_tb_result result{static_cast<::PGresult*>(nullptr), true};
    EXPECT_TRUE(result.empty());
}

TEST_F(PgsqlResultTest, NullResultHasZeroRows) {
    pgsql_tb_result result{static_cast<::PGresult*>(nullptr), true};
    EXPECT_EQ(result.row_count(), 0);
    EXPECT_EQ(result.column_count(), 0);
}

class PgsqlFactoryTest : public ::testing::Test {
protected:
    db_config config{db_config::for_postgresql("test")};
};

TEST_F(PgsqlFactoryTest, FactoryConstruction) {
    pgsql_factory factory{config};
    SUCCEED();
}

TEST_F(PgsqlFactoryTest, CreateResultWithNullReturnsNull) {
    pgsql_factory factory{config};
    auto* result = factory.create_result(nullptr);
    EXPECT_EQ(result, nullptr);
}
#endif

#ifdef NEFORCE_SUPPORT_HIREDIS
#    include <NeForce/db/redis/redis_connect.hpp>
#    include <NeForce/db/redis/redis_result.hpp>

class RedisConnectTest : public ::testing::Test {
protected:
    redis_connect conn;
};

TEST_F(RedisConnectTest, DefaultConstructedIsNotConnected) { EXPECT_FALSE(conn.connected()); }

TEST_F(RedisConnectTest, CloseOnUnconnectedIsSafe) {
    EXPECT_NO_THROW(conn.close());
    EXPECT_FALSE(conn.connected());
}

TEST_F(RedisConnectTest, NativeHandleIsNullBeforeConnect) { EXPECT_EQ(conn.native_handle(), nullptr); }

TEST_F(RedisConnectTest, GetErrnoIsZeroBeforeConnect) { EXPECT_EQ(conn.get_errno(), 0); }

TEST_F(RedisConnectTest, IsValidWithoutConnectReturnsFalse) { EXPECT_FALSE(conn.is_valid()); }

TEST_F(RedisConnectTest, SetBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.set("key", "value")); }

TEST_F(RedisConnectTest, SetexBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.setex("key", "value", 60)); }

TEST_F(RedisConnectTest, GetBeforeConnectReturnsNull) {
    auto result = conn.get("key");
    EXPECT_EQ(result, nullptr);
}

TEST_F(RedisConnectTest, DelBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.del("key")); }

TEST_F(RedisConnectTest, ExistsBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.exists("key")); }

TEST_F(RedisConnectTest, ExpireBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.expire("key", 60)); }

TEST_F(RedisConnectTest, HsetBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.hset("hash", "field", "value")); }

TEST_F(RedisConnectTest, HgetBeforeConnectReturnsNull) {
    auto result = conn.hget("hash", "field");
    EXPECT_EQ(result, nullptr);
}

TEST_F(RedisConnectTest, HgetallBeforeConnectReturnsNull) {
    auto result = conn.hgetall("hash");
    EXPECT_EQ(result, nullptr);
}

TEST_F(RedisConnectTest, LpushBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.lpush("list", "value")); }

TEST_F(RedisConnectTest, RpushBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.rpush("list", "value")); }

TEST_F(RedisConnectTest, LrangeBeforeConnectReturnsNull) {
    auto result = conn.lrange("list", 0, -1);
    EXPECT_EQ(result, nullptr);
}

TEST_F(RedisConnectTest, SaddBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.sadd("set", "member")); }

TEST_F(RedisConnectTest, SmembersBeforeConnectReturnsNull) {
    auto result = conn.smembers("set");
    EXPECT_EQ(result, nullptr);
}

TEST_F(RedisConnectTest, GetCharacterSetReturnsEmpty) { EXPECT_TRUE(conn.get_character_set().empty()); }

TEST_F(RedisConnectTest, SetCharacterSetReturnsFalse) { EXPECT_FALSE(conn.set_character_set("UTF8")); }

TEST_F(RedisConnectTest, BeginBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.begin()); }

TEST_F(RedisConnectTest, CommitBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.commit()); }

TEST_F(RedisConnectTest, RollbackBeforeConnectReturnsFalse) { EXPECT_FALSE(conn.rollback()); }

class RedisResultTest : public ::testing::Test {};

TEST_F(RedisResultTest, DefaultConstructorCreatesEmpty) {
    redis_result result;
    EXPECT_TRUE(result.empty());
}

TEST_F(RedisResultTest, DefaultConstructorTypeIsNegative) {
    redis_result result;
    EXPECT_EQ(result.type(), -1);
}

TEST_F(RedisResultTest, DefaultConstructorIsNotNil) {
    redis_result result;
    EXPECT_FALSE(result.is_nil());
}

TEST_F(RedisResultTest, NullReplyIsEmpty) {
    redis_result result{static_cast<::redisReply*>(nullptr)};
    EXPECT_TRUE(result.empty());
}

TEST_F(RedisResultTest, ValueHashIsEmptyByDefault) {
    redis_result result;
    EXPECT_TRUE(result.value_hash().empty());
}

class RedisFactoryTest : public ::testing::Test {
protected:
    db_config config{db_config::for_redis("0")};
};

TEST_F(RedisFactoryTest, FactoryConstruction) {
    redis_factory factory{config};
    SUCCEED();
}

TEST_F(RedisFactoryTest, CreateResultWithNullCreatesValidResult) {
    redis_factory factory{config};
    auto* result = factory.create_result(nullptr);
    ASSERT_NE(result, nullptr);
    delete result;
}
#endif

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_connect.hpp>

class SqliteFactoryTest : public ::testing::Test {
protected:
    db_config config{db_config::for_sqlite(":memory:")};
};

TEST_F(SqliteFactoryTest, FactoryConstruction) {
    sqlite_factory factory{config};
    SUCCEED();
}

TEST_F(SqliteFactoryTest, CreateResultWithNullCreatesValidResult) {
    sqlite_factory factory{config};
    auto* result = factory.create_result(nullptr);
    ASSERT_NE(result, nullptr);
    delete result;
}

TEST_F(SqliteFactoryTest, CreateConnectReturnsValidConnection) {
    sqlite_factory factory{config};
    auto* conn = factory.create_connect();
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->connected());
    delete conn;
}
#endif
