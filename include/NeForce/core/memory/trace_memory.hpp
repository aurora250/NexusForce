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

#include "NeForce/core/container/flat_unordered_map.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
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
template <typename T, typename Alloc = allocator<T>>
class trace_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename allocator_traits<Alloc>::size_type;
    using difference_type = ptrdiff_t;

    using propagate_on_container_copy_assignment = true_type;
    using propagate_on_container_move_assignment = true_type;
    using propagate_on_container_swap = true_type;
    using is_always_equal = false_type;

    /**
     * @struct rebind
     * @brief 分配器绑定模板
     * @tparam U 重新绑定的类型
     *
     * 提供将当前分配器重新绑定到其他类型的能力。
     */
    template <typename U>
    struct rebind {
        using other = trace_allocator<U, typename allocator_traits<Alloc>::template rebind_alloc<U>>;
    };

    using traces_map = flat_unordered_map<void*, stacktrace>;
    using traces_map_shared = shared_ptr<traces_map>;

private:
    compressed_pair<Alloc, traces_map_shared> traces_; ///< 内存分配追踪表

public:
    /**
     * @brief 默认构造函数
     */
    trace_allocator() :
    traces_(default_construct_tag{}, make_shared<traces_map>()) {}

    template <typename U, typename OtherAlloc>
    trace_allocator(const trace_allocator<U, OtherAlloc>& other) noexcept :
    traces_(exact_arg_construct_tag{}, other.get_base(), other.traces_shared()) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源分配器
     */
    trace_allocator(const trace_allocator& other) noexcept :
    traces_(other.traces_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源分配器
     * @return 自身引用
     */
    trace_allocator& operator=(const trace_allocator& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        traces_ = other.traces_;
        return *this;
    }

    /**
     * @brief 析构函数
     *
     * 检查是否存在未释放的内存，如果有则输出内存泄漏报告。
     */
    ~trace_allocator() {
        try {
            if (traces_.value && traces_.value.use_count() == 1 && !traces_.value->empty()) {
                eprintln("Memory leaks detected! \n");
                print_stacktrace();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }

    NEFORCE_NODISCARD const traces_map& traces() const noexcept { return *traces_.value; }

    NEFORCE_NODISCARD traces_map& traces() noexcept { return *traces_.value; }

    NEFORCE_NODISCARD const traces_map_shared& traces_shared() const noexcept { return traces_.value; }

    NEFORCE_NODISCARD traces_map_shared& traces_shared() noexcept { return traces_.value; }

    NEFORCE_NODISCARD const Alloc& get_base() const noexcept { return traces_.get_base(); }

    NEFORCE_NODISCARD Alloc& get_base() noexcept { return traces_.get_base(); }

    /**
     * @brief 打印所有未释放内存的调用栈
     *
     * 遍历追踪表，输出每个泄漏指针的地址和分配时的调用栈。
     */
    void print_stacktrace() const {
        for (auto& entry: traces()) {
            if (entry.first == nullptr) {
                continue;
            }
            eprintln("Leaked pointer: ", entry.first);
            eprintln("Allocation stack trace:\n", entry.second);
        }
    }

    /**
     * @brief 分配内存
     * @param n 要分配的元素数量
     * @return 指向分配内存的指针
     *
     * 分配n个T类型元素的内存，并记录当前调用栈。
     */
    NEFORCE_ALLOC_NODISCARD NEFORCE_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        pointer ptr = traces_.get_base().allocate(n);
        if (ptr != nullptr) {
            try {
                traces()[ptr] = stacktrace::current(1);
            } catch (...) {
                traces_.get_base().deallocate(ptr, n);
                throw;
            }
        }
        return ptr;
    }

    /**
     * @brief 分配单个元素的内存
     * @return 指向分配内存的指针
     *
     * 分配一个T类型元素的内存，并记录当前调用栈。
     */
    NEFORCE_ALLOC_NODISCARD NEFORCE_ALLOC_OPTIMIZE pointer allocate() { return this->allocate(1); }

    /**
     * @brief 释放内存
     * @param p 要释放的内存指针
     * @param n 元素数量
     *
     * 释放由allocate分配的内存，并从追踪表中移除记录。
     */
    void deallocate(pointer p, const size_type n) noexcept {
        if (p != nullptr) {
            try {
                auto it = traces().find(p);
                if (it != traces().end()) {
                    traces().erase(it);
                }
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (const exception& e) {
                NEFORCE_REPORT_EXCEPTION(e);
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (...) {
                // ignore
            }
        }
        traces_.get_base().deallocate(p, n);
    }

    /**
     * @brief 释放单个元素的内存
     * @param p 要释放的内存指针
     *
     * 释放由allocate()分配的单个元素内存。
     */
    void deallocate(pointer p) noexcept { this->deallocate(p, 1); }

    /**
     * @brief 获取分配器可分配的最大元素数量
     * @return 最大可分配元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept { return allocator_traits<Alloc>::max_size(get_base()); }

    NEFORCE_NODISCARD trace_allocator select_on_container_copy_construction() const noexcept { return *this; }
};

template <typename T, typename U, typename A1, typename A2>
bool operator==(const trace_allocator<T, A1>& lhs, const trace_allocator<U, A2>& rhs) noexcept {
    return lhs.traces_shared() == rhs.traces_shared() && lhs.get_base() == rhs.get_base();
}

template <typename T, typename U, typename A1, typename A2>
bool operator!=(const trace_allocator<T, A1>& lhs, const trace_allocator<U, A2>& rhs) noexcept {
    return !(lhs == rhs);
}

/** @} */ // Stacktrace

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_TRACE_MEMORY_HPP__
