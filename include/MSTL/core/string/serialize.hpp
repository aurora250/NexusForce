#ifndef MSTL_CORE_STRING_SERIALIZE_HPP__
#define MSTL_CORE_STRING_SERIALIZE_HPP__
#include "string.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct iobject : icommon<T>, istringify<T> {};


template <typename T>
struct iserialize : iobject<T> {
    using self = iserialize<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<T*>(static_cast<const T*>(o));
    }

public:
    MSTL_CONSTEXPR20 ~iserialize() = default;

    MSTL_NODISCARD static MSTL_CONSTEXPR20 child_type parse(const string_view str) {
        return child_type::parse(str);
    }
    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        return self::to_template(this)->try_parse(str);
    }
};


template <typename Collector>
struct icollector : iobject<Collector> {
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

    template <typename T>
    MSTL_NODISCARD static MSTL_CONSTEXPR20 string default_to_string(const T& c) {
        return _INNER collector_to_string(c);
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
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_SERIALIZE_HPP__
