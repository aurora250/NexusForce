#ifndef MSTL_CORE_JSON_JSON_VALUE_HPP__
#define MSTL_CORE_JSON_JSON_VALUE_HPP__
#include "../container/unordered_map.hpp"
#include "../container/vector.hpp"
#include "../memory/unique_ptr.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(json_exception, value_exception, "Json String Parse Failed")

class json_value;
class json_null;
class json_bool;
class json_number;
class json_string;
class json_object;
class json_array;


class MSTL_API json_value : public istringify<json_value> {
public:
    enum types {
        Null,
        Bool,
        Number,
        String,
        Object,
        Array
    };

    virtual ~json_value() = default;
    virtual types type() const noexcept = 0;

    virtual const json_null* as_null() const noexcept { return nullptr; }
    virtual const json_bool* as_bool() const noexcept { return nullptr; }
    virtual const json_number* as_number() const noexcept { return nullptr; }
    virtual const json_string* as_string() const noexcept { return nullptr; }
    virtual const json_object* as_object() const noexcept { return nullptr; }
    virtual const json_array* as_array() const noexcept { return nullptr; }

    MSTL_NODISCARD bool is_null() const noexcept { return type() == Null; }
    MSTL_NODISCARD bool is_bool() const noexcept { return type() == Bool; }
    MSTL_NODISCARD bool is_number() const noexcept { return type() == Number; }
    MSTL_NODISCARD bool is_string() const noexcept { return type() == String; }
    MSTL_NODISCARD bool is_object() const noexcept { return type() == Object; }
    MSTL_NODISCARD bool is_array() const noexcept { return type() == Array; }

    MSTL_NODISCARD string to_string() const;
};


class MSTL_API json_null final : public json_value {
public:
    types type() const noexcept override { return Null; }
    const json_null* as_null() const noexcept override { return this; }
};

class MSTL_API json_bool final : public json_value {
private:
    bool value;

public:
    explicit json_bool(bool v) noexcept : value(v) {}
    types type() const noexcept override { return Bool; }
    const json_bool* as_bool() const noexcept override { return this; }
    bool get_value() const noexcept { return value; }
};

class MSTL_API json_number final : public json_value {
private:
    double value;

public:
    explicit json_number(double v) noexcept : value(v) {}
    types type() const noexcept override { return Number; }
    const json_number* as_number() const noexcept override { return this; }
    double get_value() const noexcept { return value; }
};

class MSTL_API json_string final : public json_value {
private:
    string value;

public:
    explicit json_string(string v) noexcept : value(_MSTL move(v)) {}
    types type() const noexcept override { return String; }
    const json_string* as_string() const noexcept override { return this; }
    const string& get_value() const noexcept { return value; }
};

class MSTL_API json_object final : public json_value {
private:
    unordered_map<string, unique_ptr<json_value>> members{};

public:
    json_object() = default;
    json_object(const json_object&) = delete;
    json_object& operator=(const json_object&) = delete;
    json_object(json_object&&) = default;
    json_object& operator=(json_object&&) = default;

    types type() const noexcept override { return Object; }
    const json_object* as_object() const noexcept override { return this; }

    void add_member(const string& key, unique_ptr<json_value> value) {
        members[key] = _MSTL move(value);
    }

    const json_value* get_member(const string& key) const {
        const auto it = members.find(key);
        if (it != members.end()) return it->second.get();
        return nullptr;
    }

    const unordered_map<string, unique_ptr<json_value>>& get_members() const noexcept {
        return members;
    }
};

class MSTL_API json_array final : public json_value {
private:
    vector<unique_ptr<json_value>> elements;

public:
    json_array() = default;
    json_array(const json_array&) = delete;
    json_array& operator=(const json_array&) = delete;
    json_array(json_array&&) = default;
    json_array& operator=(json_array&&) = default;

    types type() const noexcept override { return Array; }
    const json_array* as_array() const noexcept override { return this; }


    void add_element(unique_ptr<json_value> value) {
        elements.emplace_back(_MSTL move(value));
    }

    const json_value* get_element(size_t index) const noexcept  {
        if (index < elements.size()) return elements[index].get();
        return nullptr;
    }

    size_t size() const noexcept { return elements.size(); }
    const vector<unique_ptr<json_value>>& get_elements() const noexcept { return elements; }
};



MSTL_BEGIN_INNER__
string MSTL_API json_value_to_string(const json_value* value);
string MSTL_API json_value_to_indent_string(const json_value* value, int indent);
MSTL_END_INNER__

MSTL_ALWAYS_INLINE_INLINE string to_string(const json_value* value) {
    return _INNER json_value_to_string(value);
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const json_value& value) {
    return _INNER json_value_to_string(&value);
}

MSTL_ALWAYS_INLINE_INLINE string to_indent_string(const unique_ptr<json_value>& value) {
    return _INNER json_value_to_indent_string(value.get() ,0);
}
MSTL_ALWAYS_INLINE_INLINE string to_indent_string(unique_ptr<json_value>&& value) {
    return _INNER json_value_to_indent_string(value.get(), 0);
}
MSTL_ALWAYS_INLINE_INLINE string to_indent_string(const json_value* value) {
    return _INNER json_value_to_indent_string(value, 0);
}
MSTL_ALWAYS_INLINE_INLINE string to_indent_string(const json_value& value) {
    return _INNER json_value_to_indent_string(&value, 0);
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string json_value::to_string() const {
    return _MSTL to_indent_string(this);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_JSON_JSON_VALUE_HPP__
