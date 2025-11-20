#include <MSTL/database/db_config.hpp>
#ifdef MSTL_SUPPORT_DB__
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_SUPPORT_POSTGRESQL__
db_config db_config::for_postgresql(const string& db) {
    db_config config;
    config.port = 5432;
    config.database = db;
    config.charset = "utf8";
    config.username = "postgres";
    return config;
}
#endif

#ifdef MSTL_SUPPORT_MYSQL__
db_config db_config::for_mysql(const string& db) {
    db_config config;
    config.port = 3306;
    config.database = db;
    config.charset = "utf8mb4";
    config.username = "root";
    return config;
}
#endif

#ifdef MSTL_SUPPORT_SQLITE3__
db_config db_config::for_sqlite(const string& file) {
    db_config config;
    config.database = file;
    return config;
}
#endif

#ifdef MSTL_SUPPORT_REDIS__
db_config db_config::for_redis(const string& db) {
    db_config config;
    config.port = 6379;
    config.database = db;
    return config;
}
#endif

MSTL_END_NAMESPACE__
#endif
