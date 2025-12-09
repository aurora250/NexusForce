#ifndef MSTL_CORE_FILE_INI_INI_VALUE_HPP__
#define MSTL_CORE_FILE_INI_INI_VALUE_HPP__
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/memory/unique_ptr.hpp"
#include "MSTL/core/interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

struct ini_exception final : value_exception {
    explicit ini_exception(
        const string& info = "INI Parse Failed",
        const char *type = __type__) noexcept
    : value_exception(type, type), msg(info) {}

    ~ini_exception() override = default;

    const char* what() const noexcept override {
        return msg.c_str();
    }

    string msg;
    static constexpr auto __type__ = "ini_exception";
};


class ini_value;
class ini_section;
class ini_property;

class MSTL_API ini_value : public istringify<ini_value> {
public:
    enum types {
        Section,
        Property
    };

    virtual ~ini_value() = default;
    MSTL_NODISCARD virtual types type() const noexcept = 0;

    MSTL_NODISCARD virtual const ini_section* as_section() const noexcept { return nullptr; }
    MSTL_NODISCARD virtual const ini_property* as_property() const noexcept { return nullptr; }

    MSTL_NODISCARD bool is_section() const noexcept { return type() == Section; }
    MSTL_NODISCARD bool is_property() const noexcept { return type() == Property; }

    MSTL_NODISCARD string to_string() const;
    MSTL_NODISCARD string to_document() const;
};

using ini_ptr = unique_ptr<ini_value>;


class MSTL_API ini_property final : public ini_value {
private:
    string value_;

public:
    explicit ini_property(string v) noexcept : value_(_MSTL move(v)) {}

    MSTL_NODISCARD types type() const noexcept override { return Property; }
    MSTL_NODISCARD const ini_property* as_property() const noexcept override { return this; }

    MSTL_NODISCARD const string& get_value() const noexcept { return value_; }
    void set_value(string v) noexcept { value_ = _MSTL move(v); }

    MSTL_NODISCARD int get_int(int default_value = 0) const noexcept;
    MSTL_NODISCARD double get_double(double default_value = 0.0) const noexcept;
    MSTL_NODISCARD bool get_bool(bool default_value = false) const noexcept;
};

class MSTL_API ini_section final : public ini_value {
private:
    unordered_map<string, unique_ptr<ini_property>> properties_;
    string name_;

public:
    explicit ini_section(string name = "") noexcept : name_(_MSTL move(name)) {}

    ini_section(const ini_section&) = delete;
    ini_section& operator=(const ini_section&) = delete;
    ini_section(ini_section&&) = default;
    ini_section& operator=(ini_section&&) = default;

    MSTL_NODISCARD types type() const noexcept override { return Section; }
    MSTL_NODISCARD const ini_section* as_section() const noexcept override { return this; }

    MSTL_NODISCARD const string& get_name() const noexcept { return name_; }
    void set_name(string name) noexcept { name_ = _MSTL move(name); }

    void add_property(const string& key, unique_ptr<ini_property> property) {
        properties_[key] = _MSTL move(property);
    }

    void set_property(const string& key, const string& value) {
        properties_[key] = make_unique<ini_property>(value);
    }

    MSTL_NODISCARD const ini_property* get_property(const string& key) const {
        const auto it = properties_.find(key);
        if (it != properties_.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD ini_property* get_property(const string& key) {
        const auto it = properties_.find(key);
        if (it != properties_.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD bool has_property(const string& key) const {
        return properties_.find(key) != properties_.end();
    }

    MSTL_NODISCARD const unordered_map<string, unique_ptr<ini_property>>& get_properties() const noexcept {
        return properties_;
    }

    MSTL_NODISCARD string get_string(const string& key, const string& default_value = "") const {
        const auto* prop = get_property(key);
        return prop ? prop->get_value() : default_value;
    }

    MSTL_NODISCARD int get_int(const string& key, int default_value = 0) const {
        const auto* prop = get_property(key);
        return prop ? prop->get_int(default_value) : default_value;
    }

    MSTL_NODISCARD double get_double(const string& key, double default_value = 0.0) const {
        const auto* prop = get_property(key);
        return prop ? prop->get_double(default_value) : default_value;
    }

    MSTL_NODISCARD bool get_bool(const string& key, bool default_value = false) const {
        const auto* prop = get_property(key);
        return prop ? prop->get_bool(default_value) : default_value;
    }
};

class MSTL_API ini_document final {
private:
    unordered_map<string, unique_ptr<ini_section>> sections_;
    unique_ptr<ini_section> global_section_;

public:
    ini_document() : global_section_(make_unique<ini_section>("")) {}

    ini_document(const ini_document&) = delete;
    ini_document& operator=(const ini_document&) = delete;
    ini_document(ini_document&&) = default;
    ini_document& operator=(ini_document&&) = default;

    void add_section(const string& name, unique_ptr<ini_section> section) {
        if (name.empty()) {
            global_section_ = _MSTL move(section);
        } else {
            sections_[name] = _MSTL move(section);
        }
    }

    MSTL_NODISCARD const ini_section* get_section(const string& name) const {
        if (name.empty()) return global_section_.get();
        const auto it = sections_.find(name);
        if (it != sections_.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD ini_section* get_section(const string& name) {
        if (name.empty()) return global_section_.get();
        const auto it = sections_.find(name);
        if (it != sections_.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD bool has_section(const string& name) const {
        if (name.empty()) return global_section_ != nullptr;
        return sections_.find(name) != sections_.end();
    }

    MSTL_NODISCARD const unordered_map<string, unique_ptr<ini_section>>& get_sections() const noexcept {
        return sections_;
    }

    MSTL_NODISCARD const ini_section* get_global_section() const noexcept {
        return global_section_.get();
    }

    MSTL_NODISCARD ini_section* get_global_section() noexcept {
        return global_section_.get();
    }

    MSTL_NODISCARD string get_string(const string& section, const string& key, const string& default_value = "") const {
        const auto* sec = get_section(section);
        return sec ? sec->get_string(key, default_value) : default_value;
    }

    MSTL_NODISCARD int get_int(const string& section, const string& key, int default_value = 0) const {
        const auto* sec = get_section(section);
        return sec ? sec->get_int(key, default_value) : default_value;
    }

    MSTL_NODISCARD double get_double(const string& section, const string& key, double default_value = 0.0) const {
        const auto* sec = get_section(section);
        return sec ? sec->get_double(key, default_value) : default_value;
    }

    MSTL_NODISCARD bool get_bool(const string& section, const string& key, bool default_value = false) const {
        const auto* sec = get_section(section);
        return sec ? sec->get_bool(key, default_value) : default_value;
    }

    MSTL_NODISCARD string to_string() const;
};

MSTL_BEGIN_INNER__
string MSTL_API ini_value_to_string(const ini_value* value);
string MSTL_API ini_document_to_string(const ini_document* doc);
MSTL_END_INNER__

MSTL_ALWAYS_INLINE_INLINE string to_string(const ini_value* value) {
    return _INNER ini_value_to_string(value);
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const ini_value& value) {
    return _INNER ini_value_to_string(&value);
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const ini_ptr& value) {
    return _INNER ini_value_to_string(value.get());
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const ini_document& doc) {
    return _INNER ini_document_to_string(&doc);
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string ini_value::to_string() const {
    return _INNER ini_value_to_string(this);
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string ini_value::to_document() const {
    return _INNER ini_value_to_string(this);
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string ini_document::to_string() const {
    return _INNER ini_document_to_string(this);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_INI_INI_VALUE_HPP__
