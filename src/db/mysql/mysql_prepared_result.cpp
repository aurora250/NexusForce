#include <NeForce/db/mysql/mysql_prepared_result.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
NEFORCE_BEGIN_NAMESPACE__

namespace {
    size_t get_buffer_size(const enum_field_types type) {
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


mysql_prepared_result::mysql_prepared_result(::MYSQL_STMT* stmt) :
stmt_(stmt) {
    if (stmt_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(database_prepared_stmt_exception("Invalid MYSQL_STMT pointer"));
    }

    metadata_ = ::mysql_stmt_result_metadata(stmt_);
    if (metadata_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(database_prepared_stmt_exception("No result metadata from prepared statement"));
    }

    column_count_ = ::mysql_num_fields(metadata_);

    const ::MYSQL_FIELD* fields = ::mysql_fetch_fields(metadata_);
    for (unsigned int i = 0; i < column_count_; ++i) {
        column_names_->push_back(string_view(fields[i].name));
        column_types_->push_back(fields[i].type);
    }

    initialize_bindings();

    if (::mysql_stmt_bind_result(stmt_, bind_results_->data()) != 0) {
        NEFORCE_THROW_EXCEPTION(database_prepared_stmt_exception(mysql_stmt_error(stmt_)));
    }
    if (::mysql_stmt_store_result(stmt_) != 0) {
        NEFORCE_THROW_EXCEPTION(database_prepared_stmt_exception(mysql_stmt_error(stmt_)));
    }

    row_count_ = ::mysql_stmt_num_rows(stmt_);
}

mysql_prepared_result::~mysql_prepared_result() {
    if (metadata_ != nullptr) {
        ::mysql_free_result(metadata_);
        metadata_ = nullptr;
    }
}

void mysql_prepared_result::initialize_bindings() const {
    bind_results_->resize(column_count_);
    buffers_->resize(column_count_);
    lengths_->resize(column_count_);
    is_null_->resize(column_count_);
    is_error_->resize(column_count_);

    const MYSQL_FIELD* fields = ::mysql_fetch_fields(metadata_);

    for (uint32_t i = 0; i < column_count_; ++i) {
        memory_zero(&(*bind_results_)[i]);

        const size_t buffer_size = get_buffer_size(fields[i].type);
        (*buffers_)[i].resize(buffer_size);

        (*bind_results_)[i].buffer_type = fields[i].type;
        (*bind_results_)[i].buffer = (*buffers_)[i].data();
        (*bind_results_)[i].buffer_length = buffer_size;
        (*bind_results_)[i].length = &(*lengths_)[i];
        (*bind_results_)[i].is_null = &(*is_null_)[i];
        (*bind_results_)[i].error = &(*is_error_)[i];
    }
}

bool mysql_prepared_result::next() {
    const int ret = ::mysql_stmt_fetch(stmt_);

    if (ret == 0) {
        has_current_row_ = true;
        return true;
    }
    if (ret == MYSQL_DATA_TRUNCATED) {
        has_current_row_ = true;
        for (unsigned int i = 0; i < column_count_; ++i) {
            if ((*is_error_)[i]) {
                (*buffers_)[i].resize((*lengths_)[i]);
                (*bind_results_)[i].buffer = (*buffers_)[i].data();
                (*bind_results_)[i].buffer_length = (*lengths_)[i];
                ::mysql_stmt_fetch_column(stmt_, &(*bind_results_)[i], i, 0);
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
    if (column_types_->at(n) != ::MYSQL_TYPE_TINY) {
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
    if (type != ::MYSQL_TYPE_SHORT && type != ::MYSQL_TYPE_TINY) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to int16 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }
    if (type == ::MYSQL_TYPE_TINY) {
        return *reinterpret_cast<const int8_t*>((*buffers_)[n].data());
    }
    return *reinterpret_cast<const int16_t*>((*buffers_)[n].data());
}

int32_t mysql_prepared_result::get_int32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (!(type == ::MYSQL_TYPE_LONG || type == ::MYSQL_TYPE_INT24 || type == ::MYSQL_TYPE_SHORT ||
          type == ::MYSQL_TYPE_TINY)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to int32 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    if (type == ::MYSQL_TYPE_TINY) {
        return *reinterpret_cast<const int8_t*>((*buffers_)[n].data());
    }
    if (type == ::MYSQL_TYPE_SHORT) {
        return *reinterpret_cast<const int16_t*>((*buffers_)[n].data());
    }
    return *reinterpret_cast<const int32_t*>((*buffers_)[n].data());
}

int64_t mysql_prepared_result::get_int64(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (!(type == ::MYSQL_TYPE_LONGLONG || type == ::MYSQL_TYPE_LONG || type == ::MYSQL_TYPE_INT24 ||
          type == ::MYSQL_TYPE_SHORT || type == ::MYSQL_TYPE_TINY)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to int64 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    if (type == ::MYSQL_TYPE_TINY) {
        return *reinterpret_cast<const int8_t*>((*buffers_)[n].data());
    } else if (type == ::MYSQL_TYPE_SHORT) {
        return *reinterpret_cast<const int16_t*>((*buffers_)[n].data());
    } else if (type == ::MYSQL_TYPE_LONG || type == ::MYSQL_TYPE_INT24) {
        return *reinterpret_cast<const int32_t*>((*buffers_)[n].data());
    } else {
        return *reinterpret_cast<const int64_t*>((*buffers_)[n].data());
    }
}

float32_t mysql_prepared_result::get_float32(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (type != ::MYSQL_TYPE_FLOAT) {
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
    if (!(type == ::MYSQL_TYPE_DOUBLE || type == ::MYSQL_TYPE_FLOAT)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to float64 mismatch"));
    }

    if ((*is_null_)[n]) {
        return 0;
    }

    if (type == ::MYSQL_TYPE_FLOAT) {
        return *reinterpret_cast<const float*>((*buffers_)[n].data());
    } else {
        return *reinterpret_cast<const double*>((*buffers_)[n].data());
    }
}

decimal_t mysql_prepared_result::get_decimal(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    const auto type = column_types_->at(n);
    if (!(type == ::MYSQL_TYPE_DECIMAL || type == ::MYSQL_TYPE_NEWDECIMAL)) {
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
    if (!(type == ::MYSQL_TYPE_BLOB || type == ::MYSQL_TYPE_TINY_BLOB || type == ::MYSQL_TYPE_MEDIUM_BLOB ||
          type == ::MYSQL_TYPE_LONG_BLOB)) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to blob mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    return vector<char>((*buffers_)[n].begin(), (*buffers_)[n].begin() + (*lengths_)[n]);
}

uint64_t mysql_prepared_result::get_bit(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != ::MYSQL_TYPE_BIT) {
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
    if (column_types_->at(n) != ::MYSQL_TYPE_DATE) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to date mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    const auto mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
    using date_type = date::date_type;
    return date(static_cast<date_type>(mt->year), static_cast<date_type>(mt->month), static_cast<date_type>(mt->day));
}

time mysql_prepared_result::get_time(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != ::MYSQL_TYPE_TIME) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to time mismatch"));
    }

    if ((*is_null_)[n]) {
        return time{};
    }

    const auto mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
    using time_type = time::time_type;
    return time(static_cast<time_type>(mt->hour), static_cast<time_type>(mt->minute),
                static_cast<time_type>(mt->second));
}

datetime mysql_prepared_result::get_datetime(const size_type n) const {
    NEFORCE_DEBUG_VERIFY(has_current_row_, "No current row to fetch data from")
    NEFORCE_DEBUG_VERIFY(n < column_count_, "Column index out of range")
    if (column_types_->at(n) != ::MYSQL_TYPE_DATETIME) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to datetime mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    const auto mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
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
    if (column_types_->at(n) != ::MYSQL_TYPE_TIMESTAMP) {
        NEFORCE_THROW_EXCEPTION(database_typecast_exception("Database type cast to timestamp mismatch"));
    }

    if ((*is_null_)[n]) {
        return {};
    }

    const auto mt = reinterpret_cast<const ::MYSQL_TIME*>((*buffers_)[n].data());
    using date_type = date::date_type;
    using time_type = time::time_type;
    return timestamp(datetime(
            date(static_cast<date_type>(mt->year), static_cast<date_type>(mt->month), static_cast<date_type>(mt->day)),
            time(static_cast<time_type>(mt->hour), static_cast<time_type>(mt->minute),
                 static_cast<time_type>(mt->second))));
}

NEFORCE_END_NAMESPACE__
#endif
