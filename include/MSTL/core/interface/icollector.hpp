#ifndef MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
#define MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
#include "../algorithm/type_erase.hpp"
#include "icommon.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct icollector : icommon<T> {
private:
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }

public:
    MSTL_NODISCARD constexpr size_t size() const
    noexcept(noexcept(derived().size())) {
        return static_cast<size_t>(derived().size());
    }

    MSTL_NODISCARD constexpr bool empty() const
    noexcept(noexcept(derived().empty())) {
        return derived().empty();
    }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        const auto& c = derived();
        size_t result = FNV_OFFSET_BASIS;
        if (_MSTL empty(c)) return result;

        hash<remove_cvref_t<decltype(*_MSTL cbegin(c))>> hasher;
        for (const auto& elem : c) {
            result ^= hasher(elem);
        }
        return result;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
