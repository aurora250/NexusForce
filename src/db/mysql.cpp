#include <MSTL/db/mysql.hpp>
#ifdef MSTL_SUPPORT_MYSQL__
MSTL_BEGIN_NAMESPACE__

db_mysql_result::db_mysql_result(mysql::MYSQL_RES* result) noexcept
: result_(result), rows_(mysql::mysql_num_rows(result)), columns_(mysql::mysql_num_fields(result)) {
    mysql::MYSQL_FIELD* field;
    while ((field = mysql::mysql_fetch_field(result))) {
        column_name_->push_back(field->name);
        column_types_->push_back(field->type);
    }
}

db_mysql_result::~db_mysql_result() {
    mysql::mysql_free_result(result_);
    delete column_name_;
    delete column_types_;
}

bool db_mysql_result::next() noexcept {
    if (!empty()) {
        cursor_ = mysql::mysql_fetch_row(result_);
        return cursor_ != nullptr;
    }
    return false;
}

_MSTL string_view db_mysql_result::get(const size_type n) const noexcept {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return cursor_[n];
}

bool db_mysql_result::get_bool(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql::MYSQL_TYPE_BOOL)
        Exception(DatabaseTypeCastError("database type cast to bool mismatch"));
    return static_cast<bool>(boolean::parse(cursor_[n]));
}

int8_t db_mysql_result::get_int8(const size_type n) const {
    return static_cast<int8_t>(this->get_int16(n));
}

int16_t db_mysql_result::get_int16(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == mysql::MYSQL_TYPE_SHORT || type == mysql::MYSQL_TYPE_TINY || type == mysql::MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int16 mismatch"));
    return integer16::parse(cursor_[n]);
}

int32_t db_mysql_result::get_int32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == mysql::MYSQL_TYPE_LONG || type == mysql::MYSQL_TYPE_INT24 || type == mysql::MYSQL_TYPE_SHORT ||
        type == mysql::MYSQL_TYPE_TINY || type == mysql::MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int32 mismatch"));
    return integer32::parse(cursor_[n]);
}

int64_t db_mysql_result::get_int64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == mysql::MYSQL_TYPE_LONGLONG || type == mysql::MYSQL_TYPE_LONG || type == mysql::MYSQL_TYPE_INT24 ||
        type == mysql::MYSQL_TYPE_SHORT || type == mysql::MYSQL_TYPE_TINY || type == mysql::MYSQL_TYPE_BOOL))
        Exception(DatabaseTypeCastError("database type cast to int64 mismatch"));
    return integer64::parse(cursor_[n]);
}

float32_t db_mysql_result::get_float32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == mysql::MYSQL_TYPE_FLOAT || type == mysql::MYSQL_TYPE_LONG
        || type == mysql::MYSQL_TYPE_SHORT || type == mysql::MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to float32 mismatch"));
    return float32::parse(cursor_[n]);
}

float64_t db_mysql_result::get_float64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == mysql::MYSQL_TYPE_DOUBLE || type == mysql::MYSQL_TYPE_FLOAT || type == mysql::MYSQL_TYPE_LONGLONG
        || type == mysql::MYSQL_TYPE_LONG || type == mysql::MYSQL_TYPE_SHORT || type == mysql::MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to float64 mismatch"));
    return float64::parse(cursor_[n]);
}

decimal_t db_mysql_result::get_decimal(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == mysql::MYSQL_TYPE_DECIMAL || type == mysql::MYSQL_TYPE_NEWDECIMAL || type == mysql::MYSQL_TYPE_DOUBLE ||
        type == mysql::MYSQL_TYPE_FLOAT || type == mysql::MYSQL_TYPE_LONGLONG || type == mysql::MYSQL_TYPE_LONG ||
        type == mysql::MYSQL_TYPE_SHORT || type == mysql::MYSQL_TYPE_TINY))
        Exception(DatabaseTypeCastError("database type cast to decimal mismatch"));
    return decimal::parse(cursor_[n]);
}

_MSTL vector<char> db_mysql_result::get_blob(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!(type == mysql::MYSQL_TYPE_BLOB || type == mysql::MYSQL_TYPE_TINY_BLOB ||
        type == mysql::MYSQL_TYPE_MEDIUM_BLOB || type == mysql::MYSQL_TYPE_LONG_BLOB))
        Exception(DatabaseTypeCastError("database type cast to blob mismatch"));
    return {cursor_[n], cursor_[n] + mysql_fetch_lengths(result_)[n]};
}

_MSTL string db_mysql_result::get_set(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql::MYSQL_TYPE_SET) {
        Exception(DatabaseTypeCastError("database type cast to SET mismatch"));
    }
    return cursor_[n];
}

uint64_t db_mysql_result::get_bit(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql::MYSQL_TYPE_BIT) {
        Exception(DatabaseTypeCastError("database type cast to BIT mismatch"));
    }
    const unsigned long length = mysql_fetch_lengths(result_)[n];
    const char* data = cursor_[n];

    uint64_t value = 0;
    for (unsigned long i = 0; i < length; ++i) {
        value = (value << 8) | static_cast<byte_t>(data[i]);
    }
    return value;
}

_MSTL date db_mysql_result::get_date(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql::MYSQL_TYPE_DATE)
        Exception(DatabaseTypeCastError("database type cast to date mismatch"));
    return _MSTL date::parse(cursor_[n]);
}

_MSTL time db_mysql_result::get_time(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql::MYSQL_TYPE_DATE)
        Exception(DatabaseTypeCastError("database type cast to time mismatch"));
    return _MSTL time::parse(cursor_[n]);
}

_MSTL datetime db_mysql_result::get_datetime(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql::MYSQL_TYPE_DATETIME)
        Exception(DatabaseTypeCastError("database type cast to datetime mismatch"));
    return _MSTL datetime::parse(cursor_[n]);
}

_MSTL timestamp db_mysql_result::get_timestamp(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql::MYSQL_TYPE_TIMESTAMP)
        Exception(DatabaseTypeCastError("database type cast to timestamp mismatch"));
    return _MSTL timestamp(_MSTL datetime::parse(cursor_[n]));
}

bool db_mysql_connect::connect_to(
        const _MSTL string& user, const _MSTL string& password,
        const _MSTL string& dbname, const _MSTL string& ip,
        const uint32_t port, const _MSTL string& character_set) noexcept {
    const mysql::MYSQL* p = mysql::mysql_real_connect(mysql_, ip.c_str(), user.c_str(),
        password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (p == nullptr) return false;
    return this->set_character_set(character_set);
}

_MSTL unique_ptr<idb_tb_result> db_mysql_connect::query(const _MSTL string& sql) const noexcept {
    if (mysql::mysql_query(mysql_, sql.c_str())) return {};
    return make_unique<db_mysql_result>(mysql::mysql_store_result(mysql_));
}

bool db_mysql_connect::reset_connect(const db_connect_config& config) {
    if (connected()) {
        mysql::mysql_close(mysql_);
        mysql_ = mysql::mysql_init(nullptr);
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

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
