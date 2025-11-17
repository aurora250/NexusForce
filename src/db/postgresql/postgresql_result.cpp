#include <MSTL/db/postgresql/postgresql_result.hpp>
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include <MSTL/core/packages.hpp>
MSTL_BEGIN_NAMESPACE__

postgresql_tb_result::postgresql_tb_result(
    _MSTL_POSTGRESQL PGresult* result, const bool owns) noexcept
: result_(result), owns_result_(owns) {
    if (result_) {
        row_count_ = _MSTL_POSTGRESQL PQntuples(result_);
        column_count_ = _MSTL_POSTGRESQL PQnfields(result_);
    }
}

postgresql_tb_result::~postgresql_tb_result() noexcept {
    if (owns_result_ && result_) {
        _MSTL_POSTGRESQL PQclear(result_);
    }
}

void postgresql_tb_result::init_column_names() const {
    if (!column_names_.empty() || !result_) {
        return;
    }

    column_names_.reserve(column_count_);
    for (int i = 0; i < column_count_; ++i) {
        column_names_.emplace_back(PQfname(result_, i));
    }
}

bool postgresql_tb_result::is_null(const size_type index) const {
    if (static_cast<int>(index) >= column_count_) {
        Exception(DatabaseError("Column index out of range"));
    }
    return _MSTL_POSTGRESQL PQgetisnull(
        result_, current_row_, static_cast<int>(index)) != 0;
}

bool postgresql_tb_result::next() noexcept {
    if (current_row_ + 1 < row_count_) {
        ++current_row_;
        return true;
    }
    return false;
}

const vector<string_view>& postgresql_tb_result::column_names() const {
    this->init_column_names();
    return column_names_;
}

string_view postgresql_tb_result::get(const size_type index) const {
    if (is_null(index)) {
        return string_view();
    }
    const char* value = _MSTL_POSTGRESQL PQgetvalue(
        result_, current_row_, static_cast<int>(index));
    return string_view(value);
}

bool postgresql_tb_result::get_bool(const size_type index) const {
    return boolean::parse(get(index)).value();
}

int8_t postgresql_tb_result::get_int8(const size_type index) const {
    return static_cast<int8_t>(get_int16(index));
}

int16_t postgresql_tb_result::get_int16(const size_type index) const {
    return integer16::parse(get(index)).value();
}

int32_t postgresql_tb_result::get_int32(const size_type index) const {
    return integer32::parse(get(index)).value();
}

int64_t postgresql_tb_result::get_int64(const size_type index) const {
    return integer64::parse(get(index)).value();
}

float32_t postgresql_tb_result::get_float32(const size_type index) const {
    return float32::parse(get(index)).value();
}

float64_t postgresql_tb_result::get_float64(const size_type index) const {
    return float64::parse(get(index)).value();
}

decimal_t postgresql_tb_result::get_decimal(const size_type index) const {
    return decimal::parse(get(index)).value();
}

vector<char> postgresql_tb_result::get_blob(const size_type index) const {
    if (is_null(index)) {
        return {};
    }
    const char* value = _MSTL_POSTGRESQL PQgetvalue(
        result_, current_row_, static_cast<int>(index));
    const int length = _MSTL_POSTGRESQL PQgetlength(
        result_, current_row_, static_cast<int>(index));

    if (length > 2 && value[0] == '\\' && value[1] == 'x') {
        size_t unescaped_length = 0;
        byte_t* unescaped = _MSTL_POSTGRESQL PQunescapeBytea(
            reinterpret_cast<const byte_t*>(value),
            &unescaped_length
        );

        if (unescaped) {
            vector<char> result(unescaped, unescaped + unescaped_length);
            _MSTL_POSTGRESQL PQfreemem(unescaped);
            return result;
        }
    }

    return vector<char>(value, value + length);
}

uint64_t postgresql_tb_result::get_bit(const size_type index) const {
    return _MSTL strtoull(get(index).data(), nullptr, 2);
}

date postgresql_tb_result::get_date(const size_type index) const {
    return date::parse(get(index));
}

time postgresql_tb_result::get_time(const size_type index) const {
    return time::parse(get(index));
}

datetime postgresql_tb_result::get_datetime(const size_type index) const {
    return datetime::parse(get(index));
}

timestamp postgresql_tb_result::get_timestamp(const size_type index) const {
    return timestamp(get_datetime(index));
}

MSTL_END_NAMESPACE__
#endif
