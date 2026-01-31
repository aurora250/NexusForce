#ifndef MSTL_CORE_MEMORY_UNIQUE_PTR_HPP__
#define MSTL_CORE_MEMORY_UNIQUE_PTR_HPP__

/**
 * @file unique_ptr.hpp
 * @brief MSTL独占智能指针
 *
 * 此文件提供了独占所有权智能指针的实现，
 * 用于管理动态分配对象的生命周期，确保资源的唯一所有权和自动释放。
 */

#include "MSTL/core/functional/functor.hpp"
#include "MSTL/core/functional/invoke.hpp"
#include "MSTL/core/utility/tuple.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup UniquePointer 独占智能指针
 * @brief 独占智能指针类和辅助工具
 * @{
 */

/**
 * @struct default_delete
 * @brief 默认删除器
 * @tparam T 元素类型
 *
 * 使用delete运算符释放单个对象的默认删除器。
 */
template <typename T>
struct default_delete {
    constexpr default_delete() noexcept = default;  ///< 默认构造函数

    /**
     * @brief 从其他default_delete转换构造
     * @tparam U 可转换为T*的类型
     */
    template <typename U, enable_if_t<is_convertible<U*, T*>::value, int> = 0>
	MSTL_CONSTEXPR20 default_delete(const default_delete<U>&) noexcept {}

    /**
     * @brief 删除操作符
     * @param ptr 要删除的指针
     */
    MSTL_CONSTEXPR20 void operator ()(const T* ptr) const noexcept {
	    delete ptr;
    }

    /**
     * @brief 重新绑定到其他类型的删除器
     * @tparam U 新的元素类型
     * @return 绑定到U的新删除器
     */
    template <typename U>
    MSTL_CONSTEXPR20 default_delete<U> rebind() && noexcept {
        return default_delete<U>();
    }
};

/**
 * @brief 数组特化的默认删除器
 * @tparam T 数组元素类型
 */
template <typename T>
struct default_delete<T[]> {
    constexpr default_delete() noexcept = default;  ///< 默认构造函数

    /**
     * @brief 从其他数组删除器转换构造
     * @tparam U 可转换为T的数组类型
     */
    template <typename U, enable_if_t<is_convertible<U(*)[], T(*)[]>::value, int> = 0>
    MSTL_CONSTEXPR20 default_delete(const default_delete<U[]>&) noexcept {}

    /**
     * @brief 删除操作符
     * @tparam U 数组元素类型
     * @param ptr 要删除的数组指针
     */
    template <typename U, enable_if_t<is_convertible<U(*)[], T(*)[]>::value, int> = 0>
    MSTL_CONSTEXPR20 void operator ()(U* ptr) const noexcept {
	    delete [] ptr;
	}

    /**
     * @brief 重新绑定到其他数组类型的删除器
     * @tparam U 新的数组元素类型
     * @return 绑定到U[]的新删除器
     */
    template <typename U>
    MSTL_CONSTEXPR20 default_delete<U[]> rebind() && noexcept {
        return default_delete<U[]>();
    }
};

/// @cond
MSTL_BEGIN_INNER__

/**
 * @class __unique_ptr_impl
 * @brief unique_ptr内部实现类
 * @tparam T 元素类型
 * @tparam Deleter 删除器类型
 *
 * 封装指针和删除器的底层实现，提供基本的内存管理功能。
 */
template <typename T, typename Deleter>
class __unique_ptr_impl {
private:
    template <typename U, typename E, typename = void>
	struct inner_ptr {
	    using type = U*;
	};
    template <typename U, typename E>
	struct inner_ptr<U, E, void_t<typename remove_reference_t<E>::pointer>> {
	    using type = typename remove_reference<E>::type::pointer;
	};

public:
    using DeleterConstraint = enable_if<is_default_constructible<Deleter>::value>;  ///< 删除器约束类型
    using pointer = typename inner_ptr<T, Deleter>::type;  ///< 指针类型

private:
    _MSTL tuple<pointer, Deleter> tup{};  ///< 存储指针和删除器的元组

    static_assert(!is_rvalue_reference<Deleter>::value,
        "deleter type of unique_ptr must be a function object type or an lvalue reference type");

public:
    __unique_ptr_impl() = default;
    MSTL_CONSTEXPR20 __unique_ptr_impl(pointer ptr) : tup() { get_ptr() = ptr; }

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_impl(pointer ptr, Del&& del)
	: tup(ptr, _MSTL forward<Del>(del)) {}

    MSTL_CONSTEXPR20 __unique_ptr_impl(__unique_ptr_impl&& ptr) noexcept
    : tup(_MSTL move(ptr.tup)) { ptr.get_ptr() = nullptr; }

    MSTL_CONSTEXPR20 __unique_ptr_impl& operator =(__unique_ptr_impl&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<Deleter>(x.get_deleter());
	    return *this;
    }

    MSTL_CONSTEXPR20 pointer& get_ptr() noexcept { return _MSTL get<0>(tup); }
    MSTL_CONSTEXPR20 pointer get_ptr() const noexcept { return _MSTL get<0>(tup); }
    MSTL_CONSTEXPR20 Deleter& get_deleter() noexcept { return _MSTL get<1>(tup); }
    MSTL_CONSTEXPR20 const Deleter& get_deleter() const noexcept { return _MSTL get<1>(tup); }

    MSTL_CONSTEXPR20 void reset(pointer ptr) noexcept {
	    const pointer old = get_ptr();
	    get_ptr() = ptr;
	    if (old) {
	        get_deleter()(old);
	    }
    }

    MSTL_CONSTEXPR20 pointer release() noexcept {
	    pointer p = get_ptr();
	    get_ptr() = nullptr;
	    return p;
    }

    MSTL_CONSTEXPR20 void swap(__unique_ptr_impl& x) noexcept {
	    _MSTL swap(get_ptr(), x.get_ptr());
	    _MSTL swap(get_deleter(), x.get_deleter());
    }
};

/**
 * @struct __unique_ptr_data
 * @brief unique_ptr数据存储类
 * @tparam T 元素类型
 * @tparam Deleter 删除器类型
 * @tparam MoveConstructible 删除器是否可移动构造
 * @tparam MoveAssignable 删除器是否可移动赋值
 *
 * 根据删除器的移动特性选择合适的特殊成员函数实现。
 */
template <typename T, typename Deleter,
    bool MoveConstructible = is_move_constructible<Deleter>::value,
    bool MoveAssignable = is_move_assignable<Deleter>::value>
struct __unique_ptr_data : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = default;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = default;
};

