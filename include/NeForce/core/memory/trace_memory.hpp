#ifndef NEFORCE_CORE_MEMORY_TRACE_MEMORY_HPP__
#define NEFORCE_CORE_MEMORY_TRACE_MEMORY_HPP__

/**
 * @file trace_memory.hpp
 * @brief 内存追踪分配器
 *
 * 此文件提供了用于内存泄漏检测的追踪分配器实现。
 * 通过记录每次内存分配的调用栈，在程序结束时检测并报告未释放的内存。
 * 主要用于调试和测试阶段的内存问题诊断。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/system/console.hpp"
#include "NeForce/core/system/stacktrace.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Stacktrace 堆栈跟踪
 * @brief 堆栈跟踪工具，用于调试和错误诊断
 * @{
 */

/**
 * @class trace_allocator
 * @brief 内存追踪分配器
 * @tparam T 分配的元素类型
 *
 * 提供内存分配追踪功能的分配器适配器。
 * 在分配内存时记录调用栈，在释放内存时清除记录。
 * 析构时检查是否有未释放的内存，并输出详细的泄漏信息。
 */
template <typename T>
class trace_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    /**
     * @struct rebind
     * @brief 分配器绑定模板
     * @tparam U 重新绑定的类型
     *
     * 提供将当前分配器重新绑定到其他类型的能力。
     */
    template <typename U>
    struct rebind {
        using other = trace_allocator<U>;
    };

private:
    unordered_map<T*, stacktrace> traces_;  ///< 内存分配追踪表

public:
    /**
     * @brief 默认构造函数
     */
    trace_allocator() = default;

    /**
     * @brief 拷贝构造函数
     * @param other 源分配器
     */
    trace_allocator(const trace_allocator& other)
    : traces_(other.traces_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源分配器
     * @return 自身引用
     */
    trace_allocator& operator =(const trace_allocator& other) {
        if (_NEFORCE addressof(other) == this) return *this;
        traces_ = other.traces_;
        return *this;
    }

    /**
     * @brief 析构函数
     *
     * 检查是否存在未释放的内存，如果有则输出内存泄漏报告。
     */
    ~trace_allocator() {
        if (!traces_.empty()) {
            _NEFORCE printcln(color::red(), "Memory leaks detected! \n");
            print_stacktrace();
        }
    }

    /**
     * @brief 打印所有未释放内存的调用栈
     *
     * 遍历追踪表，输出每个泄漏指针的地址和分配时的调用栈。
     */
    void print_stacktrace() const {
        for(auto& entry : traces_) {
            if (entry.first == 0) continue;
            _NEFORCE printcln(color::red(), "Leaked pointer: ", static_cast<void*>(entry.first));
            _NEFORCE printcln(color::red(), "Allocation stack trace:\n", entry.second);
        }
    }

    /**
     * @brief 分配内存
     * @param n 要分配的元素数量
     * @return 指向分配内存的指针
     *
     * 分配n个T类型元素的内存，并记录当前调用栈。
     */
    NEFORCE_NODISCARD NEFORCE_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        pointer ptr = allocator<T>().allocate(n);
        stacktrace st{};
        traces_[ptr] = _NEFORCE move(st);
        return ptr;
    }

    /**
     * @brief 分配单个元素的内存
     * @return 指向分配内存的指针
     *
     * 分配一个T类型元素的内存，并记录当前调用栈。
     */
    NEFORCE_NODISCARD NEFORCE_ALLOC_OPTIMIZE pointer allocate() {
        return this->allocate(1);
    }

    /**
     * @brief 释放内存
     * @param p 要释放的内存指针
     * @param n 元素数量
     *
     * 释放由allocate分配的内存，并从追踪表中移除记录。
     */
    void deallocate(pointer p, const size_type n) noexcept {
        auto it = traces_.find(p);
        if (it != traces_.end()) {
            traces_.erase(it);
        }
        allocator<T>().deallocate(p, n);
    }

    /**
     * @brief 释放单个元素的内存
     * @param p 要释放的内存指针
     *
     * 释放由allocate()分配的单个元素内存。
     */
    void deallocate(pointer p) noexcept {
        this->deallocate(p, 1);
    }
};

/**
 * @brief 相等比较运算符
 * @tparam T 左操作数类型
 * @tparam U 右操作数类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 始终返回true
 *
 * 所有trace_allocator实例都是相等的。
 */
template <typename T, typename U>
bool operator ==(const trace_allocator<T>& lhs, const trace_allocator<U>& rhs) noexcept {
    return true;
}

/**
 * @brief 不等比较运算符
 * @tparam T 左操作数类型
 * @tparam U 右操作数类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 始终返回false
 */
template <typename T, typename U>
bool operator !=(const trace_allocator<T>& lhs, const trace_allocator<U>& rhs) noexcept {
    return false;
}

/** @} */ // Stacktrace

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_TRACE_MEMORY_HPP__
