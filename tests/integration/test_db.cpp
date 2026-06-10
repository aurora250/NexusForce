#include <NeForce/db/database_pool.hpp>
#include <NeForce/db/db_config.hpp>
#include <NeForce/db/db_interface.hpp>
#include <NeForce/db/sql_builder.hpp>
#include <NeForce/db/transaction_guard.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/file/file.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    const json_object* load_db_config() {
        static string content;
        static unique_ptr<json_value> root;
        static bool tried = false;

        if (tried) {
            return root ? root->as_object() : nullptr;
        }
        tried = true;

        const path p = path::current_executable_path() / "/../../../../tests/resource/db_config.json";
        eprintln(p);

        file f(p.lexically_normal());
        if (f.is_opened()) {
            content = f.read();
        }

        if (content.empty()) {
            return nullptr;
        }

        try {
            json_parser parser{content};
            root = parser.parse();
            return root->as_object();
        } catch (...) {
            return nullptr;
        }
    }

    db_config make_config(const json_object* json, const char* section) {
        if (json == nullptr) {
            return {};
        }

        auto* sec = json->get_member(section);
        if (sec == nullptr) {
            return {};
        }
        auto* obj = sec->as_object();
        if (obj == nullptr) {
            return {};
        }

        db_config config;
        auto set_str = [&](const char* key, string& target) {
            auto* v = obj->get_member(key);
            if (v != nullptr && v->is_string()) {
                target = v->as_string()->get_value();
            }
        };
        auto set_port = [&](const char* key) {
            auto* v = obj->get_member(key);
            if (v != nullptr && v->is_number()) {
                config.port = ports{static_cast<uint16_t>(v->as_number()->get_value())};
            }
        };

        set_str("host", config.host);
        set_port("port");
        set_str("username", config.username);
        set_str("password", config.password);
        set_str("database", config.database);
        set_str("charset", config.charset);
        return config;
    }
} // namespace

#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_connect.hpp>
#    include <NeForce/db/sqlite/sqlite_prepared_statement.hpp>
#    include <NeForce/db/sqlite/sqlite_result.hpp>

class SqliteIntegrationTest : public ::testing::Test {
protected:
    db_config config;
    sqlite_connect conn;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "sqlite");
        if (config.database.empty()) {
            config = db_config::for_sqlite(":memory:");
        }
        ASSERT_TRUE(conn.connect(config));
    }

    void TearDown() override { conn.close(); }

    void create_test_table() {
        ASSERT_TRUE(conn.update("CREATE TABLE IF NOT EXISTS users ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                "name TEXT NOT NULL, "
                                "age INTEGER, "
                                "email TEXT, "
                                "salary REAL, "
                                "active INTEGER DEFAULT 1"
                                ")"));
    }

    void insert_test_data() {
        ignore = conn.update(
                "INSERT INTO users (name, age, email, salary) VALUES ('Alice', 30, 'alice@test.com', 75000.5)");
        ignore =
                conn.update("INSERT INTO users (name, age, email, salary) VALUES ('Bob', 25, 'bob@test.com', 62000.0)");
        ignore = conn.update(
                "INSERT INTO users (name, age, email, salary) VALUES ('Charlie', 35, 'charlie@test.com', 88000.0)");
        ignore = conn.update(
                "INSERT INTO users (name, age, email, salary) VALUES ('Diana', 28, 'diana@test.com', 71000.0)");
        ignore = conn.update("INSERT INTO users (name, age, email) VALUES ('Eve', 22, 'eve@test.com')");
    }
};

TEST_F(SqliteIntegrationTest, ConnectToMemoryDatabase) {
    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn.is_valid());
}

TEST_F(SqliteIntegrationTest, NativeHandleIsNotNull) { EXPECT_NE(conn.native_handle(), nullptr); }

TEST_F(SqliteIntegrationTest, CreateTableAndCheckExists) {
    create_test_table();
    EXPECT_TRUE(conn.table_exists("users"));
}

TEST_F(SqliteIntegrationTest, InsertAndQueryRows) {
    create_test_table();
    insert_test_data();

    auto result = conn.query("SELECT * FROM users ORDER BY id");
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->empty());
    EXPECT_EQ(result->column_count(), 6);

    const auto& names = result->column_names();
    EXPECT_EQ(names[0], "id");
    EXPECT_EQ(names[1], "name");

    int count = 0;
    while (result->next()) {
        ++count;
    }
    EXPECT_EQ(count, 5);
}