// 删除器可移动构造但不可移动赋值
template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, true, false> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = default;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = delete;
};

// 删除器可移动赋值但不可移动构造
template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, false, true> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = delete;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = default;
};

// 删除器既不可移动构造也不可移动赋值
template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, false, false> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR20 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = delete;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = delete;
};

MSTL_END_INNER__
/// @endcond


/**
 * @class unique_ptr
 * @brief 独占智能指针
 * @tparam T 元素类型
 * @tparam Deleter 删除器类型，默认为default_delete<T>
 *
 * 管理动态分配对象的独占所有权，确保对象在离开作用域时被正确删除。
 */
template <typename T, typename Deleter = default_delete<T>>
class unique_ptr {
private:
    template <typename U>
	using DeleterConstraint =
	    typename _INNER __unique_ptr_impl<T, U>::DeleterConstraint::type;  ///< 删除器约束类型

    _INNER __unique_ptr_data<T, Deleter> data_{};  ///< 内部数据存储

public:
    using pointer       = typename _INNER __unique_ptr_impl<T, Deleter>::pointer;  ///< 指针类型
    using element_type  = T;        ///< 元素类型
    using deleter_type  = Deleter;  ///< 删除器类型

private:
    template <typename U, typename E>
	using safe_conversion = conjunction<
	    is_convertible<typename unique_ptr<U, E>::pointer, pointer>,
        negation<is_array<U>>>;  ///< 安全转换检查

public:
    /**
     * @brief 空指针构造
     * @tparam Del 删除器类型
     */
    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
    constexpr unique_ptr(nullptr_t = nullptr) noexcept {}

