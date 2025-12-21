#ifndef MSTL_CORE_MEMORY_TEMPORARY_BUFFER_HPP__
#define MSTL_CORE_MEMORY_TEMPORARY_BUFFER_HPP__
#include "../memory/uninitialized.hpp"
#include "../numeric/numeric_limits.hpp"
#include <cstdlib> // std::malloc
#include "../config/undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename T = iter_value_t<Iterator>>
struct temporary_buffer {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "temporary buffer requires forward iterator types.");

public:
    using value_type        = T;
    using pointer           = value_type*;
    using const_pointer     = const value_type*;
    using reference         = value_type&;
    using const_reference   = const value_type&;
    using size_type         = ptrdiff_t;
    using difference_type   = ptrdiff_t;

private:
    size_type original_len_ = 0;
    size_type len_ = 0;
    pointer buffer_ = nullptr;

private:
    MSTL_CONSTEXPR20 void allocate_buffer() {
        original_len_ = len_;
        buffer_ = 0;
        if (len_ > static_cast<size_type>(numeric_limits<uint32_t>::max() / sizeof(value_type)))
            len_ = numeric_limits<uint32_t>::max() / sizeof(value_type);

        while (len_ > 0) {
            buffer_ = static_cast<pointer>(std::malloc(len_ * sizeof(value_type)));
            if (buffer_) break;
            len_ /= 2;
        }
    }

    template <typename U = value_type, enable_if_t<is_trivially_copy_assignable_v<U>, int> = 0>
    MSTL_CONSTEXPR20 void initialize_buffer(const U&) noexcept {}
    template <typename U = value_type, enable_if_t<!is_trivially_copy_assignable_v<U>, int> = 0>
    MSTL_CONSTEXPR20 void initialize_buffer(const U& val) {
        _MSTL uninitialized_fill_n(buffer_, len_, val);
    }

public:
    temporary_buffer(const temporary_buffer&) = delete;
    void operator =(const temporary_buffer&) = delete;

    MSTL_CONSTEXPR20 temporary_buffer(Iterator first, Iterator last) {
        try {
            len_ = _MSTL distance(first, last);
            this->allocate_buffer();
            if (len_ > 0) this->initialize_buffer(*first);
        }
        catch (...) {
            std::free(buffer_);
            buffer_ = 0;
            len_ = 0;
            throw;
        }
    }

    MSTL_CONSTEXPR20 ~temporary_buffer() {
        _MSTL destroy(buffer_, buffer_ + len_);
        std::free(buffer_);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type size() const noexcept { return len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type requested_size() const noexcept { return original_len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer begin() noexcept { return buffer_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer end() noexcept { return buffer_ + len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer cbegin() const noexcept { return buffer_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer cend() const noexcept { return buffer_ + len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool empty() const noexcept { return len_ == 0; }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_TEMPORARY_BUFFER_HPP__
