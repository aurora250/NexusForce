#ifndef NEFORCE_CORE_ITERATOR_RANGES_SUB_RANGE_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_SUB_RANGE_VIEW_HPP__

/**
 * @file subrange_view.hpp
 * @brief 由迭代器对构成的子范围视图
 */

#include "NeForce/core/iterator/ranges/core.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @class subrange_view
 * @brief 由一对迭代器构成的子范围视图
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
class subrange_view : public view_base<subrange_view<Iterator>> {
    Iterator first_{};
    Iterator last_{};

public:
    subrange_view() = default;

    /**
     * @brief 由迭代器对构造
     * @param first 区间起始迭代器
     * @param last 区间末尾迭代器
     */
    subrange_view(Iterator first, Iterator last) :
    first_(first),
    last_(last) {}

    /**
     * @brief 获取起始迭代器
     * @return 区间起始迭代器
     */
    Iterator begin() const { return first_; }

    /**
     * @brief 获取末尾迭代器
     * @return 区间末尾迭代器
     */
    Iterator end() const { return last_; }

    /**
     * @brief 获取区间长度
     * @return 区间内元素个数
     */
    auto size() const
        requires sized_sentinel_for<Iterator, Iterator>
    {
        const auto total = last_ - first_;
        return static_cast<size_t>(total > 0 ? total : 0);
    }
};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_SUB_RANGE_VIEW_HPP__
