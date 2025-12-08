#ifndef MSTL_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
#define MSTL_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
#include "../typeinfo/pointer_traits.hpp"
#include "../memory/construct.hpp"
#include "../numeric/numeric_limits.hpp"
MSTL_BEGIN_NAMESPACE__


struct __allocator_traits_base {
    template <typename T, typename U, typename = void>
    struct alloc_rebind {
        using type = typename replace_first_parameter<U, T>::type;
    };
    template <typename T, typename U>
    struct alloc_rebind<T, U, void_t<typename T::template rebind<U>::other>> {
        using type = typename T::template rebind<U>::other;
    };

    template <typename T, typename U>
    using alloc_rebind_t = typename alloc_rebind<T, U>::type;

protected:
    template <typename T>
    using __pointer = typename T::pointer;
    template <typename T>
    using __c_pointer = typename T::const_pointer;
    template <typename T>
    using __v_pointer = typename T::void_pointer;
    template <typename T>
    using __cv_pointer = typename T::const_void_pointer;
    template <typename T>
    using __pocca = typename T::propagate_on_container_copy_assignment;
    template <typename T>
    using __pocma = typename T::propagate_on_container_move_assignment;
    template <typename T>
    using __pocs = typename T::propagate_on_container_swap;
    template <typename T>
    using __equal = typename T::is_always_equal;
};


template <typename Alloc>
struct allocator_traits : __allocator_traits_base {
	typedef Alloc allocator_type;
	typedef typename Alloc::value_type value_type;

	using pointer = detected_or_t<value_type*, __pointer, Alloc>;

private:
	template <template <typename> class, typename T, typename = void>
	struct real_ptr {
		using type = typename pointer_traits<pointer>::template rebind<T>;
	};

	template <template <typename> class _Func, typename T>
	struct real_ptr<_Func, T, void_t<_Func<Alloc>>> {
		using type = _Func<Alloc>;
	};

	template <typename, typename _PtrT, typename = void>
	struct real_diff {
		using type = typename pointer_traits<_PtrT>::difference_type;
	};

	template <typename _A2, typename _PtrT>
	struct real_diff<_A2, _PtrT, void_t<typename _A2::difference_type>> {
		using type = typename _A2::difference_type;
	};

	template <typename, typename _DiffT, typename = void>
	struct real_size : make_unsigned<_DiffT> {};

	template <typename _A2, typename _DiffT>
	struct real_size<_A2, _DiffT, void_t<typename _A2::size_type>> {
		using type = typename _A2::size_type;
	};

public:
	using const_pointer = typename real_ptr<__c_pointer, const value_type>::type;
	using void_pointer = typename real_ptr<__v_pointer, void>::type;
	using const_void_pointer = typename real_ptr<__cv_pointer, const void>::type;
	using difference_type = typename real_diff<Alloc, pointer>::type;
	using size_type = typename real_size<Alloc, difference_type>::type;
	using propagate_on_container_copy_assignment = detected_or_t<false_type, __pocca, Alloc>;
	using propagate_on_container_move_assignment = detected_or_t<false_type, __pocma, Alloc>;
	using propagate_on_container_swap = detected_or_t<false_type, __pocs, Alloc>;
	using is_always_equal = detected_or_t<typename is_empty<Alloc>::type, __equal, Alloc>;

	template <typename T>
	using rebind_alloc = alloc_rebind<Alloc, T>;

	template <typename T>
	using rebind_traits = allocator_traits<rebind_alloc<T>>;

private:
	template <typename Alloc2>
	static constexpr auto
	__allocate_aux(Alloc2& alloc, size_type n, const_void_pointer hint, int)
	-> decltype(alloc.allocate(n, hint)) {
		return alloc.allocate(n, hint);
	}

	template <typename Alloc2>
	static constexpr pointer
	__allocate_aux(Alloc2& alloc, size_type n, const_void_pointer, ...) {
		return alloc.allocate(n);
	}

