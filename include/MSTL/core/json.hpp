#ifndef MSTL_JSON_HPP__
#define MSTL_JSON_HPP__
#include "unordered_map.hpp"
#include "vector.hpp"
#include "stack.hpp"
#include "functional.hpp"
#include "optional.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(JsonOperateError, ValueError, "Json String Parse Failed")

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


class MSTL_API json_parser {
private:
    string json;
    size_t length;
    size_t pos = 0;

    void skip_space() noexcept {
        while (pos < length && _MSTL is_space(json[pos])) {
            pos++;
        }
    }

    char current() const noexcept {
        if (pos < length) return json[pos];
        return '\0';
    }

    bool eof() const noexcept { return pos >= length; }


    unique_ptr<json_string> parse_string();
    unique_ptr<json_number> parse_number();
    unique_ptr<json_value> parse_keyword();
    unique_ptr<json_array> parse_array();
    unique_ptr<json_object> parse_object();
    unique_ptr<json_value> parse_value();

public:
    explicit json_parser(string json_str) noexcept
    : json(_MSTL move(json_str)), length(json.size()) {}

    unique_ptr<json_value> parse();
    optional<unique_ptr<json_value>> try_parse();
};


class MSTL_API json_builder {
private:
    enum RANGE_TYPE { OBJECT, ARRAY };
    
    struct frame {
        RANGE_TYPE type = OBJECT;
        union {
            json_object* object_ptr = nullptr;
            json_array* array_ptr;
        };

        frame() = default;
        frame(const RANGE_TYPE t, json_object* obj) : type(t), object_ptr(obj) {}
        frame(const RANGE_TYPE t, json_array* arr) : type(t), array_ptr(arr) {}

        frame(const frame&) = default;
        frame& operator =(const frame&) = default;
        frame(frame&&) = default;
        frame& operator =(frame&&) = default;
        ~frame() = default;
    };
    
    _MSTL stack<frame> contexts;
    unique_ptr<json_value> root;
    string current_key;

private:
    template <typename T>
    json_builder& value_impl(unique_ptr<T> v) {
        if (contexts.empty()) {
            if (root) {
                Exception(JsonOperateError("Multiple root values not allowed"));
            }
            root = _MSTL move(v);
        } else {
            const auto& top = contexts.top();
            if (top.type == ARRAY) {
                top.array_ptr->add_element(_MSTL move(v));
            } else if (top.type == OBJECT) {
                if (current_key.empty()) {
                    Exception(JsonOperateError("No key set for value in object"));
                }
                top.object_ptr->add_member(current_key, _MSTL move(v));
                current_key.clear();
            }
        }
        return *this;
    }

    template <typename Iterable, enable_if_t<is_iterable_v<Iterable>, int> = 0>
    json_builder& value_iterable_dispatch(const Iterable& t) {
        return this->value_iterable_impl(t);
    }

    template <typename Map, enable_if_t<is_maplike_v<Map>, int> = 0>
    json_builder& value_iterable_impl(const Map& map) {
        begin_object();
        for (const auto& pair : map) {
            this->key(pair.first).value(pair.second);
        }
        end_object();
        return *this;
    }

    template <typename Iterable, enable_if_t<!is_maplike_v<Iterable>, int> = 0>
    json_builder& value_iterable_impl(const Iterable& t) {
        begin_array();
        for (const auto& element : t) {
            this->value(element);
        }
        end_array();
        return *this;
    }

public:
    json_builder() = default;
    json_builder(const json_builder&) = delete;
    json_builder& operator=(const json_builder&) = delete;
    json_builder(json_builder&&) = default;
    json_builder& operator=(json_builder&&) = default;

    json_builder& begin_object();
    json_builder& begin_array();

    json_builder& end_object();
    json_builder& end_array();

    json_builder& key(const string& k);

    json_builder& value(nullptr_t) { return value_impl(make_unique<json_null>()); }
    json_builder& value(const string& v) { return value_impl(make_unique<json_string>(v)); }
    json_builder& value(const char* v) { return value(string(v)); }
    json_builder& value(const string_view v) { return value(string(v)); }
    json_builder& value(const double v) { return value_impl(make_unique<json_number>(v)); }
    json_builder& value(const int v) { return value_impl(make_unique<json_number>(static_cast<double>(v))); }
    json_builder& value(const bool v) { return value_impl(make_unique<json_bool>(v)); }
    json_builder& value(unique_ptr<json_value>&& v) { return value_impl(_MSTL move(v)); }

    template <typename Iterable>
    json_builder& value(const Iterable& t) {
        return this->value_iterable_dispatch(t);
    }

    json_builder& value_object(_MSTL function<void(json_builder&)>&& build_func);
    json_builder& value_array(_MSTL function<void(json_builder&)>&& build_func);

    unique_ptr<json_value> build();
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

inline string json_value::to_string() const {
    return _MSTL to_indent_string(this);
}

MSTL_END_NAMESPACE__
#endif // MSTL_JSON_HPP__
