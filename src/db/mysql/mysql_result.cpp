#include <NeForce/db/mysql/mysql_result.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
#    include <mysql/mysql.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr bool is_blob_type(mysql_column_type type) noexcept {
        switch (type) {
            case mysql_column_type::tiny_blob:
            case mysql_column_type::medium_blob:
            case mysql_column_type::long_blob:
            case mysql_column_type::blob:
                return true;
            default:
                return false;
        }
    }

    constexpr bool is_int16_compatible(mysql_column_type type) noexcept {
        switch (type) {
            case mysql_column_type::short_:
            case mysql_column_type::tiny:
            case mysql_column_type::bool_:
                return true;
            default:
                return false;
        }
    }

    constexpr bool is_int32_compatible(mysql_column_type type) noexcept {
        switch (type) {
            case mysql_column_type::long_:
            case mysql_column_type::int24:
            case mysql_column_type::short_:
            case mysql_column_type::tiny:
            case mysql_column_type::bool_:
                return true;
            default:
                return false;
        }
    }

    constexpr bool is_int64_compatible(mysql_column_type type) noexcept {
        switch (type) {
            case mysql_column_type::longlong:
            case mysql_column_type::long_:
            case mysql_column_type::int24:
            case mysql_column_type::short_:
            case mysql_column_type::tiny:
            case mysql_column_type::bool_:
                return true;
            default:
                return false;
        }
    }

    constexpr bool is_float32_compatible(mysql_column_type type) noexcept {
        switch (type) {
            case mysql_column_type::float_:
            case mysql_column_type::long_:
            case mysql_column_type::short_:
            case mysql_column_type::tiny:
                return true;
            default:
                return false;
        }
    }

    constexpr bool is_float64_compatible(mysql_column_type type) noexcept {
        switch (type) {
            case mysql_column_type::double_:
            case mysql_column_type::float_:
            case mysql_column_type::longlong:
            case mysql_column_type::long_:
            case mysql_column_type::short_:
            case mysql_column_type::tiny:
                return true;
            default:
                return false;
        }
    }

    constexpr bool is_decimal_compatible(mysql_column_type type) noexcept {
        switch (type) {
            case mysql_column_type::decimal:
            case mysql_column_type::newdecimal:
            case mysql_column_type::double_:
            case mysql_column_type::float_:
            case mysql_column_type::longlong:
            case mysql_column_type::long_:
            case mysql_column_type::short_:
            case mysql_column_type::tiny:
                return true;
            default:
                return false;
        }
    }
} // namespace


mysql_result::mysql_result() :
column_name_(make_unique<vector<string_view>>()),
column_types_(make_unique<vector<mysql_column_type>>()) {}

mysql_result::mysql_result(void* result) :
result_(result),
rows_(::mysql_num_rows(static_cast<::MYSQL_RES*>(result_))),
columns_(::mysql_num_fields(static_cast<::MYSQL_RES*>(result_))),
column_name_(make_unique<vector<string_view>>()),
column_types_(make_unique<vector<mysql_column_type>>()) {
    const ::MYSQL_FIELD* field = nullptr;
    while ((field = ::mysql_fetch_field(static_cast<::MYSQL_RES*>(result_))) != nullptr) {
        column_name_->push_back(field->name);
        column_types_->push_back(static_cast<mysql_column_type>(field->type));
    }
}

mysql_result::~mysql_result() {
    if (result_ != nullptr) {
        ::mysql_free_result(static_cast<::MYSQL_RES*>(result_));
    }
}

bool mysql_result::next() noexcept {
    if (!empty()) {
        cursor_ = ::mysql_fetch_row(static_cast<::MYSQL_RES*>(result_));
        return cursor_ != nullptr;
    }
    return false;
}

string_view mysql_result::get(const size_type n) const noexcept {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    return cursor_[n];
}

bool mysql_result::get_bool(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql_column_type::bool_) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to bool mismatch"));
    }
    return boolean::parse(cursor_[n]).value();
}

int16_t mysql_result::get_int16(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!is_int16_compatible(type)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to int16 mismatch"));
    }
    return integer16::parse(cursor_[n]).value();
}

int32_t mysql_result::get_int32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!is_int32_compatible(type)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to int32 mismatch"));
    }
    return integer32::parse(cursor_[n]).value();
}

int64_t mysql_result::get_int64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!is_int64_compatible(type)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to int64 mismatch"));
    }
    return integer64::parse(cursor_[n]).value();
}

float32_t mysql_result::get_float32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!is_float32_compatible(type)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to float32 mismatch"));
    }
    return float32::parse(cursor_[n]).value();
}

float64_t mysql_result::get_float64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!is_float64_compatible(type)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to float64 mismatch"));
    }
    return float64::parse(cursor_[n]).value();
}

decimal_t mysql_result::get_decimal(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!is_decimal_compatible(type)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to decimal mismatch"));
    }
    return decimal::parse(cursor_[n]).value();
}

vector<char> mysql_result::get_blob(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    const auto type = column_types_->at(n);
    if (!is_blob_type(type)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to blob mismatch"));
    }
    return {cursor_[n], cursor_[n] + ::mysql_fetch_lengths(static_cast<::MYSQL_RES*>(result_))[n]};
}

uint64_t mysql_result::get_bit(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql_column_type::bit) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to BIT mismatch"));
    }
    const unsigned long length = mysql_fetch_lengths(static_cast<::MYSQL_RES*>(result_))[n];
    const char* data = cursor_[n];

    uint64_t value = 0;
    for (unsigned long i = 0; i < length; ++i) {
        value = (value << 8) | static_cast<byte_t>(data[i]);
    }
    return value;
}

date mysql_result::get_date(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql_column_type::date) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to date mismatch"));
    }
    return date::parse(cursor_[n]);
}

time mysql_result::get_time(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql_column_type::time) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to time mismatch"));
    }
    return time::parse(cursor_[n]);
}

datetime mysql_result::get_datetime(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql_column_type::datetime) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to datetime mismatch"));
    }
    return datetime::parse(cursor_[n]);
}

timestamp mysql_result::get_timestamp(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(cursor_, "index can`t dereference nullptr.")
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    if (column_types_->at(n) != mysql_column_type::timestamp) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("database type cast to timestamp mismatch"));
    }
    return timestamp(datetime::parse(cursor_[n]));
}

column_meta mysql_result::column_metadata(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(columns_ > n, "index out of ranges.")
    column_meta meta;
    meta.name = (*column_name_)[n];
    meta.type = static_cast<int32_t>((*column_types_)[n]);
    return meta;
}

NEFORCE_END_NAMESPACE__
#endif
