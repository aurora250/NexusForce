#ifndef NEFORCE_CORE_TYPEINFO_TYPES_HPP__
#define NEFORCE_CORE_TYPEINFO_TYPES_HPP__

/**
 * @file types.hpp
 * @brief 基本类型别名
 *
 * 基本类型别名、固定大小类型和平台相关的类型定义，提供统一和跨平台的基本类型。
 */

#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CoreTypes 核心类型
 * @brief 基础类型别名和通用类型定义
 * @{
 */

/**
 * @typedef nullptr_t
 * @brief 空指针类型
 */
using nullptr_t = decltype(nullptr);

/**
 * @typedef max_align_t
 * @brief 最大对齐类型
 */
using max_align_t = double;

/**
 * @typedef byte_t
 * @brief 字节类型，定义为无符号字符
 */
using byte_t = unsigned char;


/**
 * @typedef native_id_type
 * @brief 系统ID类型
 */
using native_id_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        unsigned long;
#else
        int;
#endif

/**
 * @typedef native_handle_type
 * @brief 系统句柄类型
 */
using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        void*;
#else
        int;
#endif


/**
 * @typedef int8_t
 * @brief 8位有符号整数类型
 */
using int8_t = signed char;

/**
 * @typedef int16_t
 * @brief 16位有符号整数类型
 */
using int16_t = short;

/**
 * @typedef int32_t
 * @brief 32位有符号整数类型
 */
using int32_t = int;

/**
 * @typedef int64_t
 * @brief 64位有符号整数类型
 *
 * Linux为long，Windows为long long
 */
using int64_t =
#ifdef NEFORCE_PLATFORM_LINUX64
        long;
#else
        long long;
#endif

/**
 * @typedef uint8_t
 * @brief 8位无符号整数类型
 */
using uint8_t = unsigned char;

/**
 * @typedef uint16_t
 * @brief 16位无符号整数类型
 */
using uint16_t = unsigned short;

/**
 * @typedef uint32_t
 * @brief 32位无符号整数类型
 */
using uint32_t = unsigned int;

/**
 * @typedef uint64_t
 * @brief 64位无符号整数类型
 *
 * Linux为unsigned long，Windows为unsigned long long
 */
using uint64_t =
#ifdef NEFORCE_PLATFORM_LINUX64
        unsigned long;
#else
        unsigned long long;
#endif


/**
 * @typedef float32_t
 * @brief 32位单精度浮点数类型
 */
using float32_t = float;

/**
 * @typedef float64_t
 * @brief 64位双精度浮点数类型
 */
using float64_t = double;

/**
 * @typedef decimal_t
 * @brief 扩展精度浮点数类型
 *
 * 使用long double实现，提供比double更高的精度。
 */
using decimal_t = long double;

/** @} */ // CoreTypes

/**
 * @defgroup PlatformDependentTypes 平台相关类型
 * @brief 根据平台位数定义的大小和指针相关类型
 * @{
 */

#if defined(NEFORCE_ARCH_BITS_64) || defined(NEXUSFORCE_ENABLE_DOXYGEN)

/**
 * @typedef size_t
 * @brief 无符号大小类型
 *
 * 64位下为为uint64_t，32位下为uint32_t。文档以64位平台为例。
 */
using size_t = uint64_t;

/**
 * @typedef ssize_t
 * @brief 有符号大小类型
 *
 * 64位下为为int64_t，32位下为int32_t。文档以64位平台为例。
 */
using ssize_t = int64_t;

/**
 * @typedef ptrdiff_t
 * @brief 指针差类型
 *
 * 64位下为为int64_t，32位下为int32_t。文档以64位平台为例。
 */
using ptrdiff_t = int64_t;

/**
 * @typedef intptr_t
 * @brief 可容纳指针的有符号整数类型
 *
 * 64位下为为int64_t，32位下为int32_t。文档以64位平台为例。
 */
using intptr_t = int64_t;

/**
 * @typedef uintptr_t
 * @brief 可容纳指针的无符号整数类型
 *
 * 64位下为为uint64_t，32位下为uint32_t。文档以64位平台为例。
 */
using uintptr_t = uint64_t;

#else

/**
 * @typedef size_t
 * @brief 无符号大小类型
 */
using size_t = uint32_t;

/**
 * @typedef ssize_t
 * @brief 有符号大小类型
 */
using ssize_t = int32_t;

/**
 * @typedef ptrdiff_t
 * @brief 指针差类型
 */
using ptrdiff_t = int32_t;

/**
 * @typedef intptr_t
 * @brief 可容纳指针的有符号整数类型
 */
using intptr_t = int32_t;

/**
 * @typedef uintptr_t
 * @brief 可容纳指针的无符号整数类型
 */
using uintptr_t = uint32_t;

#endif


/**
 * @typedef int_least8_t
 * @brief 至少8位的有符号整数类型
 */
using int_least8_t = int8_t;

/**
 * @typedef int_least16_t
 * @brief 至少16位的有符号整数类型
 */
using int_least16_t = int16_t;

/**
 * @typedef int_least32_t
 * @brief 至少32位的有符号整数类型
 */
using int_least32_t = int32_t;

/**
 * @typedef int_least64_t
 * @brief 至少64位的有符号整数类型
 */
using int_least64_t = int64_t;

/**
 * @typedef uint_least8_t
 * @brief 至少8位的无符号整数类型
 */
using uint_least8_t = uint8_t;

/**
 * @typedef uint_least16_t
 * @brief 至少16位的无符号整数类型
 */
using uint_least16_t = uint16_t;

/**
 * @typedef uint_least32_t
 * @brief 至少32位的无符号整数类型
 */
using uint_least32_t = uint32_t;

/**
 * @typedef uint_least64_t
 * @brief 至少64位的无符号整数类型
 */
using uint_least64_t = uint64_t;

/**
 * @typedef int_fast8_t
 * @brief 快速8位有符号整数类型
 */
using int_fast8_t = int8_t;

/**
 * @typedef int_fast16_t
 * @brief 快速16位有符号整数类型
 */
using int_fast16_t = ssize_t;

/**
 * @typedef int_fast32_t
 * @brief 快速32位有符号整数类型
 */
using int_fast32_t = ssize_t;

/**
 * @typedef int_fast64_t
 * @brief 快速64位有符号整数类型
 */
using int_fast64_t = int64_t;

/**
 * @typedef uint_fast8_t
 * @brief 快速8位无符号整数类型
 */
using uint_fast8_t = uint8_t;

/**
 * @typedef uint_fast16_t
 * @brief 快速16位无符号整数类型
 */
using uint_fast16_t = size_t;

/**
 * @typedef uint_fast32_t
 * @brief 快速32位无符号整数类型
 */
using uint_fast32_t = size_t;

/**
 * @typedef uint_fast64_t
 * @brief 快速64位无符号整数类型
 */
using uint_fast64_t = uint64_t;


/**
 * @typedef intmax_t
 * @brief 最大有符号整数类型
 */
using intmax_t = int64_t;

/**
 * @typedef uintmax_t
 * @brief 最大无符号整数类型
 */
using uintmax_t = uint64_t;

/** @} */ // PlatformDependentTypes

/**
 * @defgroup IteratorTags 迭代器标签
 * @brief 迭代器类别标签，用于区分不同类型的迭代器
 *
 * 这些标签通过继承关系表示迭代器能力的层级关系，在算法中用于选择最优的实现或进行编译时检查。
 * @{
 */

/**
 * @struct input_iterator_tag
 * @brief 输入迭代器标签
 *
 * 表示输入迭代器类别，是最基本的迭代器类型。
 * 支持：
 * - 读取元素值
 * - 单遍遍历
 * - 单向递增
 * - 相等比较
 *
 * @note 输入迭代器只能读取，不能写入。
 */
struct input_iterator_tag {
    constexpr explicit input_iterator_tag() = default;
};

/**
 * @struct output_iterator_tag
 * @brief 输出迭代器标签
 *
 * 表示输出迭代器类别，与输入迭代器相对。
 * 支持：
 * - 写入元素值
 * - 单遍遍历
 * - 单向递增
 *
 * @note 输出迭代器只能写入，不能读取。
 */
struct output_iterator_tag {
    constexpr explicit output_iterator_tag() = default;
};

/**
 * @struct forward_iterator_tag
 * @brief 前向迭代器标签
 *
 * 表示前向迭代器类别，在输入迭代器基础上增加功能。
 * 支持：
 * - 输入迭代器的所有功能
 * - 多遍遍历
 * - 可默认构造
 *
 * @note 继承自input_iterator_tag，表示前向迭代器也是有效的输入迭代器。
 */
struct forward_iterator_tag : input_iterator_tag {
    constexpr explicit forward_iterator_tag() = default;
};

/**
 * @struct bidirectional_iterator_tag
 * @brief 双向迭代器标签
 *
 * 表示双向迭代器类别，在前向迭代器基础上增加反向移动能力。
 * 支持：
 * - 前向迭代器的所有功能
 * - 双向移动
 *
 * @note 继承自forward_iterator_tag，表示双向迭代器也是有效的前向迭代器。
 */
struct bidirectional_iterator_tag : forward_iterator_tag {
    constexpr explicit bidirectional_iterator_tag() = default;
};

/**
 * @struct random_access_iterator_tag
 * @brief 随机访问迭代器标签
 *
 * 表示随机访问迭代器类别，在双向迭代器基础上增加随机访问能力。
 * 支持：
 * - 双向迭代器的所有功能
 * - 随机访问（通过operator[]）
 * - 迭代器算术运算（+、-、+=、-=）
 * - 迭代器比较（<、<=、>、>=）
 *
 * @note 继承自bidirectional_iterator_tag，表示随机访问迭代器也是有效的双向迭代器。
 */
struct random_access_iterator_tag : bidirectional_iterator_tag {
    constexpr explicit random_access_iterator_tag() = default;
};

/**
 * @struct contiguous_iterator_tag
 * @brief 连续迭代器标签
 *
 * 表示连续迭代器类别，在随机访问迭代器基础上保证元素在内存中连续存储。
 * 支持：
 * - 随机访问迭代器的所有功能
 * - 元素在内存中连续存储
 * - 可通过指针算术直接访问底层内存
 *
 * @note 继承自random_access_iterator_tag，表示连续迭代器也是有效的随机访问迭代器。
 */
struct contiguous_iterator_tag : random_access_iterator_tag {
    constexpr explicit contiguous_iterator_tag() = default;
};

/** @} */ // IteratorTags

/**
 * @defgroup ConstructionTags 构造标签
 * @brief 构造过程相关的标签，用于区分不同的构造方式
 *
 * 这些标签用于构造函数的重载解析，实现更灵活的构造语义。
 * @{
 */

/**
 * @struct allocator_arg_tag
 * @brief 分配器参数标签
 *
 * 用于标记使用分配器作为函数参数的情况。
 * 通常在构造函数中与分配器一起使用，遵循"uses-allocator"构造模式。
 */
struct allocator_arg_tag {
    constexpr explicit allocator_arg_tag() noexcept = default;
};

/**
 * @struct default_construct_tag
 * @brief 默认构造标签
 *
 * 表示使用默认构造函数构造对象，不提供任何参数。
 */
struct default_construct_tag {
    constexpr explicit default_construct_tag() noexcept = default;
};

/**
 * @struct exact_arg_construct_tag
 * @brief 精确参数构造标签
 *
 * 表示使用提供的参数精确构造对象，直接传递参数给构造函数。
 */
struct exact_arg_construct_tag {
    constexpr explicit exact_arg_construct_tag() noexcept = default;
};

/**
 * @struct inplace_construct_tag
 * @brief 原位构造标签
 *
 * 表示在原位置构造对象，避免不必要的拷贝或移动。
 * 通常用于容器中的元素构造。
 */
struct inplace_construct_tag {
    constexpr explicit inplace_construct_tag() noexcept = default;
};

/**
 * @struct pass_template_construct_tag
 * @brief 传递模板参数构造标签
 *
 * 表示在原位置传递模板参数构造对象。
 */
template <typename... Args>
struct pass_template_construct_tag {
    constexpr explicit pass_template_construct_tag() noexcept = default;
};

template <size_t Size>
struct pass_size_construct_tag {
    constexpr explicit pass_size_construct_tag() noexcept = default;
};

/**
 * @struct unpack_utility_construct_tag
 * @brief 解包工具构造标签
 *
 * 表示通过解包tuple或pair类型的参数来构造对象。
 */
struct unpack_utility_construct_tag {
    constexpr explicit unpack_utility_construct_tag() noexcept = default;
};

/** @} */ // ConstructionTags

struct ignore_t {
    template <typename T>
    NEFORCE_CONSTEXPR14 const ignore_t& operator=(const T& /*unused*/) const noexcept {
        return *this;
    }
};

NEFORCE_INLINE17 constexpr ignore_t ignore{};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_TYPEINFO_TYPES_HPP__
