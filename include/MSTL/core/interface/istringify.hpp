#ifndef MSTL_CORE_INTERFACE_ISTRINGIFY_HPP__
#define MSTL_CORE_INTERFACE_ISTRINGIFY_HPP__
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct istringify {
private:
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }

public:
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return derived().to_string();
    }
};

template <typename T, enable_if_t<is_base_of_v<istringify<T>, T>, int> = 0>
MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const T& obj) {
    return obj.to_string();
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_ISTRINGIFY_HPP__
