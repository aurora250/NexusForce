#ifndef MSTL_CORE_CONFIG_TYPES_HPP__
#define MSTL_CORE_CONFIG_TYPES_HPP__
#include "../config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

using nullptr_t	= decltype(nullptr);
using byte_t    = unsigned char;

using int8_t	= signed char;
using int16_t	= short;
using int32_t	= int;
#ifdef MSTL_PLATFORM_LINUX64__
using int64_t	= long;
#else
using int64_t	= long long;
#endif

using uint8_t	= unsigned char;
using uint16_t	= unsigned short;
using uint32_t	= unsigned int;
#ifdef MSTL_PLATFORM_LINUX64__
using uint64_t	= unsigned long;
#else
using uint64_t	= unsigned long long;
#endif

using float32_t	= float;
using float64_t	= double;
using decimal_t = long double;

#ifdef MSTL_DATA_BUS_WIDTH_64__
#ifdef MSTL_PLATFORM_LINUX__
using size_t	= unsigned long;
using ssize_t	= long;
using ptrdiff_t = long;
using intptr_t	= long;
using uintptr_t = unsigned long;
#else
using size_t	= unsigned long long;
using ssize_t	= long long;
using ptrdiff_t = long long;
using intptr_t	= long long;
using uintptr_t = unsigned long long;
#endif
#else
using size_t	= unsigned int;
using ssize_t	= int;
using ptrdiff_t = int;
using intptr_t	= int;
using uintptr_t = unsigned int;
#endif

using int_least8_t   = int8_t;
using int_least16_t  = int16_t;
using int_least32_t  = int32_t;
using int_least64_t  = int64_t;

using uint_least8_t  = uint8_t;
using uint_least16_t = uint16_t;
using uint_least32_t = uint32_t;
using uint_least64_t = uint64_t;

using int_fast8_t    = int8_t;
using int_fast16_t   = ssize_t;
using int_fast32_t   = ssize_t;
using int_fast64_t   = int64_t;

using uint_fast8_t   = uint8_t;
using uint_fast16_t  = size_t;
using uint_fast32_t  = size_t;
using uint_fast64_t  = uint64_t;

using intmax_t	= int64_t;
using uintmax_t = uint64_t;


MSTL_INLINE17 constexpr size_t POINTER_SIZE = sizeof(void*);
MSTL_INLINE17 constexpr size_t SIZE_T_MAX_SIZE = static_cast<size_t>(-1);
MSTL_INLINE17 constexpr bool SIZE_T_SAME_WITH_ULONG = sizeof(unsigned long) == sizeof(size_t);

MSTL_INLINE17 constexpr size_t MEMORY_ALIGN_THRESHHOLD = 16UL;
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_THRESHHOLD = 4096UL;


// quickly define standard type alias.
#define MSTL_BUILD_TYPE_ALIAS(TYPE) \
using value_type        = TYPE; \
using pointer           = TYPE*; \
using reference         = TYPE&; \
using const_pointer     = const TYPE*; \
using const_reference   = const TYPE&; \
using size_type         = size_t; \
using difference_type   = ptrdiff_t;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONFIG_TYPES_HPP__
