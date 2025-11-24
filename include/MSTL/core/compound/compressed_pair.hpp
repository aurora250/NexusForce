#ifndef MSTL_CORE_COMPOUND_COMPRESSED_PAIR_HPP__
#define MSTL_CORE_COMPOUND_COMPRESSED_PAIR_HPP__
#include "../utility/interface.hpp"
#include "../typeinfo/tags.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename IfEmpty, typename T, bool Compressed = is_empty_v<IfEmpty> && !is_final_v<IfEmpty>>
struct compressed_pair final : IfEmpty, icommon<compressed_pair<IfEmpty, T, Compressed>> {
    using base_type = IfEmpty;

    T value{};

    constexpr compressed_pair() noexcept(is_nothrow_default_constructible_v<T>) = default;

    constexpr compressed_pair(const compressed_pair& p)
        noexcept(is_nothrow_copy_constructible_v<T>) : value(p.value) {}

    constexpr compressed_pair(compressed_pair&& p)
        noexcept(is_nothrow_move_constructible_v<T>) : value(_MSTL move(p.value)) {}

    constexpr compressed_pair& operator =(compressed_pair&& pir)
    noexcept(is_nothrow_move_assignable_v<T>) {
        value = _MSTL move(pir.value);
        return *this;
    }

    template <typename... Args>
    constexpr explicit compressed_pair(_MSTL_TAG default_construct_tag, Args&&... args)
        noexcept(conjunction_v<is_nothrow_default_constructible<IfEmpty>, is_nothrow_constructible<T, Args...>>)
        : IfEmpty(), value(_MSTL forward<Args>(args)...) {}

    template <typename ToEmpty, typename... Args>
    constexpr explicit compressed_pair(_MSTL_TAG exact_arg_construct_tag, ToEmpty&& first, Args&&... args)
        noexcept(conjunction_v<is_nothrow_constructible<IfEmpty, ToEmpty>, is_nothrow_constructible<T, Args...>>)
        : IfEmpty(_MSTL forward<ToEmpty>(first)), value(_MSTL forward<Args>(args)...) {}

    constexpr compressed_pair& get_base() noexcept {
        return *this;
    }
    constexpr const compressed_pair& get_base() const noexcept {
        return *this;
    }

    constexpr void swap(compressed_pair& rh)
        noexcept(is_nothrow_swappable_v<T>) {
        _MSTL swap(value, rh.value);
    }

	constexpr size_t to_hash() const
	noexcept(noexcept(hash<T>{}(value))) {
	    return hash<T>{}(value);
    }

	constexpr bool operator ==(const compressed_pair& y) const
	noexcept(noexcept(this->value == y.value)) {
    	return this->value == y.value;
    }
	constexpr bool operator !=(const compressed_pair& y) const
	noexcept(noexcept(!(*this == y))) {
    	return !(*this == y);
    }
	constexpr bool operator <(const compressed_pair& y) const
	noexcept(noexcept(this->value < y.value)) {
    	return this->value < y.value;
    }
	constexpr bool operator >(const compressed_pair& y) const
	noexcept(noexcept(y < *this)) {
    	return y < *this;
    }
	constexpr bool operator <=(const compressed_pair& y) const
	noexcept(noexcept(!(*this > y))) {
    	return !(*this > y);
    }
	constexpr bool operator >=(const compressed_pair& y) const
	noexcept(noexcept(!(*this < y))) {
    	return !(*this < y);
    }
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename IfEmpty, typename T>
compressed_pair(IfEmpty, T) -> compressed_pair<IfEmpty, T>;
#endif


template <typename IfEmpty, typename T>
struct compressed_pair<IfEmpty, T, false> final : icommon<compressed_pair<IfEmpty, T, false>> {
	IfEmpty no_compressed;
	T value;

    constexpr compressed_pair()
        noexcept(conjunction_v<is_nothrow_default_constructible<IfEmpty>,
            is_nothrow_default_constructible<T>>) = default;

	constexpr compressed_pair(const compressed_pair& pir)
        noexcept(conjunction_v<is_nothrow_copy_constructible<IfEmpty>, is_nothrow_copy_constructible<T>>)
		: no_compressed(pir.no_compressed), value(pir.value) {}

	constexpr compressed_pair(compressed_pair&& pir)
        noexcept(conjunction_v<is_nothrow_move_constructible<IfEmpty>, is_nothrow_move_constructible<T>>)
		: no_compressed(_MSTL move(pir.no_compressed)), value(_MSTL move(pir.value)) {}

    constexpr compressed_pair& operator=(compressed_pair&& pir) noexcept(
        conjunction_v<is_nothrow_move_assignable<IfEmpty>, is_nothrow_move_assignable<T>>) {
	    no_compressed = _MSTL move(pir.no_compressed);
	    value = _MSTL move(pir.value);
	    return *this;
	}

	template <typename... Args>
	constexpr explicit compressed_pair(_MSTL_TAG default_construct_tag, Args&&... args)
		noexcept(conjunction_v<is_nothrow_default_constructible<IfEmpty>, is_nothrow_constructible<T, Args...>>)
		: no_compressed(), value(_MSTL forward<Args>(args)...) {}

	template <typename ToEmpty, typename... Args>
	constexpr compressed_pair(_MSTL_TAG exact_arg_construct_tag, ToEmpty&& first, Args&&... args)
		noexcept(conjunction_v<is_nothrow_constructible<IfEmpty, ToEmpty>, is_nothrow_constructible<T, Args...>>)
		: no_compressed(_MSTL forward<ToEmpty>(first)), value(_MSTL forward<Args>(args)...) {
	}

	constexpr IfEmpty& get_base() noexcept {
		return no_compressed;
	}
	constexpr const IfEmpty& get_base() const noexcept {
		return no_compressed;
	}

	constexpr void swap(compressed_pair& rh)
	noexcept(conjunction_v<is_nothrow_swappable<IfEmpty>, is_nothrow_swappable<T>>) {
		_MSTL swap(value, rh.value);
		_MSTL swap(no_compressed, rh.no_compressed);
	}

	constexpr size_t to_hash() const
	noexcept(noexcept(hash<IfEmpty>{}(no_compressed) ^ hash<T>{}(value))) {
		return hash<IfEmpty>{}(no_compressed) ^ hash<T>{}(value);
	}

	constexpr bool operator ==(const compressed_pair& y) const
	noexcept(noexcept(this->no_compressed == y.no_compressed && this->value == y.value)) {
		return this->no_compressed == y.no_compressed && this->value == y.value;
	}
	constexpr bool operator !=(const compressed_pair& y) const
	noexcept(noexcept(!(*this == y))) {
		return !(*this == y);
	}
	constexpr bool operator <(const compressed_pair& y) const
	noexcept(noexcept(this->no_compressed < y.no_compressed || (!(y.no_compressed < this->no_compressed) && this->value < y.value))) {
		return this->no_compressed < y.no_compressed || (!(y.no_compressed < this->no_compressed) && this->value < y.value);
	}
	constexpr bool operator >(const compressed_pair& y) const
	noexcept(noexcept(y < *this)) {
		return y < *this;
	}
	constexpr bool operator <=(const compressed_pair& y) const
	noexcept(noexcept(!(*this > y))) {
		return !(*this > y);
	}
	constexpr bool operator >=(const compressed_pair& y) const
	noexcept(noexcept(!(*this < y))) {
		return !(*this < y);
	}
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_COMPOUND_COMPRESSED_PAIR_HPP__
