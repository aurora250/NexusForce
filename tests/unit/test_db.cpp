#include <NeForce/db/sql_builder.hpp>
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
    sql_builder moved(std::move(builder));
    EXPECT_EQ(moved.build(), "SELECT id FROM users;");
}

TEST_F(SqlBuilderTest, MoveAssignment) {
    builder.select("id").from("users");
    sql_builder moved;
    moved = std::move(builder);
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
