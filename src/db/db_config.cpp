#include <NeForce/db/db_config.hpp>
NEFORCE_BEGIN_NAMESPACE__

db_config db_config::for_postgresql(const string& db) {
    db_config config;
    config.port = ports::postgresql;
    config.database = db;
    config.charset = "utf8";
    config.username = "postgres";
    return config;
}

db_config db_config::for_mysql(const string& db) {
    db_config config;
    config.port = ports::mysql;
    config.database = db;
    config.charset = "utf8mb4";
    config.username = "root";
    return config;
}

db_config db_config::for_sqlite(const string& file) {
    db_config config;
    config.port = ports::undef;
    config.database = file;
    return config;
}

db_config db_config::for_redis(const string& db) {
    db_config config;
    config.port = ports::redis;
    config.database = db;
    return config;
}

NEFORCE_END_NAMESPACE__
