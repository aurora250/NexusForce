#ifndef MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
#define MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
#include "../algorithm/type_erase.hpp"
#include "../functional/hash.hpp"
#include "icommon.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Collector>
struct icollector : icommon<Collector> {
private:
    using collector_type    = Collector;
    using self              = icollector;

    static constexpr collector_type* to_template(const self* o) noexcept {
        return const_cast<collector_type*>(static_cast<const collector_type*>(o));
    }

protected:
    template <typename T>
    MSTL_NODISCARD static constexpr size_t default_to_hash(const T& c) noexcept {
        size_t result = FNV_OFFSET_BASIS;
        if (_MSTL empty(c)) return result;

        constexpr hash<remove_cvref_t<decltype(*_MSTL cbegin(c))>> hasher;
        for (auto elem : c) result ^= hasher(elem);
        return result;
    }

public:
    MSTL_CONSTEXPR20 ~icollector() = default;

    MSTL_NODISCARD constexpr size_t size() const
    noexcept(noexcept(self::to_template(this)->size())) {
        return static_cast<size_t>(self::to_template(this)->size());
    }

    MSTL_NODISCARD constexpr bool empty() const
    noexcept(noexcept(self::to_template(this)->empty())) {
        return self::to_template(this)->empty();
    }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        const auto& c = *self::to_template(this);
        size_t result = FNV_OFFSET_BASIS;
        if (_MSTL empty(c)) return result;

        constexpr hash<remove_cvref_t<decltype(*_MSTL cbegin(c))>> hasher;
        for (auto elem : c) result ^= hasher(elem);
        return result;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
