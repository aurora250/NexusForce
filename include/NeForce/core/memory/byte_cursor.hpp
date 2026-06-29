#ifndef NEFORCE_NETWORK_HTTP_BYTE_CURSOR_HPP__
#define NEFORCE_NETWORK_HTTP_BYTE_CURSOR_HPP__

/**
 * @file byte_cursor.hpp
 * @brief 带边界检查的字节游标，用于协议解析
 *
 * 封装缓冲区及读取位置，提供安全的读取操作。
 */

#include "NeForce/core/memory/endian.hpp"
#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @defgroup ByteCursor 字节游标
 * @brief 消除手动进行字节级协议解析时常见的越界/溢出错误，被用于帧解析器等模块。
 * @{
 */

/**
 * @class byte_cursor
 * @brief 带边界检查的字节游标，用于协议解析
 *
 * 封装缓冲区及读取位置，提供安全的逐字节/多位读取操作，
 */
class byte_cursor {
private:
    /// @brief 底层字节缓冲区指针
    const byte_t* data_ = nullptr;
    /// @brief 缓冲区总大小
    size_t size_ = 0;
    /// @brief 当前读取位置
    size_t pos_ = 0;
    /// @brief 位读取缓冲区
    uint64_t bit_buf_ = 0;
    /// @brief 位缓冲区中缓存的位数
    size_t bits_in_buf_ = 0;

public:
    /// @brief 构造空游标
    byte_cursor() noexcept = default;

    /// @brief 从原始指针和长度构造
    byte_cursor(const byte_t* data, size_t size) noexcept :
    data_(data),
    size_(size) {}

    /// @brief 从 cbyte_view 构造
    explicit byte_cursor(cbyte_view view) noexcept :
    data_(view.data()),
    size_(view.size()) {}

    /// @brief 剩余可读字节数
    NEFORCE_NODISCARD size_t remaining() const noexcept { return size_ - pos_; }
    /// @brief 已消费的字节数
    NEFORCE_NODISCARD size_t consumed_bytes() const noexcept { return pos_; }
    /// @brief 是否已读完所有数据
    NEFORCE_NODISCARD bool exhausted() const noexcept { return pos_ >= size_; }
    /// @brief 获取底层数据指针
    NEFORCE_NODISCARD const byte_t* data() const noexcept { return data_; }
    /// @brief 获取底层缓冲区总大小
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 尝试读取一个字节
     * @return 成功返回该字节，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<byte_t> try_read_byte() noexcept {
        if (pos_ >= size_) {
            return none;
        }
        return data_[pos_++];
    }

    /**
     * @brief 查看当前字节而不消费
     * @return 当前字节，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<byte_t> peek_byte() const noexcept {
        if (pos_ >= size_) {
            return none;
        }
        return data_[pos_];
    }

    /**
     * @brief 尝试读取大端序 16 位无符号整数
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint16_t> try_read_be16() noexcept {
        if (pos_ + 2 > size_) {
            return none;
        }
        const uint16_t val = endian::read_be16(data_ + pos_);
        pos_ += 2;
        return val;
    }

    /**
     * @brief 尝试读取大端序 24 位无符号整数
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint32_t> try_read_be24() noexcept {
        if (pos_ + 3 > size_) {
            return none;
        }
        const uint32_t val = endian::read_be24(data_ + pos_);
        pos_ += 3;
        return val;
    }

    /**
     * @brief 尝试读取大端序 32 位无符号整数
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint32_t> try_read_be32() noexcept {
        if (pos_ + 4 > size_) {
            return none;
        }
        const uint32_t val = endian::read_be32(data_ + pos_);
        pos_ += 4;
        return val;
    }

    /**
     * @brief 尝试读取大端序 64 位无符号整数
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint64_t> try_read_be64() noexcept {
        if (pos_ + 8 > size_) {
            return none;
        }
        const uint64_t val = endian::read_be64(data_ + pos_);
        pos_ += 8;
        return val;
    }

    /**
     * @brief 尝试读取小端序 16 位无符号整数
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint16_t> try_read_le16() noexcept {
        if (pos_ + 2 > size_) {
            return none;
        }
        const uint16_t val = endian::read_le16(data_ + pos_);
        pos_ += 2;
        return val;
    }

    /**
     * @brief 尝试读取小端序 32 位无符号整数
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint32_t> try_read_le32() noexcept {
        if (pos_ + 4 > size_) {
            return none;
        }
        const uint32_t val = endian::read_le32(data_ + pos_);
        pos_ += 4;
        return val;
    }

    /**
     * @brief 尝试读取小端序 64 位无符号整数
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint64_t> try_read_le64() noexcept {
        if (pos_ + 8 > size_) {
            return none;
        }
        const uint64_t val = endian::read_le64(data_ + pos_);
        pos_ += 8;
        return val;
    }

    /**
     * @brief 尝试读取指定长度的字节块
     * @param count 要读取的字节数
     * @return 成功返回字节视图，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<cbyte_view> try_read_bytes(size_t count) noexcept {
        if (pos_ + count > size_) {
            return none;
        }
        cbyte_view view(data_ + pos_, count);
        pos_ += count;
        return view;
    }

    /**
     * @brief 跳过指定数量的字节
     * @param count 要跳过的字节数
     * @return 跳过成功返回 true，数据不足返回 false
     */
    bool skip(size_t count) noexcept {
        if (pos_ + count > size_) {
            return false;
        }
        pos_ += count;
        return true;
    }

    /**
     * @brief 重置游标到新的缓冲区
     * @param data 新的缓冲区指针
     * @param size 新的缓冲区大小
     */
    void reset(const byte_t* data, size_t size) noexcept {
        data_ = data;
        size_ = size;
        pos_ = 0;
        bit_buf_ = 0;
        bits_in_buf_ = 0;
    }

    /**
     * @brief 尝试读取指定位数（用于 HPACK Huffman 解码）
     * @param n 要读取的位数（最多 64）
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint64_t> try_read_bits(uint8_t n) noexcept {
        if (n > 64) {
            return none;
        }
        while (bits_in_buf_ < n) {
            auto b = try_read_byte();
            if (!b) {
                return none;
            }
            bit_buf_ = (bit_buf_ << 8) | *b;
            bits_in_buf_ += 8;
        }
        bits_in_buf_ -= n;
        const uint64_t mask = (n == 64) ? numeric_traits<uint64_t>::max() : ((1ULL << n) - 1);
        return (bit_buf_ >> bits_in_buf_) & mask;
    }

    /**
     * @brief 预读指定位数而不消费
     * @param n 要预读的位数（最多 64）
     * @return 成功返回该值，数据不足返回 none
     */
    NEFORCE_NODISCARD optional<uint64_t> try_peek_bits(uint8_t n) const noexcept {
        if (n > 64 || bits_in_buf_ < n) {
            return none;
        }
        const uint64_t mask = (n == 64) ? numeric_traits<uint64_t>::max() : ((1ULL << n) - 1);
        return (bit_buf_ >> (bits_in_buf_ - n)) & mask;
    }

    /**
     * @brief 跳过指定位数
     * @param n 要跳过的位数
     */
    void skip_bits(uint8_t n) noexcept {
        if (n <= bits_in_buf_) {
            bits_in_buf_ -= n;
        } else {
            bits_in_buf_ = 0;
        }
    }

    /// @brief 位缓冲区中剩余的可读位数
    NEFORCE_NODISCARD size_t bits_remaining() const noexcept { return bits_in_buf_; }

    /**
     * @brief 从字节缓冲区中补充位缓冲区
     * @return 补充成功返回 true，无剩余字节返回 false
     */
    bool refill_bits() noexcept {
        if (exhausted()) {
            return false;
        }
        bit_buf_ = (bit_buf_ << 8) | data_[pos_++];
        bits_in_buf_ += 8;
        return true;
    }
};

/** @} */ // ByteCursor

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_BYTE_CURSOR_HPP__
