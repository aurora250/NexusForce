#ifndef MSTL_CORE_INTERFACE_IOBJECT_HPP__
#define MSTL_CORE_INTERFACE_IOBJECT_HPP__
#include "../interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct iobject : icommon<T>, istringify<T> {
    using self = iobject<T>;
    using child_type = T;

private:
    constexpr const child_type& derived() const noexcept {
        return static_cast<const child_type&>(*this);
    }
    constexpr child_type& derived() noexcept {
        return static_cast<child_type&>(*this);
    }

public:
    MSTL_CONSTEXPR20 ~iobject() = default;

    MSTL_NODISCARD static constexpr child_type parse(const string_view str) {
        return child_type::parse(str);
    }
    constexpr bool try_parse(const string_view str) noexcept {
        return derived().try_parse(str);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_IOBJECT_HPP__
