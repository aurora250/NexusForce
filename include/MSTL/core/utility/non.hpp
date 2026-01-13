#ifndef MSTL_CORE_UTILITY_MONOSTATE_HPP__
#define MSTL_CORE_UTILITY_MONOSTATE_HPP__
#include "../interface/icommon.hpp"
MSTL_BEGIN_NAMESPACE__

struct non : icommon<non> {
    constexpr non() noexcept = default;

    constexpr bool operator ==(const non&) const noexcept { return true; }
    constexpr bool operator <(const non&) const noexcept { return false; }

    constexpr size_t to_hash() const noexcept { return 0; }

    constexpr void swap(non&) noexcept {}
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_MONOSTATE_HPP__
