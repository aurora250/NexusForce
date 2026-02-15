#ifndef MSTL_CORE_CONTAINER_STACK_HPP__
#define MSTL_CORE_CONTAINER_STACK_HPP__

/**
 * @file stack.hpp
 * @brief MSTL栈容器适配器
 *
 * 此文件提供了栈容器适配器的实现。
 * 栈是一种后进先出（LIFO）的数据结构，
 * 元素只能从一端（栈顶）插入和删除。
 */

#include "MSTL/core/container/deque.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Stack 栈
 * @brief 后进先出（LIFO）容器适配器
 * @{
 */

/**
 * @class stack
 * @brief 栈容器适配器
 * @tparam T 元素类型
 * @tparam Sequence 底层容器类型，默认为deque<T>
 *
 * 栈是一种容器适配器，提供后进先出的数据结构特性。
 * 元素只能在栈顶插入和删除。支持基本的栈操作：
 * 压栈（push）、弹栈（pop）、访问栈顶元素（top）。
 * 默认使用deque作为底层容器，也可指定其他支持back、push_back
 * 和pop_back操作的容器（如vector、list）。
 */
template <typename T, typename Sequence = deque<T>>
class stack : public icollector<stack<T, Sequence>> {
    static_assert(is_object_v<T>, "stack only contains object types.");
    static_assert(is_same_v<T, typename Sequence::value_type>, "stack require consistent types.");

public:
    using value_type        = typename Sequence::value_type;  ///< 值类型
    using difference_type   = typename Sequence::difference_type;  ///< 差值类型
    using size_type         = typename Sequence::size_type;  ///< 大小类型
    using reference         = typename Sequence::reference;  ///< 引用类型
    using const_reference   = typename Sequence::const_reference;  ///< 常量引用类型

private:
    Sequence seq_{};  ///< 底层容器实例

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空栈。
     */
    stack() = default;

    /**
     * @brief 构造函数，使用指定的底层容器副本
     * @param seq 底层容器副本
     */
    explicit stack(const Sequence& seq)
    : seq_(seq) {}

    /**
     * @brief 移动构造函数，使用指定的底层容器
     * @param seq 要移动的底层容器
     */
    explicit stack(Sequence&& seq)
    noexcept(is_nothrow_move_constructible_v<Sequence>)
    : seq_(_MSTL move(seq)) {}

    /**
     * @brief 析构函数
     */
    ~stack() = default;

    /**
     * @brief 获取栈的大小
     * @return 栈中的元素数量
     */
    MSTL_NODISCARD size_type size() const
    noexcept(noexcept(seq_.size())) {
        return seq_.size();
    }

    /**
     * @brief 检查栈是否为空
     * @return 栈为空返回true，否则返回false
     */
    MSTL_NODISCARD bool empty() const
    noexcept(noexcept(seq_.empty())) {
        return seq_.empty();
    }

    /**
     * @brief 访问栈顶元素
     * @return 栈顶元素的引用
     */
    MSTL_NODISCARD reference top()
    noexcept(noexcept(seq_.back())) {
        return seq_.back();
    }

    /**
     * @brief 常量访问栈顶元素
     * @return 栈顶元素的常量引用
     */
    MSTL_NODISCARD const_reference top()
    const noexcept(noexcept(seq_.back())) {
        return seq_.back();
    }

    /**
     * @brief 在栈顶就地构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 新构造元素的引用
     */
    template <typename... Args>
    decltype(auto) emplace(Args&&... args) {
        return seq_.emplace(_MSTL forward<Args>(args)...);
    }

    /**
     * @brief 压栈操作（拷贝版本）
     * @param value 要压入的值
     *
     * 将元素添加到栈顶。
     */
    void push(const value_type& value) {
        seq_.push_back(value);
    }

    /**
     * @brief 压栈操作（移动版本）
     * @param value 要压入的值
     *
     * 将元素移动到栈顶。
     */
    void push(value_type&& value) {
        seq_.push_back(_MSTL move(value));
    }

    /**
     * @brief 弹栈操作
     *
     * 移除栈顶元素。
     */
    void pop()
    noexcept(noexcept(seq_.pop_back())) {
        seq_.pop_back();
    }

    /**
     * @brief 交换两个栈的内容
     * @param other 要交换的另一个栈
     */
    void swap(stack& other)
    noexcept(is_nothrow_swappable_v<Sequence>) {
        _MSTL swap(seq_, other.seq_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧栈
     * @return 如果两个栈大小相等且对应元素相等返回true
     */
    MSTL_NODISCARD bool operator ==(const stack& rhs) const
     noexcept(noexcept(seq_ == rhs.seq_)) {
        return seq_ == rhs.seq_;
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧栈
     * @return 按字典序比较结果
     */
    MSTL_NODISCARD bool operator <(const stack& rhs) const
    noexcept(noexcept(seq_ < rhs.seq_)) {
        return seq_ < rhs.seq_;
    }
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Sequence>
stack(Sequence) -> stack<typename Sequence::value_type, Sequence>;
#endif

/** @} */ // Stack

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_STACK_HPP__
