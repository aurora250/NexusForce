#ifndef MSTL_CORE_CONFIG_ASSERTION_HPP__
#define MSTL_CORE_CONFIG_ASSERTION_HPP__
#include "../config/c++config.hpp"
#include <assert.h>

#ifdef MSTL_STATE_DEBUG__
#define MSTL_DEBUG_VERIFY(CON, MESG) \
    { if (CON) {} else { assert(false && MESG); } }
#else
#define MSTL_DEBUG_VERIFY(CON, MESG)
#endif

#define __MSTL_DEBUG_MESG_OPERATE_NULLPTR(ITER, ACT) "can`t " ACT ": " #ITER " is pointing to nullptr."
#define __MSTL_DEBUG_MESG_OUT_OF_RANGE(CLASS, ACT) "can`t " ACT ": " #CLASS " out of ranges."
#define __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(ITER) "not comparable :" #ITER " container incompatible."

#define __MSTL_DEBUG_TAG_DEREFERENCE "dereference"
#define __MSTL_DEBUG_TAG_INCREMENT "increment"
#define __MSTL_DEBUG_TAG_DECREMENT "decrement"


#if defined(MSTL_STANDARD_20__) && defined(MSTL_COMPILER_GNUC__)
#define MSTL_CONSTEXPR_ASSERT(COND) \
do { \
    if (__builtin_is_constant_evaluated() && !bool(COND)) \
        __builtin_unreachable(); \
} while (false);
#elif defined(MSTL_STATE_DEBUG__)
#define MSTL_CONSTEXPR_ASSERT(COND) \
do { \
    if (!bool(COND)) \
        assert(false); \
} while (false);
#else
#define MSTL_CONSTEXPR_ASSERT(COND)
#endif

#endif // MSTL_CORE_CONFIG_ASSERTION_HPP__