    /**
     * @brief 从指针构造
     * @tparam Del 删除器类型
     * @param p 要管理的指针
     */
    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
    MSTL_CONSTEXPR20 unique_ptr(pointer p) noexcept : data_(p) {}

    /**
     * @brief 从指针和复制删除器复制构造
     * @tparam Del 删除器类型
     * @param ptr 要管理的指针
     * @param del 删除器对象
     */
    template <typename Del = deleter_type,
        enable_if_t<is_copy_constructible<Del>::value, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(pointer ptr, const deleter_type& del)
    noexcept : data_(ptr, del) {}

    /**
     * @brief 从指针和移动删除器移动构造
     * @tparam Del 删除器类型
     * @param ptr 要管理的指针
     * @param del 删除器对象
     */
    template <typename Del = deleter_type,
        enable_if_t<is_move_constructible<Del>::value, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(pointer ptr, Del&& del)
    noexcept : data_(ptr, _MSTL move(del)) {}

    /**
     * @brief 禁止从右值引用删除器构造
     */
    template <typename Del = deleter_type, typename DelMoveRef = remove_reference_t<Del>>
    MSTL_CONSTEXPR20 unique_ptr(pointer, enable_if_t<is_lvalue_reference<Del>::value, DelMoveRef&&>) = delete;

    MSTL_CONSTEXPR20 unique_ptr(unique_ptr&&) = default;  ///< 移动构造函数

    /**
     * @brief 从其他unique_ptr转换构造
     * @tparam U 源元素类型
     * @tparam E 源删除器类型
     * @param x 源unique_ptr
     */
    template <typename U, typename E, enable_if_t<conjunction<safe_conversion<U, E>,
        conditional_t<is_reference<Deleter>::value, is_same<E, Deleter>, is_convertible<E, Deleter>>>::value, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(unique_ptr<U, E>&& x) noexcept
    : data_(x.release(), _MSTL forward<E>(x.get_deleter())) {}

    /**
     * @brief 析构函数
     */
    ~unique_ptr() noexcept {
	    static_assert(is_invocable<deleter_type&, pointer>::value, "deleter of unique_ptr must be invocable with a pointer");
	    auto& ptr = data_.get_ptr();
	    if (ptr != nullptr)
	        get_deleter()(_MSTL move(ptr));
	    ptr = pointer();
    }

    unique_ptr& operator =(unique_ptr&&) = default;  ///< 移动赋值运算符

    /**
     * @brief 从其他unique_ptr移动赋值
     * @tparam U 源元素类型
     * @tparam E 源删除器类型
     * @param x 源unique_ptr
     * @return 当前对象引用
     */
    template <typename U, typename E, enable_if_t<conjunction<
        safe_conversion<U, E>, is_assignable<deleter_type&, E&&>>::value, int> = 0>
	MSTL_CONSTEXPR20 unique_ptr& operator =(unique_ptr<U, E>&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<E>(x.get_deleter());
	    return *this;
	}

    /**
     * @brief nullptr赋值运算符
     * @return 当前对象引用
     */
    MSTL_CONSTEXPR20 unique_ptr& operator =(nullptr_t) noexcept {
	    reset();
	    return *this;
    }

    /**
     * @brief 解引用运算符
     * @return 管理对象的引用
     */
    MSTL_CONSTEXPR20 add_lvalue_reference_t<element_type> operator *() const
    noexcept(noexcept(*_MSTL declval<pointer>())) {
	    return *get();
    }

