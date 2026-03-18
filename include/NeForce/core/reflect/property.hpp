#ifndef NEFORCE_CORE_REFLECT_PROPERTY_HPP__
#define NEFORCE_CORE_REFLECT_PROPERTY_HPP__
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/reflect/any.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

class meta_property {
public:
    using getter = function<any(void*)>;
    using setter = function<void(void*, const any&)>;

private:
    string_view name_;
    type_id type_id_;
    getter getter_;
    setter setter_;

public:
    meta_property(string_view name, type_id type_id, getter getter, setter setter)
    : name_(name), type_id_(type_id), getter_(move(getter)), setter_(move(setter)) {}

    string_view name() const noexcept { return name_; }
    type_id type_id() const noexcept { return type_id_; }

    any get(void* obj) const {
        if (!obj || !getter_) return any{};
        return getter_(move(obj));
    }

    bool set(void* obj, const any& value) const {
        if (!obj || !setter_) return false;
        try {
            setter_(move(obj), value);
            return true;
        } catch (...) {
            return false;
        }
    }

    template <typename T, enable_if_t<!is_same_v<any, decay_t<T>>, int> = 0>
    bool set(void* obj, T&& value) const {
        return this->set(obj, any{_NEFORCE forward<T>(value)});
    }
};

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_PROPERTY_HPP__
