#ifndef NEFORCE_CORE_REFLECT_ANY_HPP__
#define NEFORCE_CORE_REFLECT_ANY_HPP__
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/string/string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

using type_id = size_t;


template <typename T>
struct type_name {
    static constexpr string_view value = "unknown";
};

template <typename T>
NEFORCE_INLINE17 constexpr string_view type_name_v = type_name<T>::value;


#define __NEFORCE_SPECIALIZE_TYPE_NAME(T) \
    template <> struct type_name<T> { \
        static constexpr string_view value = #T; \
    };

NEFORCE_MACRO_RANGE_ARITHMETIC(__NEFORCE_SPECIALIZE_TYPE_NAME)
#undef __NEFORCE_SPECIALIZE_TYPE_NAME


class any {
private:
    struct concepts {
        virtual ~concepts() = default;
        virtual unique_ptr<concepts> clone() const = 0;
        virtual _REFLECT type_id type_id() const noexcept = 0;
    };

    template <typename T>
    struct model final : concepts {
        T value_;

        explicit model(T value)
        : value_(_NEFORCE move(value)) {}

        unique_ptr<concepts> clone() const override {
            return _NEFORCE make_unique<model<T>>(value_);
        }

        _REFLECT type_id type_id() const noexcept override {
            return type_name_v<T>.to_hash();
        }
    };

    unique_ptr<concepts> storage_{nullptr};

public:
    any() noexcept = default;

    template <typename T, typename = enable_if_t<!is_same_v<decay_t<T>, any>>>
    any(T&& value)
    : storage_(_NEFORCE make_unique<model<decay_t<T>>>(_NEFORCE forward<T>(value))) {}

    any(any&&) noexcept = default;
    any& operator =(any&&) noexcept = default;

    any(const any& other) {
        if (other.storage_) {
            storage_ = other.storage_->clone();
        }
    }

    any& operator =(const any& other) {
        if (this != &other) {
            if (other.storage_) {
                storage_ = other.storage_->clone();
            } else {
                storage_.reset();
            }
        }
        return *this;
    }

    type_id type_id() const noexcept {
        return storage_ ? storage_->type_id() : 0;
    }

    bool has_value() const noexcept { return !!storage_; }
    explicit operator bool() const noexcept { return has_value(); }

    template <typename T>
    T* cast() noexcept {
        if (!storage_) return nullptr;
        if (storage_->type_id() != type_name_v<T>.to_hash()) {
            return nullptr;
        }
        auto* md = dynamic_cast<model<T>*>(storage_.get());
        return md ? &md->value_ : nullptr;
    }

    template <typename T>
    const T* cast() const noexcept {
        if (!storage_) return nullptr;
        if (storage_->type_id() != type_name_v<T>.to_hash()) {
            return nullptr;
        }
        auto* md = dynamic_cast<model<T>*>(storage_.get());
        return md ? &md->value_ : nullptr;
    }

    template <typename T>
    T& get() {
        if (auto* ptr = cast<T>()) return *ptr;
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }

    template <typename T>
    const T& get() const {
        if (auto* ptr = cast<T>()) return *ptr;
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }

    template <typename T>
    bool can_cast() const noexcept {
        return cast<T>() != nullptr;
    }

    template <typename T>
    T convert() const {
        if (auto* ptr = cast<T>()) return *ptr;
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }
};

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_ANY_HPP__
