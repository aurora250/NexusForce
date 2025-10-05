#include <MSTL/ext/database_pool.hpp>
#include <MSTL/core/object.hpp>
MSTL_BEGIN_NAMESPACE__
#ifdef MSTL_SUPPORT_DB__

#ifdef MSTL_SUPPORT_MYSQL__
db_connect_config db_connect_config::for_mysql(const string& db) {
    db_connect_config config;
    config.port = 3306;
    config.database = db;
    config.charset = "utf8mb4";
    config.username = "root";
    return config;
}
#endif

#ifdef MSTL_SUPPORT_SQLITE3__
db_connect_config db_connect_config::for_sqlite(const string& file) {
    db_connect_config config;
    config.database = file;
    return config;
}
#endif

#ifdef MSTL_SUPPORT_REDIS__
db_connect_config db_connect_config::for_redis(const string& db) {
    db_connect_config config;
    config.port = 6379;
    config.database = db;
    return config;
}
#endif


idb_factory::idb_factory(const db_connect_config& config) : config_(config) {}


#ifdef MSTL_SUPPORT_MYSQL__

db_mysql_result::db_mysql_result(::MYSQL_RES* result) noexcept
: result(result), rows(::mysql_num_rows(result)), columns(::mysql_num_fields(result)) {
    ::MYSQL_FIELD* field;
    while ((field = ::mysql_fetch_field(result))) {
        column_name_->push_back(field->name);
        column_types_->push_back(field->type);
    }
}

db_mysql_result::~db_mysql_result() {
    ::mysql_free_result(result);
    delete column_name_;
    delete column_types_;
}

bool db_mysql_result::empty() const noexcept {
    return result == nullptr;
}

db_mysql_result::size_type db_mysql_result::row_count() const noexcept {
    return rows;
}
db_mysql_result::size_type db_mysql_result::column_count() const noexcept {
    return columns;
}

const list<string_view>& db_mysql_result::column_names() const noexcept {
    return *column_name_;
}

decltype(auto) db_mysql_result::column_types() const noexcept {
    return const_cast<const list<enum_field_types>&>(*column_types_);
}

bool db_mysql_result::next() noexcept {
    if (!empty()) {
        cursor = mysql_fetch_row(result);
        return cursor != nullptr;
    }
    return false;
}

_MSTL string_view db_mysql_result::at(const size_type n) const noexcept {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    return cursor[n];
}

bool db_mysql_result::at_bool(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    if (column_types_->at(n) != MYSQL_TYPE_BOOL)
        Exception(DatabaseTypeCastError("database type cast to bool mismatch"));
    return static_cast<bool>(boolean::parse(cursor[n]));
}

int8_t db_mysql_result::at_int8(const size_type n) const {
    return static_cast<int8_t>(this->at_int16(n));
}

int16_t db_mysql_result::at_int16(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == MYSQL_TYPE_SHORT || type == MYSQL_TYPE_TINY || type == MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int16 mismatch"));
    return integer16::parse(cursor[n]);
}

int32_t db_mysql_result::at_int32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == MYSQL_TYPE_LONG || type == MYSQL_TYPE_INT24 || type == MYSQL_TYPE_SHORT ||
        type == MYSQL_TYPE_TINY || type == MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int32 mismatch"));
    return integer32::parse(cursor[n]);
}

int64_t db_mysql_result::at_int64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == MYSQL_TYPE_LONGLONG || type == MYSQL_TYPE_LONG || type == MYSQL_TYPE_INT24 ||
        type == MYSQL_TYPE_SHORT || type == MYSQL_TYPE_TINY || type == MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int64 mismatch"));
    return integer64::parse(cursor[n]);
}

float32_t db_mysql_result::at_float32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == MYSQL_TYPE_FLOAT || type == MYSQL_TYPE_LONG
        || type == MYSQL_TYPE_SHORT || type == MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to float32 mismatch"));
    return float32::parse(cursor[n]);
}

float64_t db_mysql_result::at_float64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == MYSQL_TYPE_DOUBLE || type == MYSQL_TYPE_FLOAT || type == MYSQL_TYPE_LONGLONG
        || type == MYSQL_TYPE_LONG || type == MYSQL_TYPE_SHORT || type == MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to float64 mismatch"));
    return float64::parse(cursor[n]);
}

