#ifndef NEFORCE_CORE_ITERATOR_INSERT_ITERATOR_HPP__
#define NEFORCE_CORE_ITERATOR_INSERT_ITERATOR_HPP__

/**
 * @file insert_iterator.hpp
 * @brief 插入迭代器
 *
 * 此文件提供了插入迭代器的实现，用于将元素插入到容器中。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup InsertIterators 插入迭代器
 * @brief 用于向容器插入元素的输出迭代器
 * @{
 */

/**
 * @class back_insert_iterator
 * @brief 尾部插入迭代器
 * @tparam Container 容器类型
 *
 * 将元素插入到容器的尾部，通过调用容器的push_back()方法实现。
 */
template <typename Container>
class back_insert_iterator {
public:
    using iterator_category = output_iterator_tag; ///< 迭代器类别
    using value_type = void;                       ///< 值类型
    using difference_type = void;                  ///< 差值类型
    using pointer = void;                          ///< 指针类型
    using reference = void;                        ///< 引用类型

private:
    Container* container;

public:
    /**
     * @brief 构造函数
     * @param x 要插入的容器引用
     */
    constexpr explicit back_insert_iterator(Container& x) noexcept :
    container(_NEFORCE addressof(x)) {}

    /**
     * @brief 赋值操作符，插入元素到容器尾部
     * @param value 要插入的元素值
     * @return 迭代器自身的引用
     */
    constexpr back_insert_iterator& operator=(const typename Container::value_type& value) {
        container->push_back(value);
        return *this;
    }

    /**
     * @brief 移动赋值操作符，插入元素到容器尾部
     * @param value 要插入的元素值
     * @return 迭代器自身的引用
     */
    constexpr back_insert_iterator& operator=(typename Container::value_type&& value) {
        container->push_back(_NEFORCE move(value));
        return *this;
    }

    NEFORCE_CONSTEXPR20 ~back_insert_iterator() noexcept = default; ///< 析构函数

    NEFORCE_NODISCARD constexpr back_insert_iterator& operator*() noexcept { return *this; } ///< 解引用操作符
    constexpr back_insert_iterator& operator++() noexcept { return *this; }                  ///< 前置自增操作符
    constexpr back_insert_iterator& operator++(int) noexcept { return *this; }               ///< 后置自增操作符
};

/**
 * @brief 创建back_insert_iterator的辅助函数
 * @tparam Container 容器类型
 * @param x 目标容器的引用
 * @return 构造的back_insert_iterator
 */
template <typename Container>
NEFORCE_NODISCARD constexpr back_insert_iterator<Container> make_back_inserter(Container& x) noexcept {
    return back_insert_iterator<Container>(x);
}


/**
 * @class front_insert_iterator
 * @brief 头部插入迭代器
 * @tparam Container 容器类型
 *
 * 通过push_front方法向容器头部插入元素的输出迭代器。
 */
template <typename Container>
class front_insert_iterator {
public:
    using iterator_category = output_iterator_tag; ///< 迭代器类别
    using value_type = void;                       ///< 值类型
    using difference_type = void;                  ///< 差值类型
    using pointer = void;                          ///< 指针类型
    using reference = void;                        ///< 引用类型

private:
    Container* container; ///< 指向目标容器的指针

public:
    /**
     * @brief 构造函数
     * @param x 目标容器的引用
     */
    constexpr explicit front_insert_iterator(Container& x) noexcept :
    container(_NEFORCE addressof(x)) {}

    /**
     * @brief 赋值操作符，插入元素到容器头部
     * @param value 要插入的元素值
     * @return 迭代器自身的引用
     */
    constexpr front_insert_iterator& operator=(const typename Container::value_type& value) {
        container->push_front(value);
        return *this;
    }

    /**
     * @brief 移动赋值操作符，插入元素到容器头部
     * @param value 要插入的元素值
     * @return 迭代器自身的引用
     */
    constexpr front_insert_iterator& operator=(typename Container::value_type&& value) {
        container->push_front(_NEFORCE move(value));
        return *this;
    }

    NEFORCE_CONSTEXPR20 ~front_insert_iterator() noexcept = default; ///< 析构函数

    NEFORCE_NODISCARD constexpr front_insert_iterator& operator*() noexcept { return *this; } ///< 解引用操作符
    constexpr front_insert_iterator& operator++() noexcept { return *this; }                  ///< 前置自增操作符
    constexpr front_insert_iterator& operator++(int) noexcept { return *this; }               ///< 后置自增操作符
};

/**
 * @brief 创建front_insert_iterator的辅助函数
 * @tparam Container 容器类型
 * @param x 目标容器的引用
 * @return 构造的front_insert_iterator
 */
template <typename Container>
NEFORCE_NODISCARD constexpr front_insert_iterator<Container> make_front_inserter(Container& x) noexcept {
    return front_insert_iterator<Container>(x);
}


/**
 * @class insert_iterator
 * @brief 指定位置插入迭代器
 * @tparam Container 容器类型
 *
 * 在指定位置插入元素的输出迭代器，支持任意位置的插入。
 */
template <typename Container>
class insert_iterator {
public:
    using iterator_category = output_iterator_tag; ///< 迭代器类别
    using value_type = void;                       ///< 值类型
    using difference_type = void;                  ///< 差值类型
    using pointer = void;                          ///< 指针类型
    using reference = void;                        ///< 引用类型

private:
    Container* container;              ///< 指向目标容器的指针
    typename Container::iterator iter; ///< 插入位置的迭代器

public:
    /**
     * @brief 构造函数
     * @param x 目标容器的引用
     * @param it 插入位置的迭代器
     */
    constexpr insert_iterator(Container& x, typename Container::iterator it) noexcept :
    container(_NEFORCE addressof(x)),
    iter(_NEFORCE move(it)) {}

    /**
     * @brief 赋值操作符，在指定位置插入元素
     * @param value 要插入的元素值
     * @return 迭代器自身的引用
     */
    constexpr insert_iterator& operator=(const typename Container::value_type& value) {
        iter = container->insert(iter, value);
        ++iter;
        return *this;
    }

    /**
     * @brief 移动赋值操作符，在指定位置插入元素
     * @param value 要插入的元素值
     * @return 迭代器自身的引用
     */
    constexpr insert_iterator& operator=(typename Container::value_type&& value) {
        iter = container->insert(iter, _NEFORCE move(value));
        ++iter;
        return *this;
    }

    NEFORCE_CONSTEXPR20 ~insert_iterator() noexcept = default; ///< 析构函数

    NEFORCE_NODISCARD constexpr insert_iterator& operator*() noexcept { return *this; } ///< 解引用操作符
    constexpr insert_iterator& operator++() noexcept { return *this; }                  ///< 前置自增操作符
    constexpr insert_iterator& operator++(int) noexcept { return *this; }               ///< 后置自增操作符
};

/**
 * @brief 创建insert_iterator的辅助函数
 * @tparam Container 容器类型
 * @param x 目标容器的引用
 * @param it 插入位置的迭代器
 * @return 构造的insert_iterator
 */
template <typename Container>
NEFORCE_NODISCARD constexpr insert_iterator<Container> make_inserter(Container& x,
                                                                     typename Container::iterator it) noexcept {
    return insert_iterator<Container>(x, it);
}

/** @} */ // InsertIterators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ITERATOR_INSERT_ITERATOR_HPP__