TEST_F(SqliteIntegrationTest, TypedGetters) {
    create_test_table();
    insert_test_data();

    auto result = conn.query("SELECT name, age, salary, active FROM users WHERE id = 1");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get(0), "Alice");
    EXPECT_EQ(result->get_int32(1), 30);
    EXPECT_DOUBLE_EQ(result->get_float64(2), 75000.5);
    EXPECT_TRUE(result->get_bool(3));
}

TEST_F(SqliteIntegrationTest, UpdateAndDelete) {
    create_test_table();
    insert_test_data();

    EXPECT_TRUE(conn.update("UPDATE users SET age = 31 WHERE id = 1"));
    auto result = conn.query("SELECT age FROM users WHERE id = 1");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get_int32(0), 31);

    EXPECT_TRUE(conn.update("DELETE FROM users WHERE id = 3"));
    result = conn.query("SELECT * FROM users WHERE id = 3");
    EXPECT_FALSE(result->next());
}

TEST_F(SqliteIntegrationTest, TransactionBeginCommit) {
    create_test_table();
    conn.begin();
    ignore = conn.update("INSERT INTO users (name, age) VALUES ('TxUser', 50)");
    conn.commit();

    auto result = conn.query("SELECT name FROM users WHERE name = 'TxUser'");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get(0), "TxUser");
}

TEST_F(SqliteIntegrationTest, TransactionBeginRollback) {
    create_test_table();
    conn.begin();
    ignore = conn.update("INSERT INTO users (name, age) VALUES ('RbUser', 50)");
    conn.rollback();

    auto result = conn.query("SELECT * FROM users WHERE name = 'RbUser'");
    EXPECT_FALSE(result->next());
}

TEST_F(SqliteIntegrationTest, TransactionGuardCommits) {
    create_test_table();
    {
        transaction_guard tx{conn};
        ignore = conn.update("INSERT INTO users (name, age) VALUES ('GuardCommit', 99)");
        tx.commit();
    }
    auto result = conn.query("SELECT name FROM users WHERE name = 'GuardCommit'");
    ASSERT_TRUE(result->next());
}

TEST_F(SqliteIntegrationTest, TransactionGuardRollsBackOnScopeExit) {
    create_test_table();
    {
        transaction_guard tx{conn};
        ignore = conn.update("INSERT INTO users (name, age) VALUES ('GuardNoCommit', 99)");
    }
    auto result = conn.query("SELECT * FROM users WHERE name = 'GuardNoCommit'");
    EXPECT_FALSE(result->next());
}

TEST_F(SqliteIntegrationTest, PreparedStatementInsert) {
    create_test_table();
    auto stmt = conn.prepare_statement("INSERT INTO users (name, age, email) VALUES (?, ?, ?)");
    ASSERT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->param_count(), 3);
    stmt->bind_param(1, string("PstmtUser"));
    stmt->bind_param(2, int32_t(27));
    stmt->bind_param(3, string_view("pstmt@test.com"));
    EXPECT_TRUE(stmt->execute());
}

TEST_F(SqliteIntegrationTest, PreparedStatementExecuteQuery) {
    create_test_table();
    insert_test_data();
    auto stmt = conn.prepare_statement("SELECT name, age FROM users WHERE age > ? ORDER BY age");
    ASSERT_NE(stmt, nullptr);
    stmt->bind_param(1, int32_t(26));
    auto result = stmt->execute_query();
    ASSERT_NE(result, nullptr);
    int count = 0;
    while (result->next()) {
        ++count;
    }
    EXPECT_GE(count, 2);
}

TEST_F(SqliteIntegrationTest, PreparedStatementBindAllTypes) {
    create_test_table();
    auto stmt = conn.prepare_statement("INSERT INTO users (name, age) VALUES (?, ?)");
    ASSERT_NE(stmt, nullptr);
    EXPECT_TRUE(stmt->bind_param(1, string("StrUser")));
    EXPECT_TRUE(stmt->bind_param(1, "CStrUser"));
    EXPECT_TRUE(stmt->bind_param(1, string_view{"SvUser"}));
    EXPECT_TRUE(stmt->bind_param(2, int32_t(42)));
    EXPECT_TRUE(stmt->bind_param(2, int64_t(99)));
    EXPECT_TRUE(stmt->bind_param(2, float64_t(3.14)));
    EXPECT_TRUE(stmt->execute());
}