decimal_t db_mysql_result::at_decimal(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == MYSQL_TYPE_DECIMAL || type == MYSQL_TYPE_NEWDECIMAL || type == MYSQL_TYPE_DOUBLE ||
        type == MYSQL_TYPE_FLOAT || type == MYSQL_TYPE_LONGLONG || type == MYSQL_TYPE_LONG ||
        type == MYSQL_TYPE_SHORT || type == MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to decimal mismatch"));
    return decimal::parse(cursor[n]);
}

_MSTL vector<char> db_mysql_result::at_blob(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == MYSQL_TYPE_BLOB || type == MYSQL_TYPE_TINY_BLOB ||
        type == MYSQL_TYPE_MEDIUM_BLOB || type == MYSQL_TYPE_LONG_BLOB))
        Exception(DatabaseTypeCastError("database type cast to blob mismatch"));
    return {cursor[n], cursor[n] + mysql_fetch_lengths(result)[n]};
}

_MSTL string db_mysql_result::at_set(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    if (column_types_->at(n) != MYSQL_TYPE_SET) {
        Exception(DatabaseTypeCastError("database type cast to SET mismatch"));
    }
    return cursor[n];
}

uint64_t db_mysql_result::at_bit(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    if (column_types_->at(n) != MYSQL_TYPE_BIT) {
        Exception(DatabaseTypeCastError("database type cast to BIT mismatch"));
    }
    const unsigned long length = mysql_fetch_lengths(result)[n];
    const char* data = cursor[n];

    uint64_t value = 0;
    for (unsigned long i = 0; i < length; ++i) {
        value = (value << 8) | static_cast<byte_t>(data[i]);
    }
    return value;
}

_MSTL date db_mysql_result::at_date(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    if (column_types_->at(n) != MYSQL_TYPE_DATE)
        Exception(DatabaseTypeCastError("database type cast to date mismatch"));
    return _MSTL date::parse(cursor[n]);
}

_MSTL time db_mysql_result::at_time(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    if (column_types_->at(n) != MYSQL_TYPE_DATE)
        Exception(DatabaseTypeCastError("database type cast to time mismatch"));
    return _MSTL time::parse(cursor[n]);
}

_MSTL datetime db_mysql_result::at_datetime(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    if (column_types_->at(n) != MYSQL_TYPE_DATETIME)
        Exception(DatabaseTypeCastError("database type cast to datetime mismatch"));
    return _MSTL datetime::parse(cursor[n]);
}

_MSTL timestamp db_mysql_result::at_timestamp(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor, "database_result_row_value can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns > n, "database_result_row_value out of ranges.")
    if (column_types_->at(n) != MYSQL_TYPE_TIMESTAMP)
        Exception(DatabaseTypeCastError("database type cast to timestamp mismatch"));
    return _MSTL timestamp(_MSTL datetime::parse(cursor[n]));
}

string db_mysql_result::at_string(const size_type n) const noexcept {
    return string{at(n)};
}

string_view db_mysql_result::at_enum(const size_type n) const noexcept {
    return at(n);
}

db_mysql_connect::db_mysql_connect() noexcept {
    mysql = mysql_init(nullptr);
}

db_mysql_connect::~db_mysql_connect() noexcept {
    this->close();
}

