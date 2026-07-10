#ifndef NEFORCE_CORE_ASYNC_BUFFER_HPP__
#define NEFORCE_CORE_ASYNC_BUFFER_HPP__

/**
 * @file buffer.hpp
 * @brief 异步 I/O 缓冲区抽象
 *
 * 提供 scatter-gather 缓冲区序列和自适应扩容的动态缓冲区。
 * 与 io_context 驱动的异步 I/O 操作配合使用。
 */

#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncBuffers 缓冲区
 * @brief 异步 I/O 缓冲区抽象
 * @{
 */

/**
 * @brief 可变缓冲区（用于异步读取操作）
 *
 * 对可写内存区域的视图封装，传入 async_read 作为接收目标。
 */
using mutable_buffer = memory_view<char>;

/**
 * @brief 常量缓冲区（用于异步写入操作）
 *
 * 对只读内存区域的视图封装，传入 async_write 作为发送源。
 */
using const_buffer = memory_view<const char>;

/**
 * @class mutable_buffers
 * @brief 可变缓冲区序列（scatter-gather 读）
 *
 * 包含多个内存区域的视图，用于一次 scatter-gather 读操作。
 * 异步读取时数据依次填入每个缓冲区。
 *
 * @note 缓冲区序列不拥有底层内存，调用者需确保内存在操作期间有效。
 */
class mutable_buffers {
public:
    using value_type = mutable_buffer;                             ///< 元素类型
    using iterator = vector<mutable_buffer>::iterator;             ///< 迭代器类型
    using const_iterator = vector<mutable_buffer>::const_iterator; ///< 常量迭代器类型

private:
    vector<mutable_buffer> buffers_; ///< 缓冲区列表

public:
    /**
     * @brief 默认构造函数
     */
    mutable_buffers() = default;

    /**
     * @brief 从单个 buffer 构造
     * @param b 初始缓冲区
     */
    explicit mutable_buffers(mutable_buffer b) { buffers_.push_back(b); }

    /**
     * @brief 添加缓冲区到序列末尾
     * @param b 要添加的缓冲区
     */
    void push_back(mutable_buffer b) { buffers_.push_back(b); }

    /**
     * @brief 获取缓冲区的数量
     * @return 缓冲区数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return buffers_.size(); }

    /**
     * @brief 检查序列是否为空
     * @return 为空返回 true
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return buffers_.empty(); }

    /**
     * @brief 访问指定索引的缓冲区
     * @param i 索引
     * @return 缓冲区引用
     */
    NEFORCE_NODISCARD const mutable_buffer& operator[](size_t i) const { return buffers_[i]; }

    /**
     * @brief 访问指定索引的缓冲区（可修改）
     * @param i 索引
     * @return 缓冲区引用
     */
    NEFORCE_NODISCARD mutable_buffer& operator[](size_t i) { return buffers_[i]; }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个缓冲区的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept { return buffers_.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return buffers_.end(); }

    /**
     * @brief 获取起始常量迭代器
     * @return 指向第一个缓冲区的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept { return buffers_.begin(); }

    /**
     * @brief 获取结束常量迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept { return buffers_.end(); }
};

/**
 * @class const_buffers
 * @brief 常量缓冲区序列（scatter-gather 写）
 *
 * 包含多个只读内存区域的视图，用于一次 scatter-gather 写操作。
 * 异步写入时数据依次从每个缓冲区发送。
 *
 * @note 缓冲区序列不拥有底层内存，调用者需确保内存在操作期间有效。
 */
class const_buffers {
public:
    using value_type = const_buffer;                             ///< 元素类型
    using iterator = vector<const_buffer>::iterator;             ///< 迭代器类型
    using const_iterator = vector<const_buffer>::const_iterator; ///< 常量迭代器类型

private:
    vector<const_buffer> buffers_; ///< 缓冲区列表

public:
    /**
     * @brief 默认构造函数
     */
    const_buffers() = default;

    /**
     * @brief 从单个 buffer 构造
     * @param b 初始缓冲区
     */
    explicit const_buffers(const_buffer b) { buffers_.push_back(b); }

    /**
     * @brief 添加缓冲区到序列末尾
     * @param b 要添加的缓冲区
     */
    void push_back(const_buffer b) { buffers_.push_back(b); }

    /**
     * @brief 获取缓冲区的数量
     * @return 缓冲区数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return buffers_.size(); }

    /**
     * @brief 检查序列是否为空
     * @return 为空返回 true
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return buffers_.empty(); }

    /**
     * @brief 访问指定索引的缓冲区
     * @param i 索引
     * @return 缓冲区常量引用
     */
    NEFORCE_NODISCARD const const_buffer& operator[](size_t i) const { return buffers_[i]; }

    /**
     * @brief 访问指定索引的缓冲区（可修改）
     * @param i 索引
     * @return 缓冲区引用
     */
    NEFORCE_NODISCARD const_buffer& operator[](size_t i) { return buffers_[i]; }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个缓冲区的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept { return buffers_.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return buffers_.end(); }

    /**
     * @brief 获取起始常量迭代器
     * @return 指向第一个缓冲区的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept { return buffers_.begin(); }

    /**
     * @brief 获取结束常量迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept { return buffers_.end(); }
};

/**
 * @class dynamic_buffer
 * @brief 自适应扩容的动态缓冲区
 *
 * 适用于异步读取场景：prepare() 获取可写区域，commit() 标记已填充，
 * consume() 移除已处理数据。内部自动扩容。
 *
 * 使用示例：
 * @code
 * dynamic_buffer buf;
 * sock.async_read(ctx, buf.prepare(1024), [&](error_code ec, size_t n) {
 *     buf.commit(n);
 *     process(buf.data(), buf.size());
 *     buf.consume(processed);
 * });
 * @endcode
 *
 * @note prepare/commit/consume 操作不保证线程安全，调用者负责同步。
 */
class dynamic_buffer {
private:
    vector<char> data_;   ///< 内部存储
    size_t write_pos_{0}; ///< 当前写入位置

public:
    /**
     * @brief 准备写入区域
     * @param n 至少需要的字节数
     * @return 可写的缓冲区序列
     *
     * 保证返回的缓冲区至少 n 字节可用。若内部空间不足则自动扩容。
     * 返回的 mutable_buffers 指向内部存储，在下次 prepare 或对象析构前有效。
     *
     * @note 多次 prepare 不重叠——每次 prepare 返回的缓冲区基于当前 write_pos_。
     */
    mutable_buffers prepare(size_t n) {
        if (data_.size() - write_pos_ < n) {
            data_.resize(write_pos_ + n);
        }
        return mutable_buffers(mutable_buffer(data_.data() + write_pos_, data_.size() - write_pos_));
    }

    /**
     * @brief 标记已写入的字节数
     * @param n 已写入的字节数
     *
     * 将 write_pos_ 前移 n 字节，使已写入数据可通过 data()/size() 访问。
     * 必须在 prepare() 返回的缓冲区被填充后调用。
     *
     * @note commit 在 prepare 之后调用，通常由 I/O 完成回调触发。
     */
    void commit(size_t n) { write_pos_ += n; }

    /**
     * @brief 移除已消费的数据
     * @param n 已消费的字节数
     *
     * 将剩余未消费的数据移到缓冲区开头，释放已消费的空间供后续写入。
     * 通常在处理完部分数据后调用。
     */
    void consume(size_t n) {
        if (n >= write_pos_) {
            write_pos_ = 0;
            return;
        }
        const size_t remaining = write_pos_ - n;
        memmove(data_.data(), data_.data() + n, remaining);
        write_pos_ = remaining;
    }

    /**
     * @brief 当前可读数据大小
     * @return 已写入但未消费的字节数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return write_pos_; }

    /**
     * @brief 获取可读数据的原始指针
     * @return 可读数据起始指针
     */
    NEFORCE_NODISCARD const char* data() const noexcept { return data_.data(); }

    /**
     * @brief 获取可写数据的原始指针
     * @return 可写数据起始指针
     */
    NEFORCE_NODISCARD char* data() noexcept { return data_.data(); }

    /**
     * @brief 内部缓冲区容量
     * @return 已分配的字节数
     */
    NEFORCE_NODISCARD size_t capacity() const noexcept { return data_.size(); }

    /**
     * @brief 最大允许大小
     * @return 最大容量
     */
    NEFORCE_NODISCARD size_t max_size() const noexcept { return numeric_traits<size_t>::max(); }
};

/** @} */ // AsyncBuffers

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_BUFFER_HPP__
