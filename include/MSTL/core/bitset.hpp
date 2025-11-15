#ifndef MSTL_BITSET_HPP__
#define MSTL_BITSET_HPP__
#include "bit.hpp"
#include "array.hpp"
MSTL_BEGIN_NAMESPACE__

template <size_t N>
class bitset : public icollector<bitset<N>> {
private:
    using block_type = size_t;
    static constexpr size_t bits_per_block = sizeof(block_type) * 8;
    static constexpr size_t block_count = (N + bits_per_block - 1) / bits_per_block;

    array<block_type, block_count> blocks{};

    static constexpr block_type last_block_mask() noexcept {
        const size_t excess = block_count * bits_per_block - N;
        if (excess == 0) return static_cast<block_type>(~0ULL);
        return (static_cast<block_type>(1ULL) << (bits_per_block - excess)) - 1;
    }

    MSTL_ALWAYS_INLINE constexpr void check_range(const size_t pos) const noexcept {
        MSTL_DEBUG_VERIFY(pos < N, "bitset position out of range");
    }

    MSTL_ALWAYS_INLINE constexpr size_t block_index(const size_t pos) const noexcept {
        return pos / bits_per_block;
    }
    MSTL_ALWAYS_INLINE constexpr size_t bit_index(const size_t pos) const noexcept {
        return pos % bits_per_block;
    }

public:
    constexpr bitset() noexcept {
        blocks.fill(0);
    }

    constexpr explicit bitset(const block_type val) noexcept {
        blocks.fill(0);
        if constexpr (block_count > 0) {
            blocks[0] = val & last_block_mask();
        }
    }

    constexpr explicit bitset(const string_view str,
        const char zero = '0', const char one = '1') {
        blocks.fill(0);
        const size_t str_len = str.length();
        const size_t M = (str_len < N) ? str_len : N;

        for (size_t i = 0; i < M; ++i) {
            const char c = str[i];
            size_t pos = N - 1 - i;
            if (c == one) {
                set(pos);
            } else if (c == zero) {
                reset(pos);
            } else {
                Exception(ValueError("bitset string ctor: invalid character"));
            }
        }
    }

    constexpr explicit bitset(const string& str, const char zero = '0', const char one = '1')
    : bitset(str.view(), zero, one) {}

    constexpr explicit bitset(const char* str, const char zero = '0', const char one = '1')
    : bitset(string_view{str}, zero, one) {}


    constexpr bitset& set() noexcept {
        for (auto& b : blocks) b = ~static_cast<block_type>(0ULL);
        blocks[block_count - 1] &= last_block_mask();
        return *this;
    }

    constexpr bitset& set(const size_t pos, const bool value = true) noexcept {
        check_range(pos);
        const size_t idx = block_index(pos);
        const size_t bit = bit_index(pos);
        if (value)
            blocks[idx] |= (static_cast<block_type>(1) << bit);
        else
            blocks[idx] &= ~(static_cast<block_type>(1) << bit);
        return *this;
    }

    constexpr bitset& reset() noexcept {
        blocks.fill(0);
        return *this;
    }

    constexpr bitset& reset(const size_t pos) noexcept {
        check_range(pos);
        const size_t idx = block_index(pos);
        const size_t bit = bit_index(pos);
        blocks[idx] &= ~(static_cast<block_type>(1) << bit);
        return *this;
    }

    constexpr bitset& flip() noexcept {
        for (auto& b : blocks) {
            b = ~b;
        }
        blocks[block_count - 1] &= last_block_mask();
        return *this;
    }

    constexpr bitset& flip(const size_t pos) noexcept {
        check_range(pos);
        const size_t idx = block_index(pos);
        const size_t bit = bit_index(pos);
        blocks[idx] ^= (static_cast<block_type>(1) << bit);
        return *this;
    }


    class reference {
    private:
        bitset& bs;
        size_t pos;
    public:
        constexpr reference(bitset& b, const size_t p) noexcept
        : bs(b), pos(p) {}

        constexpr reference& operator =(const bool x) noexcept {
            bs.set(pos, x);
            return *this;
        }
        constexpr reference& operator =(const reference& x) noexcept {
            return *this = static_cast<bool>(x);
        }
        constexpr operator bool() const noexcept {
            return bs.test(pos);
        }
        constexpr reference& flip() noexcept {
            bs.flip(pos);
            return *this;
        }
        constexpr void flip() volatile noexcept = delete;
    };

    MSTL_NODISCARD constexpr bool operator [](const size_t pos) const noexcept {
        return test(pos);
    }
    MSTL_NODISCARD constexpr reference operator [](size_t pos) noexcept {
        check_range(pos);
        return reference(*this, pos);
    }