TEST_F(SqliteIntegrationTest, PreparedStatementErrorReporting) {
    create_test_table();
    auto stmt = conn.prepare_statement("SELECT name FROM nonexistent");
    if (stmt != nullptr) {
        auto err = stmt->get_error();
        SUCCEED();
    }
}

TEST_F(SqliteIntegrationTest, BatchInsert) {
    create_test_table();
    size_t inserted = conn.batch_insert(
            "users", {"name", "age", "email"},
            {{"Batch1", "25", "b1@test.com"}, {"Batch2", "30", "b2@test.com"}, {"Batch3", "35", "b3@test.com"}});
    EXPECT_EQ(inserted, 3);
    auto result = conn.query("SELECT COUNT(*) FROM users");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get_int32(0), 3);
}

TEST_F(SqliteIntegrationTest, BatchInsertEdgeCases) {
    create_test_table();
    EXPECT_EQ(conn.batch_insert("users", {"name"}, vector<vector<string>>{}), 0);
    EXPECT_EQ(conn.batch_insert("users", {}, {{"x"}}), 0);
}

TEST_F(SqliteIntegrationTest, ReconnectAndClose) {
    create_test_table();
    insert_test_data();
    conn.close();
    EXPECT_FALSE(conn.connected());
    EXPECT_TRUE(conn.connect(config));
    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn.is_valid());
}

TEST_F(SqliteIntegrationTest, CleanupActionResetAllowsReuse) {
    create_test_table();
    insert_test_data();
    auto stmt = conn.prepare_statement("SELECT name FROM users WHERE id = ?");
    ASSERT_NE(stmt, nullptr);
    stmt->bind_param(1, int32_t(1));
    {
        auto result = stmt->execute_query();
        ASSERT_TRUE(result->next());
        EXPECT_EQ(result->get(0), "Alice");
    }
    stmt->bind_param(1, int32_t(2));
    auto result = stmt->execute_query();
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get(0), "Bob");
}

TEST_F(SqliteIntegrationTest, ColumnMetadata) {
    create_test_table();
    auto result = conn.query("SELECT id, name FROM users LIMIT 1");
    auto meta0 = result->column_metadata(0);
    EXPECT_EQ(meta0.name, "id");
    auto meta1 = result->column_metadata(1);
    EXPECT_EQ(meta1.name, "name");
}

TEST_F(SqliteIntegrationTest, NullColumnReturnsEmptyString) {
    create_test_table();
    insert_test_data();
    auto result = conn.query("SELECT salary FROM users WHERE id = 5");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get(0), "");
}

class SqlitePoolTest : public ::testing::Test {
protected:
    db_config config;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "sqlite");
        if (config.database.empty()) {
            config = db_config::for_sqlite(":memory:");
        }
    }
};

TEST_F(SqlitePoolTest, AcquireAndQuery) {
    database_pool pool{db_type::SQLITE3, config, {2, 2, 8, seconds{60}, milliseconds{5000}}};
    EXPECT_TRUE(pool.is_running());
    auto conn = pool.get_tb_connect();
    ASSERT_NE(conn, nullptr);
    ignore = conn->update("CREATE TABLE t(x INTEGER)");
    ignore = conn->update("INSERT INTO t VALUES (42)");
    auto result = conn->query("SELECT x FROM t");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get_int32(0), 42);
}

TEST_F(SqlitePoolTest, TotalCountAndIdleCount) {
    database_pool pool{db_type::SQLITE3, config, {1, 1, 8, seconds{60}, milliseconds{5000}}};
    EXPECT_GE(pool.total_count(), 1);
    auto c1 = pool.get_connect();
    auto c2 = pool.get_connect();
    EXPECT_GE(pool.total_count(), 2);
}

TEST_F(SqlitePoolTest, StopDeniesFurtherAcquires) {
    database_pool pool{db_type::SQLITE3, config, {2, 2, 8, seconds{60}, milliseconds{3000}}};
    pool.stop();
    EXPECT_FALSE(pool.is_running());
    EXPECT_EQ(pool.get_connect(), nullptr);
}

