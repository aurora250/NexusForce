#ifndef NEFORCE_CORE_SIMD_SCALAR_LANE_HPP__
#define NEFORCE_CORE_SIMD_SCALAR_LANE_HPP__

/**
 * @file lane.hpp
 * @brief 标量后端的向量字节访问
 *
 * 标量后端需要逐 lane 读写任意后端使用的向量类型，
 * 不依赖具体类型是否带 data 成员，统一按字节访问对象表示。
 */

#include "NeForce/core/simd/types.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SIMD__
NEFORCE_BEGIN_SCALAR__

/**
 * @addtogroup SIMD
 * @{
 */

/**
 * @brief 取得向量的字节视图
 * @tparam V 向量类型
 * @param v 向量对象
 * @return 指向该向量首字节的可写指针
 */
template <typename V>
NEFORCE_ALWAYS_INLINE_INLINE byte_t* lane_bytes(V& v) noexcept {
    return reinterpret_cast<byte_t*>(&v);
}

/**
 * @brief 取得向量的只读字节视图
 * @tparam V 向量类型
 * @param v 向量对象
 * @return 指向该向量首字节的只读指针
 */
template <typename V>
NEFORCE_ALWAYS_INLINE_INLINE const byte_t* lane_bytes(const V& v) noexcept {
    return reinterpret_cast<const byte_t*>(&v);
}

/**
 * @brief 按指定 lane 类型取得向量的可写视图
 * @tparam Lane 通道类型
 * @tparam V 向量类型
 * @param v 向量对象
 * @return 指向首个通道的指针
 */
template <typename Lane, typename V>
NEFORCE_ALWAYS_INLINE_INLINE Lane* lane_view(V& v) noexcept {
    return reinterpret_cast<Lane*>(&v);
}

/**
 * @brief 按指定 lane 类型取得向量的只读视图
 * @tparam Lane 通道类型
 * @tparam V 向量类型
 * @param v 向量对象
 * @return 指向首个通道的只读指针
 */
template <typename Lane, typename V>
NEFORCE_ALWAYS_INLINE_INLINE const Lane* lane_view(const V& v) noexcept {
    return reinterpret_cast<const Lane*>(&v);
}

/** @} */ // SIMD

NEFORCE_END_SCALAR__
NEFORCE_END_SIMD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SIMD_SCALAR_LANE_HPP__
