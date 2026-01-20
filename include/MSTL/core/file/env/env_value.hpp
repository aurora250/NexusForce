#ifndef MSTL_CORE_FILE_ENV_ENV_VALUE_HPP__
#define MSTL_CORE_FILE_ENV_ENV_VALUE_HPP__
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/memory/unique_ptr.hpp"
#include "MSTL/core/interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(env_exception, value_exception, "ENV Operation Failed.")


class env_value;
class env_variable;

class MSTL_API env_value : public istringify<env_value> {
public:
    enum types {
        Variable
    };

    virtual ~env_value() = default;
    MSTL_NODISCARD virtual types type() const noexcept = 0;

    MSTL_NODISCARD virtual const env_variable* as_variable() const noexcept { return nullptr; }

    MSTL_NODISCARD bool is_variable() const noexcept { return type() == Variable; }

    MSTL_NODISCARD string to_string() const;
    MSTL_NODISCARD string to_document() const;
};

using env_ptr = unique_ptr<env_value>;


class MSTL_API env_variable final : public env_value {
public:
    enum quote_type { None, Single, Double };

private:
    string value_;
    quote_type quote_type_ = None;
    bool is_exported_ = false;

public:
    explicit env_variable(string v, const quote_type qt = None, const bool exported = false) noexcept
    : value_(_MSTL move(v)), quote_type_(qt), is_exported_(exported) {}

    MSTL_NODISCARD types type() const noexcept override { return Variable; }
    MSTL_NODISCARD const env_variable* as_variable() const noexcept override { return this; }

    MSTL_NODISCARD const string& get_value() const noexcept { return value_; }
    void set_value(string v) noexcept { value_ = _MSTL move(v); }

    MSTL_NODISCARD quote_type get_quote_type() const noexcept { return quote_type_; }
    void set_quote_type(const quote_type qt) noexcept { quote_type_ = qt; }

    MSTL_NODISCARD bool is_exported() const noexcept { return is_exported_; }
    void set_exported(const bool exported) noexcept { is_exported_ = exported; }

    MSTL_NODISCARD int get_int(int default_value = 0) const noexcept;
    MSTL_NODISCARD int64_t get_int64(int64_t default_value = 0) const noexcept;
    MSTL_NODISCARD double get_double(double default_value = 0.0) const noexcept;
    MSTL_NODISCARD bool get_bool(bool default_value = false) const noexcept;
};

class MSTL_API env_document final {
private:
    unordered_map<string, unique_ptr<env_variable>> variables_;
    vector<string> comments_;

public:
    env_document() = default;

    env_document(const env_document&) = delete;
    env_document& operator =(const env_document&) = delete;
    env_document(env_document&&) = default;
    env_document& operator =(env_document&&) = default;

    void add_variable(const string& name, unique_ptr<env_variable> variable) {
        variables_[name] = _MSTL move(variable);
    }

    void set_variable(const string& name, const string& value,
        env_variable::quote_type qt = env_variable::None,
        bool exported = false) {
        variables_[name] = make_unique<env_variable>(value, qt, exported);
    }

    MSTL_NODISCARD const env_variable* get_variable(const string& name) const {
        const auto it = variables_.find(name);
        if (it != variables_.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD env_variable* get_variable(const string& name) {
        const auto it = variables_.find(name);
        if (it != variables_.end()) return it->second.get();
        return nullptr;
    }

    MSTL_NODISCARD bool has_variable(const string& name) const {
        return variables_.find(name) != variables_.end();
    }

    void remove_variable(const string& name) {
        variables_.erase(name);
    }

    MSTL_NODISCARD const unordered_map<string, unique_ptr<env_variable>>& get_variables() const noexcept {
        return variables_;
    }

    void add_comment(const string& comment) {
        comments_.push_back(comment);
    }

    MSTL_NODISCARD const vector<string>& get_comments() const noexcept {
        return comments_;
    }

    MSTL_NODISCARD string get_string(const string& name, const string& default_value = "") const {
        const auto* var = get_variable(name);
        return var ? var->get_value() : default_value;
    }

    MSTL_NODISCARD int get_int(const string& name, const int default_value = 0) const {
        const auto* var = get_variable(name);
        return var ? var->get_int(default_value) : default_value;
    }

    MSTL_NODISCARD int64_t get_int64(const string& name, const int64_t default_value = 0) const {
        const auto* var = get_variable(name);
        return var ? var->get_int64(default_value) : default_value;
    }

    MSTL_NODISCARD double get_double(const string& name, const double default_value = 0.0) const {
        const auto* var = get_variable(name);
        return var ? var->get_double(default_value) : default_value;
    }

    MSTL_NODISCARD bool get_bool(const string& name, const bool default_value = false) const {
        const auto* var = get_variable(name);
        return var ? var->get_bool(default_value) : default_value;
    }

    MSTL_NODISCARD string to_string() const;
};

MSTL_BEGIN_INNER__
string MSTL_API env_value_to_string(const env_value* value, const string& key = "");
string MSTL_API env_document_to_string(const env_document* doc);
MSTL_END_INNER__

MSTL_ALWAYS_INLINE_INLINE string to_string(const env_value* value) {
    return _INNER env_value_to_string(value);
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const env_value& value) {
    return _INNER env_value_to_string(&value);
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const env_ptr& value) {
    return _INNER env_value_to_string(value.get());
}
MSTL_ALWAYS_INLINE_INLINE string to_string(const env_document& doc) {
    return _INNER env_document_to_string(&doc);
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string env_value::to_string() const {
    return _INNER env_value_to_string(this);
}
MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string env_value::to_document() const {
    return _INNER env_value_to_string(this);
}
MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string env_document::to_string() const {
    return _INNER env_document_to_string(this);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_ENV_ENV_VALUE_HPP__
