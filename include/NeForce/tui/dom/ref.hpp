#ifndef NEFORCE_TUI_REF_HPP__
#define NEFORCE_TUI_REF_HPP__

/**
 * @file ref.hpp
 * @brief 状态绑定适配器
 *
 * 提供 Ref<T> / ConstRef<T> 等 "own-or-borrow" 适配器，
 * 使组件可以绑定外部变量或持有内部状态。
 */

#include "NeForce/core/string/utf.hpp"
#include "NeForce/core/utility/variant.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 可变引用适配器
 * @tparam T 值类型
 *
 * 可持有 T 值或引用外部 T*。
 * 组件通过 Ref<T> 接受绑定参数，调用者可以
 * 传递值（组件拥有状态）或指针（绑定外部状态）。
 */
template <typename T>
class ref {
public:
    /**
     * @brief 默认构造
     */
    ref() :
    storage_(T{}) {}

    /**
     * @brief 从值构造
     * @param value 初始值
     */
    explicit ref(T value) :
    storage_(_NEFORCE move(value)) {}

    /**
     * @brief 从指针构造
     * @param ptr 外部变量指针
     */
    explicit ref(T* ptr) :
    storage_(ptr) {}

    /**
     * @brief 解引用
     * @return 值的引用
     */
    T& operator*() {
        if (holds_pointer()) {
            return *pointer();
        }
        return value();
    }

    /**
     * @brief 常量解引用
     * @return 值的常量引用
     */
    const T& operator*() const {
        if (holds_pointer()) {
            return *pointer();
        }
        return value();
    }

    /**
     * @brief 成员访问
     * @return 值的指针
     */
    T* operator->() { return &operator*(); }

    /**
     * @brief 常量成员访问
     * @return 值的常量指针
     */
    const T* operator->() const { return &operator*(); }

    /**
     * @brief 赋值（同时写入绑定的外部变量）
     * @param v 新值
     * @return 自身引用
     */
    ref& operator=(T v) {
        if (holds_pointer()) {
            *pointer() = _NEFORCE move(v);
        } else {
            value() = _NEFORCE move(v);
        }
        return *this;
    }

    /**
     * @brief 检查是否绑定外部指针
     * @return 是否借用外部状态
     */
    NEFORCE_NODISCARD bool holds_pointer() const noexcept { return storage_.index() == 1; }

private:
    _NEFORCE variant<T, T*> storage_;

    NEFORCE_NODISCARD T& value() { return _NEFORCE get<0>(storage_); }
    NEFORCE_NODISCARD const T& value() const { return _NEFORCE get<0>(storage_); }
    NEFORCE_NODISCARD T* pointer() const { return _NEFORCE get<1>(storage_); }
};

/**
 * @brief 只读引用适配器
 * @tparam T 值类型
 *
 * 可持有 const T 值或引用外部 const T*。
 */
template <typename T>
class const_ref {
public:
    /**
     * @brief 默认构造
     */
    const_ref() :
    storage_(T{}) {}

    /**
     * @brief 从值构造
     * @param value 值
     */
    const_ref(T value) :
    storage_(_NEFORCE move(value)) {}

    /**
     * @brief 从常量指针构造
     * @param ptr 外部常量指针
     */
    const_ref(const T* ptr) :
    storage_(ptr) {}

    /**
     * @brief 解引用
     * @return 常量引用
     */
    const T& operator*() const {
        if (holds_pointer()) {
            return *_NEFORCE get<1>(storage_);
        }
        return _NEFORCE get<0>(storage_);
    }

    /**
     * @brief 成员访问
     * @return 常量指针
     */
    const T* operator->() const { return &operator*(); }

    /**
     * @brief 调用运算符
     * @return 常量引用
     */
    const T& operator()() const { return operator*(); }

    /**
     * @brief 检查是否绑定外部指针
     * @return 是否借用外部状态
     */
    NEFORCE_NODISCARD bool holds_pointer() const noexcept { return storage_.index() == 1; }

private:
    _NEFORCE variant<T, const T*> storage_;
};

/**
 * @brief 字符串可变引用适配器
 */
class string_ref : public ref<string> {
public:
    using ref<string>::ref;

    /**
     * @brief 从 C 字符串构造
     * @param s C 字符串
     */
    string_ref(const char* s) :
    ref(string(s)) {}

    /**
     * @brief 从 string_view 构造
     * @param sv 字符串视图
     */
    string_ref(string_view sv) :
    ref(string(sv)) {}

    /**
     * @brief 从宽字符串构造
     * @param ws 宽字符串
     */
    string_ref(const wchar_t* ws) :
    ref(to_string(ws)) {}
};

/**
 * @brief 字符串只读引用适配器
 */
class const_string_ref : public const_ref<string> {
public:
    using const_ref<string>::const_ref;

    /**
     * @brief 从 C 字符串构造
     * @param s C 字符串
     */
    const_string_ref(const char* s) :
    const_ref(string(s)) {}

    /**
     * @brief 从 string_view 构造
     * @param sv 字符串视图
     */
    const_string_ref(string_view sv) :
    const_ref(string(sv)) {}
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_REF_HPP__
