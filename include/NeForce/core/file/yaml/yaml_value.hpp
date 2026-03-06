#ifndef NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/time/datetime.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_ERROR_BUILD_FINAL_CLASS(yaml_exception, value_exception, "YAML Operation Failed.")


class yaml_value;
class yaml_null;
class yaml_boolean;
class yaml_integer;
class yaml_float;
class yaml_string;
class yaml_timestamp;
class yaml_sequence;
class yaml_mapping;


class NEFORCE_API yaml_value : public istringify<yaml_value> {
public:
    string anchor;
    string tag;

    enum types {
        Null,
        Boolean,
        Integer,
        Float,
        String,
        Timestamp,
        Sequence,
        Mapping
    };

    virtual ~yaml_value() = default;
    NEFORCE_NODISCARD virtual types type() const noexcept = 0;

    NEFORCE_NODISCARD virtual const yaml_null* as_null() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_boolean* as_boolean() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_integer* as_integer() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_float* as_float() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_string* as_string() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_timestamp* as_timestamp() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_sequence* as_sequence() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_mapping* as_mapping() const noexcept { return nullptr; }

    NEFORCE_NODISCARD bool is_null() const noexcept { return type() == Null; }
    NEFORCE_NODISCARD bool is_boolean() const noexcept { return type() == Boolean; }
    NEFORCE_NODISCARD bool is_integer() const noexcept { return type() == Integer; }
    NEFORCE_NODISCARD bool is_float() const noexcept { return type() == Float; }
    NEFORCE_NODISCARD bool is_string() const noexcept { return type() == String; }
    NEFORCE_NODISCARD bool is_timestamp() const noexcept { return type() == Timestamp; }
    NEFORCE_NODISCARD bool is_sequence() const noexcept { return type() == Sequence; }
    NEFORCE_NODISCARD bool is_mapping() const noexcept { return type() == Mapping; }

    void set_anchor(const string& a) { this->anchor = a; }
    void set_tag(const string& t) { this->tag = t; }

    NEFORCE_NODISCARD string to_string() const;
    NEFORCE_NODISCARD string to_document() const;
};

using yaml_ptr = shared_ptr<yaml_value>;


class NEFORCE_API yaml_null final : public yaml_value {
public:
    yaml_null() = default;
    NEFORCE_NODISCARD types type() const noexcept override { return Null; }
    NEFORCE_NODISCARD const yaml_null* as_null() const noexcept override { return this; }
};

class NEFORCE_API yaml_boolean final : public yaml_value {
private:
    bool value;

public:
    explicit yaml_boolean(const bool v) noexcept : value(v) {}
    NEFORCE_NODISCARD types type() const noexcept override { return Boolean; }
    NEFORCE_NODISCARD const yaml_boolean* as_boolean() const noexcept override { return this; }
    NEFORCE_NODISCARD bool get_value() const noexcept { return value; }
};

class NEFORCE_API yaml_integer final : public yaml_value {
private:
    int64_t value;

public:
    explicit yaml_integer(const int64_t v) noexcept : value(v) {}
    NEFORCE_NODISCARD types type() const noexcept override { return Integer; }
    NEFORCE_NODISCARD const yaml_integer* as_integer() const noexcept override { return this; }
    NEFORCE_NODISCARD int64_t get_value() const noexcept { return value; }
};

class NEFORCE_API yaml_float final : public yaml_value {
private:
    double value;

public:
    explicit yaml_float(const double v) noexcept : value(v) {}
    NEFORCE_NODISCARD types type() const noexcept override { return Float; }
    NEFORCE_NODISCARD const yaml_float* as_float() const noexcept override { return this; }
    NEFORCE_NODISCARD double get_value() const noexcept { return value; }
};

class NEFORCE_API yaml_string final : public yaml_value {
public:
    enum string_style {
        Plain,
        SingleQuoted,
        DoubleQuoted,
        Literal,
        Folded
    };

private:
    string value;
    string_style style;

public:
    explicit yaml_string(string v, const string_style s = Plain) noexcept
    : value(_NEFORCE move(v)), style(s) {}

    NEFORCE_NODISCARD types type() const noexcept override { return String; }
    NEFORCE_NODISCARD const yaml_string* as_string() const noexcept override { return this; }
    NEFORCE_NODISCARD const string& get_value() const noexcept { return value; }
    NEFORCE_NODISCARD string_style get_style() const noexcept { return style; }
};

class NEFORCE_API yaml_timestamp final : public yaml_value {
private:
    datetime value;

public:
    explicit yaml_timestamp(const string_view v) {
        datetime dt;
        if (dt.try_parse_ISO_UTC(v) || dt.try_parse_ISO(v)) {
            value = dt;
        } else {
            throw_exception(yaml_exception(("Invalid timestamp format: " + string(v)).data()));
        }
    }

    explicit yaml_timestamp(const datetime& dt) noexcept : value(dt) {}

