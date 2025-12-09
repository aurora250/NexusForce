#ifndef MSTL_CORE_FILE_TOML_TOML_VALUE_HPP__
#define MSTL_CORE_FILE_TOML_TOML_VALUE_HPP__
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/memory/unique_ptr.hpp"
#include "MSTL/core/interface/istringify.hpp"
#include "MSTL/core/time/datetime.hpp"
MSTL_BEGIN_NAMESPACE__

struct toml_exception final : value_exception {
    explicit toml_exception(
        const string& info = "TOML Parse Failed",
        const char *type = __type__)
    : value_exception(type, type), msg(info) {
        msg = info;
    }

    ~toml_exception() override = default;

    const char* what() const noexcept override {
        return msg.c_str();
    }

    string msg;
    static constexpr auto __type__ = "toml_exception";
};


class toml_value;
class toml_boolean;
class toml_integer;
class toml_float;
class toml_string;
class toml_datetime;
class toml_array;
class toml_table;

class MSTL_API toml_value : public istringify<toml_value> {
public:
    enum types {
        Boolean,
        Integer,
        Float,
        String,
        DateTime,
        Array,
        Table
    };

    virtual ~toml_value() = default;
    MSTL_NODISCARD virtual types type() const noexcept = 0;

    MSTL_NODISCARD virtual const toml_boolean* as_boolean() const noexcept { return nullptr; }
    MSTL_NODISCARD virtual const toml_integer* as_integer() const noexcept { return nullptr; }
    MSTL_NODISCARD virtual const toml_float* as_float() const noexcept { return nullptr; }
    MSTL_NODISCARD virtual const toml_string* as_string() const noexcept { return nullptr; }
    MSTL_NODISCARD virtual const toml_datetime* as_datetime() const noexcept { return nullptr; }
    MSTL_NODISCARD virtual const toml_array* as_array() const noexcept { return nullptr; }
    MSTL_NODISCARD virtual const toml_table* as_table() const noexcept { return nullptr; }

    MSTL_NODISCARD bool is_boolean() const noexcept { return type() == Boolean; }
    MSTL_NODISCARD bool is_integer() const noexcept { return type() == Integer; }
    MSTL_NODISCARD bool is_float() const noexcept { return type() == Float; }
    MSTL_NODISCARD bool is_string() const noexcept { return type() == String; }
    MSTL_NODISCARD bool is_datetime() const noexcept { return type() == DateTime; }
    MSTL_NODISCARD bool is_array() const noexcept { return type() == Array; }
    MSTL_NODISCARD bool is_table() const noexcept { return type() == Table; }

    MSTL_NODISCARD string to_string() const;
    MSTL_NODISCARD string to_document() const;
};

using toml_ptr = unique_ptr<toml_value>;


class MSTL_API toml_boolean final : public toml_value {
private:
    bool value;

public:
    explicit toml_boolean(const bool v) noexcept : value(v) {}
    MSTL_NODISCARD types type() const noexcept override { return Boolean; }
    MSTL_NODISCARD const toml_boolean* as_boolean() const noexcept override { return this; }
    MSTL_NODISCARD bool get_value() const noexcept { return value; }
};

class MSTL_API toml_integer final : public toml_value {
private:
    int64_t value;

public:
    explicit toml_integer(const int64_t v) noexcept : value(v) {}
    MSTL_NODISCARD types type() const noexcept override { return Integer; }
    MSTL_NODISCARD const toml_integer* as_integer() const noexcept override { return this; }
    MSTL_NODISCARD int64_t get_value() const noexcept { return value; }
};

class MSTL_API toml_float final : public toml_value {
private:
    double value;

public:
    explicit toml_float(const double v) noexcept : value(v) {}
    MSTL_NODISCARD types type() const noexcept override { return Float; }
    MSTL_NODISCARD const toml_float* as_float() const noexcept override { return this; }
    MSTL_NODISCARD double get_value() const noexcept { return value; }
};

class MSTL_API toml_string final : public toml_value {
public:
    enum string_type {
        Basic,          // "string"
        Literal,        // 'string'
        MultiBasic,     // """string"""
        MultiLiteral    // '''string'''
    };

private:
    string value;
    string_type str_type;

public:
    explicit toml_string(string v, const string_type t = Basic) noexcept
        : value(_MSTL move(v)), str_type(t) {}

    MSTL_NODISCARD types type() const noexcept override { return String; }
    MSTL_NODISCARD const toml_string* as_string() const noexcept override { return this; }
    MSTL_NODISCARD const string& get_value() const noexcept { return value; }
    MSTL_NODISCARD string_type get_string_type() const noexcept { return str_type; }
};

