#ifndef MSTL_JSON_HPP__
#define MSTL_JSON_HPP__
#include "unordered_map.hpp"
#include "string.hpp"
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


class MSTL_API json_value {
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

    virtual const json_null* as_null() const noexcept;
    virtual const json_bool* as_bool() const noexcept;
    virtual const json_number* as_number() const noexcept;
    virtual const json_string* as_string() const noexcept;
    virtual const json_object* as_object() const noexcept;
    virtual const json_array* as_array() const noexcept;

    MSTL_NODISCARD bool is_null() const noexcept;
    MSTL_NODISCARD bool is_bool() const noexcept;
    MSTL_NODISCARD bool is_number() const noexcept;
    MSTL_NODISCARD bool is_string() const noexcept;
    MSTL_NODISCARD bool is_object() const noexcept;
    MSTL_NODISCARD bool is_array() const noexcept;
};


class MSTL_API json_null final : public json_value {
public:
    types type() const noexcept override;
    const json_null* as_null() const noexcept override;
};

class MSTL_API json_bool final : public json_value {
private:
    bool value;

public:
    explicit json_bool(bool v) noexcept;
    types type() const noexcept override;
    const json_bool* as_bool() const noexcept override;
    bool get_value() const noexcept;
};

class MSTL_API json_number final : public json_value {
private:
    double value;

public:
    explicit json_number(double v) noexcept;
    types type() const noexcept override;
    const json_number* as_number() const noexcept override;
    double get_value() const noexcept;
};

class MSTL_API json_string final : public json_value {
private:
    string value;

public:
    explicit json_string(string v) noexcept;
    types type() const noexcept override;
    const json_string* as_string() const noexcept override;
    const string& get_value() const noexcept;
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

    types type() const noexcept override;
    const json_object* as_object() const noexcept override;

    void add_member(const string& key, unique_ptr<json_value> value);
    const json_value* get_member(const string& key) const;

    const unordered_map<string, unique_ptr<json_value>>& get_members() const noexcept;
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

    types type() const noexcept override;
    const json_array* as_array() const noexcept override;

    void add_element(unique_ptr<json_value> value);
    const json_value* get_element(size_t index) const noexcept;

    size_t size() const noexcept;

    const vector<unique_ptr<json_value>>& get_elements() const noexcept;
};


class MSTL_API json_parser {
private:
    string json;
    size_t pos;
    size_t length;

    void skip_space() noexcept;
    char current() const noexcept;
    bool eof() const noexcept;

    unique_ptr<json_string> parse_string();
    unique_ptr<json_number> parse_number();
    unique_ptr<json_value> parse_keyword();
    unique_ptr<json_array> parse_array();
    unique_ptr<json_object> parse_object();
    unique_ptr<json_value> parse_value();

public:
    explicit json_parser(string json_str) noexcept;

    unique_ptr<json_value> parse();
    optional<unique_ptr<json_value>> try_parse();
};


class MSTL_API json_builder {
private:
    enum types {
        Object,
        Array
    };
    
    struct frame {
        types type = Object;
        union {
            json_object* object_ptr = nullptr;
            json_array* array_ptr;
        };

        frame() = default;
        frame(types t, json_object* obj);
        frame(types t, json_array* arr);

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
            if (top.type == Array) {
                top.array_ptr->add_element(_MSTL move(v));
            } else if (top.type == Object) {
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

    json_builder& value(nullptr_t);
    json_builder& value(const string& v);
    json_builder& value(const char* v);
    json_builder& value(const string_view& v);
    json_builder& value(double v);
    json_builder& value(int v);
    json_builder& value(bool v);
    json_builder& value(unique_ptr<json_value>&& v);

    template <typename Iterable>
    json_builder& value(const Iterable& t) {
        return this->value_iterable_dispatch(t);
    }

    json_builder& value_object(_MSTL function<void(json_builder&)>&& build_func);
    json_builder& value_array(_MSTL function<void(json_builder&)>&& build_func);

    unique_ptr<json_value> build();
};

MSTL_API string json_to_string(const unique_ptr<json_value>& value);
MSTL_API string json_to_string(unique_ptr<json_value>&& value);
MSTL_API string json_to_string(const json_value* value);

MSTL_END_NAMESPACE__
#endif // MSTL_JSON_HPP__