    /**
     * @brief 成员访问运算符
     * @return 管理对象的指针
     */
    MSTL_CONSTEXPR20 pointer operator ->() const noexcept {
	    return get();
    }

    MSTL_CONSTEXPR20 pointer get() const noexcept { return data_.get_ptr(); }  ///< 获取原始指针
    MSTL_CONSTEXPR20 deleter_type& get_deleter() noexcept { return data_.get_deleter(); }  ///< 获取删除器引用
    MSTL_CONSTEXPR20 const deleter_type& get_deleter() const noexcept { return data_.get_deleter(); }  ///< 获取const删除器引用

    /**
     * @brief bool转换运算符
     * @return 是否管理非空对象
     */
    MSTL_CONSTEXPR20 explicit operator bool() const noexcept {
        return get() == pointer() ? false : true;
    }

    /**
     * @brief 释放所有权
     */
    MSTL_CONSTEXPR20 pointer release() noexcept {
        return data_.release();
    }

    /**
     * @brief 重置管理的指针
     */
    MSTL_CONSTEXPR20 void reset(pointer ptr = pointer()) noexcept {
	    static_assert(is_invocable<deleter_type&, pointer>::value,
	        "deleter of unique_ptr must be invocable with a pointer");
	    data_.reset(_MSTL move(ptr));
    }

    /**
     * @brief 交换两个unique_ptr
     * @param x 要交换的对象
     */
    MSTL_CONSTEXPR20 void swap(unique_ptr& x) noexcept {
	    static_assert(is_swappable<Deleter>::value, "deleter must be swappable.");
	    data_.swap(x.data_);
    }

    unique_ptr(const unique_ptr&) = delete;  ///< 禁止复制构造
    unique_ptr& operator =(const unique_ptr&) = delete;  ///< 禁止复制赋值
};

/**
 * @brief 数组特化的unique_ptr
 * @tparam T 数组元素类型
 * @tparam Deleter 删除器类型
 */
template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
    template <typename U>
    using DeleterConstraint =
        typename _INNER __unique_ptr_impl<T, U>::DeleterConstraint::type;  ///< 删除器约束类型

    _INNER __unique_ptr_data<T, Deleter> data_;  ///< 内部数据存储

public:
    using pointer	        = typename _INNER __unique_ptr_impl<T, Deleter>::pointer;  ///< 指针类型
    using element_type    = T;        ///< 元素类型
    using deleter_type    = Deleter;  ///< 删除器类型

private:
    template <typename U, typename E, typename UP = unique_ptr<U, E>,
        typename UP_pointer = typename UP::pointer,
        typename UP_element_type = typename UP::element_type>
	using safe_conversion = conjunction<is_array<U>, is_same<pointer, element_type*>,
          is_same<UP_pointer, UP_element_type*>, is_convertible<UP_element_type(*)[], element_type(*)[]>>;

    template <typename U>
    using safe_conversion_raw = conjunction<disjunction<disjunction<is_same<U, pointer>, is_same<U, nullptr_t>>,
        conjunction<is_pointer<U>, is_same<pointer, element_type*>,
            is_convertible<remove_pointer_t<U>(*)[],element_type(*)[]>>>>;