	template <typename T, typename... Args>
	static constexpr enable_if_t<has_construct_v<Alloc, T, Args...>>
	__construct_aux(Alloc& alloc, T* ptr, Args&&... args)
	noexcept(noexcept(alloc.construct(ptr, _MSTL forward<Args>(args)...))) {
		alloc.construct(ptr, _MSTL forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	static constexpr
	enable_if_t<conjunction_v<negation<has_construct<Alloc, T, Args...>>, is_constructible<T, Args...>>>
	__construct_aux(Alloc&, T* ptr, Args&&... args)
	noexcept(_MSTL is_nothrow_constructible<T, Args...>::value) {
		_MSTL construct(ptr, _MSTL forward<Args>(args)...);
	}

	template <typename Alloc2, typename T>
	static constexpr auto __destroy_aux(Alloc2& alloc, T* ptr, int)
	noexcept(noexcept(alloc.destroy(ptr))) -> decltype(alloc.destroy(ptr)) {
		alloc.destroy(ptr);
	}

	template <typename Alloc2, typename T>
	static constexpr void __destroy_aux(Alloc2&, T* ptr, ...)
	noexcept(is_nothrow_destructible_v<T>) {
		_MSTL destroy(ptr);
	}

	template <typename Alloc2>
	static constexpr auto __max_size_aux(Alloc2& alloc, int)
	-> decltype(alloc.max_size()) {
		return alloc.max_size();
	}

	template <typename Alloc2>
	static constexpr size_type __max_size_aux(Alloc2&, ...) {
		return _MSTL numeric_limits<size_type>::max() / sizeof(value_type);
	}

	template <typename Alloc2>
	static constexpr auto __select_aux(Alloc2& alloc, int)
	-> decltype(alloc.select_on_container_copy_construction()) {
		return alloc.select_on_container_copy_construction();
	}

	template <typename Alloc2>
	static constexpr Alloc2 __select_aux(Alloc2& alloc, ...) {
		return alloc;
	}

public:
	MSTL_NODISCARD static MSTL_CONSTEXPR20 pointer allocate(Alloc& alloc, size_type n) {
		return alloc.allocate(n);
	}
	MSTL_NODISCARD static MSTL_CONSTEXPR20 pointer allocate(Alloc& alloc, size_type n, const_void_pointer hint) {
		return __allocate_aux(alloc, n, hint, 0);
	}

	static MSTL_CONSTEXPR20 void deallocate(Alloc& alloc, pointer ptr, size_type n) {
		alloc.deallocate(ptr, n);
	}

	template <typename T, typename... Args>
	static MSTL_CONSTEXPR20 void construct(Alloc& alloc, T* ptr, Args&&... args)
	noexcept(noexcept(__construct_aux(alloc, ptr, _MSTL forward<Args>(args)...))) {
		__construct_aux(alloc, ptr, _MSTL forward<Args>(args)...);
	}

	template <typename T>
	static MSTL_CONSTEXPR20 void destroy(Alloc& alloc, T* ptr)
	noexcept(noexcept(__destroy_aux(alloc, ptr, 0))) {
		__destroy_aux(alloc, ptr, 0);
	}

	static MSTL_CONSTEXPR20 size_type max_size(const Alloc& alloc) noexcept {
		return __max_size_aux(alloc, 0);
	}

	static MSTL_CONSTEXPR20 Alloc select_on_container_copy_construction(const Alloc& __rhs) {
		return __select_aux(__rhs, 0);
	}
};


MSTL_BEGIN_INNER__

template <typename Alloc, typename,
	typename = remove_cvref_t<typename Alloc::value_type>,
	typename = void>
struct __is_alloc_insertable_impl : false_type {};

template <typename Alloc, typename T, typename Value>
struct __is_alloc_insertable_impl<Alloc, T, Value,
	void_t<decltype(allocator_traits<Alloc>::construct(
		declval<Alloc&>(), declval<Value*>(), declval<T>()))>>

	: true_type {};
MSTL_END_INNER__


template <typename Alloc>
struct is_copy_insertable
	: _INNER __is_alloc_insertable_impl<Alloc, typename Alloc::value_type const&>::type {};
template <typename T>
struct is_copy_insertable<allocator<T>> : is_copy_constructible<T> {};

template <typename Alloc>
MSTL_INLINE17 constexpr bool is_copy_insertable_v = is_copy_insertable<Alloc>::value;


template <typename Alloc>
struct is_move_insertable
	: _INNER __is_alloc_insertable_impl<Alloc, typename Alloc::value_type>::type {};

template <typename T>
struct is_move_insertable<allocator<T>> : is_move_constructible<T> {};

template <typename Alloc>
MSTL_INLINE17 constexpr bool is_move_insertable_v = is_move_insertable<Alloc>::value;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
