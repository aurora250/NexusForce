#include <NeForce/db/mysql/mysql_prepared_result.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
#    include <mysql/mysql.h>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr size_t get_buffer_size(const enum_field_types type) noexcept {
        switch (type) {
            case ::MYSQL_TYPE_TINY:
                return 1;
            case ::MYSQL_TYPE_SHORT:
            case ::MYSQL_TYPE_YEAR:
                return 2;
            case ::MYSQL_TYPE_INT24:
            case ::MYSQL_TYPE_LONG:
            case ::MYSQL_TYPE_FLOAT:
                return 4;
            case ::MYSQL_TYPE_LONGLONG:
            case ::MYSQL_TYPE_DOUBLE:
            case MYSQL_TYPE_BIT:
                return 8;
            case MYSQL_TYPE_NEWDECIMAL:
            case MYSQL_TYPE_DECIMAL:
                return 64;
            case ::MYSQL_TYPE_DATE:
            case ::MYSQL_TYPE_TIME:
            case ::MYSQL_TYPE_DATETIME:
            case ::MYSQL_TYPE_TIMESTAMP:
                return sizeof(::MYSQL_TIME);
            case ::MYSQL_TYPE_STRING:
            case ::MYSQL_TYPE_VAR_STRING:
            case ::MYSQL_TYPE_VARCHAR:
                return 4096;
            case ::MYSQL_TYPE_BLOB:
            case ::MYSQL_TYPE_TINY_BLOB:
            case ::MYSQL_TYPE_MEDIUM_BLOB:
            case ::MYSQL_TYPE_LONG_BLOB:
                return 65536;
            case ::MYSQL_TYPE_SET:
            case ::MYSQL_TYPE_ENUM:
                return 512;
            default:
                return MEMORY_BIG_ALLOC_THRESHHOLD;
        }
    }
} // namespace


mysql_prepared_result::mysql_prepared_result(void* stmt) :
stmt_(stmt) {
    if (stmt_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(database_stmt_exception("Invalid MYSQL_STMT pointer"));
    }

    metadata_ = ::mysql_stmt_result_metadata(static_cast<::MYSQL_STMT*>(stmt_));
    if (metadata_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(database_stmt_exception("No result metadata from prepared statement"));
    }

    column_count_ = ::mysql_num_fields(static_cast<::MYSQL_RES*>(metadata_));

    const auto* fields = ::mysql_fetch_fields(static_cast<::MYSQL_RES*>(metadata_));
    for (unsigned int i = 0; i < column_count_; ++i) {
        column_names_->push_back(string_view(fields[i].name));
        column_types_->push_back(static_cast<mysql_column_type>(fields[i].type));
    }

    initialize_bindings();

    if (::mysql_stmt_bind_result(static_cast<::MYSQL_STMT*>(stmt_),
                                 static_cast<::MYSQL_BIND*>(*bind_results_->data()))) {
        NEFORCE_THROW_EXCEPTION(database_stmt_exception(mysql_stmt_error(static_cast<::MYSQL_STMT*>(stmt_))));
    }
    if (::mysql_stmt_store_result(static_cast<::MYSQL_STMT*>(stmt_)) != 0) {
        NEFORCE_THROW_EXCEPTION(database_stmt_exception(mysql_stmt_error(static_cast<::MYSQL_STMT*>(stmt_))));
    }

    row_count_ = ::mysql_stmt_num_rows(static_cast<::MYSQL_STMT*>(stmt_));
}

mysql_prepared_result::~mysql_prepared_result() {
    if (metadata_ != nullptr) {
        ::mysql_free_result(static_cast<::MYSQL_RES*>(metadata_));
        metadata_ = nullptr;
    }
}

void mysql_prepared_result::initialize_bindings() const {
    bind_results_->resize(column_count_);
    buffers_->resize(column_count_);
    lengths_->resize(column_count_);
    is_null_->resize(column_count_);
    is_error_->resize(column_count_);

    const ::MYSQL_FIELD* fields = ::mysql_fetch_fields(static_cast<::MYSQL_RES*>(metadata_));

    for (uint32_t i = 0; i < column_count_; ++i) {
        auto& buffer = (*buffers_)[i];
        memory_zero(static_cast<::MYSQL_BIND*>((*bind_results_)[i]));

        const size_t buffer_size = get_buffer_size(fields[i].type);
        buffer.resize(buffer_size);

        static_cast<::MYSQL_BIND*>((*bind_results_)[i])->buffer_type = fields[i].type;
        static_cast<::MYSQL_BIND*>((*bind_results_)[i])->buffer = buffer.data();
        static_cast<::MYSQL_BIND*>((*bind_results_)[i])->buffer_length = buffer_size;
        static_cast<::MYSQL_BIND*>((*bind_results_)[i])->length = &(*lengths_)[i];
        static_cast<::MYSQL_BIND*>((*bind_results_)[i])->is_null = &(*is_null_)[i];
        static_cast<::MYSQL_BIND*>((*bind_results_)[i])->error = &(*is_error_)[i];
    }
}

bool mysql_prepared_result::next() {
    const int ret = ::mysql_stmt_fetch(static_cast<::MYSQL_STMT*>(stmt_));

    if (ret == 0) {
        has_current_row_ = true;
        return true;
    }
    if (ret == MYSQL_DATA_TRUNCATED) {
        has_current_row_ = true;
        for (unsigned int i = 0; i < column_count_; ++i) {
            if ((*is_error_)[i]) {
                (*buffers_)[i].resize((*lengths_)[i]);
                static_cast<::MYSQL_BIND*>((*bind_results_)[i])->buffer = (*buffers_)[i].data();
                static_cast<::MYSQL_BIND*>((*bind_results_)[i])->buffer_length = (*lengths_)[i];
                ::mysql_stmt_fetch_column(static_cast<::MYSQL_STMT*>(stmt_),
                                          static_cast<::MYSQL_BIND*>((*bind_results_)[i]), i, 0);
            }
        }
        return true;
    }
    has_current_row_ = false;
    return false;
}