public:
    /**
     * @brief 从指针构造
     * @tparam U 指针类型
     * @tparam Del 删除器类型
     * @param ptr 要管理的数组指针
     */
    template <typename U, typename Del = Deleter, typename = DeleterConstraint<Del>,
        enable_if_t<safe_conversion_raw<U>::value, int> = 0>
    MSTL_CONSTEXPR20 explicit unique_ptr(U ptr) noexcept : data_(ptr) {}

    /**
     * @brief 从指针和复制删除器构造
     * @tparam U 指针类型
     * @tparam Del 删除器类型
     * @param ptr 要管理的数组指针
     * @param del 删除器对象
     */
    template <typename U, typename Del = deleter_type,
        enable_if_t<conjunction<safe_conversion_raw<U>, is_copy_constructible<Del>>::value, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(U ptr, const deleter_type& del) noexcept
    : data_(ptr, del) {}

    /**
     * @brief 从指针和移动删除器构造
     * @tparam U 指针类型
     * @tparam Del 删除器类型
     * @param ptr 要管理的数组指针
     * @param del 删除器对象
     */
    template <typename U, typename Del = deleter_type,
        enable_if_t<conjunction<safe_conversion_raw<U>, is_move_constructible<Del>>::value, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(U ptr, enable_if_t<!is_lvalue_reference<Del>::value, Del&&> del) noexcept
    : data_(_MSTL move(ptr), _MSTL move(del)) {}

    /**
     * @brief 禁止从右值引用删除器构造
     */
    template <typename U, typename Del = deleter_type, typename DelMoveRef = remove_reference_t<Del>,
        enable_if_t<safe_conversion_raw<U>::value, int> = 0>
	unique_ptr(U, enable_if_t<is_lvalue_reference<Del>::value, DelMoveRef&&>) = delete;

    unique_ptr(unique_ptr&&) = default;  ///< 移动构造函数

    /**
     * @brief 空指针构造
     * @tparam Del 删除器类型
     */
    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
	constexpr unique_ptr(nullptr_t = nullptr) noexcept : data_() {}

    /**
     * @brief 从其他unique_ptr转换构造
     * @tparam U 源元素类型
     * @tparam E 源删除器类型
     * @param x 源unique_ptr
     */
    template <typename U, typename E, enable_if_t<conjunction<safe_conversion<U, E>,
	       conditional_t<is_reference<Deleter>::value, is_same<E, Deleter>, is_convertible<E, Deleter>>>::value, int> = 0>
    MSTL_CONSTEXPR20 unique_ptr(unique_ptr<U, E>&& x) noexcept
    : data_(x.release(), _MSTL forward<E>(x.get_deleter())) {}

    /**
     * @brief 析构函数
     */
    MSTL_CONSTEXPR20 ~unique_ptr() {
	    auto& ptr = data_.get_ptr();
	    if (ptr != nullptr) {
	        get_deleter()(ptr);
	    }
	    ptr = pointer();
    }

    unique_ptr& operator =(unique_ptr&&) = default;  ///< 移动赋值运算符

    /**
     * @brief 从其他unique_ptr移动赋值
     * @tparam U 源元素类型
     * @tparam E 源删除器类型
     * @param x 源unique_ptr
     * @return 当前对象引用
     */
    template <typename U, typename E, enable_if_t<conjunction<
        safe_conversion<U, E>, is_assignable<deleter_type&, E&&>, int>::value> = 0>
    MSTL_CONSTEXPR20 unique_ptr& operator =(unique_ptr<U, E>&& x) noexcept {
	    unique_ptr::reset(x.release());
	    get_deleter() = _MSTL forward<E>(x.get_deleter());
	    return *this;
	}

    /**
     * @brief nullptr赋值运算符
     * @return 当前对象引用
     */
    MSTL_CONSTEXPR20 unique_ptr& operator =(nullptr_t) noexcept {
	    reset();
	    return *this;
    }

    /**
     * @brief 数组下标运算符
     * @param idx 索引
     * @return 数组元素的引用
     */
    MSTL_CONSTEXPR20 add_lvalue_reference_t<element_type> operator [](size_t idx) const {
	    MSTL_DEBUG_VERIFY(get() != pointer(), "_MSTL add_lvalue_reference_t<element_type> failed");
	    return get()[idx];
    }

    MSTL_CONSTEXPR20 pointer get() const noexcept { return data_.get_ptr(); }  ///< 获取原始指针
    MSTL_CONSTEXPR20 deleter_type& get_deleter() noexcept { return data_.get_deleter(); }  ///< 获取删除器引用
    MSTL_CONSTEXPR20 const deleter_type& get_deleter() const noexcept { return data_.get_deleter(); }  ///< 获取const删除器引用

    /**
     * @brief bool转换运算符
     * @return 是否管理非空数组
     */
    MSTL_CONSTEXPR20 explicit operator bool() const noexcept {
        return get() == pointer() ? false : true;
    }

    /**
     * @brief 释放所有权
     */
    MSTL_CONSTEXPR20 pointer release() noexcept {
        return data_.release();
    }

    /**
     * @brief 重置管理的指针
     * @tparam U 指针类型
     * @param ptr 新的指针
     */
    template <typename U, enable_if_t<
        conjunction<disjunction<is_same<U, pointer>,
            conjunction<is_same<pointer, element_type*>,
                is_pointer<U>,
                is_convertible<remove_pointer_t<U>(*)[],element_type(*)[]>>>
        >::value, int> = 0>
    MSTL_CONSTEXPR20 void reset(U ptr) noexcept { data_.reset(_MSTL move(ptr)); }

    /**
     * @brief 重置为空指针
     */
    MSTL_CONSTEXPR20 void reset(nullptr_t = nullptr) noexcept {
        unique_ptr::reset(pointer());
    }

    /**
     * @brief 交换两个unique_ptr
     * @param x 要交换的对象
     */
    MSTL_CONSTEXPR20 void swap(unique_ptr& x) noexcept {
	    static_assert(is_swappable<Deleter>::value, "deleter must be swappable");
	    data_.swap(x.data_);
    }

    unique_ptr(const unique_ptr&) = delete;  ///< 禁止复制构造
    unique_ptr& operator =(const unique_ptr&) = delete;  ///< 禁止复制赋值
};

