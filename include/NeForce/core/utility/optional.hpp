#ifndef NEFORCE_CORE_UTILITY_OPTIONAL_HPP__
#define NEFORCE_CORE_UTILITY_OPTIONAL_HPP__

/**
 * @file optional.hpp
 * @brief 可选值类型
 *
 * 此文件提供了可选值类型，用于表示可能存在或可能不存在的值。
 */

#include <initializer_list>
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/functional/invoke.hpp"
#include "NeForce/core/memory/construct.hpp"
#include "NeForce/core/utility/none.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct optional_exception
 * @brief optional访问异常
 */
struct optional_exception final : memory_exception {
    explicit optional_exception(const char* info = "Access the Null Value of Optional.", const char* type = static_type,
                                const int code = 0) noexcept :
    memory_exception(info, type, code) {}

    explicit optional_exception(const exception& e) :
    memory_exception(e) {}

    ~optional_exception() override = default;
    static constexpr auto static_type = "optional_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup Optional 可选值
 * @brief 可选值类型及相关操作
 * @{
 */

template <typename T>
class optional;


template <typename T>
struct is_optional : false_type {};

template <typename T>
struct is_optional<optional<T>> : true_type {};

template <typename T>
NEFORCE_INLINE17 bool is_optional_v = is_optional<T>::value;


/**
 * @class optional
 * @brief 可选值类
 * @tparam T 存储值的类型
 *
 * 表示一个可能包含值也可能为空的对象。类似于指针，但拥有值语义。
 */
template <typename T>
class optional : icommon<optional<T>> {
    static_assert(!is_any_of_v<remove_cv_t<T>, none_t, inplace_construct_tag>,
                  "optional do not contains none_t and inplace_construct_tag types.");
    static_assert(is_object_v<T> && !is_array_v<T>, "optional only contains non-array object types.");
    static_assert(!is_reference_v<T>, "optional of reference type should use optional<T&> specialization.");

public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using const_pointer = const T*;
    using const_reference = const T&;

private:
    template <typename U>
    using is_valid_optional = bool_constant<!is_any_of_v<remove_cv_t<U>, none_t, inplace_construct_tag> &&
                                            is_object_v<U> && !is_array_v<U>>;

    template <typename U>
    using convertible_from_optional =
            disjunction<is_constructible<T, const optional<U>&>, is_constructible<T, optional<U>&>,
                        is_constructible<T, const optional<U>&&>, is_constructible<T, optional<U>&&>,
                        is_convertible<const optional<U>&, T>, is_convertible<optional<U>&, T>,
                        is_convertible<const optional<U>&&, T>, is_convertible<optional<U>&&, T>>;

    template <typename U>
    using assignable_from_optional =
            disjunction<is_assignable<T&, const optional<U>&>, is_assignable<T&, optional<U>&>,
                        is_assignable<T&, const optional<U>&&>, is_assignable<T&, optional<U>&&>>;

    bool have_value_ = false;
    aligned_storage_t<sizeof(T), alignof(T)> storage_;

    constexpr T* get_ptr() noexcept { return reinterpret_cast<T*>(&storage_); }
    constexpr const T* get_ptr() const noexcept { return reinterpret_cast<const T*>(&storage_); }

public:
    /**
     * @brief 从空值构造
     * @param n 空值
     *
     * 构造一个空的可选值。
     */
    constexpr optional(none_t n = none) noexcept {}

    /**
     * @brief 空值赋值运算符
     * @param n 空值标签
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR20 optional& operator=(none_t n) noexcept {
        reset();
        return *this;
    }

    /**
     * @brief 从值隐式转换构造
     * @tparam U 源值类型
     * @param value 源值
     */
    template <typename U, enable_if_t<is_valid_optional<U>::value && !is_same_v<remove_cvref_t<U>, optional> &&
                                              is_constructible_v<T, U> && is_convertible_v<U, T>,
                                      int> = 0>
    constexpr optional(U&& value) noexcept(is_nothrow_constructible_v<T, U>) :
    have_value_(true) {
        _NEFORCE construct(get_ptr(), _NEFORCE forward<U>(value));
    }

    /**
     * @brief 从值显式转换构造
     * @tparam U 源值类型
     * @param value 源值
     */
    template <typename U, enable_if_t<is_valid_optional<U>::value && !is_same_v<remove_cvref_t<U>, optional> &&
                                              is_constructible_v<T, U> && !is_convertible_v<U, T>,
                                      int> = 0>
    explicit constexpr optional(U&& value) noexcept(is_nothrow_constructible_v<T, U>) :
    have_value_(true) {
        _NEFORCE construct(get_ptr(), _NEFORCE forward<U>(value));
    }

    /**
     * @brief 从值拷贝构造
     * @param value 源值
     */
    explicit constexpr optional(const T& value) noexcept(is_nothrow_copy_constructible_v<T>) :
    have_value_(true) {
        _NEFORCE construct(get_ptr(), value);
    }

    /**
     * @brief 从值移动构造
     * @param value 源值
     */
    explicit constexpr optional(T&& value) noexcept(is_nothrow_move_constructible_v<T>) :
    have_value_(true) {
        _NEFORCE construct(get_ptr(), _NEFORCE move(value));
    }

    /**
     * @brief 从值赋值
     * @tparam U 源值类型
     * @param value 源值
     * @return 当前对象的引用
     */
    template <typename U = T, enable_if_t<!is_same_v<remove_cvref_t<U>, optional> &&
                                                  negation_v<conjunction<is_scalar<T>, is_same<T, decay_t<U>>>> &&
                                                  is_constructible_v<T, U> && is_assignable_v<T&, U>,
                                          int> = 0>
    NEFORCE_CONSTEXPR20 optional& operator=(U&& value) noexcept(is_nothrow_constructible_v<T, U> &&
                                                                is_nothrow_assignable_v<T&, U>) {
        if (have_value_) {
            auto temp = T(_NEFORCE forward<U>(value));
            *get_ptr() = _NEFORCE move(temp);
        } else {
            _NEFORCE construct(get_ptr(), _NEFORCE forward<U>(value));
            have_value_ = true;
        }
        return *this;
    }

    /**
     * @brief 从可选值隐式转换复制构造
     * @tparam U 源可选值类型
     * @param other 源可选值
     */
    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, const U&> &&
                                              is_convertible_v<const U&, T> && convertible_from_optional<U>::value,
                                      int> = 0>
    constexpr optional(const optional<U>& other) noexcept(is_nothrow_constructible_v<T, const U&>) {
        if (other) {
            _NEFORCE construct(get_ptr(), *other);
            have_value_ = true;
        }
    }

    /**
     * @brief 从可选值显式转换复制构造
     * @tparam U 源可选值类型
     * @param other 源可选值
     */
    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, const U&> &&
                                              !is_convertible_v<const U&, T> && convertible_from_optional<U>::value,
                                      int> = 0>
    constexpr explicit optional(const optional<U>& other) noexcept(is_nothrow_constructible_v<T, const U&>) {
        if (other) {
            _NEFORCE construct(get_ptr(), *other);
            have_value_ = true;
        }
    }

    /**
     * @brief 从可选值复制赋值
     * @tparam U 源可选值类型
     * @param other 源可选值
     * @return 当前对象的引用
     */
    template <typename U = T,
              enable_if_t<!is_same_v<remove_cvref_t<U>, optional> && is_constructible_v<T, const U&> &&
                                  is_assignable_v<T&, const U&> && !convertible_from_optional<U>::value &&
                                  !assignable_from_optional<U>::value,
                          int> = 0>
    NEFORCE_CONSTEXPR20 optional&
    operator=(const optional<U>& other) noexcept(is_nothrow_constructible_v<T, const U&> &&
                                                 is_nothrow_assignable_v<T&, const U&>) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        if (other) {
            if (have_value_) {
                *get_ptr() = *other;
            } else {
                _NEFORCE construct(get_ptr(), *other);
                have_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    /**
     * @brief 复制构造函数
     * @param other 源可选值
     */
    optional(const optional& other) {
        if (other.have_value_) {
            _NEFORCE construct(get_ptr(), *other);
            have_value_ = true;
        }
    }

    /**
     * @brief 复制赋值运算符
     * @param other 源可选值
     * @return 当前对象的引用
     */
    optional& operator=(const optional& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        if (other.have_value_) {
            if (have_value_) {
                *get_ptr() = *other;
            } else {
                _NEFORCE construct(get_ptr(), *other);
                have_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    /**
     * @brief 从可选值隐式转换移动构造
     * @tparam U 源可选值类型
     * @param other 源可选值
     */
    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, U> && is_convertible_v<U, T> &&
                                              convertible_from_optional<U>::value,
                                      int> = 0>
    constexpr optional(optional<U>&& other) noexcept(is_nothrow_constructible_v<T, U>) {
        if (other) {
            _NEFORCE construct(get_ptr(), _NEFORCE move(*other));
            have_value_ = true;
        }
    }

    /**
     * @brief 从可选值显式转换移动构造
     * @tparam U 源可选值类型
     * @param other 源可选值
     */
    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, U> && !is_convertible_v<U, T> &&
                                              convertible_from_optional<U>::value,
                                      int> = 0>
    constexpr optional(optional<U>&& other) noexcept(is_nothrow_constructible_v<T, U>) {
        if (other) {
            _NEFORCE construct(get_ptr(), _NEFORCE move(*other));
            have_value_ = true;
        }
    }

    /**
     * @brief 从可选值移动赋值
     * @tparam U 源可选值类型
     * @param other 源可选值
     * @return 当前对象的引用
     */
    template <typename U = T, enable_if_t<!is_same_v<remove_cvref_t<U>, optional> && is_constructible_v<T, U> &&
                                                  is_assignable_v<T&, U> && !convertible_from_optional<U>::value &&
                                                  !assignable_from_optional<U>::value,
                                          int> = 0>
    NEFORCE_CONSTEXPR20 optional& operator=(optional<U>&& other) noexcept(is_nothrow_constructible_v<T, U> &&
                                                                          is_nothrow_assignable_v<T&, U>) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        if (other) {
            if (have_value_) {
                *get_ptr() = _NEFORCE move(*other);
            } else {
                _NEFORCE construct(get_ptr(), _NEFORCE move(*other));
                have_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源可选值
     */
    optional(optional&& other) noexcept {
        if (other.have_value_) {
            _NEFORCE construct(get_ptr(), _NEFORCE move(*other));
            have_value_ = true;
            other.reset();
        }
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源可选值
     * @return 当前对象的引用
     */
    optional& operator=(optional&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }

        if (other.have_value_) {
            if (have_value_) {
                *get_ptr() = _NEFORCE move(*other);
            } else {
                _NEFORCE construct(get_ptr(), _NEFORCE move(*other));
                have_value_ = true;
            }
            other.reset();
        } else {
            reset();
        }

        return *this;
    }

    /**
     * @brief 从引用可选值赋值
     * @tparam U 源引用类型
     * @param other 源可选值
     */
    template <typename U, enable_if_t<is_constructible_v<T, U&>, int> = 0>
    constexpr optional(const optional<U&>& other) {
        if (other) {
            _NEFORCE construct(get_ptr(), *other);
            have_value_ = true;
        }
    }

    /**
     * @brief 从引用可选值赋值
     * @tparam U 源引用类型
     * @param other 源可选值
     * @return 当前对象的引用
     */
    template <typename U, enable_if_t<is_assignable_v<T&, U&>, int> = 0>
    NEFORCE_CONSTEXPR20 optional& operator=(const optional<U&>& other) {
        if (other) {
            if (have_value_) {
                *get_ptr() = *other;
            } else {
                _NEFORCE construct(get_ptr(), *other);
                have_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    /**
     * @brief 原位构造
     * @tparam Types 参数类型
     * @param args 构造参数
     */
    template <typename... Types, enable_if_t<is_constructible_v<T, Types...>, int> = 0>
    constexpr explicit optional(inplace_construct_tag,
                                Types&&... args) noexcept(is_nothrow_constructible_v<T, Types...>) :
    have_value_(true) {
        _NEFORCE construct(get_ptr(), _NEFORCE forward<Types>(args)...);
    }

    /**
     * @brief 使用初始化列表原位构造
     * @tparam U 初始化列表元素类型
     * @tparam Types 参数类型
     * @param ilist 初始化列表
     * @param args 构造参数
     */
    template <typename U, typename... Types,
              enable_if_t<is_constructible_v<T, std::initializer_list<U>&, Types...>, int> = 0>
    constexpr explicit optional(inplace_construct_tag, std::initializer_list<U> ilist, Types&&... args) noexcept(
            is_nothrow_constructible_v<T, std::initializer_list<U>&, Types...>) :
    have_value_(true) {
        _NEFORCE construct(get_ptr(), ilist, _NEFORCE forward<Types>(args)...);
    }

    /**
     * @brief 析构函数
     */
    NEFORCE_CONSTEXPR20 ~optional() noexcept { reset(); }

    /**
     * @brief 原位构造值
     * @tparam Types 参数类型
     * @param args 构造参数
     */
    template <typename... Types, enable_if_t<is_constructible_v<T, Types...>, int> = 0>
    NEFORCE_CONSTEXPR20 void emplace(Types&&... args) noexcept(is_nothrow_constructible_v<T, Types...>) {
        reset();
        _NEFORCE construct(get_ptr(), _NEFORCE forward<Types>(args)...);
        have_value_ = true;
    }

    /**
     * @brief 使用初始化列表原位构造值
     * @tparam U 初始化列表元素类型
     * @tparam Types 参数类型
     * @param ilist 初始化列表
     * @param args 构造参数
     */
    template <typename U, typename... Types,
              enable_if_t<is_constructible_v<T, std::initializer_list<U>&, Types...>, int> = 0>
    NEFORCE_CONSTEXPR20 void
    emplace(std::initializer_list<U> ilist,
            Types&&... args) noexcept(is_nothrow_constructible_v<T, std::initializer_list<U>&, Types...>) {
        reset();
        _NEFORCE construct(get_ptr(), ilist, _NEFORCE forward<Types>(args)...);
        have_value_ = true;
    }

    /**
     * @brief 重置可选值为空
     */
    NEFORCE_CONSTEXPR20 void reset() noexcept {
        if (have_value_) {
            _NEFORCE destroy(get_ptr());
            have_value_ = false;
        }
    }

    /**
     * @brief 检查是否包含值
     * @return 是否包含值
     */
    NEFORCE_NODISCARD constexpr bool has_value() const noexcept { return have_value_; }

    /**
     * @brief 转换为布尔值
     * @return 是否包含值
     */
    constexpr explicit operator bool() const noexcept { return have_value_; }

    /**
     * @brief 取出存储的值
     * @return 存储的值的常量左值引用
     * @throws optional_exception 如果值未存储
     */
    constexpr const_reference value() const& {
        if (!have_value_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no value"));
        }
        return *get_ptr();
    }

    /**
     * @brief 取出存储的值
     * @return 存储的值的左值引用
     * @throws optional_exception 如果值未存储
     */
    constexpr reference value() & {
        if (!have_value_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no value"));
        }
        return *get_ptr();
    }

    /**
     * @brief 取出存储的值
     * @return 存储的值的常量右值引用
     * @throws optional_exception 如果值未存储
     */
    constexpr const value_type&& value() const&& {
        if (!have_value_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no value"));
        }
        return _NEFORCE move(*get_ptr());
    }

    /**
     * @brief 取出存储的值
     * @return 存储的值的右值引用
     * @throws optional_exception 如果值未存储
     */
    constexpr value_type&& value() && {
        if (!have_value_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no value"));
        }
        return _NEFORCE move(*get_ptr());
    }

    /**
     * @brief 取出存储的值的拷贝值
     * @param value 值不存在时返回的值
     * @return 值存在时返回其值的拷贝，不存在时返回参数value
     */
    constexpr value_type value_or(value_type value) const& noexcept(is_nothrow_copy_constructible_v<value_type>) {
        if (!have_value_) {
            return value;
        }
        return *get_ptr();
    }

    /**
     * @brief 取出存储的值的移出值
     * @param value 值不存在时返回的值
     * @return 值存在时返回其值的引用，不存在时返回参数value
     */
    constexpr value_type value_or(value_type value) && noexcept(is_nothrow_move_constructible_v<value_type>) {
        if (!have_value_) {
            return value;
        }
        return _NEFORCE move(*get_ptr());
    }

    /**
     * @brief 常量左值否则操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 值存在时返回自身的拷贝，不存在时调用函数并返回结果
     */
    template <typename F, enable_if_t<is_invocable_v<F> && is_copy_constructible_v<T>, int> = 0>
    constexpr optional or_else(F&& f) const& {
        if (have_value_) {
            return *this;
        }
        return _NEFORCE forward<F>(f)();
    }

    /**
     * @brief 右值否则操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 值存在时返回自身的移动，不存在时调用函数并返回结果
     */
    template <typename F, enable_if_t<is_invocable_v<F> && is_move_constructible_v<T>, int> = 0>
    constexpr optional or_else(F&& f) && {
        if (have_value_) {
            return _NEFORCE move(*this);
        }
        return _NEFORCE forward<F>(f)();
    }

    /**
     * @brief 常量左值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) const& {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(*get_ptr());
        }
        return remove_cvref_t<decltype(f(*get_ptr()))>{};
    }

    /**
     * @brief 左值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) & {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(*get_ptr());
        }
        return remove_cvref_t<decltype(f(*get_ptr()))>{};
    }

    /**
     * @brief 常量右值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) const&& {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*get_ptr()));
        }
        return remove_cvref_t<decltype(f(_NEFORCE move(*get_ptr())))>{};
    }

    /**
     * @brief 右值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) && {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*get_ptr()));
        }
        return remove_cvref_t<decltype(f(_NEFORCE move(*get_ptr())))>{};
    }

    /**
     * @brief 常量左值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) const& -> optional<remove_cvref_t<decltype(f(*get_ptr()))>> {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(*get_ptr());
        }
        return none;
    }

    /**
     * @brief 左值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) & -> optional<remove_cvref_t<decltype(f(*get_ptr()))>> {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(*get_ptr());
        }
        return none;
    }

    /**
     * @brief 右值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值的移动并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) const&& -> optional<remove_cvref_t<decltype(f(_NEFORCE move(*get_ptr())))>> {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*get_ptr()));
        }
        return none;
    }

    /**
     * @brief 常量右值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的值的移动并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) && -> optional<remove_cvref_t<decltype(f(_NEFORCE move(*get_ptr())))>> {
        if (have_value_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*get_ptr()));
        }
        return none;
    }

    /**
     * @brief 常量箭头运算符
     * @return 指向值的常量指针
     */
    constexpr const_pointer operator->() const noexcept { return get_ptr(); }

    /**
     * @brief 箭头运算符
     * @return 指向值的常量指针
     */
    constexpr pointer operator->() noexcept { return get_ptr(); }

    /**
     * @brief 常量左值解引用运算符
     * @return 值的常量左值引用
     */
    constexpr const_reference operator*() const& noexcept { return *get_ptr(); }

    /**
     * @brief 左值解引用运算符
     * @return 值的左值引用
     */
    constexpr reference operator*() & noexcept { return *get_ptr(); }

    /**
     * @brief 常量右值解引用运算符
     * @return 值的常量右值引用
     */
    constexpr const value_type&& operator*() const&& noexcept { return _NEFORCE move(*get_ptr()); }

    /**
     * @brief 右值解引用运算符
     * @return 值的右值引用
     */
    constexpr value_type&& operator*() && noexcept { return _NEFORCE move(*get_ptr()); }

    /**
     * @brief 等于比较运算符
     * @param rhs 右操作数
     * @return 两个可选值是否相等
     */
    constexpr bool operator==(const optional& rhs) const noexcept {
        if (have_value_ != rhs.have_value_) {
            return false;
        }
        if (have_value_) {
            return *get_ptr() == *rhs.get_ptr();
        }
        return true;
    }

    /**
     * @brief 小于比较运算符
     * @param rhs 右操作数
     * @return 当前值是否小于右操作数值
     */
    constexpr bool operator<(const optional& rhs) const noexcept {
        if (!have_value_ || !rhs.have_value_) {
            return false;
        }
        return *get_ptr() < *rhs.get_ptr();
    }

    constexpr bool operator==(none_t) const noexcept { return !have_value_; }
    constexpr bool operator!=(none_t) const noexcept { return have_value_; }
    constexpr bool operator>(none_t) const noexcept { return have_value_; }
    constexpr bool operator<(none_t) const noexcept { return false; }
    constexpr bool operator>=(none_t) const noexcept { return true; }
    constexpr bool operator<=(none_t) const noexcept { return !have_value_; }

    friend constexpr bool operator==(none_t, const optional& rhs) noexcept { return !rhs.have_value_; }
    friend constexpr bool operator!=(none_t, const optional& rhs) noexcept { return rhs.have_value_; }
    friend constexpr bool operator>(none_t, const optional&) noexcept { return false; }
    friend constexpr bool operator<(none_t, const optional& rhs) noexcept { return rhs.have_value_; }
    friend constexpr bool operator>=(none_t, const optional& rhs) noexcept { return !rhs.have_value_; }
    friend constexpr bool operator<=(none_t, const optional&) noexcept { return true; }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    constexpr size_t to_hash() const noexcept {
        return have_value_ ? hash<T>()(*get_ptr()) : constants::FNV_OFFSET_BASIS;
    }

    /**
     * @brief 交换两个可选值
     * @param other 要交换的可选值
     */
    NEFORCE_CONSTEXPR20 void swap(optional& other) noexcept(is_nothrow_move_constructible_v<T> &&
                                                            is_nothrow_swappable_v<T>) {
        if (_NEFORCE addressof(other) == this) {
            return;
        }
        if (have_value_ && other.have_value_) {
            _NEFORCE swap(*this, other);
        } else if (have_value_) {
            other.emplace(_NEFORCE move(**this));
            reset();
        } else if (other.have_value_) {
            optional::emplace(_NEFORCE move(*other));
            other.reset();
        }
    }
};


/**
 * @brief 引用可选值类模板
 * @tparam T 存储的引用类型
 *
 * 引用类型的可选值，存储对现有对象的引用。
 *
 * @note
 * 对存储引用的optional的任何赋值行为都被定义为更新引用位置，而非更新引用的地址的值，这是optional<T&>两种设计方式中的一种
 */
template <typename T>
class optional<T&> : icommon<optional<T&>> {
    static_assert(is_object_v<T> && !is_array_v<T>, "optional<T&> requires T to be an object type.");

public:
    using value_type = T&;            ///< 值类型
    using reference = T&;             ///< 引用类型
    using const_reference = const T&; ///< 常量引用类型
    using pointer = T*;               ///< 指针类型
    using const_pointer = const T*;   ///< 常量指针类型

private:
    T* ptr_ = nullptr; ///< 指向引用的指针

    template <typename U>
    using convertible_from_optional_ref = disjunction<is_convertible<U&, T&>, is_convertible<const U&, T&>>;

public:
    /**
     * @brief 默认构造函数
     * @param n 空值标签
     */
    constexpr optional(none_t n = none) noexcept {}

    /**
     * @brief 从引用构造
     * @param value 引用值
     */
    constexpr optional(T& value) noexcept :
    ptr_(_NEFORCE addressof(value)) {}

    /**
     * @brief 从可转换引用隐式转换构造
     * @tparam U 源引用类型
     * @param value 引用值
     */
    template <typename U, enable_if_t<is_convertible_v<U&, T&>, int> = 0>
    constexpr optional(U& value) noexcept :
    ptr_(_NEFORCE addressof(value)) {}

    /**
     * @brief 从可转换引用显式转换构造
     * @tparam U 源引用类型
     * @param value 引用值
     */
    template <typename U, enable_if_t<!is_convertible_v<U&, T&> && is_constructible_v<T&, U&>, int> = 0>
    constexpr explicit optional(U& value) noexcept :
    ptr_(_NEFORCE addressof(value)) {}

    /**
     * @brief 从引用可选值隐式转换复制构造
     * @tparam U 源引用类型
     * @param other 源可选值
     */
    template <typename U, enable_if_t<convertible_from_optional_ref<U>::value, int> = 0>
    constexpr optional(const _NEFORCE optional<U&>& other) noexcept :
    ptr_(other.ptr_) {}

    /**
     * @brief 从引用可选值显式转换复制构造
     * @tparam U 源引用类型
     * @param other 源可选值
     */
    template <typename U, enable_if_t<!convertible_from_optional_ref<U>::value && is_constructible_v<T&, U&>, int> = 0>
    constexpr explicit optional(const _NEFORCE optional<U&>& other) noexcept :
    ptr_(other.ptr_) {}

    /**
     * @brief 空值赋值运算符
     * @param n 空值标签
     * @return 当前对象的引用
     *
     * 取消对目标的引用
     */
    NEFORCE_CONSTEXPR20 optional& operator=(none_t n) noexcept {
        ptr_ = nullptr;
        return *this;
    }

    /**
     * @brief 从引用赋值
     * @tparam U 源引用类型
     * @param value 引用值
     * @return 当前对象的引用
     */
    template <typename U = T, enable_if_t<is_assignable_v<T&, U&>, int> = 0>
    NEFORCE_CONSTEXPR20 optional& operator=(U& value) {
        if (ptr_) {
            *ptr_ = value;
        } else {
            ptr_ = _NEFORCE addressof(value);
        }
        return *this;
    }

    /**
     * @brief 从引用可选值赋值
     * @tparam U 源引用类型
     * @param other 源可选值
     * @return 当前对象的引用
     */
    template <typename U, enable_if_t<is_assignable_v<T&, U&>, int> = 0>
    NEFORCE_CONSTEXPR20 optional& operator=(const _NEFORCE optional<U&>& other) {
        if (this != _NEFORCE addressof(other)) {
            if (other.ptr_) {
                if (ptr_) {
                    *ptr_ = *other.ptr_;
                } else {
                    ptr_ = other.ptr_;
                }
            } else {
                ptr_ = nullptr;
            }
        }
        return *this;
    }

    /**
     * @brief 复制构造函数
     * @param other 源可选值
     */
    constexpr optional(const optional& other) noexcept = default;

    /**
     * @brief 复制赋值运算符
     * @param other 源可选值
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR20 optional& operator=(const optional& other) noexcept = default;

    /**
     * @brief 移动构造函数
     * @param other 源可选值
     */
    constexpr optional(optional&& other) noexcept :
    ptr_(other.ptr_) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源可选值
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR20 optional& operator=(optional&& other) noexcept {
        ptr_ = other.ptr_;
        return *this;
    }

    template <typename... Types>
    constexpr optional(inplace_construct_tag, Types&&...) = delete;

    /**
     * @brief 析构函数
     */
    ~optional() noexcept { ptr_ = nullptr; }

    /**
     * @brief 隐式转换原位构造引用
     * @tparam U 源引用类型
     * @param value 引用值
     * @return 存储的引用
     */
    template <typename U, enable_if_t<is_convertible_v<U&, T&>, int> = 0>
    NEFORCE_CONSTEXPR20 T& emplace(U& value) noexcept {
        ptr_ = _NEFORCE addressof(value);
        return *ptr_;
    }

    /**
     * @brief 显式转换原位构造引用
     * @tparam U 源引用类型
     * @param value 引用值
     * @return 存储的引用
     */
    template <typename U, enable_if_t<!is_convertible_v<U&, T&> && is_constructible_v<T&, U&>, int> = 0>
    NEFORCE_CONSTEXPR20 T& emplace(U& value) noexcept {
        ptr_ = _NEFORCE addressof(value);
        return *ptr_;
    }

    /**
     * @brief 重置引用
     */
    NEFORCE_CONSTEXPR20 void reset() noexcept { ptr_ = nullptr; }

    /**
     * @brief 检查是否持有引用
     */
    NEFORCE_NODISCARD constexpr bool has_value() const noexcept { return ptr_ != nullptr; }

    /**
     * @brief 转换为布尔值
     * @return 是否持有引用
     */
    constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /**
     * @brief 获取常量左值引用
     * @return 存储的常量左值引用
     * @throws optional_exception 如果引用未存储
     */
    constexpr const T& value() const& {
        if (!ptr_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no reference"));
        }
        return *ptr_;
    }

    /**
     * @brief 获取左值引用
     * @return 存储的左值引用
     * @throws optional_exception 如果引用未存储
     */
    constexpr T& value() & {
        if (!ptr_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no reference"));
        }
        return *ptr_;
    }

    /**
     * @brief 获取常量右值引用
     * @return 存储的常量右值引用
     * @throws optional_exception 如果引用未存储
     */
    constexpr const T&& value() const&& {
        if (!ptr_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no reference"));
        }
        return *ptr_;
    }

    /**
     * @brief 获取右值引用
     * @return 存储的右值引用
     * @throws optional_exception 如果引用未存储
     */
    constexpr T&& value() && {
        if (!ptr_) {
            NEFORCE_THROW_EXCEPTION(optional_exception("optional have no reference"));
        }
        return *ptr_;
    }

    /**
     * @brief 取出存储的引用的拷贝值
     * @param value 引用不存在时返回的值
     * @return 引用存在时返回其值的拷贝，不存在时返回参数value
     */
    template <typename U>
    constexpr T value_or(U&& value) const& {
        if (ptr_) {
            return *ptr_;
        }
        return _NEFORCE forward<U>(value);
    }

    /**
     * @brief 取出存储的引用的移动值
     * @param value 引用不存在时返回的值
     * @return 引用存在时返回其值的移动，不存在时返回参数value
     */
    template <typename U>
    constexpr T value_or(U&& value) && {
        if (ptr_) {
            return _NEFORCE move(*ptr_);
        }
        return _NEFORCE forward<U>(value);
    }

    /**
     * @brief 常量左值否则操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 引用存在时返回自身的拷贝，不存在时调用函数并返回结果
     */
    template <typename F, enable_if_t<is_invocable_v<F>, int> = 0>
    constexpr optional or_else(F&& f) const& {
        if (ptr_) {
            return *this;
        }
        return _NEFORCE forward<F>(f)();
    }

    /**
     * @brief 右值否则操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 引用存在时返回自身的移动，不存在时调用函数并返回结果
     */
    template <typename F, enable_if_t<is_invocable_v<F>, int> = 0>
    constexpr optional or_else(F&& f) && {
        if (ptr_) {
            return _NEFORCE move(*this);
        }
        return _NEFORCE forward<F>(f)();
    }

    /**
     * @brief 常量左值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) const& {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(*ptr_);
        }
        return remove_cvref_t<decltype(f(*ptr_))>{};
    }

    /**
     * @brief 左值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) & {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(*ptr_);
        }
        return remove_cvref_t<decltype(f(*ptr_))>{};
    }

    /**
     * @brief 常量右值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值的移动并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) const&& {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*ptr_));
        }
        return remove_cvref_t<decltype(f(*ptr_))>{};
    }

    /**
     * @brief 右值然后操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值的移动并返回函数结果，或返回函数返回类型的默认构造类型
     */
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) && {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*ptr_));
        }
        return remove_cvref_t<decltype(f(*ptr_))>{};
    }

    /**
     * @brief 常量左值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) const& -> _NEFORCE optional<remove_cvref_t<decltype(f(*ptr_))>> {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(*ptr_);
        }
        return none;
    }

    /**
     * @brief 左值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) & -> _NEFORCE optional<remove_cvref_t<decltype(f(*ptr_))>> {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(*ptr_);
        }
        return none;
    }

    /**
     * @brief 常量右值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值的移动并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) const&& -> _NEFORCE optional<remove_cvref_t<decltype(f(*ptr_))>> {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*ptr_));
        }
        return none;
    }

    /**
     * @brief 右值转换操作
     * @tparam F 函数类型
     * @param f 函数对象
     * @return 函数处理存储的引用值的移动并返回新的optional，或返回none的optional
     */
    template <typename F>
    constexpr auto transform(F&& f) && -> _NEFORCE optional<remove_cvref_t<decltype(f(*ptr_))>> {
        if (ptr_) {
            return _NEFORCE forward<F>(f)(_NEFORCE move(*ptr_));
        }
        return none;
    }

    /**
     * @brief 常量箭头运算符
     * @return 指向值的常量指针
     */
    constexpr const T* operator->() const noexcept { return ptr_; }

    /**
     * @brief 箭头运算符
     * @return 指向值的指针
     */
    constexpr T* operator->() noexcept { return ptr_; }

    /**
     * @brief 常量左值解引用运算符
     * @return 值的常量左值引用
     */
    constexpr const T& operator*() const& noexcept { return *ptr_; }

    /**
     * @brief 左值解引用运算符
     * @return 值的左值引用
     */
    constexpr T& operator*() & noexcept { return *ptr_; }

    /**
     * @brief 常量右值解引用运算符
     * @return 值的常量右值引用
     */
    constexpr const T&& operator*() const&& noexcept { return *ptr_; }

    /**
     * @brief 右值解引用运算符
     * @return 值的右值引用
     */
    constexpr T&& operator*() && noexcept { return *ptr_; }

    /**
     * @brief 等于比较运算符
     * @param rhs 右操作数
     * @return 两个可选值是否相等
     */
    constexpr bool operator==(const optional& rhs) const noexcept {
        if (ptr_ == nullptr || rhs.ptr_ == nullptr) {
            return ptr_ == rhs.ptr_;
        }
        return *ptr_ == *rhs.ptr_;
    }

    /**
     * @brief 小于比较运算符
     * @param rhs 右操作数
     * @return 当前值是否小于右操作数值
     */
    constexpr bool operator<(const optional& rhs) const noexcept { return ptr_ && rhs.ptr_ && *ptr_ < *rhs.ptr_; }

    constexpr bool operator==(none_t) const noexcept { return ptr_ == nullptr; }
    constexpr bool operator!=(none_t) const noexcept { return ptr_ != nullptr; }
    constexpr bool operator>(none_t) const noexcept { return ptr_ != nullptr; }
    constexpr bool operator<(none_t) const noexcept { return false; }
    constexpr bool operator>=(none_t) const noexcept { return true; }
    constexpr bool operator<=(none_t) const noexcept { return ptr_ == nullptr; }

    friend constexpr bool operator==(none_t, const optional& rhs) noexcept { return rhs.ptr_ == nullptr; }
    friend constexpr bool operator!=(none_t, const optional& rhs) noexcept { return rhs.ptr_ != nullptr; }
    friend constexpr bool operator>(none_t, const optional&) noexcept { return false; }
    friend constexpr bool operator<(none_t, const optional& rhs) noexcept { return rhs.ptr_ != nullptr; }
    friend constexpr bool operator>=(none_t, const optional& rhs) noexcept { return rhs.ptr_ == nullptr; }
    friend constexpr bool operator<=(none_t, const optional&) noexcept { return true; }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    constexpr size_t to_hash() const noexcept {
        return ptr_ ? hash<remove_cvref_t<T>>()(*ptr_) : constants::FNV_OFFSET_BASIS;
    }

    /**
     * @brief 交换两个可选值
     * @param other 要交换的可选值
     */
    NEFORCE_CONSTEXPR20 void swap(optional& other) noexcept { _NEFORCE swap(ptr_, other.ptr_); }
};

#ifdef NEFORCE_STANDARD_17
template <typename T>
optional(T) -> optional<T>;
#endif


/**
 * @brief 从值创建可选值
 * @tparam T 值类型
 * @param value 要包装的值
 * @return 包装值的可选值
 */
template <typename T, enable_if_t<is_constructible_v<decay_t<T>, T>, int> = 0>
constexpr optional<decay_t<T>> make_optional(T&& value) noexcept(is_nothrow_constructible_v<optional<decay_t<T>>, T>) {
    return optional<decay_t<T>>{_NEFORCE forward<T>(value)};
}

/**
 * @brief 原位构造可选值
 * @tparam T 值类型
 * @tparam Args 参数类型
 * @param args 构造参数
 * @return 构造的可选值
 */
template <typename T, typename... Args, enable_if_t<is_constructible_v<T, Args...>, int> = 0>
constexpr optional<T> make_optional(Args&&... args) noexcept(is_nothrow_constructible_v<T, Args...>) {
    return optional<T>{inplace_construct_tag{}, _NEFORCE forward<Args>(args)...};
}

/**
 * @brief 使用初始化列表原位构造可选值
 * @tparam T 值类型
 * @tparam U 初始化列表元素类型
 * @tparam Args 参数类型
 * @param ilist 初始化列表
 * @param args 构造参数
 * @return 构造的可选值
 */
template <typename T, typename U, typename... Args>
constexpr enable_if_t<is_constructible_v<T, std::initializer_list<U>&, Args...>, optional<T>>
make_optional(std::initializer_list<U> ilist,
              Args&&... args) noexcept(is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>) {
    return optional<T>{inplace_construct_tag{}, ilist, _NEFORCE forward<Args>(args)...};
}

/**
 * @brief 创建引用可选值
 * @tparam T 引用类型
 * @param value 引用值
 * @return 包装引用的可选值
 */
template <typename T>
constexpr optional<T&> make_optional(T& value) noexcept {
    return optional<T&>{value};
}

/**
 * @note 禁用从右值创建引用可选值，防止悬垂引用
 */
template <typename T>
constexpr optional<remove_reference_t<T>&> make_optional(T&&) = delete;


/**
 * @brief 获取可选值中的值
 * @tparam T 可选值持有的值类型
 * @param opt 可选值
 * @return 指定位置元素的常量左值引用
 * @throws optional_exception 如果值未存储
 */
template <typename T>
constexpr const T& get(const optional<T>& opt) {
    return static_cast<const T&>(static_cast<const optional<T>&>(opt).value());
}

/**
 * @brief 获取可选值中的值
 * @tparam T 可选值持有的值类型
 * @param opt 可选值
 * @return 指定位置元素的左值引用
 * @throws optional_exception 如果值未存储
 */
template <typename T>
constexpr T& get(optional<T>& opt) {
    return static_cast<T&>(static_cast<optional<T>&>(opt).value());
}

/**
 * @brief 获取可选值中的值
 * @tparam T 可选值持有的值类型
 * @param opt 可选值
 * @return 指定位置元素的常量右值引用
 * @throws optional_exception 如果值未存储
 */
template <typename T>
constexpr const T&& get(const optional<T>&& opt) {
    return static_cast<const T&&>(static_cast<const optional<T>&&>(opt).value());
}

/**
 * @brief 获取可选值中的值
 * @tparam T 可选值持有的值类型
 * @param opt 可选值
 * @return 指定位置元素的右值引用
 * @throws optional_exception 如果值未存储
 */
template <typename T>
constexpr T&& get(optional<T>&& opt) {
    return static_cast<T&&>(static_cast<optional<T>&&>(opt).value());
}

/** @} */ // Optional

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_OPTIONAL_HPP__
