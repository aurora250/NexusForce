#ifndef NEFORCE_CORE_ITERATOR_RANGES_REPEAT_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_REPEAT_VIEW_HPP__

/**
 * @file repeat_view.hpp
 * @brief 重复值视图
 */

#include "NeForce/core/iterator/ranges/core.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @brief 重复值迭代器
 * @tparam T 重复值的类型
 *
 * 内部保存指向所属视图所存值的指针与剩余重复次数，递增使剩余次数递减，负数表示无界重复。
 * @warning 迭代器不拥有被重复的值：指针指向 repeat_view 的内部存储，视图移动或拷贝后原迭代器失效。
 */
template <typename T>
class repeat_iterator {
public:
    using iterator_category = forward_iterator_tag; ///< 迭代器类别
    using value_type = T;                           ///< 值类型
    using difference_type = ptrdiff_t;              ///< 差值类型
    using pointer = const T*;                       ///< 指针类型
    using reference = const T&;                     ///< 引用类型

    /**
     * @brief 默认构造函数
     * @warning 默认构造的迭代器不持有有效的值指针，解引用行为未定义
     */
    constexpr repeat_iterator() = default;
    /**
     * @brief 构造函数
     * @param value 指向重复值的指针
     * @param count 剩余重复次数，负数表示无界
     * @warning value 必须在迭代器使用期间保持有效
     */
    constexpr repeat_iterator(const T* value, difference_type count) :
    value_(value),
    count_(count) {}

    /**
     * @brief 解引用操作符
     * @return 重复值的常量引用
     * @warning 解引用不持有有效指针的迭代器（默认构造，或所属视图已移动/销毁）行为未定义
     */
    constexpr reference operator*() const { return *value_; }
    /**
     * @brief 成员访问操作符
     * @return 指向重复值的指针
     */
    constexpr pointer operator->() const { return value_; }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     * @note 剩余次数大于 0 时递减；无界（次数为负）时保持不变，因此递增后仍不等于哨兵
     */
    constexpr repeat_iterator& operator++() {
        if (count_ > 0) {
            --count_;
        }
        return *this;
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    constexpr repeat_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一重复值迭代器
     * @return 两者的剩余重复次数是否相等
     * @note 仅比较剩余次数，不比较所指的值，因此来自不同视图的迭代器也可能判等
     */
    constexpr bool operator==(const repeat_iterator& other) const {
        return value_ == other.value_ && count_ == other.count_;
    }
    /**
     * @brief 不等比较操作符
     * @param other 另一重复值迭代器
     * @return 两者的剩余重复次数是否不相等
     */
    constexpr bool operator!=(const repeat_iterator& other) const { return !(*this == other); }

    /**
     * @brief 获取剩余重复次数
     * @return 剩余次数，负数表示无界重复
     */
    NEFORCE_NODISCARD ptrdiff_t count() const noexcept { return count_; }

private:
    const T* value_ = nullptr;
    difference_type count_ = 0;
};

/**
 * @brief 重复值迭代器的哨兵类型
 * @tparam T 重复值的类型
 *
 * 有界时迭代器剩余次数不大于边界计数即表示到达末尾；无界时不与任何迭代器相等。
 * @note 无界哨兵不会终止遍历，必须由 take 一类的截断算子结束。
 */
template <typename T>
class repeat_sentinel {
    bool has_bound_ = false;
    ptrdiff_t bound_count_ = 0;

public:
    /**
     * @brief 默认构造函数
     * @note 构造的哨兵无边界，与任何迭代器都不相等，用于表示无界重复
     */
    constexpr repeat_sentinel() noexcept = default;
    /**
     * @brief 构造函数
     * @param bound 边界计数，迭代器剩余次数不大于该值时视为到达末尾
     * @note 有界 repeat_view 以 0 作为边界计数
     */
    constexpr explicit repeat_sentinel(ptrdiff_t bound) noexcept :
    has_bound_(true),
    bound_count_(bound) {}