/**
 * @brief 交换两个unique_ptr
 * @tparam T 元素类型
 * @tparam Deleter 删除器类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 */
template <typename T, typename Deleter,
    enable_if_t<is_swappable<Deleter>::value && is_swappable<T>::value, int> = 0>
void swap(unique_ptr<T, Deleter>& lhs, unique_ptr<T, Deleter>& rhs) noexcept {
    lhs.swap(rhs);
}

/**
 * @brief 相等比较运算符
 * @tparam T 左操作数元素类型
 * @tparam D 左操作数删除器类型
 * @tparam U 右操作数元素类型
 * @tparam E 右操作数删除器类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 是否相等
 */
template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const unique_ptr<T, D>& lhs, const unique_ptr<U, E>& rhs) {
    return lhs.get() == rhs.get();
}

/**
 * @brief 与nullptr的相等比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param lhs unique_ptr对象
 * @return 是否为空
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const unique_ptr<T, D>& lhs, nullptr_t) {
    return !lhs;
}

/**
 * @brief nullptr与unique_ptr的相等比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param rhs unique_ptr对象
 * @return 是否为空
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    nullptr_t, const unique_ptr<T, D>& rhs) {
    return !rhs;
}

/**
 * @brief 不等比较运算符
 * @tparam T 左操作数元素类型
 * @tparam D 左操作数删除器类型
 * @tparam U 右操作数元素类型
 * @tparam E 右操作数删除器类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 是否不等
 */
template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const unique_ptr<T, D>& lhs, const unique_ptr<U, E>& rhs) {
    return lhs.get() != rhs.get();
}

/**
 * @brief 与nullptr的不等比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param lhs unique_ptr对象
 * @return 是否非空
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const unique_ptr<T, D>& lhs, nullptr_t) {
    return static_cast<bool>(lhs);
}

/**
 * @brief nullptr与unique_ptr的不等比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param rhs unique_ptr对象
 * @return 是否非空
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    nullptr_t, const unique_ptr<T, D>& rhs) {
    return static_cast<bool>(rhs);
}

/**
 * @brief 小于比较运算符
 * @tparam T 左操作数元素类型
 * @tparam D 左操作数删除器类型
 * @tparam U 右操作数元素类型
 * @tparam E 右操作数删除器类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 是否小于
 */
