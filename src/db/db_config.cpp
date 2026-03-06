#include <NeForce/db/db_config.hpp>
#ifdef NEFORCE_SUPPORT_DB
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_SUPPORT_POSTGRESQL
db_config db_config::for_postgresql(const string& db) {
    db_config config;
    config.port = 5432;
    config.database = db;
    config.charset = "utf8";
    config.username = "postgres";
    return config;
}
#endif

#ifdef NEFORCE_SUPPORT_MYSQL
db_config db_config::for_mysql(const string& db) {
    db_config config;
    config.port = 3306;
    config.database = db;
    config.charset = "utf8mb4";
    config.username = "root";
    return config;
}
#endif

#ifdef NEFORCE_SUPPORT_SQLITE3
db_config db_config::for_sqlite(const string& file) {
    db_config config;
    config.database = file;
    return config;
}
#endif

#ifdef NEFORCE_SUPPORT_HIREDIS
db_config db_config::for_redis(const string& db) {
    db_config config;
    config.port = 6379;
    config.database = db;
    return config;
}
#endif

NEFORCE_END_NAMESPACE__
#endif
