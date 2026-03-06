#ifndef NEFORCE_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
#define NEFORCE_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__

/**
 * @file allocator_traits.hpp
 * @brief 分配器特性
 *
 * 此文件提供了分配器特性模板实现，
 * 用于统一不同分配器的接口，提供分配器的标准访问和操作方式。
 */

#include "NeForce/core/memory/construct.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AllocationTraits 分配器特性
 * @brief 实现通用的分配器特性
 * @{
 */

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @struct __allocator_traits_base
 * @brief 分配器特性基类
 *
 * 提供分配器特性的基础类型提取和元函数。
 */
struct __allocator_traits_base {
	/**
	 * @brief 分配器重新绑定元函数
	 * @tparam T 新的元素类型
	 * @tparam U 原始分配器类型
	 */
    template <typename T, typename U, typename = void>
    struct alloc_rebind {
        using type = replace_first_para_t<U, T>;
    };

	/**
	 * @brief 分配器重新绑定元函数
	 * @tparam T 新的元素类型
	 * @tparam U 原始分配器类型
	 */
    template <typename T, typename U>
    struct alloc_rebind<T, U, void_t<typename T::template rebind<U>::other>> {
        using type = typename T::template rebind<U>::other;
    };

	/**
	 * @brief 分配器重新绑定类型别名
	 */
    template <typename T, typename U>
    using alloc_rebind_t = typename alloc_rebind<T, U>::type;

protected:
    template <typename T>
    using __pointer = typename T::pointer;
    template <typename T>
    using __c_pointer = typename T::const_pointer;
};

NEFORCE_END_INNER__
/// @endcond

/**
 * @struct allocator_traits
 * @brief 分配器特性模板
 * @tparam Alloc 分配器类型
 *
 * 提供对分配器类型的统一访问接口，即使分配器不提供某些成员类型或函数。
 *
 * 主要功能：
 * 1. 提取分配器的各种类型特征
 * 2. 提供分配器操作的统一接口
 * 3. 为不完整的分配器接口提供默认实现
 * 4. 支持分配器的重新绑定
 */
template <typename Alloc>
struct allocator_traits : _INNER __allocator_traits_base {
	using allocator_type = Alloc;
	using value_type	 = typename Alloc::value_type;
	using pointer		 = detected_or_t<value_type*, __pointer, Alloc>;

private:
	template <template <typename> class Func, typename T, typename = void>
	struct real_ptr {
		using type = typename pointer_traits<pointer>::template rebind<T>;
	};

	template <template <typename> class Func, typename T>
	struct real_ptr<Func, T, void_t<Func<Alloc>>> {
		using type = Func<Alloc>;
	};

	template <typename, typename Ptr, typename = void>
	struct real_diff {
		using type = typename pointer_traits<Ptr>::difference_type;
	};

	template <typename AllocU, typename Ptr>
	struct real_diff<AllocU, Ptr, void_t<typename AllocU::difference_type>> {
		using type = typename AllocU::difference_type;
	};

	template <typename, typename Diff, typename = void>
	struct real_size : make_unsigned<Diff> {};

	template <typename AllocU, typename Diff>
	struct real_size<AllocU, Diff, void_t<typename AllocU::size_type>> {
		using type = typename AllocU::size_type;
	};

public:
	using const_pointer = typename real_ptr<__c_pointer, const value_type>::type;   ///< 常量指针类型
	using difference_type = typename real_diff<Alloc, pointer>::type;     ///< 指针差异类型
	using size_type = typename real_size<Alloc, difference_type>::type;   ///< 大小类型

	/**
	 * @brief 重新绑定分配器类型
	 * @tparam T 新的元素类型
	 */
	template <typename T>
	using rebind_alloc = alloc_rebind<Alloc, T>;

	/**
	 * @brief 重新绑定分配器特性类型
	 * @tparam T 新的元素类型
	 */
	template <typename T>
	using rebind_traits = allocator_traits<rebind_alloc<T>>;

private:
	template <typename T, typename... Args>
	static constexpr enable_if_t<has_construct_v<Alloc, T, Args...>>
	__construct_aux(Alloc& alloc, T* ptr, Args&&... args)
	noexcept(noexcept(alloc.construct(ptr, _NEFORCE forward<Args>(args)...))) {
		alloc.construct(ptr, _NEFORCE forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	static constexpr
	enable_if_t<conjunction_v<negation<has_construct<Alloc, T, Args...>>, is_constructible<T, Args...>>>
	__construct_aux(Alloc&, T* ptr, Args&&... args)
	noexcept(_NEFORCE is_nothrow_constructible<T, Args...>::value) {
		_NEFORCE construct(ptr, _NEFORCE forward<Args>(args)...);
	}

	template <typename Alloc2, typename T>
	static constexpr auto __destroy_aux(Alloc2& alloc, T* ptr, int)
	noexcept(noexcept(alloc.destroy(ptr))) -> decltype(alloc.destroy(ptr)) {
		alloc.destroy(ptr);
	}

	template <typename Alloc2, typename T>
	static constexpr void __destroy_aux(Alloc2&, T* ptr, ...)
	noexcept(is_nothrow_destructible_v<T>) {
		_NEFORCE destroy(ptr);
	}

	template <typename Alloc2>
	static constexpr auto __max_size_aux(Alloc2& alloc, int)
	noexcept(noexcept(alloc.max_size()))
	-> decltype(alloc.max_size()) {
		return alloc.max_size();
	}

	template <typename Alloc2>
	static constexpr size_type __max_size_aux(Alloc2&, ...) noexcept {
		return _NEFORCE numeric_traits<size_type>::max() / sizeof(value_type);
	}

public:
	/**
	 * @brief 分配内存
	 * @param alloc 分配器对象
	 * @param n 要分配的元素数量
	 * @return 指向分配内存的指针
	 */
	NEFORCE_NODISCARD static NEFORCE_CONSTEXPR20 pointer allocate(Alloc& alloc, size_type n) {
		return alloc.allocate(n);
	}

	/**
	 * @brief 释放内存
	 * @param alloc 分配器对象
	 * @param ptr 要释放的内存指针
	 * @param n 先前分配的元素数量
	 */
	static NEFORCE_CONSTEXPR20 void deallocate(Alloc& alloc, pointer ptr, size_type n) {
		alloc.deallocate(ptr, n);
	}

	/**
	 * @brief 在已分配内存上构造对象
	 * @tparam T 要构造的对象类型
	 * @tparam Args 构造函数参数类型
	 * @param alloc 分配器对象
	 * @param ptr 指向已分配内存的指针
	 * @param args 构造函数参数
	 */
	template <typename T, typename... Args>
	static NEFORCE_CONSTEXPR20 void construct(Alloc& alloc, T* ptr, Args&&... args)
	noexcept(noexcept(allocator_traits::__construct_aux(alloc, ptr, _NEFORCE forward<Args>(args)...))) {
		allocator_traits::__construct_aux(alloc, ptr, _NEFORCE forward<Args>(args)...);
	}

	/**
	 * @brief 销毁对象
	 * @tparam T 要销毁的对象类型
	 * @param alloc 分配器对象
	 * @param ptr 指向要销毁对象的指针
	 */
	template <typename T>
	static NEFORCE_CONSTEXPR20 void destroy(Alloc& alloc, T* ptr)
	noexcept(noexcept(allocator_traits::__destroy_aux(alloc, ptr, 0))) {
		allocator_traits::__destroy_aux(alloc, ptr, 0);
	}

	/**
	 * @brief 获取分配器支持的最大大小
	 * @param alloc 分配器对象
	 * @return 分配器可以分配的最大元素数量
	 */
	static NEFORCE_CONSTEXPR20 size_type max_size(const Alloc& alloc)
	noexcept(noexcept(allocator_traits::__max_size_aux(alloc, 0))) {
		return allocator_traits::__max_size_aux(alloc, 0);
	}
};

/** @} */ // AllocationTraits

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
