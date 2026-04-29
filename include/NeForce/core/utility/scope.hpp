#ifndef NEFORCE_CORE_UTILITY_SCOPE_HPP__
#define NEFORCE_CORE_UTILITY_SCOPE_HPP__

/**
 * @file scope.hpp
 * @brief RAII作用域守卫实现
 *
 * 此文件提供了三种RAII作用域守卫类，用于在作用域退出时自动执行清理代码。
 * 基于异常安全机制，确保资源正确释放。
 *
 * 提供的守卫类型：
 * - scope_exit：作用域退出时无条件执行
 * - scope_fail：作用域因异常退出时执行
 * - scope_success：作用域正常退出时执行
 *
 * 使用场景：
 * - 资源自动释放
 * - 事务回滚
 * - 状态恢复
 * - 操作日志记录
 */

#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/utility/compressed_pair.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ScopeGuard 作用域守卫
 * @brief 作用域守卫类集合
 * @{
 */

/**
 * @class scope_exit
 * @brief 作用域退出守卫
 * @tparam Func 可调用对象类型
 *
 * 在作用域退出时无条件执行指定的函数（无论是否发生异常）。
 *
 * @note 守卫对象的析构函数不应抛出异常。
 *       如果函数可能抛出异常，建议使用scope_fail或scope_success。
 */
template <typename Func>
class scope_exit {
private:
    compressed_pair<Func, bool> func_pair_; ///< 函数对象和激活标志

public:
    /**
     * @brief 构造函数（异常不安全）
     * @tparam F 函数对象类型
     * @param func 要执行的函数对象
     *
     * 如果函数对象构造过程中抛出异常，会立即执行func()进行清理。
     */
    template <typename F, enable_if_t<!is_same_v<remove_cvref_t<F>, scope_exit> && is_constructible_v<Func, F> &&
                                              !is_nothrow_constructible_v<Func, F>,
                                      int> = 0>
    explicit scope_exit(F&& func) try :
    func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), true) {
    } catch (...) {
        func();
    }

    /**
     * @brief 构造函数（异常安全）
     * @tparam F 函数对象类型
     * @param func 要执行的函数对象
     *
     * 如果函数对象的构造不抛出异常，使用此版本提高性能。
     */
    template <typename F, enable_if_t<!is_same_v<remove_cvref_t<F>, scope_exit> && is_constructible_v<Func, F> &&
                                              is_nothrow_constructible_v<Func, F>,
                                      int> = 0>
    explicit scope_exit(F&& func) noexcept :
    func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), true) {}

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;

    /**
     * @brief 移动构造函数
     * @param rhs 要移动的守卫对象
     *
     * 转移所有权，原守卫对象被释放。
     */
    scope_exit(scope_exit&& rhs) noexcept(is_nothrow_move_constructible_v<Func>) :
    func_pair_(_NEFORCE move(rhs.func_pair_)) {
        rhs.release();
    }

    scope_exit& operator=(scope_exit&&) = delete;

    /**
     * @brief 析构函数
     *
     * 如果守卫处于激活状态，执行函数对象。
     * 必须保证不抛出异常。
     */
    ~scope_exit() noexcept {
        if (func_pair_.value) {
            func_pair_.get_base()();
        }
    }

    /**
     * @brief 释放守卫
     *
     * 使守卫不再执行清理操作，转移所有权后使用。
     */
    void release() noexcept { func_pair_.value = false; }
};

#ifdef NEFORCE_STANDARD_17
template <typename Func>
scope_exit(Func) -> scope_exit<Func>;
#endif


/**
 * @class scope_fail
 * @brief 作用域失败守卫
 * @tparam Func 可调用对象类型
 *
 * 仅在作用域因异常退出时执行指定的函数。
 * 用于实现事务回滚等异常安全机制。
 */
template <typename Func>
class scope_fail {
private:
    compressed_pair<Func, int> func_pair_; ///< 函数对象和异常计数快照

public:
    /**
     * @brief 构造函数（异常不安全）
     * @tparam F 函数对象类型
     * @param func 要执行的函数对象
     *
     * 记录当前未捕获异常数量，用于析构时判断是否因异常退出。
     */
    template <typename F, enable_if_t<!is_same_v<remove_cvref_t<F>, scope_fail> && is_constructible_v<Func, F> &&
                                              !is_nothrow_constructible_v<Func, F>,
                                      int> = 0>
    explicit scope_fail(F&& func) try :
    func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {
    } catch (...) {
        func();
    }

