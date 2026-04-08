#include <NeForce/db/pgsql/pgsql_result.hpp>
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

pgsql_tb_result::pgsql_tb_result(::PGresult* result, const bool owns) :
result_(result),
owns_result_(owns) {
    if (result_ != nullptr) {
        row_count_ = ::PQntuples(result_);
        column_count_ = ::PQnfields(result_);
    }
}

pgsql_tb_result::~pgsql_tb_result() {
    try {
        if (owns_result_ && result_ != nullptr) {
            ::PQclear(result_);
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

bool pgsql_tb_result::is_null(const size_type index) const {
    if (index >= column_count_) {
        NEFORCE_THROW_EXCEPTION(database_exception("Column index out of range"));
    }
    return ::PQgetisnull(result_, static_cast<int>(current_row_), static_cast<int>(index)) != 0;
}

bool pgsql_tb_result::next() noexcept {
    if (current_row_ + 1 < row_count_) {
        ++current_row_;
        return true;
    }
    return false;
}

const vector<string_view>& pgsql_tb_result::column_names() const {
    if (!column_names_.empty() || result_ == nullptr) {
        column_names_.reserve(column_count_);
        for (size_type i = 0; i < column_count_; ++i) {
            column_names_.emplace_back(::PQfname(result_, static_cast<int>(i)));
        }
    }
    return column_names_;
}

string_view pgsql_tb_result::get(const size_type index) const {
    if (is_null(index)) {
        return {};
    }
    return ::PQgetvalue(result_, static_cast<int>(current_row_), static_cast<int>(index));
}

bool pgsql_tb_result::get_bool(const size_type index) const { return boolean::parse(get(index)).value(); }

int16_t pgsql_tb_result::get_int16(const size_type index) const { return integer16::parse(get(index)).value(); }

int32_t pgsql_tb_result::get_int32(const size_type index) const { return integer32::parse(get(index)).value(); }

int64_t pgsql_tb_result::get_int64(const size_type index) const { return integer64::parse(get(index)).value(); }

float32_t pgsql_tb_result::get_float32(const size_type index) const { return float32::parse(get(index)).value(); }

float64_t pgsql_tb_result::get_float64(const size_type index) const { return float64::parse(get(index)).value(); }

decimal_t pgsql_tb_result::get_decimal(const size_type index) const { return decimal::parse(get(index)).value(); }

vector<char> pgsql_tb_result::get_blob(const size_type index) const {
    if (is_null(index)) {
        return {};
    }

    const string_view value = ::PQgetvalue(result_, static_cast<int>(current_row_), static_cast<int>(index));
    const int length = ::PQgetlength(result_, static_cast<int>(current_row_), static_cast<int>(index));

    if (length > 2 && value[0] == '\\' && value[1] == 'x') {
        size_t unescaped_length = 0;
        byte_t* unescaped = ::PQunescapeBytea(reinterpret_cast<const byte_t*>(value.data()), &unescaped_length);

        if (unescaped != nullptr) {
            vector<char> result(unescaped, unescaped + unescaped_length);
            ::PQfreemem(unescaped);
            return result;
        }
    }

    return {value.data(), value.data() + length};
}

uint64_t pgsql_tb_result::get_bit(const size_type index) const { return to_uint64(get(index).data(), nullptr, 2); }

date pgsql_tb_result::get_date(const size_type index) const { return date::parse(get(index)); }

time pgsql_tb_result::get_time(const size_type index) const { return time::parse(get(index)); }

datetime pgsql_tb_result::get_datetime(const size_type index) const { return datetime::parse(get(index)); }

timestamp pgsql_tb_result::get_timestamp(const size_type index) const { return timestamp(get_datetime(index)); }

NEFORCE_END_NAMESPACE__
#endif