    NEFORCE_NODISCARD types type() const noexcept override { return Timestamp; }
    NEFORCE_NODISCARD const yaml_timestamp* as_timestamp() const noexcept override { return this; }
    NEFORCE_NODISCARD const datetime& get_value() const noexcept { return value; }

    NEFORCE_NODISCARD string get_string_value() const noexcept {
        return value.to_string_ISO_UTC();
    }
};

class NEFORCE_API yaml_sequence final : public yaml_value {
public:
    enum sequence_style {
        Block,
        Flow
    };

private:
    vector<yaml_ptr> elements;
    sequence_style style;

public:
    explicit yaml_sequence(const sequence_style s = Block) : style(s) {}

    yaml_sequence(const yaml_sequence&) = delete;
    yaml_sequence& operator =(const yaml_sequence&) = delete;
    yaml_sequence(yaml_sequence&&) = default;
    yaml_sequence& operator =(yaml_sequence&&) = default;

    NEFORCE_NODISCARD types type() const noexcept override { return Sequence; }
    NEFORCE_NODISCARD const yaml_sequence* as_sequence() const noexcept override { return this; }

    void add_element(yaml_ptr value) {
        elements.emplace_back(_NEFORCE move(value));
    }

    NEFORCE_NODISCARD const yaml_value* get_element(const size_t index) const noexcept {
        if (index < elements.size()) return elements[index].get();
        return nullptr;
    }

    NEFORCE_NODISCARD yaml_value* get_element(const size_t index) noexcept {
        if (index < elements.size()) return elements[index].get();
        return nullptr;
    }

    NEFORCE_NODISCARD size_t size() const noexcept { return elements.size(); }
    NEFORCE_NODISCARD const vector<yaml_ptr>& get_elements() const noexcept { return elements; }
    NEFORCE_NODISCARD sequence_style get_style() const noexcept { return style; }
    void set_style(const sequence_style s) noexcept { style = s; }
};

class NEFORCE_API yaml_mapping final : public yaml_value {
public:
    enum mapping_style {
        Block,
        Flow
    };

private:
    unordered_map<string, yaml_ptr> members;
    mapping_style style;

public:
    explicit yaml_mapping(const mapping_style s = Block) : style(s) {}

    yaml_mapping(const yaml_mapping&) = delete;
    yaml_mapping& operator =(const yaml_mapping&) = delete;
    yaml_mapping(yaml_mapping&&) = default;
    yaml_mapping& operator =(yaml_mapping&&) = default;

    NEFORCE_NODISCARD types type() const noexcept override { return Mapping; }
    NEFORCE_NODISCARD const yaml_mapping* as_mapping() const noexcept override { return this; }

    void add_member(const string& key, yaml_ptr value) {
        members[key] = _NEFORCE move(value);
    }

    NEFORCE_NODISCARD const yaml_value* get_member(const string& key) const {
        const auto it = members.find(key);
        if (it != members.end()) return it->second.get();
        return nullptr;
    }

    NEFORCE_NODISCARD yaml_value* get_member(const string& key) {
        const auto it = members.find(key);
        if (it != members.end()) return it->second.get();
        return nullptr;
    }

    NEFORCE_NODISCARD bool has_member(const string& key) const {
        return members.find(key) != members.end();
    }

    NEFORCE_NODISCARD const unordered_map<string, yaml_ptr>& get_members() const noexcept {
        return members;
    }

    NEFORCE_NODISCARD mapping_style get_style() const noexcept { return style; }
    void set_style(const mapping_style s) noexcept { style = s; }

    void merge_from(const yaml_mapping* other) {
        if (!other) return;
        for (const auto& pair : other->get_members()) {
            if (members.find(pair.first) == members.end()) {
                members[pair.first] = pair.second;
            }
        }
    }
};

NEFORCE_BEGIN_INNER__
string NEFORCE_API yaml_value_to_string(const yaml_value* value);
string NEFORCE_API yaml_value_document(const yaml_value* root);
NEFORCE_END_INNER__

NEFORCE_ALWAYS_INLINE_INLINE string to_string(const yaml_value* value) {
    return _INNER yaml_value_to_string(value);
}
NEFORCE_ALWAYS_INLINE_INLINE string to_string(const yaml_value& value) {
    return _INNER yaml_value_to_string(&value);
}
NEFORCE_ALWAYS_INLINE_INLINE string to_string(const yaml_ptr& value) {
    return _INNER yaml_value_to_string(value.get());
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE string yaml_value::to_string() const {
    return _INNER yaml_value_to_string(this);
}

NEFORCE_ALWAYS_INLINE_INLINE string yaml_document(const yaml_value* value) {
    return _INNER yaml_value_document(value);
}
NEFORCE_ALWAYS_INLINE_INLINE string yaml_document(const yaml_value& value) {
    return _INNER yaml_value_document(&value);
}
NEFORCE_ALWAYS_INLINE_INLINE string yaml_document(const yaml_ptr& value) {
    return _INNER yaml_value_document(value.get());
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE string yaml_value::to_document() const {
    return _INNER yaml_value_document(this);
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