template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const unique_ptr<T, D>& lhs, const unique_ptr<U, E>& rhs) {
    using common_t = common_type_t<
        typename unique_ptr<T, D>::pointer,
        typename unique_ptr<U, E>::pointer>;
    return _MSTL less<common_t>()(lhs.get(), rhs.get());
}

/**
 * @brief 与nullptr的小于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param lhs unique_ptr对象
 * @return 是否小于nullptr
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const unique_ptr<T, D>& lhs, nullptr_t) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(lhs.get(), nullptr);
}

/**
 * @brief nullptr与unique_ptr的小于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param rhs unique_ptr对象
 * @return nullptr是否小于
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    nullptr_t, const unique_ptr<T, D>& rhs) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(nullptr, rhs.get());
}

/**
 * @brief 大于比较运算符
 * @tparam T 左操作数元素类型
 * @tparam D 左操作数删除器类型
 * @tparam U 右操作数元素类型
 * @tparam E 右操作数删除器类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 是否大于
 */
template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const unique_ptr<T, D>& lhs, const unique_ptr<U, E>& rhs) {
    return rhs.get() < lhs.get();
}

/**
 * @brief 与nullptr的大于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param lhs unique_ptr对象
 * @return 是否大于nullptr
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const unique_ptr<T, D>& lhs, nullptr_t) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(nullptr, lhs.get());
}

/**
 * @brief nullptr与unique_ptr的大于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param rhs unique_ptr对象
 * @return nullptr是否大于
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    nullptr_t, const unique_ptr<T, D>& rhs) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(rhs.get(), nullptr);
}

/**
 * @brief 小于等于比较运算符
 * @tparam T 左操作数元素类型
 * @tparam D 左操作数删除器类型
 * @tparam U 右操作数元素类型
 * @tparam E 右操作数删除器类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 是否小于等于
 */
template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const unique_ptr<T, D>& lhs, const unique_ptr<U, E>& rhs) {
    return !(lhs > rhs);
}

/**
 * @brief 与nullptr的小于等于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param lhs unique_ptr对象
 * @return 是否小于等于nullptr
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const unique_ptr<T, D>& lhs, nullptr_t) {
    return !(lhs > nullptr);
}

/**
 * @brief nullptr与unique_ptr的小于等于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param rhs unique_ptr对象
 * @return nullptr是否小于等于
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    nullptr_t, const unique_ptr<T, D>& rhs) {
    return !(nullptr > rhs);
}

/**
 * @brief 大于等于比较运算符
 * @tparam T 左操作数元素类型
 * @tparam D 左操作数删除器类型
 * @tparam U 右操作数元素类型
 * @tparam E 右操作数删除器类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 是否大于等于
 */
template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const unique_ptr<T, D>& lhs, const unique_ptr<U, E>& rhs) {
    return !(lhs < rhs);
}

/**
 * @brief 与nullptr的大于等于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param lhs unique_ptr对象
 * @return 是否大于等于nullptr
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const unique_ptr<T, D>& lhs, nullptr_t) {
    return !(lhs < nullptr);
}

/**
 * @brief nullptr与unique_ptr的大于等于比较运算符
 * @tparam T 元素类型
 * @tparam D 删除器类型
 * @param rhs unique_ptr对象
 * @return nullptr是否大于等于
 */
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    nullptr_t, const unique_ptr<T, D>& rhs) {
    return !(nullptr < rhs);
}


/**
 * @brief 禁止的static_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @note 由于unique_ptr是独占所有权，不能从const引用转换
 */
template <typename T, typename U>
unique_ptr<T> static_pointer_cast(const unique_ptr<U>& ptr) = delete;

/**
 * @brief 禁止的const_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @note 由于unique_ptr是独占所有权，不能从const引用转换
 */
template <typename T, typename U>
unique_ptr<T> const_pointer_cast(const unique_ptr<U>& ptr) = delete;

