#ifndef NEFORCE_CORE_MEMORY_TEMPORARY_BUFFER_HPP__
#define NEFORCE_CORE_MEMORY_TEMPORARY_BUFFER_HPP__

/**
 * @file temporary_buffer.hpp
 * @brief 临时缓冲区
 *
 * 此文件提供了临时缓冲区实现，
 * 用于在算法执行期间分配和自动管理临时内存。
 */

#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/memory/uninitialized.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup TemporaryBuffer 临时缓冲区
 * @brief 临时缓冲区的实现
 * @{
 */

/**
 * @struct temporary_buffer
 * @brief 临时缓冲区类
 * @tparam Iterator 迭代器类型
 *
 * 管理临时内存缓冲区的RAII包装器，用于算法中需要临时存储的场景。
 * 自动管理内存分配和释放，确保异常安全。
 */
template <typename Iterator>
struct temporary_buffer {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "temporary buffer requires forward iterator types.");

public:
    using value_type        = iter_value_t<Iterator>;   ///< 元素类型
    using pointer           = value_type*;              ///< 指针类型
    using const_pointer     = const value_type*;        ///< 常量指针类型
    using reference         = value_type&;              ///< 引用类型
    using const_reference   = const value_type&;        ///< 常量引用类型
    using size_type         = size_t;                ///< 大小类型
    using difference_type   = ptrdiff_t;                ///< 差异类型
    using allocator_type    = standard_allocator<value_type>;    ///< 分配器类型

private:
    size_type original_len_ = 0;  ///< 请求的缓冲区大小
    size_type len_ = 0;           ///< 实际分配的缓冲区大小
    pointer buffer_ = nullptr;    ///< 缓冲区指针

private:
    /**
     * @brief 分配缓冲区内存
     * @throws allocate_exception 如果内存分配失败
     *
     * 尝试分配请求大小的内存，如果失败则尝试分配一半大小，直到成功或大小为0。
     * 调整内存大小以避开数值溢出问题。
     */
    NEFORCE_CONSTEXPR20 void allocate_buffer() {
        original_len_ = len_;
        buffer_ = 0;
        constexpr size_t max = numeric_traits<uint32_t>::max() / sizeof(value_type);
        if (len_ > max) {
            len_ = max;
        }

        while (len_ > 0) {
            buffer_ = allocator_type::allocate(len_);
            if (buffer_) break;
            len_ /= 2;
        }
    }

    /**
     * @brief 初始化缓冲区（平凡可复制类型特化）
     * @tparam U 值类型
     * @param val 初始化值
     *
     * 对于平凡可复制类型，不需要初始化缓冲区。
     */
    template <typename U = value_type, enable_if_t<is_trivially_copy_assignable_v<U>, int> = 0>
    NEFORCE_ALWAYS_INLINE NEFORCE_CONSTEXPR20 void initialize_buffer(const U& val) noexcept {}

    /**
     * @brief 初始化缓冲区（非平凡可复制类型）
     * @tparam U 值类型
     * @param val 初始化值
     * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
     *
     * 对于非平凡可复制类型，使用未初始化填充算法初始化缓冲区。
     */
    template <typename U = value_type, enable_if_t<!is_trivially_copy_assignable_v<U>, int> = 0>
    NEFORCE_ALWAYS_INLINE NEFORCE_CONSTEXPR20 void initialize_buffer(const U& val) {
        _NEFORCE uninitialized_fill_n(buffer_, len_, val);
    }

public:
    temporary_buffer(const temporary_buffer&) = delete;  ///< 禁止复制构造
    void operator =(const temporary_buffer&) = delete;   ///< 禁止复制赋值

    /**
     * @brief 构造函数
     * @param first 范围起始迭代器
     * @param last 范围结束迭代器
     *
     * 根据迭代器范围计算缓冲区大小，分配内存并初始化。
     * 如果分配失败或初始化失败，抛出memory_exception异常，但保证不会泄漏内存。
     *
     * @throws allocate_exception 如果内存分配失败
     * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
     */
    NEFORCE_CONSTEXPR20 temporary_buffer(Iterator first, Iterator last) {
        try {
            len_ = _NEFORCE distance(first, last);
            this->allocate_buffer();
            if (len_ > 0) this->initialize_buffer(*first);
        } catch (...) {
            allocator_type::deallocate(buffer_);
            buffer_ = 0;
            len_ = 0;
            throw;
        }
    }

    /**
     * @brief 析构函数
     *
     * 销毁缓冲区中的对象并释放内存。
     */
    NEFORCE_CONSTEXPR20 ~temporary_buffer() {
        _NEFORCE destroy(buffer_, buffer_ + len_);
        allocator_type::deallocate(buffer_);
    }

    /**
     * @brief 获取缓冲区实际大小
     * @return 实际分配的缓冲区大小
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type size() const noexcept {
        return len_;
    }

    /**
     * @brief 获取请求的缓冲区大小
     * @return 构造函数请求的缓冲区大小
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type requested_size() const noexcept {
        return original_len_;
    }

    /**
     * @brief 获取缓冲区起始迭代器
     * @return 指向缓冲区首元素的指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 pointer begin() noexcept {
        return buffer_;
    }

    /**
     * @brief 获取缓冲区结束迭代器
     * @return 指向缓冲区末尾的指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 pointer end() noexcept {
        return buffer_ + len_;
    }

    /**
     * @brief 获取常量缓冲区起始迭代器
     * @return 指向缓冲区首元素的常量指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_pointer cbegin() const noexcept {
        return buffer_;
    }

    /**
     * @brief 获取常量缓冲区结束迭代器
     * @return 指向缓冲区末尾的常量指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_pointer cend() const noexcept {
        return buffer_ + len_;
    }

    /**
     * @brief 检查缓冲区是否为空
     * @return 如果缓冲区大小为0则返回true，否则返回false
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool empty() const noexcept {
        return len_ == 0;
    }
};

/** @} */ // TemporaryBuffer

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_TEMPORARY_BUFFER_HPP__
