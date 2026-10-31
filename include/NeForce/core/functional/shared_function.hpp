#ifndef NEFORCE_CORE_FUNCTIONAL_SHARED_FUNCTION_HPP__
#define NEFORCE_CORE_FUNCTIONAL_SHARED_FUNCTION_HPP__

/**
 * @file shared_function.hpp
 * @brief 可共享的函数包装器别名
 */

#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup FunctionWrapper 函数包装器
 * @{
 */

/**
 * @brief 可共享的函数包装器
 * @tparam Sign 函数签名类型
 */
template <typename Sign>
using shared_function = function<Sign>;

/** @} */ // FunctionWrapper

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FUNCTIONAL_SHARED_FUNCTION_HPP__
