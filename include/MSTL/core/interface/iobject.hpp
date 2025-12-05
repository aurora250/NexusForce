#ifndef MSTL_CORE_INTERFACE_IOBJECT_HPP__
#define MSTL_CORE_INTERFACE_IOBJECT_HPP__
#include "../interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct iobject : icommon<T>, istringify<T>{
    using self = iobject<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<T*>(static_cast<const T*>(o));
    }

public:
    MSTL_CONSTEXPR20 ~iobject() = default;

    MSTL_NODISCARD static MSTL_CONSTEXPR20 child_type parse(const string_view str) {
        return child_type::parse(str);
    }
    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        return self::to_template(this)->try_parse(str);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_IOBJECT_HPP__