bool db_mysql_connect::connect_to(
        const _MSTL string& user, const _MSTL string& password,
        const _MSTL string& dbname, const _MSTL string& ip,
        const uint32_t port, const _MSTL string& character_set) noexcept {
    const ::MYSQL* p = mysql_real_connect(mysql, ip.c_str(), user.c_str(),
        password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (p == nullptr) return false;

    return this->set_character_set(character_set);
}

bool db_mysql_connect::connect_to(const db_connect_config& config) noexcept {
    return connect_to(
        config.username,
        config.password,
        config.database,
        config.host,
        config.port,
        config.charset
    );
}

bool db_mysql_connect::set_character_set(
        const _MSTL string& encoding) const noexcept {
    if (connected()) {
        if (mysql_set_character_set(mysql, encoding.data())) {
            return false;
        }
        return true;
    }
    return false;
}

string_view db_mysql_connect::get_character_set() const noexcept {
    return mysql_character_set_name(mysql);
}

bool db_mysql_connect::set_options(
        const mysql_option option, const _MSTL string& str) const noexcept {
    if (connected()) {
        if (mysql_options(mysql, option, str.c_str())) {
            return false;
        }
        return true;
    }
    return false;
}

string_view db_mysql_connect::get_error() const noexcept {
    return mysql_error(mysql);
}

uint32_t db_mysql_connect::get_errno() const noexcept {
    return mysql_errno(mysql);
}

bool db_mysql_connect::update(const _MSTL string& sql) const noexcept {
    if (mysql_query(mysql, sql.c_str())) {
        return false;
    }
    return true;
}

_MSTL unique_ptr<idb_result> db_mysql_connect::query(const _MSTL string& sql) const noexcept {
    if (mysql_query(mysql, sql.c_str())) {
        return {};
    }
    return make_unique<db_mysql_result>(mysql_store_result(mysql));
}

bool db_mysql_connect::connected() const noexcept {
    return mysql != nullptr;
}

bool db_mysql_connect::is_valid() const noexcept {
    return mysql_ping(mysql) == 0;
}

void db_mysql_connect::close() noexcept {
    if (connected()) {
        mysql_close(mysql);
    }
}

void db_mysql_connect::refresh_alive() noexcept {
    alive_time_ = std::clock();
}

db_mysql_connect::clock_type db_mysql_connect::get_alive() const noexcept {
    return std::clock() - alive_time_;
}

bool db_mysql_connect::reset_connect(const db_connect_config& config) {
    if (connected()) {
        mysql_close(mysql);
        mysql = mysql_init(nullptr);
        return connect_to(config);
    }
    return false;
}


db_mysql_factory::db_mysql_factory(const db_connect_config& config)
        : idb_factory(config) {}

idb_connect* db_mysql_factory::create_connect() {
    const auto conn = new db_mysql_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* db_mysql_factory::create_result(void* native_result) {
    return new db_mysql_result(static_cast<::MYSQL_RES*>(native_result));
}

#endif


#ifdef MSTL_SUPPORT_SQLITE3__

db_sqlite_result::db_sqlite_result(::sqlite3_stmt* statement) noexcept : stmt(statement) {
    if (stmt) {
        columns = ::sqlite3_column_count(stmt);
        for (int i = 0; i < columns; ++i) {
            column_names_->push_back(::sqlite3_column_name(stmt, i));
            column_types_->push_back(::sqlite3_column_type(stmt, i));
        }
    }
}

db_sqlite_result::~db_sqlite_result() {
    if (stmt) {
        ::sqlite3_finalize(stmt);
    }
    delete column_names_;
    delete column_types_;
}

db_sqlite_result::size_type db_sqlite_result::row_count() const noexcept {
    return 0;
}

db_sqlite_result::size_type db_sqlite_result::column_count() const noexcept {
    return columns;
}

bool db_sqlite_result::empty() const noexcept {
    return stmt == nullptr;
}

const list<string_view>& db_sqlite_result::column_names() const {
    return *column_names_;
}

bool db_sqlite_result::next() noexcept {
    if (!empty()) {
        if (::sqlite3_step(stmt) == SQLITE_ROW) {
            ++cursor_;
            return true;
        }
    }
    return false;
}

_MSTL string_view db_sqlite_result::at(const size_type n) const noexcept {
    const auto text = reinterpret_cast<const char*>(::sqlite3_column_text(stmt, n));
    return text ? string_view(text) : string_view{};
}

bool db_sqlite_result::at_bool(const size_type n) const {
    return ::sqlite3_column_int(stmt, n) != 0;
}

int8_t db_sqlite_result::at_int8(const size_type n) const {
    return static_cast<int8_t>(::sqlite3_column_int(stmt, n));
}

int16_t db_sqlite_result::at_int16(const size_type n) const {
    return static_cast<int16_t>(::sqlite3_column_int(stmt, n));
}

int32_t db_sqlite_result::at_int32(const size_type n) const {
    return ::sqlite3_column_int(stmt, n);
}

int64_t db_sqlite_result::at_int64(const size_type n) const {
    return ::sqlite3_column_int64(stmt, n);
}

float32_t db_sqlite_result::at_float32(const size_type n) const {
    return static_cast<float32_t>(::sqlite3_column_double(stmt, n));
}

float64_t db_sqlite_result::at_float64(const size_type n) const {
    return static_cast<float64_t>(::sqlite3_column_double(stmt, n));
}

decimal_t db_sqlite_result::at_decimal(const size_type n) const {
    return static_cast<decimal_t>(::sqlite3_column_double(stmt, n));
}

_MSTL vector<char> db_sqlite_result::at_blob(const size_type n) const {
    const string_view view = this->at(n);
    return vector<char>{view.data(), view.data() + view.size()};
}

_MSTL string db_sqlite_result::at_set(const size_type n) const {
    return at_string(n);
}

uint64_t db_sqlite_result::at_bit(const size_type n) const noexcept {
    const auto data = at(n);
    uint64_t value = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        value = (value << 8) | static_cast<byte_t>(data.at(i));
    }
    return value;
}

_MSTL date db_sqlite_result::at_date(const size_type n) const noexcept {
    return at_datetime(n).date();
}

_MSTL time db_sqlite_result::at_time(const size_type n) const noexcept {
    return at_datetime(n).time();
}

_MSTL datetime db_sqlite_result::at_datetime(const size_type n) const {
    const auto text = reinterpret_cast<const char*>(::sqlite3_column_text(stmt, n));
    if (text) {
        return _MSTL datetime::parse(text);
    }
    return _MSTL datetime{};
}

_MSTL timestamp db_sqlite_result::at_timestamp(const size_type n) const {
    return timestamp{static_cast<long>(::sqlite3_column_int64(stmt, n))};
}

_MSTL string db_sqlite_result::at_string(const size_type n) const {
    return string{at(n)};
}

_MSTL string_view db_sqlite_result::at_enum(const size_type n) const {
    return at(n);
}


db_sqlite_connect::db_sqlite_connect() noexcept {
    ::sqlite3_open(nullptr, &db);
}

db_sqlite_connect::~db_sqlite_connect() noexcept {
    this->close();
}

bool db_sqlite_connect::connect_to(const _MSTL string&, const _MSTL string&,
        const _MSTL string& dbname, const _MSTL string&,
        uint32_t, const _MSTL string&) {
    ::sqlite3_open(dbname.c_str(), &db);
    return connect_to_file(dbname);
}

bool db_sqlite_connect::connect_to(const db_connect_config& config) {
    ::sqlite3_open(config.database.c_str(), &db);
    return connect_to_file(config.database);
}

bool db_sqlite_connect::set_character_set(const _MSTL string& encoding) const {
    const string sql = "PRAGMA encoding = '" + encoding + "';";
    return update(sql);
}

_MSTL string_view db_sqlite_connect::get_character_set() const {
    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(db, "PRAGMA encoding;", -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    string_view encoding;
    if (::sqlite3_step(stmt) == SQLITE_ROW) {
        encoding = reinterpret_cast<const char*>(::sqlite3_column_text(stmt, 0));
    }
    ::sqlite3_finalize(stmt);
    return encoding;
}

_MSTL string_view db_sqlite_connect::get_error() const {
    if (db) {
        last_error_ = ::sqlite3_errmsg(db);
    }
    return last_error_;
}

uint32_t db_sqlite_connect::get_errno() const {
    return db ? ::sqlite3_errcode(db) : 0;
}

bool db_sqlite_connect::update(const string& sql) const {
    if (!connected()) return false;

    char* error_msg = nullptr;
    if (::sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_msg) != SQLITE_OK) {
        if (error_msg) {
            last_error_ = error_msg;
            ::sqlite3_free(error_msg);
        }
        return false;
    }
    return true;
}

_MSTL unique_ptr<idb_result> db_sqlite_connect::query(const string& sql) const {
    if (!connected()) return {};

    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    return make_unique<db_sqlite_result>(stmt);
}

bool db_sqlite_connect::connected() const {
    return db != nullptr;
}

bool db_sqlite_connect::is_valid() const {
    if (!connected()) return false;
    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(db, "SELECT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        ::sqlite3_finalize(stmt);
        return true;
    }
    return false;
}

void db_sqlite_connect::close() noexcept {
    if (db) {
        ::sqlite3_close(db);
    }
}

void db_sqlite_connect::refresh_alive() noexcept {
    alive_time_ = std::clock();
}

db_sqlite_connect::clock_type db_sqlite_connect::get_alive() const noexcept {
    return std::clock() - alive_time_;
}

bool db_sqlite_connect::reset_connect(const db_connect_config& config) {
    if (connected()) {
        ::sqlite3_close(db);
        return connect_to(config);
    }
    return false;
}

bool db_sqlite_connect::connect_to_file(const string& file_path) {
    if (connected()) {
        close();
    }

    if (::sqlite3_open(file_path.c_str(), &db) != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(db);
        close();
        return false;
    }
    return true;
}


db_sqlite_factory::db_sqlite_factory(const db_connect_config& config) : idb_factory(config) {}

idb_connect* db_sqlite_factory::create_connect() {
    auto conn = new db_sqlite_connect();
    if (!conn->connect_to(config_)) {
        return nullptr;
    }
    return conn;
}

idb_result* db_sqlite_factory::create_result(void* native_result) {
    return new db_sqlite_result(static_cast<::sqlite3_stmt*>(native_result));
}

#endif


#ifdef MSTL_SUPPORT_REDIS__

_MSTL string db_redis_result::format_redis_reply_element(::redisReply* element) {
    switch (element->type) {
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
            return {element->str, element->len};
        case REDIS_REPLY_INTEGER:
            return integer32(element->integer).to_string();
        case REDIS_REPLY_NIL:
            return {};
        case REDIS_REPLY_ARRAY: {
            string result;
            for (size_t i = 0; i < element->elements; ++i) {
                if (i > 0) result += " ";
                result += format_redis_reply_element(element->element[i]);
            }
            return result;
        }
        default:
            return "unsupported-type";
    }
}

db_redis_result::db_redis_result(::redisReply* reply) noexcept
    : reply_(reply) {
    if (reply_) {
        if (reply_->type == REDIS_REPLY_ARRAY) {
            is_array_ = true;
            rows_ = reply_->elements;
            column_names_.push_back("value");
        } else {
            rows_ = 1;
            column_names_.push_back("result");
        }
    }
}

db_redis_result::~db_redis_result() {
    if (reply_) {
        ::freeReplyObject(reply_);
    }
}

bool db_redis_result::empty() const noexcept {
    return !reply_ || rows_ == 0;
}

db_redis_result::size_type db_redis_result::row_count() const noexcept {
    return rows_;
}

db_redis_result::size_type db_redis_result::column_count() const noexcept {
    return 1;
}

const list<string_view>& db_redis_result::column_names() const noexcept {
    return column_names_;
}

bool db_redis_result::next() noexcept {
    if (empty() || cursor_ >= rows_) return false;
    ++cursor_;
    return cursor_ <= rows_;
}

string_view db_redis_result::at(size_type) const noexcept {
    return {at_string(0).data(), at_string(0).length()};
}

bool db_redis_result::at_bool(size_type) const {
    const string s = at_string(0);
    return s == "1" || s == "true" || s == "TRUE" || s == "yes";
}

int8_t db_redis_result::at_int8(size_type) const {
    return static_cast<int8_t>(this->at_int16(int()));
}

int16_t db_redis_result::at_int16(size_type) const {
    return integer16::parse(at_string(0).c_str());
}

int32_t db_redis_result::at_int32(size_type) const {
    return integer32::parse(at_string(0).c_str());
}

int64_t db_redis_result::at_int64(size_type) const {
    return integer64::parse(at_string(0).c_str());
}

float32_t db_redis_result::at_float32(size_type) const {
    return float32::parse(at_string(0).c_str());
}

float64_t db_redis_result::at_float64(size_type) const {
    return float64::parse(at_string(0).c_str());
}

decimal_t db_redis_result::at_decimal(size_type) const {
    return decimal::parse(at_string(0).c_str());
}

vector<char> db_redis_result::at_blob(size_type) const {
    const string s = at_string(0);
    return {s.begin(), s.end()};
}

string db_redis_result::at_set(size_type) const {
    return at_string(0);
}

uint64_t db_redis_result::at_bit(size_type) const {
    const string data = at_string(0);
    uint64_t value = 0;
    for (unsigned long i = 0; i < data.size(); ++i) {
        value = (value << 8) | static_cast<byte_t>(data[i]);
    }
    return value;
}

date db_redis_result::at_date(size_type) const {
    return date::parse(at_string(0));
}

time db_redis_result::at_time(size_type) const {
    return time::parse(at_string(0));
}

datetime db_redis_result::at_datetime(size_type) const {
    return datetime::parse(at_string(0));
}

timestamp db_redis_result::at_timestamp(size_type) const {
    return timestamp(integer64::parse(at_string(0).c_str()));
}

string db_redis_result::at_string(size_type) const {
    if (empty() || cursor_ == 0) return {};

    if (is_array_) {
        ::redisReply* element = reply_->element[cursor_ - 1];
        return format_redis_reply_element(element);
    }
    return format_redis_reply_element(reply_);
}

string_view db_redis_result::at_enum(size_type) const noexcept {
    return {at_string(0).c_str(), at_string(0).length()};
}


bool db_redis_connect::authenticate(const string& password) const {
    if (password.empty()) return true;
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, "AUTH %s", password.c_str()));
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Authentication failed";
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool db_redis_connect::select_database(const string& db_index) const {
    if (db_index.empty()) return true;
    try {
        const int db = integer32::parse(db_index.c_str());
        const auto reply = static_cast<::redisReply*>(::redisCommand(context_, "SELECT %d", db));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) {
                last_error_ = reply->str ? reply->str : "SELECT failed";
                ::freeReplyObject(reply);
            }
            return false;
        }
        ::freeReplyObject(reply);
        return true;
    } catch (...) {
        last_error_ = "Invalid database index";
        return false;
    }
}

