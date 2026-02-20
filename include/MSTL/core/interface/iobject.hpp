#ifndef MSTL_CORE_INTERFACE_IOBJECT_HPP__
#define MSTL_CORE_INTERFACE_IOBJECT_HPP__

/**
 * @file iobject.hpp
 * @brief MSTL可解析对象接口
 *
 * 此文件提供了可解析对象接口的定义。
 */

#include "MSTL/core/interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @class iobject
 * @brief 可解析对象接口
 * @tparam T 派生类类型
 *
 * iobject继承自istringify，在字符串化的基础上增加了从字符串解析的能力。
 * 派生类需要实现：
 * - to_string() 方法
 * - 静态 parse 方法
 */
template <typename T>
struct iobject : istringify<T> {
public:
    /**
     * @brief 从字符串解析对象
     * @param str 包含对象表示的字符串视图
     * @return 解析得到的对象
     *
     * 静态方法，调用派生类的parse实现。
     */
    MSTL_NODISCARD static constexpr T parse(const string_view str) {
        return T::parse(str);
    }

    /**
     * @brief 尝试从字符串解析对象
     * @param str 包含对象表示的字符串视图
     * @return 解析成功返回true，失败返回false
     *
     * 尝试解析字符串，如果解析失败则返回false，对象状态不变。
     * 如果解析成功，则更新当前对象。
     */
    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        T tmp;
        try {
            tmp = T::parse(str);
        } catch (...) {
            return false;
        }
        *this = _MSTL move(tmp);
        return true;
    }
};

/** @} */ // CRTPInterfaces

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_IOBJECT_HPP__