    /**
     * @brief 迭代器与哨兵的相等比较
     * @param it 重复值迭代器
     * @param s 重复值哨兵
     * @return 哨兵有界且迭代器剩余次数不大于边界计数时返回 true；无界哨兵始终返回 false
     */
    friend constexpr bool operator==(const repeat_iterator<T>& it, const repeat_sentinel& s) noexcept {
        if (s.has_bound_) {
            return it.count() <= s.bound_count_;
        } else {
            return false;
        }
    }
    /**
     * @brief 迭代器与哨兵的不等比较
     * @param it 重复值迭代器
     * @param s 重复值哨兵
     * @return 是否尚未到达末尾
     * @note 无界哨兵始终返回 true
     */
    friend constexpr bool operator!=(const repeat_iterator<T>& it, const repeat_sentinel& s) noexcept {
        return !(it == s);
    }
    /**
     * @brief 哨兵与迭代器的相等比较
     * @param s 重复值哨兵
     * @param it 重复值迭代器
     * @return 与 it == s 结果相同
     */
    friend constexpr bool operator==(const repeat_sentinel& s, const repeat_iterator<T>& it) noexcept {
        return it == s;
    }
    /**
     * @brief 哨兵与迭代器的不等比较
     * @param s 重复值哨兵
     * @param it 重复值迭代器
     * @return 与 it != s 结果相同
     */
    friend constexpr bool operator!=(const repeat_sentinel& s, const repeat_iterator<T>& it) noexcept {
        return it != s;
    }
};

/**
 * @brief 重复同一个值的视图
 * @tparam T 重复值的类型
 *
 * 以指定次数或无限次重复同一个值，值存放在视图内部，惰性求值，所有元素共享同一份存储。
 * 单参构造产生无界视图，双参构造产生有界视图。
 * @warning 迭代器保存指向视图内部值的指针，视图被移动或拷贝后原迭代器失效（悬垂）；
 *          无界视图必须配合 take 等截断算子使用，否则遍历不会终止。
 */
template <typename T>
class repeat_view : public view_base<repeat_view<T>> {
public:
    using iterator = repeat_iterator<T>; ///< 迭代器类型
    using sentinel = repeat_sentinel<T>; ///< 哨兵类型
    using difference_type = ptrdiff_t;   ///< 差值类型

    /**
     * @brief 默认构造函数
     * @note 默认构造的视图重复值初始化的 T，且为无界视图
     */
    constexpr repeat_view() = default;

    /**
     * @brief 构造无界重复视图
     * @param value 被重复的值
     * @note 无界视图的 end() 不与任何迭代器相等，必须配合 take 等截断算子使用
     */
    constexpr explicit repeat_view(T value) :
    value_(_NEFORCE move(value)) {}

    /**
     * @brief 构造有界重复视图
     * @param value 被重复的值
     * @param count 重复次数
     * @note count 为非正数时视图为空（begin() 与 end() 立即判等）
     */
    constexpr repeat_view(T value, const difference_type count) :
    value_(_NEFORCE move(value)),
    count_(count > 0 ? count : 0),
    has_bound_(true) {}

    /**
     * @brief 获取起始迭代器
     * @return 有界视图返回剩余次数为 count 的迭代器；无界视图返回剩余次数为负的迭代器
     * @warning 迭代器持有指向本视图内部值的指针，视图移动或拷贝后不可继续使用
     */
    constexpr iterator begin() const { return has_bound_ ? iterator(&value_, count_) : iterator(&value_, -1); }
    /**
     * @brief 获取哨兵
     * @return 有界视图返回边界计数为 0 的哨兵；无界视图返回与任何迭代器都不相等的哨兵
     */
    constexpr sentinel end() const { return has_bound_ ? sentinel(0) : sentinel(); }

private:
    T value_{};
    difference_type count_ = 0;
    bool has_bound_ = false;
};

/**
 * @brief 推导指引：由重复值类型推导无界 repeat_view 的模板参数
 */
template <typename T>
repeat_view(T) -> repeat_view<T>;

/**
 * @brief 推导指引：由重复值类型与次数类型推导有界 repeat_view 的模板参数
 */
template <typename T, typename N>
repeat_view(T, N) -> repeat_view<T>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_REPEAT_VIEW_HPP__