bool db_redis_connect::connect_to_host(
    const string& host, const uint16_t port,
    const string& password, const string& dbname) {
    context_ = redisConnect(host.c_str(), port);
    if (!context_ || context_->err) {
        if (context_) {
            last_error_ = context_->errstr;
            ::redisFree(context_);
            context_ = nullptr;
        } else {
            last_error_ = "Connection failed";
        }
        return false;
    }
    if (!authenticate(password)) {
        close();
        return false;
    }
    if (!select_database(dbname)) {
        close();
        return false;
    }
    return true;
}


db_redis_connect::~db_redis_connect() {
    close();
}

bool db_redis_connect::connect_to(const string&, const string& password,
    const string& dbname, const string& host,
    const uint32_t port, const string&) {
    return connect_to_host(host, port, password, dbname);
}

bool db_redis_connect::connect_to(const db_connect_config& config) {
    return connect_to_host(config.host, config.port, config.password, config.database);
}

bool db_redis_connect::set_character_set(const string&) const noexcept {
    return true;
}
string_view db_redis_connect::get_character_set() const noexcept {
    return {};
}

string_view db_redis_connect::get_error() const noexcept {
    if (context_ && context_->errstr[0] != '\0') {
        last_error_ = context_->errstr;
    }
    return {last_error_.data(), last_error_.size()};
}

