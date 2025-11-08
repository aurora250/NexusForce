#include <MSTL/db/sqlite.hpp>
#ifdef MSTL_SUPPORT_DB__
MSTL_BEGIN_NAMESPACE__
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
    return at_datetime(n).dates();
}

_MSTL time db_sqlite_result::at_time(const size_type n) const noexcept {
    return at_datetime(n).times();
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

#endif // MSTL_SUPPORT_SQLITE3__
MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_DB__
