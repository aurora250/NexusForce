#ifndef NEFORCE_CORE_ITERATOR_RANGES_GROUP_BY_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_GROUP_BY_VIEW_HPP__

/**
 * @file group_by_view.hpp
 * @brief 按键分组的视图
 */

#include "NeForce/core/algorithm/sort.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/functor.hpp"
#include "NeForce/core/iterator/ranges/core.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @class grouping
 * @brief 分组结果
 * @tparam Key 键类型
 * @tparam Iterator 组内迭代器类型
 * @tparam Sentinel 组内哨兵类型
 */
template <typename Key, typename Iterator, typename Sentinel>
class grouping : public view_base<grouping<Key, Iterator, Sentinel>> {
public:
    using key_type = Key;      ///< 键类型
    using iterator = Iterator; ///< 迭代器类型
    using sentinel = Sentinel; ///< 哨兵类型

    /**
     * @brief 默认构造函数
     */
    constexpr grouping() = default;

    /**
     * @brief 构造分组
     * @param key 分组键
     * @param first 组内起始迭代器
     * @param last 组内结束哨兵
     */
    constexpr grouping(Key key, Iterator first, Sentinel last) :
    key_(_NEFORCE move(key)),
    first_(first),
    last_(last) {}

    /**
     * @brief 获取分组键
     * @return 分组键的引用
     */
    NEFORCE_NODISCARD constexpr const Key& key() const noexcept { return key_; }

    /**
     * @brief 获取组内起始迭代器
     * @return 起始迭代器
     */
    NEFORCE_NODISCARD constexpr iterator begin() const { return first_; }

    /**
     * @brief 获取组内结束哨兵
     * @return 结束哨兵
     */
    NEFORCE_NODISCARD constexpr sentinel end() const { return last_; }

private:
    Key key_{};
    Iterator first_{};
    Sentinel last_{};
};


/**
 * @class group_by_view
 * @brief 按键分组的视图
 * @tparam V 底层视图类型
 * @tparam Key 键函数类型
 * @tparam Cmp 键比较器类型
 */
template <View V, typename Key, typename Cmp>
class group_by_view : public view_base<group_by_view<V, Key, Cmp>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin());         ///< 底层迭代器类型
    using value_type = iter_value_t<base_iterator>;                                         ///< 元素类型
    using buffer_iterator = typename vector<value_type>::iterator;                          ///< 缓冲区迭代器类型
    using key_type = remove_cvref_t<invoke_result_t<Key, iter_reference_t<base_iterator>>>; ///< 键类型
    using grouping_type = grouping<key_type, buffer_iterator, buffer_iterator>;             ///< 分组类型

    /**
     * @class iterator
     * @brief 分组视图的迭代器
     */
    class iterator {
    public:
        using iterator_category = forward_iterator_tag; ///< 迭代器类别
        using value_type = grouping_type;               ///< 元素类型
        using difference_type = ptrdiff_t;              ///< 迭代器距离类型
        using pointer = void;                           ///< 元素指针类型
        using reference = grouping_type;                ///< 元素引用类型

        /**
         * @brief 默认构造函数
         */
        iterator() = default;

        /**
         * @brief 构造分组迭代器
         * @param current 当前组起始迭代器
         * @param end 缓冲区结束迭代器
         * @param key 键函数的指针
         * @param cmp 键比较器的指针
         */
        iterator(buffer_iterator current, buffer_iterator end, const Key* key, const Cmp* cmp) :
        current_(current),
        end_(end),
        group_end_(current),
        key_(key),
        cmp_(cmp) {
            satisfy();
        }

        /**
         * @brief 解引用迭代器，得到当前分组
         * @return 当前分组
         */
        NEFORCE_NODISCARD reference operator*() const {
            return grouping_type((*key_)(*current_), current_, group_end_);
        }

        /**
         * @brief 前置递增，移到下一个分组
         * @return 递增后的迭代器引用
         */
        iterator& operator++() {
            current_ = group_end_;
            satisfy();
            return *this;
        }

        /**
         * @brief 后置递增
         * @return 递增前的迭代器副本
         */
        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        /**
         * @brief 相等比较，只比较当前组起始迭代器
         * @param other 另一个迭代器
         * @return 是否相等
         */
        NEFORCE_NODISCARD bool operator==(const iterator& other) const { return current_ == other.current_; }

        /**
         * @brief 不等比较
         * @param other 另一个迭代器
         * @return 是否不相等
         */
        NEFORCE_NODISCARD bool operator!=(const iterator& other) const { return !(*this == other); }

    private:
        void satisfy() {
            group_end_ = current_;
            while (group_end_ != end_) {
                auto next = group_end_;
                ++next;
                if (next == end_ || (*cmp_)((*key_)(*group_end_), (*key_)(*next))) {
                    group_end_ = next;
                    return;
                }
                group_end_ = next;
            }
        }

        buffer_iterator current_{};
        buffer_iterator end_{};
        buffer_iterator group_end_{};
        const Key* key_ = nullptr;
        const Cmp* cmp_ = nullptr;
    };

    using const_iterator = iterator; ///< 常量迭代器类型
    using sentinel = iterator;       ///< 哨兵类型
    using const_sentinel = iterator; ///< 常量哨兵类型

private:
    V base_;
    Key key_;
    Cmp cmp_;
    mutable vector<value_type> buffer_;
    mutable bool built_ = false;

    void ensure() const {
        if (built_) {
            return;
        }
        buffer_.clear();
        auto& self = const_cast<remove_const_t<V>&>(base_);
        for (auto&& element: self) {
            buffer_.push_back(element);
        }
        _NEFORCE merge_sort(buffer_.begin(), buffer_.end(), [this](const value_type& lhs, const value_type& rhs) {
            return cmp_(key_(lhs), key_(rhs));
        });
        built_ = true;
    }

public:
    /**
     * @brief 默认构造函数
     */
    group_by_view() = default;

    /**
     * @brief 构造分组视图
     * @param base 底层视图
     * @param key 键函数
     * @param cmp 键比较器
     */
    constexpr group_by_view(V base, Key key, Cmp cmp) :
    base_(_NEFORCE move(base)),
    key_(_NEFORCE move(key)),
    cmp_(_NEFORCE move(cmp)) {}

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() {
        ensure();
        return iterator(buffer_.begin(), buffer_.end(), &key_, &cmp_);
    }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() {
        ensure();
        return sentinel(buffer_.end(), buffer_.end(), &key_, &cmp_);
    }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    const_iterator begin() const {
        ensure();
        return const_iterator(buffer_.begin(), buffer_.end(), &key_, &cmp_);
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const {
        ensure();
        return const_sentinel(buffer_.end(), buffer_.end(), &key_, &cmp_);
    }

    /**
     * @brief 获取分组个数
     * @return 分组个数
     */
    NEFORCE_NODISCARD size_t group_count() const {
        size_t total = 0;
        for (auto iter = begin(); iter != end(); ++iter) {
            ++total;
        }
        return total;
    }

    /**
     * @brief 获取底层视图
     * @return 底层视图的副本
     */
    constexpr V base() const&
        requires copy_constructible<V>
    {
        return base_;
    }

    /**
     * @brief 获取底层视图
     * @return 底层视图
     */
    constexpr V base() && { return _NEFORCE move(base_); }
};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_GROUP_BY_VIEW_HPP__
