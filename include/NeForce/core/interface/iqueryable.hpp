#ifndef NEFORCE_CORE_INTERFACE_IQUERYABLE_HPP__
#define NEFORCE_CORE_INTERFACE_IQUERYABLE_HPP__

/**
 * @file iqueryable.hpp
 * @brief 链式查询接口
 *
 * 此文件定义了链式查询 CRTP 接口，为派生类型提供惰性算子与终端算子的链式调用能力。
 */

#include "NeForce/core/container/list.hpp"
#include "NeForce/core/container/map.hpp"
#include "NeForce/core/container/set.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/functor.hpp"
#include "NeForce/core/interface/icommon.hpp"
#include "NeForce/core/iterator/ranges/chunk_view.hpp"
#include "NeForce/core/iterator/ranges/distinct_view.hpp"
#include "NeForce/core/iterator/ranges/enumerate_view.hpp"
#include "NeForce/core/iterator/ranges/group_by_view.hpp"
#include "NeForce/core/iterator/ranges/order_by_view.hpp"
#include "NeForce/core/iterator/ranges/scan_view.hpp"
#include "NeForce/core/iterator/ranges/sliding_view.hpp"
#include "NeForce/core/iterator/ranges/views.hpp"
#include "NeForce/core/iterator/ranges/zip_view.hpp"
#include "NeForce/core/utility/optional.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @struct iqueryable
 * @brief 链式查询接口模板
 * @tparam Derived 派生的查询类型
 *
 * 以 CRTP 方式为派生类型提供链式算子。
 *
 * 派生类型需要提供：
 * - `using value_type`：元素类型
 * - `using base_view_t`：底层视图类型
 * - `view()`：返回底层视图
 * - `make(view)`：由给定视图构造新的查询对象
 */
template <typename Derived>
struct iqueryable : icomparable<Derived> {
private:
    constexpr Derived& derived() noexcept { return static_cast<Derived&>(*this); }

    constexpr const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }

public:
    /**
     * @brief 保留满足谓词的元素
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 新的查询对象
     */
    template <typename Pred>
    NEFORCE_NODISCARD constexpr auto filter(Pred pred) const& {
        return derived().make(derived().view() | ranges::views::filter(_NEFORCE move(pred)));
    }

    /**
     * @brief 保留满足谓词的元素，并移动底层视图
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 新的查询对象
     */
    template <typename Pred>
    NEFORCE_NODISCARD constexpr auto filter(Pred pred) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::filter(_NEFORCE move(pred)));
    }

    /**
     * @brief 将每个元素变换为新元素
     * @tparam Func 变换函数类型
     * @param func 变换函数
     * @return 新的查询对象
     */
    template <typename Func>
    NEFORCE_NODISCARD constexpr auto transform(Func func) const& {
        return derived().make(derived().view() | ranges::views::transform(_NEFORCE move(func)));
    }

    /**
     * @brief 将每个元素变换为新元素，并移动底层视图
     * @tparam Func 变换函数类型
     * @param func 变换函数
     * @return 新的查询对象
     */
    template <typename Func>
    NEFORCE_NODISCARD constexpr auto transform(Func func) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::transform(_NEFORCE move(func)));
    }

    /**
     * @brief 将每个元素变换为范围后展平
     * @tparam Func 变换函数类型
     * @param func 返回范围的变换函数
     * @return 新的查询对象
     */
    template <typename Func>
    NEFORCE_NODISCARD constexpr auto transform_many(Func func) const& {
        return derived().make(derived().view() | ranges::views::transform(_NEFORCE move(func)) | ranges::views::join());
    }

    /**
     * @brief 将每个元素变换为范围后展平，并移动底层视图
     * @tparam Func 变换函数类型
     * @param func 返回范围的变换函数
     * @return 新的查询对象
     */
    template <typename Func>
    NEFORCE_NODISCARD constexpr auto transform_many(Func func) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::transform(_NEFORCE move(func)) |
                              ranges::views::join());
    }

    /**
     * @brief 取前若干个元素
     * @tparam Size 计数类型
     * @param count 保留的元素个数
     * @return 新的查询对象
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto take(Size count) const& {
        return derived().make(derived().view() | ranges::views::take(count));
    }

    /**
     * @brief 取前若干个元素，并移动底层视图
     * @tparam Size 计数类型
     * @param count 保留的元素个数
     * @return 新的查询对象
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto take(Size count) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::take(count));
    }

    /**
     * @brief 取满足谓词的最长前缀
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 新的查询对象
     */
    template <typename Pred>
    NEFORCE_NODISCARD constexpr auto take_while(Pred pred) const& {
        return derived().make(derived().view() | ranges::views::take_while(_NEFORCE move(pred)));
    }

    /**
     * @brief 取满足谓词的最长前缀，并移动底层视图
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 新的查询对象
     */
    template <typename Pred>
    NEFORCE_NODISCARD constexpr auto take_while(Pred pred) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::take_while(_NEFORCE move(pred)));
    }

    /**
     * @brief 丢弃前若干个元素
     * @tparam Size 计数类型
     * @param count 丢弃的元素个数
     * @return 新的查询对象
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto drop(Size count) const& {
        return derived().make(derived().view() | ranges::views::drop(count));
    }

    /**
     * @brief 丢弃前若干个元素，并移动底层视图
     * @tparam Size 计数类型
     * @param count 丢弃的元素个数
     * @return 新的查询对象
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto drop(Size count) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::drop(count));
    }

    /**
     * @brief 丢弃满足谓词的最长前缀
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 新的查询对象
     */
    template <typename Pred>
    NEFORCE_NODISCARD constexpr auto drop_while(Pred pred) const& {
        return derived().make(derived().view() | ranges::views::drop_while(_NEFORCE move(pred)));
    }

    /**
     * @brief 丢弃满足谓词的最长前缀，并移动底层视图
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 新的查询对象
     */
    template <typename Pred>
    NEFORCE_NODISCARD constexpr auto drop_while(Pred pred) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::drop_while(_NEFORCE move(pred)));
    }

    /**
     * @brief 逆序遍历元素
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto reverse() const& {
        return derived().make(derived().view() | ranges::views::reverse());
    }

    /**
     * @brief 逆序遍历元素，并移动底层视图
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto reverse() && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::reverse());
    }

    /**
     * @brief 在末尾衔接另一个范围
     * @tparam R 范围类型
     * @param range 待衔接的范围
     * @return 新的查询对象
     */
    template <typename R>
    NEFORCE_NODISCARD constexpr auto concat(R&& range) const& {
        return derived().make(ranges::views::concat(derived().view(), _NEFORCE forward<R>(range)));
    }

    /**
     * @brief 在末尾衔接另一个范围，并移动底层视图
     * @tparam R 范围类型
     * @param range 待衔接的范围
     * @return 新的查询对象
     */
    template <typename R>
    NEFORCE_NODISCARD constexpr auto concat(R&& range) && {
        return derived().make(ranges::views::concat(_NEFORCE move(derived().view()), _NEFORCE forward<R>(range)));
    }

    /**
     * @brief 取元组或键值对中的第 N 个元素
     * @tparam N 元素下标
     * @return 新的查询对象
     */
    template <size_t N>
    NEFORCE_NODISCARD constexpr auto elements() const& {
        return derived().make(derived().view() | ranges::views::elements<N>());
    }

    /**
     * @brief 取元组或键值对中的第 N 个元素，并移动底层视图
     * @tparam N 元素下标
     * @return 新的查询对象
     */
    template <size_t N>
    NEFORCE_NODISCARD constexpr auto elements() && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::elements<N>());
    }

    /**
     * @brief 取键值对中的键
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto keys() const& { return elements<0>(); }

    /**
     * @brief 取键值对中的键，并移动底层视图
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto keys() && { return _NEFORCE move(*this).template elements<0>(); }

    /**
     * @brief 取键值对中的值
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto values() const& { return elements<1>(); }

    /**
     * @brief 取键值对中的值，并移动底层视图
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto values() && { return _NEFORCE move(*this).template elements<1>(); }

    /**
     * @brief 按分隔元素切分为多个子范围
     * @tparam T 分隔元素类型
     * @param delimiter 分隔元素
     * @return 新的查询对象
     */
    template <typename T>
    NEFORCE_NODISCARD constexpr auto split(T delimiter) const& {
        return derived().make(derived().view() | ranges::views::split(_NEFORCE move(delimiter)));
    }

    /**
     * @brief 按分隔元素切分为多个子范围，并移动底层视图
     * @tparam T 分隔元素类型
     * @param delimiter 分隔元素
     * @return 新的查询对象
     */
    template <typename T>
    NEFORCE_NODISCARD constexpr auto split(T delimiter) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::split(_NEFORCE move(delimiter)));
    }

    /**
     * @brief 截取下标区间
     * @param start 起始下标
     * @param end 结束下标
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto slice(ptrdiff_t start, ptrdiff_t end) const& {
        return derived().make(derived().view() | ranges::views::slice(start, end));
    }

    /**
     * @brief 截取下标区间，并移动底层视图
     * @param start 起始下标
     * @param end 结束下标
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto slice(ptrdiff_t start, ptrdiff_t end) && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::slice(start, end));
    }

    /**
     * @brief 展平嵌套范围
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto join() const& { return derived().make(derived().view() | ranges::views::join()); }

    /**
     * @brief 展平嵌套范围，并移动底层视图
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto join() && {
        return derived().make(_NEFORCE move(derived().view()) | ranges::views::join());
    }

    /**
     * @brief 按元素自身去重
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto distinct() const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using value_type = typename Derived::value_type;
        return derived().make(ranges::distinct_view<base_view_t, identity<value_type>, set<value_type>>{
                derived().view(), identity<value_type>{}});
    }

    /**
     * @brief 按元素自身去重，并移动底层视图
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto distinct() && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using value_type = typename Derived::value_type;
        return derived().make(ranges::distinct_view<base_view_t, identity<value_type>, set<value_type>>{
                _NEFORCE move(derived().view()), identity<value_type>{}});
    }

    /**
     * @brief 按键去重
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto distinct_by(Key key) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(
                ranges::distinct_view<base_view_t, Key, set<key_type>>{derived().view(), _NEFORCE move(key)});
    }

    /**
     * @brief 按键去重，并移动底层视图
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto distinct_by(Key key) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(ranges::distinct_view<base_view_t, Key, set<key_type>>{_NEFORCE move(derived().view()),
                                                                                     _NEFORCE move(key)});
    }

    /**
     * @brief 按键升序排序
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto order_by(Key key) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(ranges::order_by_view<base_view_t, Key, less<key_type>>{
                derived().view(), _NEFORCE move(key), less<key_type>{}});
    }

    /**
     * @brief 按键升序排序，并移动底层视图
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto order_by(Key key) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(ranges::order_by_view<base_view_t, Key, less<key_type>>{
                _NEFORCE move(derived().view()), _NEFORCE move(key), less<key_type>{}});
    }

    /**
     * @brief 按键降序排序
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto order_by_descending(Key key) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(ranges::order_by_view<base_view_t, Key, ranges::descending_order<less<key_type>>>{
                derived().view(), _NEFORCE move(key), ranges::descending_order<less<key_type>>{less<key_type>{}}});
    }

    /**
     * @brief 按键降序排序，并移动底层视图
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto order_by_descending(Key key) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(ranges::order_by_view<base_view_t, Key, ranges::descending_order<less<key_type>>>{
                _NEFORCE move(derived().view()), _NEFORCE move(key),
                ranges::descending_order<less<key_type>>{less<key_type>{}}});
    }

    /**
     * @brief 在当前排序结果上追加一级升序排序键
     * @tparam Key 键函数类型
     * @param key 次级键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto then_by(Key key) const& {
        using inner_view_t = remove_cvref_t<decltype(derived().view())>;
        static_assert(ranges::is_order_by_view_v<inner_view_t>,
                      "then_by requires the result of order_by / order_by_descending");
        using reference = decltype(*derived().view().begin());
        using first_cmp_t = typename inner_view_t::compare_type;
        using second_key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        using key_t = ranges::composite_key<typename inner_view_t::key_function_type, Key>;
        using compare_t = ranges::pair_compare<first_cmp_t, less<second_key_type>>;
        return derived().make(ranges::order_by_view<inner_view_t, key_t, compare_t>{
                derived().view(), key_t{derived().view().key_function(), _NEFORCE move(key)},
                compare_t{first_cmp_t{}, less<second_key_type>{}}});
    }

    /**
     * @brief 在当前排序结果上追加一级升序排序键，并移动底层视图
     * @tparam Key 键函数类型
     * @param key 次级键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto then_by(Key key) && {
        using inner_view_t = remove_cvref_t<decltype(derived().view())>;
        static_assert(ranges::is_order_by_view_v<inner_view_t>,
                      "then_by requires the result of order_by / order_by_descending");
        using reference = decltype(*derived().view().begin());
        using first_cmp_t = typename inner_view_t::compare_type;
        using second_key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        using key_t = ranges::composite_key<typename inner_view_t::key_function_type, Key>;
        using compare_t = ranges::pair_compare<first_cmp_t, less<second_key_type>>;
        auto composite = key_t{derived().view().key_function(), _NEFORCE move(key)};
        return derived().make(ranges::order_by_view<inner_view_t, key_t, compare_t>{
                _NEFORCE move(derived().view()), _NEFORCE move(composite),
                compare_t{first_cmp_t{}, less<second_key_type>{}}});
    }

    /**
     * @brief 在当前排序结果上追加一级降序排序键
     * @tparam Key 键函数类型
     * @param key 次级键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto then_by_descending(Key key) const& {
        using inner_view_t = remove_cvref_t<decltype(derived().view())>;
        static_assert(ranges::is_order_by_view_v<inner_view_t>,
                      "then_by_descending requires the result of order_by / order_by_descending");
        using reference = decltype(*derived().view().begin());
        using first_cmp_t = typename inner_view_t::compare_type;
        using second_key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        using key_t = ranges::composite_key<typename inner_view_t::key_function_type, Key>;
        using compare_t = ranges::pair_compare<first_cmp_t, ranges::descending_order<less<second_key_type>>>;
        return derived().make(ranges::order_by_view<inner_view_t, key_t, compare_t>{
                derived().view(), key_t{derived().view().key_function(), _NEFORCE move(key)},
                compare_t{first_cmp_t{}, ranges::descending_order<less<second_key_type>>{less<second_key_type>{}}}});
    }

    /**
     * @brief 在当前排序结果上追加一级降序排序键，并移动底层视图
     * @tparam Key 键函数类型
     * @param key 次级键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto then_by_descending(Key key) && {
        using inner_view_t = remove_cvref_t<decltype(derived().view())>;
        static_assert(ranges::is_order_by_view_v<inner_view_t>,
                      "then_by_descending requires the result of order_by / order_by_descending");
        using reference = decltype(*derived().view().begin());
        using first_cmp_t = typename inner_view_t::compare_type;
        using second_key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        using key_t = ranges::composite_key<typename inner_view_t::key_function_type, Key>;
        using compare_t = ranges::pair_compare<first_cmp_t, ranges::descending_order<less<second_key_type>>>;
        auto composite = key_t{derived().view().key_function(), _NEFORCE move(key)};
        return derived().make(ranges::order_by_view<inner_view_t, key_t, compare_t>{
                _NEFORCE move(derived().view()), _NEFORCE move(composite),
                compare_t{first_cmp_t{}, ranges::descending_order<less<second_key_type>>{less<second_key_type>{}}}});
    }

    /**
     * @brief 按键分组
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象，元素为分组对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto group_by(Key key) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(ranges::group_by_view<base_view_t, Key, less<key_type>>{
                derived().view(), _NEFORCE move(key), less<key_type>{}});
    }

    /**
     * @brief 按键分组，并移动底层视图
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 新的查询对象
     */
    template <typename Key>
    NEFORCE_NODISCARD constexpr auto group_by(Key key) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return derived().make(ranges::group_by_view<base_view_t, Key, less<key_type>>{
                _NEFORCE move(derived().view()), _NEFORCE move(key), less<key_type>{}});
    }

    /**
     * @brief 按元素自身分组
     * @return 新的查询对象，元素为分组对象
     * @note 需要元素支持小于比较。
     */
    NEFORCE_NODISCARD constexpr auto group_by() const& {
        using value_type = typename Derived::value_type;
        return group_by(identity<value_type>{});
    }

    /**
     * @brief 按元素自身分组，并移动底层视图
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto group_by() && {
        using value_type = typename Derived::value_type;
        return _NEFORCE move(*this).group_by(identity<value_type>{});
    }

    /**
     * @brief 按固定长度切分为若干块
     * @tparam Size 计数类型
     * @param count 每块长度
     * @return 新的查询对象，元素为子范围
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto chunk(Size count) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using difference_type = iter_difference_t<decltype(derived().view().begin())>;
        return derived().make(ranges::chunk_view<base_view_t>{derived().view(), static_cast<difference_type>(count)});
    }

    /**
     * @brief 按固定长度切分为若干块，并移动底层视图
     * @tparam Size 计数类型
     * @param count 每块长度
     * @return 新的查询对象
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto chunk(Size count) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using difference_type = iter_difference_t<decltype(derived().view().begin())>;
        return derived().make(
                ranges::chunk_view<base_view_t>{_NEFORCE move(derived().view()), static_cast<difference_type>(count)});
    }

    /**
     * @brief 产出所有定长滑动窗口
     * @tparam Size 计数类型
     * @param window 窗口宽度
     * @return 新的查询对象，元素为子范围
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto sliding(Size window) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using difference_type = iter_difference_t<decltype(derived().view().begin())>;
        return derived().make(
                ranges::sliding_view<base_view_t>{derived().view(), static_cast<difference_type>(window)});
    }

    /**
     * @brief 产出所有定长滑动窗口，并移动底层视图
     * @tparam Size 计数类型
     * @param window 窗口宽度
     * @return 新的查询对象
     */
    template <typename Size>
    NEFORCE_NODISCARD constexpr auto sliding(Size window) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using difference_type = iter_difference_t<decltype(derived().view().begin())>;
        return derived().make(ranges::sliding_view<base_view_t>{_NEFORCE move(derived().view()),
                                                                static_cast<difference_type>(window)});
    }

    /**
     * @brief 与另一个范围逐元素配对
     * @tparam R 范围类型
     * @param range 参与配对的另一个范围
     * @return 新的查询对象，元素为 pair
     */
    template <typename R>
    NEFORCE_NODISCARD constexpr auto zip(R&& range) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using other_view_t = decltype(ranges::all(_NEFORCE forward<R>(range)));
        return derived().make(
                ranges::zip_view<base_view_t, other_view_t>{derived().view(), ranges::all(_NEFORCE forward<R>(range))});
    }

    /**
     * @brief 与另一个范围逐元素配对，并移动底层视图
     * @tparam R 范围类型
     * @param range 参与配对的另一个范围
     * @return 新的查询对象
     */
    template <typename R>
    NEFORCE_NODISCARD constexpr auto zip(R&& range) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        using other_view_t = decltype(ranges::all(_NEFORCE forward<R>(range)));
        return derived().make(ranges::zip_view<base_view_t, other_view_t>{_NEFORCE move(derived().view()),
                                                                          ranges::all(_NEFORCE forward<R>(range))});
    }

    /**
     * @brief 为每个元素附带从 0 开始的连续下标
     * @return 新的查询对象，元素为 pair
     */
    NEFORCE_NODISCARD constexpr auto enumerate() const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        return derived().make(ranges::enumerate_view<base_view_t>{derived().view()});
    }

    /**
     * @brief 为每个元素附带从 0 开始的连续下标，并移动底层视图
     * @return 新的查询对象
     */
    NEFORCE_NODISCARD constexpr auto enumerate() && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        return derived().make(ranges::enumerate_view<base_view_t>{_NEFORCE move(derived().view())});
    }

    /**
     * @brief 以初值做包含式前缀累计
     * @tparam T 累加值类型
     * @tparam Op 累加函数类型
     * @param init 初值
     * @param op 累加函数，签名为 acc(acc, element)
     * @return 新的查询对象
     */
    template <typename T, typename Op>
    NEFORCE_NODISCARD constexpr auto scan(T init, Op op) const& {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        return derived().make(
                ranges::scan_view<base_view_t, T, Op>{derived().view(), _NEFORCE move(init), _NEFORCE move(op)});
    }

    /**
     * @brief 以初值做包含式前缀累计，并移动底层视图
     * @tparam T 累加值类型
     * @tparam Op 累加函数类型
     * @param init 初值
     * @param op 累加函数，签名为 acc(acc, element)
     * @return 新的查询对象
     */
    template <typename T, typename Op>
    NEFORCE_NODISCARD constexpr auto scan(T init, Op op) && {
        using base_view_t = remove_cvref_t<decltype(derived().view())>;
        return derived().make(ranges::scan_view<base_view_t, T, Op>{_NEFORCE move(derived().view()),
                                                                    _NEFORCE move(init), _NEFORCE move(op)});
    }

    /**
     * @brief 以元素中的键值对收集到 map
     * @return 键值映射，键重复时保留第一次出现的元素
     */
    NEFORCE_NODISCARD auto to_map() const {
        using value_type = typename Derived::value_type;
        using key_type = remove_cvref_t<decltype(_NEFORCE get<0>(_NEFORCE declval<value_type>()))>;
        using mapped_type = remove_cvref_t<decltype(_NEFORCE get<1>(_NEFORCE declval<value_type>()))>;
        map<key_type, mapped_type> out;
        for (auto&& element: derived().view()) {
            out.insert(pair<const key_type, mapped_type>{_NEFORCE get<0>(element), _NEFORCE get<1>(element)});
        }
        return out;
    }

    /**
     * @brief 按键函数与值函数收集到 map
     * @tparam Key 键函数类型
     * @tparam Value 值函数类型
     * @param key 键函数
     * @param value 值函数
     * @return 键值映射，键重复时保留第一次出现的元素
     */
    template <typename Key, typename Value>
    NEFORCE_NODISCARD auto to_map(Key key, Value value) const {
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        using mapped_type = remove_cvref_t<invoke_result_t<Value, reference>>;
        map<key_type, mapped_type> out;
        for (auto&& element: derived().view()) {
            out.insert(pair<const key_type, mapped_type>{key(element), value(element)});
        }
        return out;
    }

    /**
     * @brief 按键函数收集成分组映射
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 键到元素集合的映射
     */
    template <typename Key>
    NEFORCE_NODISCARD auto to_lookup(Key key) const {
        using value_type = typename Derived::value_type;
        using reference = decltype(*derived().view().begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        map<key_type, vector<value_type>> out;
        for (auto&& element: derived().view()) {
            auto iter = out.find(key(element));
            if (iter == out.end()) {
                iter = out.insert(pair<const key_type, vector<value_type>>{key(element), vector<value_type>{}}).first;
            }
            iter->second.push_back(element);
        }
        return out;
    }

    /**
     * @brief 收集到任意支持 push_back 的容器
     * @tparam Container 目标容器类型
     * @return 收集得到的容器
     */
    template <typename Container>
    NEFORCE_NODISCARD Container to() const {
        Container out;
        const auto& view = derived().view();
        if constexpr (requires(Container& c, size_t n) { c.reserve(n); }) {
            if constexpr (ranges::sized_view<remove_cvref_t<decltype(view)>>) {
                out.reserve(view.size());
            }
        }
        for (auto&& value: view) {
            out.push_back(value);
        }
        return out;
    }

    /**
     * @brief 收集到 vector
     * @return 元素集合
     */
    NEFORCE_NODISCARD auto to_vector() const { return to<vector<typename Derived::value_type>>(); }

    /**
     * @brief 收集到 list
     * @return 元素集合
     */
    NEFORCE_NODISCARD auto to_list() const { return to<list<typename Derived::value_type>>(); }

    /**
     * @brief 收集到 set
     * @return 去重且有序的元素集合
     */
    NEFORCE_NODISCARD auto to_set() const {
        set<typename Derived::value_type> out;
        for (auto&& value: derived().view()) {
            out.insert(value);
        }
        return out;
    }

    /**
     * @brief 统计元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD size_t count() const {
        size_t total = 0;
        for (auto&& value: derived().view()) {
            (void) value;
            ++total;
        }
        return total;
    }

    /**
     * @brief 统计满足谓词的元素个数
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 满足谓词的元素个数
     */
    template <typename Pred>
    NEFORCE_NODISCARD size_t count_if(Pred pred) const {
        size_t total = 0;
        for (auto&& value: derived().view()) {
            if (pred(value)) {
                ++total;
            }
        }
        return total;
    }

    /**
     * @brief 以指定初值求和
     * @tparam T 累加类型
     * @param init 初值
     * @return 累加结果
     */
    template <typename T>
    NEFORCE_NODISCARD T sum(T init) const {
        for (auto&& value: derived().view()) {
            init += value;
        }
        return init;
    }

    /**
     * @brief 求和
     * @return 累加结果
     */
    NEFORCE_NODISCARD auto sum() const { return sum(typename Derived::value_type{}); }

    /**
     * @brief 以指定初值求积
     * @tparam T 累乘类型
     * @param init 初值
     * @return 累乘结果
     */
    template <typename T>
    NEFORCE_NODISCARD T product(T init) const {
        for (auto&& value: derived().view()) {
            init *= value;
        }
        return init;
    }

    /**
     * @brief 求平均值
     * @return 平均值，空序列返回 0
     */
    NEFORCE_NODISCARD auto average() const {
        using value_type = typename Derived::value_type;
        static_assert(is_arithmetic_v<value_type>, "average() requires arithmetic element type");
        double total = 0.0;
        size_t total_count = 0;
        for (auto&& value: derived().view()) {
            total += static_cast<typename Derived::value_type>(value);
            ++total_count;
        }
        return total_count == 0 ? 0.0 : total / static_cast<double>(total_count);
    }

    /**
     * @brief 求最小值
     * @return 最小元素，空序列返回空
     */
    NEFORCE_NODISCARD auto min() const {
        using value_type = typename Derived::value_type;
        const auto& view = derived().view();
        auto iter = view.begin();
        if (iter == view.end()) {
            return optional<value_type>{};
        }
        value_type best = *iter;
        ++iter;
        for (; iter != view.end(); ++iter) {
            if (*iter < best) {
                best = *iter;
            }
        }
        return optional<value_type>{_NEFORCE move(best)};
    }

    /**
     * @brief 求最大值
     * @return 最大元素，空序列返回空
     */
    NEFORCE_NODISCARD auto max() const {
        using value_type = typename Derived::value_type;
        const auto& view = derived().view();
        auto iter = view.begin();
        if (iter == view.end()) {
            return optional<value_type>{};
        }
        value_type best = *iter;
        ++iter;
        for (; iter != view.end(); ++iter) {
            if (best < *iter) {
                best = *iter;
            }
        }
        return optional<value_type>{_NEFORCE move(best)};
    }

    /**
     * @brief 按键取最小元素
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 键最小的元素，空序列返回空
     */
    template <typename Key>
    NEFORCE_NODISCARD auto min_by(Key key) const {
        using value_type = typename Derived::value_type;
        const auto& view = derived().view();
        auto iter = view.begin();
        if (iter == view.end()) {
            return optional<value_type>{};
        }
        value_type best = *iter;
        auto best_key = key(best);
        ++iter;
        for (; iter != view.end(); ++iter) {
            auto current_key = key(*iter);
            if (current_key < best_key) {
                best = *iter;
                best_key = _NEFORCE move(current_key);
            }
        }
        return optional<value_type>{_NEFORCE move(best)};
    }

    /**
     * @brief 按键取最大元素
     * @tparam Key 键函数类型
     * @param key 键函数
     * @return 键最大的元素，空序列返回空
     */
    template <typename Key>
    NEFORCE_NODISCARD auto max_by(Key key) const {
        using value_type = typename Derived::value_type;
        const auto& view = derived().view();
        auto iter = view.begin();
        if (iter == view.end()) {
            return optional<value_type>{};
        }
        value_type best = *iter;
        auto best_key = key(best);
        ++iter;
        for (; iter != view.end(); ++iter) {
            auto current_key = key(*iter);
            if (best_key < current_key) {
                best = *iter;
                best_key = _NEFORCE move(current_key);
            }
        }
        return optional<value_type>{_NEFORCE move(best)};
    }

    /**
     * @brief 取第一个元素
     * @return 第一个元素，空序列返回空
     */
    NEFORCE_NODISCARD auto first() const {
        using value_type = typename Derived::value_type;
        const auto& view = derived().view();
        auto iter = view.begin();
        if (iter == view.end()) {
            return optional<value_type>{};
        }
        return optional<value_type>{*iter};
    }

    /**
     * @brief 取第一个元素，空序列返回给定默认值
     * @tparam U 默认值类型
     * @param fallback 空序列时使用的默认值
     * @return 第一个元素或默认值
     */
    template <typename U>
    NEFORCE_NODISCARD auto first_or(U&& fallback) const {
        using value_type = typename Derived::value_type;
        const auto& view = derived().view();
        auto iter = view.begin();
        if (iter == view.end()) {
            return value_type(_NEFORCE forward<U>(fallback));
        }
        return value_type(*iter);
    }

    /**
     * @brief 取最后一个元素
     * @return 最后一个元素，空序列返回空
     */
    NEFORCE_NODISCARD auto last() const {
        using value_type = typename Derived::value_type;
        optional<value_type> result;
        for (auto&& value: derived().view()) {
            result = value_type(value);
        }
        return result;
    }

    /**
     * @brief 取最后一个元素，空序列返回给定默认值
     * @tparam U 默认值类型
     * @param fallback 空序列时使用的默认值
     * @return 最后一个元素或默认值
     */
    template <typename U>
    NEFORCE_NODISCARD auto last_or(U&& fallback) const {
        using value_type = typename Derived::value_type;
        auto result = last();
        if (!result) {
            return value_type(_NEFORCE forward<U>(fallback));
        }
        return value_type(*result);
    }

    /**
     * @brief 取指定下标的元素
     * @tparam Size 下标类型
     * @param index 目标下标
     * @return 目标元素，越界返回空
     */
    template <typename Size>
    NEFORCE_NODISCARD auto element_at(Size index) const {
        using value_type = typename Derived::value_type;
        const auto& view = derived().view();
        auto iter = view.begin();
        for (Size current = 0; iter != view.end(); ++iter, ++current) {
            if (!(current < index)) {
                return optional<value_type>{*iter};
            }
        }
        return optional<value_type>{};
    }

    /**
     * @brief 判断是否存在满足谓词的元素
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 是否存在
     */
    template <typename Pred>
    NEFORCE_NODISCARD bool any_of(Pred pred) const {
        for (auto&& value: derived().view()) {
            if (pred(value)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 判断是否所有元素都满足谓词
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 是否全部满足，空序列返回 true
     */
    template <typename Pred>
    NEFORCE_NODISCARD bool all_of(Pred pred) const {
        for (auto&& value: derived().view()) {
            if (!pred(value)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 判断是否没有元素满足谓词
     * @tparam Pred 谓词类型
     * @param pred 判定谓词
     * @return 是否全部不满足
     */
    template <typename Pred>
    NEFORCE_NODISCARD bool none_of(Pred pred) const {
        return !any_of(_NEFORCE move(pred));
    }

    /**
     * @brief 判断是否包含指定元素
     * @tparam U 目标值类型
     * @param value 目标值
     * @return 是否包含
     */
    template <typename U>
    NEFORCE_NODISCARD bool contains(const U& value) const {
        for (auto&& element: derived().view()) {
            if (element == value) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 以初值和累加函数折叠查询
     * @tparam Acc 累加器类型
     * @tparam Op 累加函数类型
     * @param init 初值
     * @param op 累加函数，签名为 acc(acc, element)
     * @return 折叠结果
     */
    template <typename Acc, typename Op>
    NEFORCE_NODISCARD Acc aggregate(Acc init, Op op) const {
        for (auto&& value: derived().view()) {
            init = op(_NEFORCE move(init), value);
        }
        return init;
    }

    /**
     * @brief 以初值和累加函数折叠查询
     * @tparam Acc 累加器类型
     * @tparam Op 累加函数类型
     * @param init 初值
     * @param op 累加函数，签名为 acc(acc, element)
     * @return 折叠结果
     */
    template <typename Acc, typename Op>
    NEFORCE_NODISCARD Acc fold(Acc init, Op op) const {
        return aggregate(_NEFORCE move(init), _NEFORCE move(op));
    }

    /**
     * @brief 对每个元素执行操作
     * @tparam Func 操作类型
     * @param func 操作函数
     * @return 传入的操作函数
     */
    template <typename Func>
    Func for_each(Func func) const {
        for (auto&& value: derived().view()) {
            func(value);
        }
        return func;
    }

    /**
     * @brief 判断查询是否为空
     * @return 是否为空
     * @note 最多遍历一个元素。
     */
    NEFORCE_NODISCARD bool is_empty() const {
        const auto& view = derived().view();
        return view.begin() == view.end();
    }
};

/** @} */ // CRTPInterfaces

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_INTERFACE_IQUERYABLE_HPP__