/**
 * @brief 禁止的reinterpret_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @note 由于unique_ptr是独占所有权，不能从const引用转换
 */
template <typename T, typename U>
unique_ptr<T> reinterpret_pointer_cast(const unique_ptr<U>& ptr) = delete;

/**
 * @brief 禁止的dynamic_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @note 由于unique_ptr是独占所有权，不能从const引用转换
 */
template <typename T, typename U>
unique_ptr<T> dynamic_pointer_cast(const unique_ptr<U>& ptr) = delete;


/**
 * @brief static_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @return 转换后的unique_ptr
 */
template <typename T, typename U>
MSTL_CONSTEXPR20 unique_ptr<T> static_pointer_cast(unique_ptr<U>&& ptr) {
    return unique_ptr<T>(static_cast<T*>(ptr.release()), ptr.get_deleter());
}

/**
 * @brief const_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @return 转换后的unique_ptr
 */
template <typename T, typename U>
MSTL_CONSTEXPR20 unique_ptr<T> const_pointer_cast(unique_ptr<U>&& ptr) {
    return unique_ptr<T>(const_cast<T*>(ptr.release()), ptr.get_deleter());
}

/**
 * @brief const_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @return 转换后的unique_ptr
 */
template <typename T, typename U>
unique_ptr<T> reinterpret_pointer_cast(unique_ptr<U>&& ptr) {
    return unique_ptr<T>(reinterpret_cast<T*>(ptr.release()), ptr.get_deleter());
}

/**
 * @brief reinterpret_pointer_cast
 * @tparam T 目标类型
 * @tparam U 源类型
 * @param ptr 源unique_ptr
 * @return 转换后的unique_ptr
 */
template <typename T, typename U>
unique_ptr<T> dynamic_pointer_cast(unique_ptr<U>&& ptr) {
    T* tmp = dynamic_cast<T*>(ptr.release());
    if (tmp != nullptr) {
        return unique_ptr<T>(tmp, _MSTL move(ptr.get_deleter()).template rebind<T>());
    }
    return nullptr;
}


/**
 * @brief unique_ptr的哈希特化
 * @tparam T 元素类型
 * @tparam Deleter 删除器类型
 */
template <typename T, typename Deleter>
struct hash<unique_ptr<T, Deleter>> {
    /**
     * @brief 哈希函数
     * @param ptr 要哈希的unique_ptr
     * @return 哈希值
     */
    MSTL_CONSTEXPR20 size_t operator ()(const unique_ptr<T, Deleter>& ptr) const
    noexcept(noexcept(hash<T>()(ptr.get()))) {
        return hash<T>()(ptr.get());
    }
};


/**
 * @brief 创建unique_ptr
 * @tparam T 元素类型
 * @tparam Args 构造函数参数类型
 * @param args 构造函数参数
 * @return 管理新对象的unique_ptr
 */
template <typename T, typename... Args, enable_if_t<!is_array<T>::value, int> = 0>
MSTL_CONSTEXPR20 unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(_MSTL forward<Args>(args)...));
}

/**
 * @brief 创建unique_ptr
 * @tparam T 数组元素类型
 * @param len 数组长度
 * @return 管理新数组的unique_ptr
 */
template <typename T, enable_if_t<is_unbounded_array<T>::value, int> = 0>
MSTL_CONSTEXPR20 unique_ptr<T> make_unique(const size_t len) {
    return unique_ptr<T>(new remove_extent_t<T>[len]());
}

/**
 * @brief 禁止创建已知边界数组的make_unique
 * @tparam T 已知边界数组类型
 * @tparam Args 参数类型
 */
template <typename T, typename... Args, enable_if_t<is_bounded_array<T>::value, int> = 0>
unique_ptr<T> make_unique(Args&&...) = delete;

/** @} */ // UniquePointer

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_UNIQUE_PTR_HPP__
