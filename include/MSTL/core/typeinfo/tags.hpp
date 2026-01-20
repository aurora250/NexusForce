#ifndef MSTL_CORE_TYPEINFO_TAGS_HPP__
#define MSTL_CORE_TYPEINFO_TAGS_HPP__

/**
 * @file tags.hpp
 * @brief MSTL核心标签类型
 *
 * 此文件定义了MSTL库中使用的各种标签类型，用于标签分发和类型选择。
 *
 * 标签类型是空结构体，仅通过类型本身携带信息，不包含数据成员。
 */

#include "../config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

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
 * @extends input_iterator_tag
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
 * @extends forward_iterator_tag
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
 * @extends bidirectional_iterator_tag
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
 * @extends random_access_iterator_tag
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
    constexpr explicit exact_arg_construct_tag() noexcept  = default;
};

/**
 * @struct inplace_construct_tag
 * @brief 原位构造标签
 *
 * 表示在原位置构造对象，避免不必要的拷贝或移动。
 * 通常用于容器中的元素构造。
 */
struct inplace_construct_tag {
    constexpr explicit inplace_construct_tag() noexcept  = default;
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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TYPEINFO_TAGS_HPP__