uint32_t db_redis_connect::get_errno() const noexcept {
    return context_ ? context_->err : 0;
}

bool db_redis_connect::update(const string& sql) const noexcept {
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, sql.c_str()));
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Command failed";
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_result> db_redis_connect::query(const string& sql) const {
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, sql.c_str()));
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Query failed";
            ::freeReplyObject(reply);
        }
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

bool db_redis_connect::connected() const noexcept {
    return context_ != nullptr && !context_->err;
}

bool db_redis_connect::is_valid() const noexcept {
    if (!connected()) return false;
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, "PING"));
    if (!reply || reply->type != REDIS_REPLY_STATUS ||
        string_compare(reply->str, "PONG") != 0) {
        if (reply) ::freeReplyObject(reply);
        return false;
        }
    ::freeReplyObject(reply);
    return true;
}

void db_redis_connect::close() noexcept {
    if (context_) {
        ::redisFree(context_);
        context_ = nullptr;
    }
}

void db_redis_connect::refresh_alive() noexcept {
    alive_time_ = std::clock();
}

db_redis_connect::clock_type db_redis_connect::get_alive() const noexcept {
    return std::clock() - alive_time_;
}

bool db_redis_connect::reset_connect(const db_connect_config& config) {
    close();
    return connect_to(config);
}


