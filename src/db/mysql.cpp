#include <MSTL/db/mysql.hpp>
#ifdef MSTL_SUPPORT_DB__
MSTL_BEGIN_NAMESPACE__
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

#endif // MSTL_SUPPORT_MYSQL__
MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_DB__