class MSTL_API toml_datetime final : public toml_value {
public:
    enum datetime_type {
        OffsetDateTime,    // 1979-05-27T07:32:00Z
        LocalDateTime,     // 1979-05-27T07:32:00
        LocalDate,         // 1979-05-27
        LocalTime          // 07:32:00
    };

private:
    datetime value;
    datetime_type dt_type;

public:
    explicit toml_datetime(const string_view v, const datetime_type type) noexcept
        : dt_type(type) {
        switch (dt_type) {
            case datetime_type::OffsetDateTime: {
                datetime dt;
                dt.try_parse_ISO_UTC(v);
                value = dt;
                break;
            }
            case datetime_type::LocalDateTime: {
                datetime dt;
                dt.try_parse_ISO(v);
                value = dt;
                break;
            }
            case datetime_type::LocalDate: {
                date d{};
                d.try_parse(v);
                value = d;
                break;
            }
            case datetime_type::LocalTime: default: {
                time t{};
                t.try_parse(v);
                value = t;
                break;
            }
        }
    }

    MSTL_NODISCARD types type() const noexcept override { return DateTime; }
    MSTL_NODISCARD const toml_datetime* as_datetime() const noexcept override { return this; }
    MSTL_NODISCARD const datetime& get_value() const noexcept { return value; }

    MSTL_NODISCARD string get_string_value() const noexcept {
        switch (dt_type) {
            case datetime_type::OffsetDateTime: {
                return value.to_string_ISO_UTC();
            }
            case datetime_type::LocalDateTime: {
                return value.to_string_ISO();
            }
            case datetime_type::LocalDate: {
                return value.date().to_string();
            }
            case datetime_type::LocalTime: default: {
                return value.time().to_string();
            }
        }
    }

    MSTL_NODISCARD datetime_type get_datetime_type() const noexcept { return dt_type; }
};

class MSTL_API toml_array final : public toml_value {
private:
    vector<toml_ptr> elements;

public:
    toml_array() = default;
    toml_array(const toml_array&) = delete;
    toml_array& operator=(const toml_array&) = delete;
    toml_array(toml_array&&) = default;
    toml_array& operator=(toml_array&&) = default;

    MSTL_NODISCARD types type() const noexcept override { return Array; }
    MSTL_NODISCARD const toml_array* as_array() const noexcept override { return this; }

    void add_element(unique_ptr<toml_value> value) {
        elements.emplace_back(_MSTL move(value));
    }

    MSTL_NODISCARD const toml_value* get_element(const size_t index) const noexcept {
        if (index < elements.size()) return elements[index].get();
        return nullptr;
    }

    MSTL_NODISCARD size_t size() const noexcept { return elements.size(); }
    MSTL_NODISCARD const vector<toml_ptr>& get_elements() const noexcept { return elements; }
};

class MSTL_API toml_table final : public toml_value {
private:
    unordered_map<string, toml_ptr> members{};
    bool is_inline_table = false;

public:
    toml_table() = default;
    explicit toml_table(const bool is_inline) : is_inline_table(is_inline) {}

    toml_table(const toml_table&) = delete;
    toml_table& operator=(const toml_table&) = delete;
    toml_table(toml_table&&) = default;
    toml_table& operator=(toml_table&&) = default;

    MSTL_NODISCARD types type() const noexcept override { return Table; }
    MSTL_NODISCARD const toml_table* as_table() const noexcept override { return this; }

    void add_member(const string& key, unique_ptr<toml_value> value) {
        members[key] = _MSTL move(value);
    }

    MSTL_NODISCARD const toml_value* get_member(const string& key) const {
        const auto it = members.find(key);
        if (it != members.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD toml_value* get_member(const string& key) {
        const auto it = members.find(key);
        if (it != members.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD bool has_member(const string& key) const {
        return members.find(key) != members.end();
    }

    MSTL_NODISCARD const unordered_map<string, toml_ptr>& get_members() const noexcept {
        return members;
    }

    MSTL_NODISCARD bool is_inline() const noexcept { return is_inline_table; }
    void set_inline(const bool is_inline) noexcept { is_inline_table = is_inline; }
};

MSTL_BEGIN_INNER__
string MSTL_API toml_value_to_string(const toml_value* value);
string MSTL_API toml_value_document(const toml_value* root);
MSTL_END_INNER__

MSTL_ALWAYS_INLINE_INLINE string to_string(const toml_value* value) {
    return _INNER toml_value_to_string(value);
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const toml_value& value) {
    return _INNER toml_value_to_string(&value);
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const toml_ptr& value) {
    return _INNER toml_value_to_string(value.get());
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string toml_value::to_string() const {
    return _INNER toml_value_to_string(this);
}


MSTL_ALWAYS_INLINE_INLINE string toml_document(const toml_value* value) {
    return _INNER toml_value_document(value);
}
MSTL_ALWAYS_INLINE_INLINE string toml_document(const toml_value& value) {
    return _INNER toml_value_document(&value);
}
MSTL_ALWAYS_INLINE_INLINE string toml_document(const toml_ptr& value) {
    return _INNER toml_value_document(value.get());
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string toml_value::to_document() const {
    return _INNER toml_value_document(this);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_TOML_TOML_VALUE_HPP__
