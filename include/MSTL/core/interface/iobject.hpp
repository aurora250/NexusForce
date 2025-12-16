#ifndef MSTL_CORE_INTERFACE_IOBJECT_HPP__
#define MSTL_CORE_INTERFACE_IOBJECT_HPP__
#include "../interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct iobject : istringify<T> {
public:
    MSTL_NODISCARD static constexpr T parse(const string_view str) {
        return T::parse(str);
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        T tmp;
        try {
            tmp = T::parse(str);
        } catch (...) {
            return false;
        }
        *this = _MSTL move(tmp);
        return true;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_IOBJECT_HPP__
