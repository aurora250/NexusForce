#include <MSTL/db/sqlite.hpp>
#ifdef MSTL_SUPPORT_SQLITE3__
MSTL_BEGIN_NAMESPACE__

db_sqlite_result::db_sqlite_result(sqlite::sqlite3_stmt* statement) noexcept : stmt_(statement) {
    if (stmt_) {
        columns_ = sqlite::sqlite3_column_count(stmt_);
        for (int i = 0; i < columns_; ++i) {
            column_names_->push_back(sqlite::sqlite3_column_name(stmt_, i));
            column_types_->push_back(sqlite::sqlite3_column_type(stmt_, i));
        }
    }
}

db_sqlite_result::~db_sqlite_result() {
    if (stmt_) {
        sqlite::sqlite3_finalize(stmt_);
    }
    delete column_names_;
    delete column_types_;
}

bool db_sqlite_result::next() noexcept {
    if (empty()) return false;
    return sqlite::sqlite3_step(stmt_) == SQLITE_ROW && ++cursor_;
}

_MSTL string_view db_sqlite_result::get(const size_type n) const noexcept {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto text = reinterpret_cast<const char*>(sqlite::sqlite3_column_text(stmt_, n));
    return text ? string_view(text) : string_view{};
}

bool db_sqlite_result::get_bool(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return sqlite::sqlite3_column_int(stmt_, n) != 0;
}

int8_t db_sqlite_result::get_int8(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<int8_t>(sqlite::sqlite3_column_int(stmt_, n));
}

int16_t db_sqlite_result::get_int16(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<int16_t>(sqlite::sqlite3_column_int(stmt_, n));
}

int32_t db_sqlite_result::get_int32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return sqlite::sqlite3_column_int(stmt_, n);
}

int64_t db_sqlite_result::get_int64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return sqlite::sqlite3_column_int64(stmt_, n);
}

float32_t db_sqlite_result::get_float32(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<float32_t>(sqlite::sqlite3_column_double(stmt_, n));
}

float64_t db_sqlite_result::get_float64(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<float64_t>(sqlite::sqlite3_column_double(stmt_, n));
}

decimal_t db_sqlite_result::get_decimal(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return static_cast<decimal_t>(sqlite::sqlite3_column_double(stmt_, n));
}

_MSTL vector<char> db_sqlite_result::get_blob(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const string_view view = this->get(n);
    return vector<char>{view.data(), view.data() + view.size()};
}

uint64_t db_sqlite_result::get_bit(const size_type n) const noexcept {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto data = get(n);
    uint64_t value = 0;
    for (const char i : data) {
        value = value << 8 | static_cast<byte_t>(i);
    }
    return value;
}

_MSTL datetime db_sqlite_result::get_datetime(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto text = reinterpret_cast<const char*>(sqlite::sqlite3_column_text(stmt_, n));
    if (text) return _MSTL datetime::parse(text);
    return {};
}

_MSTL timestamp db_sqlite_result::get_timestamp(const size_type n) const {
    MSTL_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    MSTL_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return timestamp{static_cast<long>(sqlite::sqlite3_column_int64(stmt_, n))};
}

bool db_sqlite_connect::connect_to(const _MSTL string&, const _MSTL string&,
        const _MSTL string& dbname, const _MSTL string&,
        uint32_t, const _MSTL string&) {
    sqlite::sqlite3_open(dbname.c_str(), &db);
    return connect_to_file(dbname);
}

bool db_sqlite_connect::connect_to(const db_connect_config& config) {
    sqlite::sqlite3_open(config.database.c_str(), &db);
    return connect_to_file(config.database);
}

bool db_sqlite_connect::set_character_set(const _MSTL string& encoding) const {
    const string sql = "PRAGMA encoding = '" + encoding + "';";
    return update(sql);
}

_MSTL string_view db_sqlite_connect::get_character_set() const {
    sqlite::sqlite3_stmt* stmt = nullptr;
    if (sqlite::sqlite3_prepare_v2(db, "PRAGMA encoding;", -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    string_view encoding;
    if (sqlite::sqlite3_step(stmt) == SQLITE_ROW) {
        encoding = reinterpret_cast<const char*>(sqlite::sqlite3_column_text(stmt, 0));
    }
    sqlite::sqlite3_finalize(stmt);
    return encoding;
}

_MSTL string_view db_sqlite_connect::get_error() const {
    if (db) last_error_ = sqlite::sqlite3_errmsg(db);
    return last_error_;
}

bool db_sqlite_connect::update(const string& sql) const {
    if (!connected()) return false;

    char* error_msg = nullptr;
    if (sqlite::sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_msg) != SQLITE_OK) {
        if (error_msg) {
            last_error_ = error_msg;
            sqlite::sqlite3_free(error_msg);
        }
        return false;
    }
    return true;
}

_MSTL unique_ptr<idb_tb_result> db_sqlite_connect::query(const string& sql) const {
    if (!connected()) return {};

    sqlite::sqlite3_stmt* stmt = nullptr;
    if (sqlite::sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    return make_unique<db_sqlite_result>(stmt);
}

bool db_sqlite_connect::is_valid() const {
    if (!connected()) return false;
    sqlite::sqlite3_stmt* stmt = nullptr;
    if (sqlite::sqlite3_prepare_v2(db, "SELECT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite::sqlite3_finalize(stmt);
        return true;
    }
    return false;
}

bool db_sqlite_connect::reset_connect(const db_connect_config& config) {
    if (connected()) {
        sqlite::sqlite3_close(db);
        return connect_to(config);
    }
    return false;
}

bool db_sqlite_connect::connect_to_file(const string& file_path) {
    if (connected()) {
        close();
    }

    if (sqlite::sqlite3_open(file_path.c_str(), &db) != SQLITE_OK) {
        last_error_ = sqlite::sqlite3_errmsg(db);
        close();
        return false;
    }
    return true;
}

idb_connect* db_sqlite_factory::create_connect() {
    auto conn = new db_sqlite_connect();
    if (!conn->connect_to(config_)) {
        return nullptr;
    }
    return conn;
}

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_SQLITE3__