TEST_F(SqlitePoolTest, ConnectionReturnAndReuse) {
    database_pool pool{db_type::SQLITE3, config, {1, 1, 8, seconds{60}, milliseconds{5000}}};
    {
        auto conn = pool.get_connect();
        ASSERT_NE(conn, nullptr);
    }
    auto conn = pool.get_connect();
    ASSERT_NE(conn, nullptr);
}
#endif

#ifdef NEFORCE_SUPPORT_MYSQL
#    include <NeForce/db/mysql/mysql_connect.hpp>
#    include <NeForce/db/mysql/mysql_result.hpp>
#    include <NeForce/db/mysql/mysql_prepared_statement.hpp>

class MysqlIntegrationTest : public ::testing::Test {
protected:
    db_config config;
    mysql_connect conn;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "mysql");
        if (config.database.empty()) {
            config = db_config::for_mysql("testdb");
        }

        // Create database if needed: connect without database first
        {
            db_config no_db = config;
            no_db.database.clear();
            mysql_connect setup_conn;
            if (!setup_conn.connect(no_db)) {
                eprintln(setup_conn.get_error());
                GTEST_SKIP() << "MySQL server not available";
            }
            ignore = setup_conn.update("CREATE DATABASE IF NOT EXISTS `" + config.database + "`");
            setup_conn.close();
        }

        if (!conn.connect(config)) {
            eprintln(conn.get_error());
            GTEST_SKIP() << "MySQL server not available";
        }
    }

    void TearDown() override { conn.close(); }

    void create_test_table() {
        ignore = conn.update("CREATE TABLE IF NOT EXISTS test_users ("
                             "id INT AUTO_INCREMENT PRIMARY KEY, "
                             "name VARCHAR(100) NOT NULL, "
                             "age INT, "
                             "email VARCHAR(200), "
                             "salary DOUBLE)");
        ignore = conn.update("TRUNCATE TABLE test_users");
    }
};

TEST_F(MysqlIntegrationTest, ConnectAndPing) {
    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn.is_valid());
}

TEST_F(MysqlIntegrationTest, CreateTableAndQuery) {
    create_test_table();
    ignore = conn.update("INSERT INTO test_users (name, age) VALUES ('MySQL_Alice', 30)");
    ignore = conn.update("INSERT INTO test_users (name, age) VALUES ('MySQL_Bob', 25)");

    auto result = conn.query("SELECT name, age FROM test_users ORDER BY id");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get(0), "MySQL_Alice");
    EXPECT_EQ(result->get_int32(1), 30);
}

TEST_F(MysqlIntegrationTest, TransactionBeginCommitRollback) {
    create_test_table();
    conn.begin();
    ignore = conn.update("INSERT INTO test_users (name, age) VALUES ('TxUser', 50)");
    conn.commit();

    conn.begin();
    ignore = conn.update("INSERT INTO test_users (name, age) VALUES ('RbUser', 50)");
    conn.rollback();

    auto result = conn.query("SELECT name FROM test_users WHERE name = 'TxUser'");
    ASSERT_TRUE(result->next());

    result = conn.query("SELECT name FROM test_users WHERE name = 'RbUser'");
    EXPECT_FALSE(result->next());
}

TEST_F(MysqlIntegrationTest, TransactionGuardRAII) {
    create_test_table();
    {
        transaction_guard tx{conn};
        ignore = conn.update("INSERT INTO test_users (name, age) VALUES ('GdCommit', 99)");
        tx.commit();
    }
    {
        transaction_guard tx{conn};
        ignore = conn.update("INSERT INTO test_users (name, age) VALUES ('GdRollback', 99)");
    }
    auto result = conn.query("SELECT name FROM test_users WHERE name = 'GdCommit'");
    ASSERT_TRUE(result->next());
    result = conn.query("SELECT name FROM test_users WHERE name = 'GdRollback'");
    EXPECT_FALSE(result->next());
}

