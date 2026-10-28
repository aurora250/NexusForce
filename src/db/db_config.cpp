#include <NeForce/db/db_config.hpp>
NEFORCE_BEGIN_NAMESPACE__

db_config db_config::for_postgresql(const string& db) {
    db_config config;
    config.port = ports::POSTGRESQL;
    config.database = db;
    config.charset = "utf8";
    config.username = "postgres";
    return config;
}

db_config db_config::for_mysql(const string& db) {
    db_config config;
    config.port = ports::MYSQL;
    config.database = db;
    config.charset = "utf8mb4";
    config.username = "root";
    return config;
}

db_config db_config::for_sqlite(const string& file) {
    db_config config;
    config.port = ports::UNDEF;
    config.database = file;
    return config;
}

db_config db_config::for_redis(const string& db) {
    db_config config;
    config.port = ports::REDIS;
    config.database = db;
    return config;
}

NEFORCE_END_NAMESPACE__
