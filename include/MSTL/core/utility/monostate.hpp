#ifndef MSTL_CORE_UTILITY_MONOSTATE_HPP__
#define MSTL_CORE_UTILITY_MONOSTATE_HPP__
#include "../interface/icommon.hpp"
MSTL_BEGIN_NAMESPACE__

struct monostate : icommon<monostate> {
    constexpr monostate() noexcept = default;

    constexpr bool operator ==(const monostate&) const noexcept { return true; }
    constexpr bool operator <(const monostate&) const noexcept { return false; }

    constexpr size_t to_hash() const noexcept { return 0; }

    constexpr void swap(monostate&) noexcept {}
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_MONOSTATE_HPP__