    MSTL_NODISCARD constexpr size_t size() const noexcept { return N; }
    MSTL_NODISCARD constexpr bool empty() const noexcept { return N == 0; }

    MSTL_NODISCARD constexpr bool test(const size_t pos) const noexcept {
        check_range(pos);
        const size_t idx = block_index(pos);
        const size_t bit = bit_index(pos);
        return (blocks[idx] & (static_cast<block_type>(1) << bit)) != 0;
    }

    MSTL_NODISCARD constexpr bitset& operator &=(const bitset& other) noexcept {
        for (size_t i = 0; i < block_count; ++i)
            blocks[i] &= other.blocks[i];
        return *this;
    }

    MSTL_NODISCARD constexpr bitset& operator |=(const bitset& other) noexcept {
        for (size_t i = 0; i < block_count; ++i)
            blocks[i] |= other.blocks[i];
        return *this;
    }

    MSTL_NODISCARD constexpr bitset& operator ^=(const bitset& other) noexcept {
        for (size_t i = 0; i < block_count; ++i)
            blocks[i] ^= other.blocks[i];
        return *this;
    }

    MSTL_NODISCARD constexpr bitset operator ~() const noexcept {
        bitset res = *this;
        res.flip();
        return res;
    }

    MSTL_NODISCARD constexpr bitset& operator <<=(const size_t pos) noexcept {
        if (pos >= N) {
            reset();
            return *this;
        }
        const size_t block_shift = pos / bits_per_block;
        const size_t bit_shift = pos % bits_per_block;

        if (block_shift > 0) {
            for (size_t i = block_count - 1; i >= block_shift; --i) {
                blocks[i] = blocks[i - block_shift];
            }
            for (size_t i = 0; i < block_shift; ++i) {
                blocks[i] = 0;
            }
        }
        if (bit_shift > 0) {
            for (size_t i = block_count - 1; i > 0; --i) {
                blocks[i] = (blocks[i] << bit_shift) | (blocks[i - 1] >> (bits_per_block - bit_shift));
            }
            blocks[0] <<= bit_shift;
        }
        blocks[block_count - 1] &= last_block_mask();

        return *this;
    }

    MSTL_NODISCARD constexpr bitset& operator >>=(const size_t pos) noexcept {
        if (pos >= N) {
            reset();
            return *this;
        }
        const size_t block_shift = pos / bits_per_block;
        const size_t bit_shift = pos % bits_per_block;

        if (block_shift > 0) {
            for (size_t i = 0; i < block_count - block_shift; ++i) {
                blocks[i] = blocks[i + block_shift];
            }
            for (size_t i = block_count - block_shift; i < block_count; ++i) {
                blocks[i] = 0;
            }
        }
        if (bit_shift > 0) {
            for (size_t i = 0; i < block_count - 1; ++i) {
                blocks[i] = (blocks[i] >> bit_shift) | (blocks[i + 1] << (bits_per_block - bit_shift));
            }
            blocks[block_count - 1] >>= bit_shift;
            blocks[block_count - 1] &= last_block_mask();
        }
        return *this;
    }

    MSTL_NODISCARD constexpr size_t count() const noexcept {
        size_t cnt = 0;
        for (size_t i = 0; i < block_count; ++i) {
            cnt += _MSTL popcount(blocks[i]);
        }
        return cnt;
    }

    MSTL_NODISCARD constexpr bool all() const noexcept {
        for (size_t i = 0; i < block_count - 1; ++i) {
            if (blocks[i] != ~static_cast<block_type>(0ULL)) return false;
        }
        return blocks[block_count - 1] == last_block_mask();
    }

    MSTL_NODISCARD constexpr bool any() const noexcept {
        for (auto b : blocks)
            if (b != 0) return true;
        return false;
    }

    MSTL_NODISCARD constexpr bool none() const noexcept {
        return !any();
    }