string_view mysql_prepared_result::get(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if ((*is_null_)[n]) {
        return {};
    }
    return {(*buffers_)[n].data(), (*lengths_)[n]};
}

bool mysql_prepared_result::get_bool(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != mysql_column_type::tiny) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to bool mismatch"));
    }
    if ((*is_null_)[n]) {
        return false;
    }
    return *reinterpret_cast<const int8_t*>((*buffers_)[n].data()) != 0;
}

int16_t mysql_prepared_result::get_int16(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != mysql_column_type::short_ && type != mysql_column_type::tiny) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to int16 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }
    if (type == mysql_column_type::tiny) {
        return *reinterpret_cast<const int8_t*>((*buffers_)[n].data());
    }
    return *reinterpret_cast<const int16_t*>((*buffers_)[n].data());
}

int32_t mysql_prepared_result::get_int32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != mysql_column_type::long_ && type != mysql_column_type::int24 && type != mysql_column_type::short_ &&
        type != mysql_column_type::tiny) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to int32 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    if (type == mysql_column_type::tiny) {
        return *reinterpret_cast<const int8_t*>((*buffers_)[n].data());
    }
    if (type == mysql_column_type::short_) {
        return *reinterpret_cast<const int16_t*>((*buffers_)[n].data());
    }
    return *reinterpret_cast<const int32_t*>((*buffers_)[n].data());
}

int64_t mysql_prepared_result::get_int64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != mysql_column_type::longlong && type != mysql_column_type::long_ && type != mysql_column_type::int24 &&
        type != mysql_column_type::short_ && type != mysql_column_type::tiny) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to int64 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    if (type == mysql_column_type::tiny) {
        return *reinterpret_cast<const int8_t*>((*buffers_)[n].data());
    } else if (type == mysql_column_type::short_) {
        return *reinterpret_cast<const int16_t*>((*buffers_)[n].data());
    } else if (type == mysql_column_type::long_ || type == mysql_column_type::int24) {
        return *reinterpret_cast<const int32_t*>((*buffers_)[n].data());
    } else {
        return *reinterpret_cast<const int64_t*>((*buffers_)[n].data());
    }
}

float32_t mysql_prepared_result::get_float32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != mysql_column_type::float_) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to float32 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    return *reinterpret_cast<const float*>((*buffers_)[n].data());
}

float64_t mysql_prepared_result::get_float64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != mysql_column_type::double_ && type != mysql_column_type::float_) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to float64 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    if (type == mysql_column_type::float_) {
        return *reinterpret_cast<const float*>((*buffers_)[n].data());
    } else {
        return *reinterpret_cast<const double*>((*buffers_)[n].data());
    }
}

decimal_t mysql_prepared_result::get_decimal(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != mysql_column_type::decimal && type != mysql_column_type::newdecimal) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to decimal mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    return decimal::parse({(*buffers_)[n].data(), (*lengths_)[n]}).value();
}

vector<char> mysql_prepared_result::get_blob(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != mysql_column_type::blob && type != mysql_column_type::tiny_blob &&
        type != mysql_column_type::medium_blob && type != mysql_column_type::long_blob) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to blob mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    return {(*buffers_)[n].begin(), (*buffers_)[n].begin() + (*lengths_)[n]};
}

uint64_t mysql_prepared_result::get_bit(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != mysql_column_type::bit) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to BIT mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    uint64_t value = 0;
    for (unsigned long i = 0; i < (*lengths_)[n]; ++i) {
        value = (value << 8) | static_cast<byte_t>((*buffers_)[n][i]);
    }
    return value;
}

date mysql_prepared_result::get_date(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != mysql_column_type::date) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to date mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    const auto* mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
    using date_type = date::date_type;
    return date(static_cast<date_type>(mt->year), static_cast<date_type>(mt->month), static_cast<date_type>(mt->day));
}

time mysql_prepared_result::get_time(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != mysql_column_type::time) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to time mismatch"));
    }

    if ((*is_null_)[n]) {
        return time{};
    }

    const auto* mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
    using time_type = time::time_type;
    return time(static_cast<time_type>(mt->hour), static_cast<time_type>(mt->minute),
                static_cast<time_type>(mt->second));
}

datetime mysql_prepared_result::get_datetime(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != mysql_column_type::datetime) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to datetime mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    const auto* mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
    using date_type = date::date_type;
    using time_type = time::time_type;
    return datetime(
            date(static_cast<date_type>(mt->year), static_cast<date_type>(mt->month), static_cast<date_type>(mt->day)),
            time(static_cast<time_type>(mt->hour), static_cast<time_type>(mt->minute),
                 static_cast<time_type>(mt->second)));
}

timestamp mysql_prepared_result::get_timestamp(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != mysql_column_type::timestamp) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to timestamp mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    const auto* mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
    using date_type = date::date_type;
    using time_type = time::time_type;
    return timestamp(datetime(
            date(static_cast<date_type>(mt->year), static_cast<date_type>(mt->month), static_cast<date_type>(mt->day)),
            time(static_cast<time_type>(mt->hour), static_cast<time_type>(mt->minute),
                 static_cast<time_type>(mt->second))));
}

column_meta mysql_prepared_result::column_metadata(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    column_meta meta;
    meta.name = (*column_names_)[n];
    meta.type = static_cast<int32_t>((*column_types_)[n]);
    return meta;
}

NEFORCE_END_NAMESPACE__
#endif
