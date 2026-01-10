#ifndef MSTL_CORE_UTILITY_TAGS_HPP__
#define MSTL_CORE_UTILITY_TAGS_HPP__
#include "../config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

struct input_iterator_tag {
    constexpr input_iterator_tag() = default;
};
struct output_iterator_tag {
    constexpr output_iterator_tag() = default;
};
struct forward_iterator_tag : input_iterator_tag {
    constexpr forward_iterator_tag() = default;
};
struct bidirectional_iterator_tag : forward_iterator_tag {
    constexpr bidirectional_iterator_tag() = default;
};
struct random_access_iterator_tag : bidirectional_iterator_tag {
    constexpr random_access_iterator_tag() = default;
};
#ifdef MSTL_STANDARD_20__
struct contiguous_iterator_tag : random_access_iterator_tag {
    constexpr contiguous_iterator_tag() = default;
};
#endif


// use allocator as the argument of functions.
struct allocator_arg_tag {
    constexpr allocator_arg_tag() noexcept = default;
};

// construct without arguments.
struct default_construct_tag {
    constexpr default_construct_tag() noexcept = default;
};
// construct by arguments.
struct exact_arg_construct_tag {
    constexpr exact_arg_construct_tag() noexcept  = default;
};
// construct by arguments inplace.
struct inplace_construct_tag {
    constexpr inplace_construct_tag() noexcept  = default;
};
// construct by unpacking tuple or pair type.
struct unpack_utility_construct_tag {
    constexpr unpack_utility_construct_tag() noexcept = default;
};


struct inplace_invoke_tag {
    constexpr inplace_invoke_tag() noexcept  = default;
};
struct unexpect_invoke_tag {
    constexpr unexpect_invoke_tag() noexcept  = default;
};


struct allocate_cpu_tag {
    constexpr allocate_cpu_tag() noexcept = default;
};
struct allocate_gpu_tag {
    constexpr allocate_gpu_tag() noexcept = default;
};


struct defer_lock_tag {
    constexpr defer_lock_tag() noexcept = default;
};
MSTL_INLINE17 constexpr defer_lock_tag defer_lock{};

struct try_lock_tag {
    constexpr try_lock_tag() noexcept = default;
};
MSTL_INLINE17 constexpr try_lock_tag try_lock{};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_TAGS_HPP__