    MSTL_NODISCARD constexpr unsigned long to_ulong() const noexcept {
        MSTL_IF_CONSTEXPR (N > sizeof(unsigned long) * 8) {
            constexpr size_t ulong_blocks = (sizeof(unsigned long) * 8 + bits_per_block - 1) / bits_per_block;
            for (size_t i = ulong_blocks; i < block_count; ++i) {
                if (blocks[i] != 0)
                    Exception(ValueError("bitset to_ulong overflow"));
            }

            constexpr size_t ulong_bits = sizeof(unsigned long) * 8;
            constexpr size_t remainder_bits = ulong_bits % bits_per_block;
            MSTL_IF_CONSTEXPR (remainder_bits != 0) {
                constexpr size_t last_ulong_block = ulong_bits / bits_per_block;
                block_type mask = (~static_cast<block_type>(0ULL)) << remainder_bits;
                if ((blocks[last_ulong_block] & mask) != 0) {
                    Exception(ValueError("bitset to_ulong overflow"));
                }
            }
        }

        MSTL_IF_CONSTEXPR (sizeof(unsigned long) >= sizeof(block_type)) {
            return static_cast<unsigned long>(blocks[0]);
        } else {
            unsigned long result = 0;
            constexpr size_t ulong_blocks = (sizeof(unsigned long) * 8 + bits_per_block - 1) / bits_per_block;
            constexpr size_t blocks_to_use = ulong_blocks < block_count ? ulong_blocks : block_count;
            for (size_t i = 0; i < blocks_to_use; ++i) {
                result |= static_cast<unsigned long>(blocks[i]) << (i * bits_per_block);
            }
            return result;
        }
    }


    MSTL_NODISCARD constexpr unsigned long long to_ullong() const noexcept {
        MSTL_IF_CONSTEXPR (N > sizeof(unsigned long long) * 8) {
            constexpr size_t ullong_blocks = (sizeof(unsigned long long) * 8 + bits_per_block - 1) / bits_per_block;
            for (size_t i = ullong_blocks; i < block_count; ++i) {
                if (blocks[i] != 0)
                    Exception(ValueError("bitset to_ullong overflow"));
            }

            constexpr size_t ullong_bits = sizeof(unsigned long long) * 8;
            constexpr size_t remainder_bits = ullong_bits % bits_per_block;
            MSTL_IF_CONSTEXPR (remainder_bits != 0) {
                constexpr size_t last_ullong_block = ullong_bits / bits_per_block;
                block_type mask = (~static_cast<block_type>(0ULL)) << remainder_bits;
                if ((blocks[last_ullong_block] & mask) != 0) {
                    Exception(ValueError("bitset to_ullong overflow"));
                }
            }
        }

        MSTL_IF_CONSTEXPR (sizeof(unsigned long long) >= sizeof(block_type)) {
            return static_cast<unsigned long long>(blocks[0]);
        } else {
            unsigned long long result = 0;
            constexpr size_t ullong_blocks = (sizeof(unsigned long long) * 8 + bits_per_block - 1) / bits_per_block;
            constexpr size_t blocks_to_use = ullong_blocks < block_count ? ullong_blocks : block_count;
            for (size_t i = 0; i < blocks_to_use; ++i) {
                result |= static_cast<unsigned long long>(blocks[i]) << (i * bits_per_block);
            }
            return result;
        }
    }

    MSTL_NODISCARD constexpr bool operator ==(const bitset& other) const noexcept {
        return blocks == other.blocks;
    }
    MSTL_NODISCARD constexpr bool operator !=(const bitset& other) const noexcept {
        return blocks != other.blocks;
    }
    MSTL_NODISCARD constexpr bool operator <(const bitset& other) const noexcept {
        return blocks < other.blocks;
    }
    MSTL_NODISCARD constexpr bool operator >(const bitset& other) const noexcept {
        return blocks > other.blocks;
    }
    MSTL_NODISCARD constexpr bool operator <=(const bitset& other) const noexcept {
        return blocks <= other.blocks;
    }
    MSTL_NODISCARD constexpr bool operator >=(const bitset& other) const noexcept {
        return blocks >= other.blocks;
    }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return blocks.to_hash();
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const char zero, const char one) const {
        string result;
        result.reserve(N);
        for (size_t i = N; i > 0; --i) {
            result.push_back(test(i - 1) ? one : zero);
        }
        return result;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return this->to_string('0', '1');
    }

    constexpr void swap(bitset& other) noexcept {
        blocks.swap(other.blocks);
    }
};

template <size_t N>
bitset<N> operator &(const bitset<N>& lhs, const bitset<N>& rhs) {
    bitset<N> res = lhs;
    res &= rhs;
    return res;
}

template <size_t N>
bitset<N> operator |(const bitset<N>& lhs, const bitset<N>& rhs) {
    bitset<N> res = lhs;
    res |= rhs;
    return res;
}

template <size_t N>
bitset<N> operator ^(const bitset<N>& lhs, const bitset<N>& rhs) {
    bitset<N> res = lhs;
    res ^= rhs;
    return res;
}

MSTL_END_NAMESPACE__
#endif // MSTL_BITSET_HPP__