TEST_F(MysqlIntegrationTest, PreparedStatementInsertAndQuery) {
    create_test_table();
    auto stmt = conn.prepare_statement("INSERT INTO test_users (name, age, email) VALUES (?, ?, ?)");
    ASSERT_NE(stmt, nullptr);
    stmt->bind_param(0, string("PstmtUser"));
    stmt->bind_param(1, int32_t(27));
    stmt->bind_param(2, string_view("pstmt@test.com"));
    EXPECT_TRUE(stmt->execute());
}

TEST_F(MysqlIntegrationTest, BatchInsert) {
    create_test_table();
    size_t inserted =
            conn.batch_insert("test_users", {"name", "age"}, {{"Batch1", "21"}, {"Batch2", "22"}, {"Batch3", "23"}});
    EXPECT_EQ(inserted, 3);
}

TEST_F(MysqlIntegrationTest, TableExists) {
    create_test_table();
    EXPECT_TRUE(conn.table_exists("test_users"));
    EXPECT_FALSE(conn.table_exists("nonexistent"));
}

class MysqlPoolTest : public ::testing::Test {
protected:
    db_config config;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "mysql");
        if (config.database.empty()) {
            config = db_config::for_mysql("testdb");
        }

        {
            db_config no_db = config;
            no_db.database.clear();
            mysql_connect setup_conn;
            if (setup_conn.connect(no_db)) {
                ignore = setup_conn.update("CREATE DATABASE IF NOT EXISTS `" + config.database + "`");
                setup_conn.close();
            }
        }
    }
};

TEST_F(MysqlPoolTest, AcquireAndOperate) {
    database_pool* pool;
    try {
        pool = new database_pool{db_type::MYSQL, config, {2, 2, 8, seconds{60}, milliseconds{5000}}};
    } catch (const database_exception&) {
        GTEST_SKIP() << "MySQL pool unavailable";
    }
    if (!pool->is_running()) {
        delete pool;
        GTEST_SKIP() << "MySQL pool unavailable";
    }
    auto conn = pool->get_tb_connect();
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->is_valid());
    delete pool;
}
#endif

#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <NeForce/db/pgsql/pgsql_connect.hpp>
#    include <NeForce/db/pgsql/pgsql_result.hpp>
#    include <NeForce/db/pgsql/pgsql_prepared_statement.hpp>

class PgsqlIntegrationTest : public ::testing::Test {
protected:
    db_config config;
    pgsql_connect conn;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "postgresql");
        if (config.database.empty()) {
            config = db_config::for_postgresql("testdb");
        }

        // PostgreSQL requires a database to connect; use default "postgres" database first
        {
            db_config pg_config = config;
            pg_config.database = "postgres";
            pgsql_connect setup_conn;
            if (!setup_conn.connect(pg_config)) {
                eprintln(setup_conn.get_error());
                GTEST_SKIP() << "PostgreSQL server not available";
            }
            setup_conn.update("CREATE DATABASE " + config.database);
            setup_conn.close();
        }

        if (!conn.connect(config)) {
            GTEST_SKIP() << "PostgreSQL server not available";
        }
    }

    void TearDown() override { conn.close(); }

    void create_test_table() {
        conn.update("CREATE TABLE IF NOT EXISTS test_users ("
                    "id SERIAL PRIMARY KEY, "
                    "name VARCHAR(100) NOT NULL, "
                    "age INTEGER, "
                    "email VARCHAR(200), "
                    "salary DOUBLE PRECISION)");
        conn.update("DELETE FROM test_users");
    }
};

TEST_F(PgsqlIntegrationTest, ConnectAndCheck) {
    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn.is_valid());
}

TEST_F(PgsqlIntegrationTest, CreateTableAndQuery) {
    create_test_table();
    conn.update("INSERT INTO test_users (name, age) VALUES ('Pg_Alice', 30)");
    conn.update("INSERT INTO test_users (name, age) VALUES ('Pg_Bob', 25)");

    auto result = conn.query("SELECT name, age FROM test_users ORDER BY id");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get(0), "Pg_Alice");
    EXPECT_EQ(result->get_int32(1), 30);
}