    /**
     * @brief 构造函数（异常安全）
     * @tparam F 函数对象类型
     * @param func 要执行的函数对象
     */
    template <typename F, enable_if_t<!is_same_v<remove_cvref_t<F>, scope_fail> && is_constructible_v<Func, F> &&
                                              is_nothrow_constructible_v<Func, F>,
                                      int> = 0>
    explicit scope_fail(F&& func) noexcept :
    func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {}

    scope_fail(const scope_fail&) = delete;
    scope_fail& operator=(const scope_fail&) = delete;

    /**
     * @brief 移动构造函数
     * @param rhs 要移动的守卫对象
     */
    scope_fail(scope_fail&& rhs) noexcept :
    func_pair_(_NEFORCE move(rhs.func_pair_)) {
        rhs.release();
    }

    scope_fail& operator=(scope_fail&&) = delete;

    /**
     * @brief 析构函数
     *
     * 如果当前未捕获异常数量大于构造时记录的值，说明因异常退出，
     * 执行函数对象。
     */
    ~scope_fail() noexcept {
        if (uncaught_exceptions() > func_pair_.value) {
            func_pair_.get_base()();
        }
    }

    /**
     * @brief 释放守卫
     *
     * 使守卫不再执行清理操作，通过将异常计数设置为最大值来实现。
     */
    void release() noexcept { func_pair_.value = numeric_traits<int>::max(); }
};

#ifdef NEFORCE_STANDARD_17
template <typename Func>
scope_fail(Func) -> scope_fail<Func>;
#endif


/**
 * @class scope_success
 * @brief 作用域成功守卫
 * @tparam Func 可调用对象类型
 *
 * 仅在作用域正常退出时执行指定的函数。
 * 用于实现提交操作、日志记录等正常流程的清理。
 *
 * @note 函数对象的执行不能抛出异常。
 */
template <typename Func>
class scope_success {
private:
    compressed_pair<Func, int> func_pair_; ///< 函数对象和异常计数快照

public:
    /**
     * @brief 构造函数（异常不安全）
     * @tparam F 函数对象类型
     * @param func 要执行的函数对象
     */
    template <typename F, enable_if_t<!is_same_v<remove_cvref_t<F>, scope_success> && is_constructible_v<Func, F> &&
                                              !is_nothrow_constructible_v<Func, F>,
                                      int> = 0>
    explicit scope_success(F&& func) try :
    func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {
    } catch (...) {
        func();
    }

    /**
     * @brief 构造函数（异常安全）
     * @tparam F 函数对象类型
     * @param func 要执行的函数对象
     */
    template <typename F, enable_if_t<!is_same_v<remove_cvref_t<F>, scope_success> && is_constructible_v<Func, F> &&
                                              is_nothrow_constructible_v<Func, F>,
                                      int> = 0>
    explicit scope_success(F&& func) noexcept :
    func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {}

    scope_success(const scope_success&) = delete;
    scope_success& operator=(const scope_success&) = delete;

    /**
     * @brief 移动构造函数
     * @param rhs 要移动的守卫对象
     */
    scope_success(scope_success&& rhs) noexcept(is_nothrow_move_assignable_v<Func>) :
    func_pair_(_NEFORCE move(rhs.func_pair_)) {
        rhs.release();
    }

    scope_success& operator=(scope_success&&) = delete;

    /**
     * @brief 析构函数
     *
     * 如果当前未捕获异常数量不大于构造时记录的值，说明正常退出，
     * 执行函数对象。
     */
    ~scope_success() noexcept(is_nothrow_invocable_v<Func>) {
        if (uncaught_exceptions() <= func_pair_.value) {
            func_pair_.get_base()();
        }
    }

    /**
     * @brief 释放守卫
     *
     * 使守卫不再执行清理操作，通过将异常计数设置为极小值来实现。
     */
    void release() noexcept { func_pair_.value = -numeric_traits<int>::max(); }
};

#ifdef NEFORCE_STANDARD_17
template <typename Func>
scope_success(Func) -> scope_success<Func>;
#endif

/** @} */ // ScopeGuard

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_SCOPE_HPP__