db_redis_factory::db_redis_factory(const db_connect_config& config) : idb_factory(config) {}

idb_connect* db_redis_factory::create_connect() {
    auto conn = new db_redis_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* db_redis_factory::create_result(void* native_result) {
    return new db_redis_result(static_cast<::redisReply*>(native_result));
}

#endif


void database_pool::produce_connect_task() {
    while (true) {
        if (!running_) break;
        std::unique_lock<std::mutex> lock(queue_mtx_);
        while (!connect_queue_.empty()) {
            cv_.wait(lock);
            if (!running_) break;
        }
        if (!running_) break;
        if (connect_queue_.size() < max_size_) {
            auto* p = factory_->create_connect();
            if (p != nullptr) {
                p->refresh_alive();
                connect_queue_.push(p);
            }
        }
        cv_.notify_all();
    }
}

void database_pool::scanner_connect_task() {
    while (true) {
        if (!running_) break;
        std::this_thread::sleep_for(std::chrono::seconds(max_idle_time_));
        if (!running_) break;
        std::unique_lock<std::mutex> lock(queue_mtx_);

        while (connect_queue_.size() > init_size_) {
            const idb_connect* ptr = connect_queue_.front();
            if (ptr->get_alive() >= max_idle_time_ * 1000) {
                connect_queue_.pop();
                delete ptr;
            }
            else
                break;
        }
    }
}

database_pool::database_pool(const DB_TYPE type, const db_connect_config& config,
        const size_t init_size, const size_t max_size,
        const size_t max_idle_time, const size_t connect_timeout) :
    config_(config), init_size_(init_size), max_size_(max_size), max_idle_time_(max_idle_time),
    connect_timeout_(connect_timeout), running_(true) {
    switch(type) {
#ifdef MSTL_SUPPORT_MYSQL__
        case DB_TYPE::MYSQL:
            factory_ = make_unique<db_mysql_factory>(config);
        break;
#endif
#ifdef MSTL_SUPPORT_SQLITE3__
        case DB_TYPE::SQLITE3:
            factory_ = make_unique<db_sqlite_factory>(config);
        break;
#endif
#ifdef MSTL_SUPPORT_REDIS__
        case DB_TYPE::REDIS:
            factory_ = make_unique<db_redis_factory>(config);
        break;
#endif
        default:
            // never run:
            Exception(DatabaseError("Useless Database Type"));
            break;
    }

    for (size_t i = 0; i < init_size_; i++) {
        auto* p = factory_->create_connect();
        if (p != nullptr) {
            p->refresh_alive();
            connect_queue_.push(p);
        }
        else {
            --i;
        }
    }
    produce_ = std::thread([this] { produce_connect_task(); });
    scanner_ = std::thread([this] { scanner_connect_task(); });
}

database_pool::~database_pool() {
    running_ = false;
    cv_.notify_all();

    if (produce_.joinable()) {
        produce_.join();
    }
    if (scanner_.joinable()) {
        scanner_.join();
    }

    while (!connect_queue_.empty()) {
        delete connect_queue_.front();
        connect_queue_.pop();
    }
}

_MSTL shared_ptr<idb_connect> database_pool::get_connect() {
    std::unique_lock<std::mutex> lock(queue_mtx_);

    while (connect_queue_.empty()) {
        if (cv_.wait_for(lock,
            std::chrono::milliseconds(connect_timeout_)) == std::cv_status::timeout) {
            if (connect_queue_.empty()) {
                if (connect_queue_.size() < max_size_) {
                    auto* new_conn = factory_->create_connect();
                    if (new_conn != nullptr) {
                        new_conn->refresh_alive();
                        connect_queue_.push(new_conn);
                        continue;
                    }
                }
                return nullptr;
            }
            }
    }

    idb_connect* raw_conn = connect_queue_.front();
    connect_queue_.pop();

    if (!raw_conn->is_valid()) {
        try {
            if (!raw_conn->reset_connect(config_)) {
                delete raw_conn;
                raw_conn = factory_->create_connect();
                if (raw_conn == nullptr) {
                    cv_.notify_all();
                    return nullptr;
                }
            }
        }
        catch (...) {
            delete raw_conn;
            cv_.notify_all();
            return nullptr;
        }
    }

    auto conn_ptr = _MSTL shared_ptr<idb_connect>(raw_conn,
        [this](idb_connect* p) {
            std::unique_lock<std::mutex> lock1(queue_mtx_);
            if (p->is_valid()) {
                p->refresh_alive();
                connect_queue_.push(p);
            }
            else {
                delete p;
            }
            cv_.notify_all();
        }
    );

    cv_.notify_all();
    return conn_ptr;
}

#endif
MSTL_END_NAMESPACE__