TEST_F(PgsqlIntegrationTest, TransactionBeginCommitRollback) {
    create_test_table();
    conn.begin();
    conn.update("INSERT INTO test_users (name, age) VALUES ('TxUser', 50)");
    conn.commit();

    conn.begin();
    conn.update("INSERT INTO test_users (name, age) VALUES ('RbUser', 50)");
    conn.rollback();

    auto result = conn.query("SELECT name FROM test_users WHERE name = 'TxUser'");
    ASSERT_TRUE(result->next());
    result = conn.query("SELECT name FROM test_users WHERE name = 'RbUser'");
    EXPECT_TRUE(result->empty());
}

TEST_F(PgsqlIntegrationTest, TransactionGuardRAII) {
    create_test_table();
    {
        transaction_guard tx{conn};
        conn.update("INSERT INTO test_users (name, age) VALUES ('GdCommit', 99)");
        tx.commit();
    }
    {
        transaction_guard tx{conn};
        conn.update("INSERT INTO test_users (name, age) VALUES ('GdRollback', 99)");
    }
    auto result = conn.query("SELECT name FROM test_users WHERE name = 'GdCommit'");
    ASSERT_TRUE(result->next());
    result = conn.query("SELECT name FROM test_users WHERE name = 'GdRollback'");
    EXPECT_TRUE(result->empty());
}

TEST_F(PgsqlIntegrationTest, PreparedStatementInsertAndQuery) {
    create_test_table();
    auto stmt = conn.prepare_statement("INSERT INTO test_users (name, age) VALUES ($1, $2)");
    ASSERT_NE(stmt, nullptr);
    stmt->bind_param(1, string("PstmtUser"));
    stmt->bind_param(2, int32_t(27));
    EXPECT_TRUE(stmt->execute());
}

TEST_F(PgsqlIntegrationTest, BatchInsert) {
    create_test_table();
    size_t inserted =
            conn.batch_insert("test_users", {"name", "age"}, {{"Batch1", "21"}, {"Batch2", "22"}, {"Batch3", "23"}});
    EXPECT_EQ(inserted, 3);
}

TEST_F(PgsqlIntegrationTest, TableExists) {
    create_test_table();
    EXPECT_TRUE(conn.table_exists("test_users"));
    EXPECT_FALSE(conn.table_exists("nonexistent"));
}

TEST_F(PgsqlIntegrationTest, QueryTypedGetters) {
    create_test_table();
    conn.update("INSERT INTO test_users (name, age, salary) VALUES ('Types', 42, 75000.5)");
    auto result = conn.query("SELECT name, age, salary FROM test_users WHERE name = 'Types'");
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->get(0), "Types");
    EXPECT_EQ(result->get_int32(1), 42);
    EXPECT_DOUBLE_EQ(result->get_float64(2), 75000.5);
}

class PgsqlPoolTest : public ::testing::Test {
protected:
    db_config config;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "postgresql");
        if (config.database.empty()) {
            config = db_config::for_postgresql("testdb");
        }

        {
            db_config pg_config = config;
            pg_config.database = "postgres";
            pgsql_connect setup_conn;
            if (setup_conn.connect(pg_config)) {
                setup_conn.update("CREATE DATABASE " + config.database);
                setup_conn.close();
            }
        }
    }
};

TEST_F(PgsqlPoolTest, AcquireAndOperate) {
    database_pool* pool;
    try {
        pool = new database_pool{db_type::POSTGRESQL, config, {2, 2, 8, seconds{60}, milliseconds{5000}}};
    } catch (const database_exception&) {
        GTEST_SKIP() << "PostgreSQL pool unavailable";
    }
    if (!pool->is_running()) {
        delete pool;
        GTEST_SKIP() << "PostgreSQL pool unavailable";
    }
    auto conn = pool->get_tb_connect();
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->is_valid());
    delete pool;
}
#endif

#ifdef NEFORCE_SUPPORT_HIREDIS
#    include <NeForce/db/redis/redis_connect.hpp>
#    include <NeForce/db/redis/redis_result.hpp>

class RedisIntegrationTest : public ::testing::Test {
protected:
    db_config config;
    redis_connect conn;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "redis");
        if (config.database.empty()) {
            config = db_config::for_redis("0");
        }
        if (!conn.connect(config)) {
            GTEST_SKIP() << "Redis server not available";
        }
        conn.update("FLUSHDB");
    }

    void TearDown() override { conn.close(); }
};

TEST_F(RedisIntegrationTest, ConnectAndPing) {
    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn.is_valid());
}

TEST_F(RedisIntegrationTest, SetAndGet) {
    EXPECT_TRUE(conn.set("test_key", "test_value"));
    auto result = conn.get("test_key");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->value(), "test_value");
}

TEST_F(RedisIntegrationTest, SetexWithExpiry) {
    EXPECT_TRUE(conn.setex("expire_key", "temp_value", 600));
    auto result = conn.get("expire_key");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->value(), "temp_value");
}

TEST_F(RedisIntegrationTest, DelAndExists) {
    conn.set("del_key", "value");
    EXPECT_TRUE(conn.exists("del_key"));
    EXPECT_TRUE(conn.del("del_key"));
    EXPECT_FALSE(conn.exists("del_key"));
}

TEST_F(RedisIntegrationTest, Expire) {
    conn.set("exp_test", "value");
    EXPECT_TRUE(conn.expire("exp_test", 600));
}

TEST_F(RedisIntegrationTest, HashOperations) {
    EXPECT_TRUE(conn.hset("hash_key", "field1", "value1"));
    EXPECT_TRUE(conn.hset("hash_key", "field2", "value2"));

    auto result = conn.hget("hash_key", "field1");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->value(), "value1");

    result = conn.hgetall("hash_key");
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->value_hash().empty());
    EXPECT_EQ(result->value_hash().size(), 2);
}

TEST_F(RedisIntegrationTest, ListOperations) {
    EXPECT_TRUE(conn.lpush("list_key", "first"));
    EXPECT_TRUE(conn.rpush("list_key", "last"));
    EXPECT_TRUE(conn.lpush("list_key", "new_first"));

    auto result = conn.lrange("list_key", 0, -1);
    ASSERT_NE(result, nullptr);
    auto arr = result->value_array();
    EXPECT_EQ(arr.size(), 3);
}

TEST_F(RedisIntegrationTest, SetOperations) {
    EXPECT_TRUE(conn.sadd("set_key", "member1"));
    EXPECT_TRUE(conn.sadd("set_key", "member2"));

    auto result = conn.smembers("set_key");
    ASSERT_NE(result, nullptr);
    auto arr = result->value_array();
    EXPECT_EQ(arr.size(), 2);
}

TEST_F(RedisIntegrationTest, TransactionMultiExec) {
    EXPECT_TRUE(conn.begin());
    EXPECT_TRUE(conn.set("tx_key", "tx_value"));
    EXPECT_TRUE(conn.commit());
    auto result = conn.get("tx_key");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->value(), "tx_value");
}

TEST_F(RedisIntegrationTest, TransactionMultiDiscard) {
    EXPECT_TRUE(conn.begin());
    EXPECT_TRUE(conn.set("discard_key", "should_not_exist"));
    EXPECT_TRUE(conn.rollback());
    EXPECT_FALSE(conn.exists("discard_key"));
}

TEST_F(RedisIntegrationTest, QueryReturnsCorrectType) {
    conn.set("query_key", "query_value");
    auto result = conn.query("GET query_key");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->value(), "query_value");
}

TEST_F(RedisIntegrationTest, GetNonExistentKey) {
    auto result = conn.get("no_such_key");
    if (result != nullptr) {
        EXPECT_TRUE(dynamic_cast<redis_result*>(result.get())->is_nil());
    }
}

class RedisPoolTest : public ::testing::Test {
protected:
    db_config config;

    void SetUp() override {
        auto* json = load_db_config();
        config = make_config(json, "redis");
        if (config.database.empty()) {
            config = db_config::for_redis("0");
        }
    }
};

TEST_F(RedisPoolTest, AcquireAndOperate) {
    database_pool* pool;
    try {
        pool = new database_pool{db_type::REDIS, config, {2, 2, 8, seconds{60}, milliseconds{5000}}};
    } catch (const database_exception&) {
        GTEST_SKIP() << "Redis pool unavailable";
    }
    if (!pool->is_running()) {
        delete pool;
        GTEST_SKIP() << "Redis pool unavailable";
    }
    auto conn = pool->get_kv_connect();
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->is_valid());
    conn->set("pool_test", "ok");
    auto result = conn->get("pool_test");
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->next());
    EXPECT_EQ(result->value(), "ok");
    delete pool;
}
#endif
