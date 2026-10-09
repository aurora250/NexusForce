#ifndef NEFORCE_CORE_ITERATOR_RANGES_VIEWS_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_VIEWS_HPP__

/**
 * @file views.hpp
 * @brief 视图适配器与 views 命名空间
 */

#include "NeForce/core/container/set.hpp"
#include "NeForce/core/functional/functor.hpp"
#include "NeForce/core/iterator/ranges/chunk_view.hpp"
#include "NeForce/core/iterator/ranges/core.hpp"
#include "NeForce/core/iterator/ranges/distinct_view.hpp"
#include "NeForce/core/iterator/ranges/enumerate_view.hpp"
#include "NeForce/core/iterator/ranges/group_by_view.hpp"
#include "NeForce/core/iterator/ranges/order_by_view.hpp"
#include "NeForce/core/iterator/ranges/scan_view.hpp"
#include "NeForce/core/iterator/ranges/sliding_view.hpp"
#include "NeForce/core/iterator/ranges/zip_view.hpp"
#include "NeForce/core/iterator/ranges/ref_view.hpp"
#include "NeForce/core/iterator/ranges/filter_view.hpp"
#include "NeForce/core/iterator/ranges/transform_view.hpp"
#include "NeForce/core/iterator/ranges/take_view.hpp"
#include "NeForce/core/iterator/ranges/take_while_view.hpp"
#include "NeForce/core/iterator/ranges/drop_view.hpp"
#include "NeForce/core/iterator/ranges/drop_while_view.hpp"
#include "NeForce/core/iterator/ranges/reverse_view.hpp"
#include "NeForce/core/iterator/ranges/iota_view.hpp"
#include "NeForce/core/iterator/ranges/repeat_view.hpp"
#include "NeForce/core/iterator/ranges/join_view.hpp"
#include "NeForce/core/iterator/ranges/element_view.hpp"
#include "NeForce/core/iterator/ranges/common_view.hpp"
#include "NeForce/core/iterator/ranges/counted_view.hpp"
#include "NeForce/core/iterator/ranges/concat_view.hpp"
#include "NeForce/core/iterator/ranges/subrange_view.hpp"
#include "NeForce/core/iterator/ranges/split_view.hpp"
#include "NeForce/core/iterator/ranges/slice_view.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

NEFORCE_BEGIN_RANGES_VIEWS__

/**
 * @brief 范围到视图的工厂适配器
 */
struct all_adaptor {
    /**
     * @brief 把范围转换为视图
     * @param range 任意 Range
     * @return 与实参对应的视图对象
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return all(_NEFORCE forward<R>(range));
    }
};

/**
 * @brief 可管道使用的 all 适配器对象
 */
NEFORCE_INLINE17 constexpr all_adaptor all;


/**
 * @brief filter 适配器闭包，持有过滤谓词
 * @tparam Pred 一元谓词类型
 */
template <typename Pred>
struct filter_adaptor_closure : range_adaptor_closure<filter_adaptor_closure<Pred>> {
    Pred pred; ///< 判定元素是否保留的谓词

    /**
     * @brief 构造闭包并接管谓词
     * @param p 谓词
     */
    constexpr explicit filter_adaptor_closure(Pred p) :
    pred(_NEFORCE move(p)) {}

    /**
     * @brief 保留满足谓词的元素
     * @param range 待过滤的 Range
     * @return 仅含满足谓词元素的 filter_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return filter_view{all(_NEFORCE forward<R>(range)), pred};
    }
};

/**
 * @brief filter 适配器，绑定谓词得到闭包或直接过滤范围
 */
struct filter_adaptor {
    /**
     * @brief 绑定谓词，得到可管道闭包
     * @tparam Pred 一元谓词类型
     * @param pred 谓词
     * @return filter_adaptor_closure<Pred>
     */
    template <typename Pred>
    constexpr auto operator()(Pred pred) const {
        return filter_adaptor_closure<Pred>{_NEFORCE move(pred)};
    }

    /**
     * @brief 直接对范围应用过滤
     * @tparam R 范围类型
     * @tparam Pred 一元谓词类型
     * @param range 待过滤的 Range
     * @param pred 谓词
     * @return 过滤后的 filter_view
     */
    template <Range R, typename Pred>
    constexpr auto operator()(R&& range, Pred pred) const {
        return filter_view{all(_NEFORCE forward<R>(range)), _NEFORCE move(pred)};
    }
};

/**
 * @brief 可管道使用的 filter 适配器对象
 */
NEFORCE_INLINE17 constexpr filter_adaptor filter;


/**
 * @brief transform 适配器闭包，持有变换函数
 * @tparam Func 变换函数类型
 */
template <typename Func>
struct transform_adaptor_closure : range_adaptor_closure<transform_adaptor_closure<Func>> {
    Func func; ///< 逐元素变换函数

    /**
     * @brief 构造闭包并接管变换函数
     * @param f 变换函数
     */
    constexpr explicit transform_adaptor_closure(Func f) :
    func(_NEFORCE move(f)) {}

    /**
     * @brief 对每个元素应用变换函数
     * @param range 待变换的 Range
     * @return 逐元素变换结果的 transform_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return transform_view{all(_NEFORCE forward<R>(range)), func};
    }
};

/**
 * @brief transform 适配器，绑定变换函数得到闭包或直接作用于范围
 */
struct transform_adaptor {
    /**
     * @brief 绑定变换函数，得到可管道闭包
     * @tparam Func 变换函数类型
     * @param func 变换函数
     * @return transform_adaptor_closure<Func>
     */
    template <typename Func>
    constexpr auto operator()(Func func) const {
        return transform_adaptor_closure<Func>{_NEFORCE move(func)};
    }

    /**
     * @brief 直接对范围做逐元素变换
     * @tparam R 范围类型
     * @tparam Func 变换函数类型
     * @param range 待变换的 Range
     * @param func 变换函数
     * @return 变换后的 transform_view
     */
    template <Range R, typename Func>
    constexpr auto operator()(R&& range, Func func) const {
        return transform_view{all(_NEFORCE forward<R>(range)), _NEFORCE move(func)};
    }
};

/**
 * @brief 可管道使用的 transform 适配器对象
 */
NEFORCE_INLINE17 constexpr transform_adaptor transform;


/**
 * @brief take 适配器闭包，持有要保留的元素个数
 * @tparam DiffType 计数经适配器转换后的差值类型
 */
template <typename DiffType>
struct take_adaptor_closure : range_adaptor_closure<take_adaptor_closure<DiffType>> {
    DiffType count; ///< 保留的元素个数

    /**
     * @brief 构造闭包并保存计数
     * @param n 保留的元素个数
     */
    constexpr explicit take_adaptor_closure(DiffType n) :
    count(n) {}

    /**
     * @brief 取范围的前 count 个元素
     * @param range 源 Range
     * @return 前 count 个元素构成的 take_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using difference_type = iter_difference_t<decltype(view.begin())>;
        return take_view{_NEFORCE move(view), static_cast<difference_type>(count)};
    }
};

/**
 * @brief take 适配器，绑定计数得到闭包或直接截取范围前段
 */
struct take_adaptor {
    /**
     * @brief 绑定计数，得到可管道闭包
     * @tparam N 整型计数类型
     * @param count 保留的元素个数
     * @return take_adaptor_closure<N>
     */
    template <typename N>
        requires is_integral_v<N>
    constexpr auto operator()(N count) const {
        return take_adaptor_closure<N>{count};
    }

    /**
     * @brief 直接取范围的前 count 个元素
     * @tparam R 范围类型
     * @tparam N 整型计数类型
     * @param range 源 Range
     * @param count 保留的元素个数
     * @return 前 count 个元素构成的 take_view
     */
    template <Range R, typename N>
        requires is_integral_v<N>
    constexpr auto operator()(R&& range, N count) const {
        auto view = all(_NEFORCE forward<R>(range));
        using difference_type = iter_difference_t<decltype(view.begin())>;
        return take_view{_NEFORCE move(view), static_cast<difference_type>(count)};
    }
};

/**
 * @brief 可管道使用的 take 适配器对象
 */
NEFORCE_INLINE17 constexpr take_adaptor take;


/**
 * @brief take_while 适配器闭包，持有前缀判定谓词
 * @tparam Pred 一元谓词类型
 */
template <typename Pred>
struct take_while_adaptor_closure : range_adaptor_closure<take_while_adaptor_closure<Pred>> {
    Pred pred; ///< 判定前缀是否延续的谓词

    /**
     * @brief 构造闭包并接管谓词
     * @param p 谓词
     */
    constexpr explicit take_while_adaptor_closure(Pred p) :
    pred(_NEFORCE move(p)) {}

    /**
     * @brief 取满足谓词的最长前缀
     * @param range 源 Range
     * @return 满足谓词的最长前缀构成的 take_while_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return take_while_view{all(_NEFORCE forward<R>(range)), pred};
    }
};

/**
 * @brief take_while 适配器，绑定谓词得到闭包或直接截取前缀
 */
struct take_while_adaptor {
    /**
     * @brief 绑定谓词，得到可管道闭包
     * @tparam Pred 一元谓词类型
     * @param pred 谓词
     * @return take_while_adaptor_closure<Pred>
     */
    template <typename Pred>
    constexpr auto operator()(Pred pred) const {
        return take_while_adaptor_closure<Pred>{_NEFORCE move(pred)};
    }

    /**
     * @brief 直接取范围中满足谓词的最长前缀
     * @tparam R 范围类型
     * @tparam Pred 一元谓词类型
     * @param range 源 Range
     * @param pred 谓词
     * @return 前缀构成的 take_while_view
     */
    template <Range R, typename Pred>
    constexpr auto operator()(R&& range, Pred pred) const {
        return take_while_view{all(_NEFORCE forward<R>(range)), _NEFORCE move(pred)};
    }
};

/** @brief 可管道使用的 take_while 适配器对象 */
NEFORCE_INLINE17 constexpr take_while_adaptor take_while;


/**
 * @brief drop 适配器闭包，持有要丢弃的元素个数
 * @tparam DiffType 计数经适配器转换后的差值类型
 */
template <typename DiffType>
struct drop_adaptor_closure : range_adaptor_closure<drop_adaptor_closure<DiffType>> {
    DiffType count; ///< 丢弃的元素个数

    /**
     * @brief 构造闭包并保存计数
     * @param n 丢弃的元素个数
     */
    constexpr explicit drop_adaptor_closure(DiffType n) :
    count(n) {}

    /**
     * @brief 丢弃范围的前 count 个元素
     * @param range 源 Range
     * @return 跳过前 count 个元素后的 drop_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using difference_type = iter_difference_t<decltype(view.begin())>;
        return drop_view{_NEFORCE move(view), static_cast<difference_type>(count)};
    }
};

/**
 * @brief drop 适配器，绑定计数得到闭包或直接跳过范围前段
 */
struct drop_adaptor {
    /**
     * @brief 绑定计数，得到可管道闭包
     * @tparam N 整型计数类型
     * @param count 丢弃的元素个数
     * @return drop_adaptor_closure<N>
     */
    template <typename N>
        requires is_integral_v<N>
    constexpr auto operator()(N count) const {
        return drop_adaptor_closure<N>{count};
    }

    /**
     * @brief 直接丢弃范围的前 count 个元素
     * @tparam R 范围类型
     * @tparam N 整型计数类型
     * @param range 源 Range
     * @param count 丢弃的元素个数
     * @return 跳过前 count 个元素后的 drop_view
     */
    template <Range R, typename N>
        requires is_integral_v<N>
    constexpr auto operator()(R&& range, N count) const {
        auto view = all(_NEFORCE forward<R>(range));
        using difference_type = iter_difference_t<decltype(view.begin())>;
        return drop_view{_NEFORCE move(view), static_cast<difference_type>(count)};
    }
};

/** @brief 可管道使用的 drop 适配器对象 */
NEFORCE_INLINE17 constexpr drop_adaptor drop;


/**
 * @brief drop_while 适配器闭包，持有前缀判定谓词
 * @tparam Pred 一元谓词类型
 */
template <typename Pred>
struct drop_while_adaptor_closure : range_adaptor_closure<drop_while_adaptor_closure<Pred>> {
    Pred pred; ///< 判定前缀是否丢弃的谓词

    /**
     * @brief 构造闭包并接管谓词
     * @param p 谓词
     */
    constexpr explicit drop_while_adaptor_closure(Pred p) :
    pred(_NEFORCE move(p)) {}

    /**
     * @brief 丢弃满足谓词的最长前缀
     * @param range 源 Range
     * @return 前缀之后剩余元素构成的 drop_while_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return drop_while_view{all(_NEFORCE forward<R>(range)), pred};
    }
};

/**
 * @brief drop_while 适配器，绑定谓词得到闭包或直接跳过前缀
 */
struct drop_while_adaptor {
    /**
     * @brief 绑定谓词，得到可管道闭包
     * @tparam Pred 一元谓词类型
     * @param pred 谓词
     * @return drop_while_adaptor_closure<Pred>
     */
    template <typename Pred>
    constexpr auto operator()(Pred pred) const {
        return drop_while_adaptor_closure<Pred>{_NEFORCE move(pred)};
    }

    /**
     * @brief 直接丢弃范围中满足谓词的最长前缀
     * @tparam R 范围类型
     * @tparam Pred 一元谓词类型
     * @param range 源 Range
     * @param pred 谓词
     * @return 剩余元素构成的 drop_while_view
     */
    template <Range R, typename Pred>
    constexpr auto operator()(R&& range, Pred pred) const {
        return drop_while_view{all(_NEFORCE forward<R>(range)), _NEFORCE move(pred)};
    }
};

/** @brief 可管道使用的 drop_while 适配器对象 */
NEFORCE_INLINE17 constexpr drop_while_adaptor drop_while;


/**
 * @brief reverse 适配器闭包，无参数
 */
struct reverse_adaptor_closure : range_adaptor_closure<reverse_adaptor_closure> {
public:
    reverse_adaptor_closure() noexcept = default;

    /**
     * @brief 反向遍历范围
     * @param range 源 Range
     * @return 逆序结果的 reverse_view
     */
    template <Range R>
        requires bidirectional_iterator<decltype(_NEFORCE declval<R>().begin())> && common_range<R>
    constexpr auto operator()(R&& range) const {
        return reverse_view{all(_NEFORCE forward<R>(range))};
    }
};

/**
 * @brief reverse 适配器，产生闭包或直接反转范围
 */
struct reverse_adaptor {
    /**
     * @brief 得到可管道闭包
     * @return reverse_adaptor_closure
     */
    constexpr auto operator()() const { return reverse_adaptor_closure{}; }

    /**
     * @brief 直接反转范围
     * @tparam R 范围类型
     * @param range 源 Range
     * @return 逆序结果的 reverse_view
     */
    template <Range R>
        requires bidirectional_iterator<decltype(_NEFORCE declval<R>().begin())> && common_range<R>
    constexpr auto operator()(R&& range) const {
        return reverse_view{all(_NEFORCE forward<R>(range))};
    }
};

/** @brief 可管道使用的 reverse 适配器对象 */
NEFORCE_INLINE17 constexpr reverse_adaptor reverse;


/**
 * @brief iota 适配器，产生递增序列视图
 */
struct iota_adaptor {
    /**
     * @brief 从 start 开始的无限递增序列
     * @tparam T 序列值类型
     * @param start 起始值
     * @return 无界 iota_view
     */
    template <typename T>
    constexpr auto operator()(T start) const {
        return iota_view<T>{start};
    }

    /**
     * @brief 从 start 开始、以 bound 为上界的递增序列
     * @tparam T 序列值类型
     * @param start 起始值
     * @param bound 上界（不含）
     * @return 有界 iota_view
     */
    template <typename T, typename U>
        requires is_convertible_v<U, T>
    constexpr auto operator()(T start, U bound) const {
        return iota_view<T>{start, static_cast<T>(bound)};
    }
};

/** @brief 可管道使用的 iota 适配器对象 */
NEFORCE_INLINE17 constexpr iota_adaptor iota;


/**
 * @brief repeat 适配器，产生重复同一值的视图
 */
struct repeat_adaptor {
    /**
     * @brief 无限重复同一值
     * @tparam T 值类型
     * @param value 被重复的值
     * @return 无界 repeat_view
     */
    template <typename T>
    constexpr auto operator()(T value) const {
        return repeat_view<T>{_NEFORCE move(value)};
    }

    /**
     * @brief 重复同一值 count 次
     * @tparam T 值类型
     * @tparam N 整型计数类型
     * @param value 被重复的值
     * @param count 重复次数
     * @return 有界 repeat_view
     */
    template <typename T, typename N>
        requires is_integral_v<N>
    constexpr auto operator()(T value, N count) const {
        return repeat_view<T>{_NEFORCE move(value), static_cast<ptrdiff_t>(count)};
    }
};

/** @brief 可管道使用的 repeat 适配器对象 */
NEFORCE_INLINE17 constexpr repeat_adaptor repeat;


/**
 * @brief common 适配器闭包，无参数
 */
struct common_adaptor_closure : range_adaptor_closure<common_adaptor_closure> {
public:
    common_adaptor_closure() noexcept = default;

    /**
     * @brief 把范围统一为 common 范围
     * @param range 源 Range
     * @return 已是 common_range 时原样返回对应视图，否则返回 common_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        if constexpr (common_range<R>) {
            return all(_NEFORCE forward<R>(range));
        } else {
            return common_view{all(_NEFORCE forward<R>(range))};
        }
    }
};

/**
 * @brief common 适配器，产生闭包或直接把范围统一为 common 范围
 */
struct common_adaptor {
    /**
     * @brief 得到可管道闭包
     * @return common_adaptor_closure
     */
    constexpr auto operator()() const { return common_adaptor_closure{}; }

    /**
     * @brief 直接把范围统一为 common 范围
     * @tparam R 范围类型
     * @param range 源 Range
     * @return 已是 common_range 时原样返回对应视图，否则返回 common_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        if constexpr (common_range<R>) {
            return all(_NEFORCE forward<R>(range));
        } else {
            return common_view{all(_NEFORCE forward<R>(range))};
        }
    }
};

/** @brief 可管道使用的 common 适配器对象 */
NEFORCE_INLINE17 constexpr common_adaptor common;


/**
 * @brief join 适配器闭包，无参数
 */
struct join_adaptor_closure : range_adaptor_closure<join_adaptor_closure> {
public:
    /** @brief 默认构造空闭包 */
    join_adaptor_closure() noexcept = default;

    /**
     * @brief 展平嵌套范围
     * @param range 元素本身也是 Range 的范围
     * @return 展平后的 join_view
     */
    template <Range R>
        requires Range<iter_reference_t<decltype(_NEFORCE declval<R>().begin())>>
    constexpr auto operator()(R&& range) const {
        return join_view{all(_NEFORCE forward<R>(range)) | common()};
    }
};

/**
 * @brief join 适配器，产生闭包或直接展平嵌套范围
 */
struct join_adaptor {
    /**
     * @brief 得到可管道闭包
     * @return join_adaptor_closure
     */
    constexpr auto operator()() const { return join_adaptor_closure{}; }

    /**
     * @brief 直接展平嵌套范围
     * @tparam R 嵌套范围类型
     * @param range 元素本身也是 Range 的范围
     * @return 展平后的 join_view
     */
    template <Range R>
        requires Range<iter_reference_t<decltype(_NEFORCE declval<R>().begin())>>
    constexpr auto operator()(R&& range) const {
        return join_view{all(_NEFORCE forward<R>(range)) | common()};
    }
};

/** @brief 可管道使用的 join 适配器对象 */
NEFORCE_INLINE17 constexpr join_adaptor join;


/**
 * @brief elements 适配器闭包，取出每个元素的第 N 项
 * @tparam N 要提取的元素下标
 */
template <size_t N>
struct elements_adaptor_closure : range_adaptor_closure<elements_adaptor_closure<N>> {
public:
    elements_adaptor_closure() noexcept = default;

    /**
     * @brief 获取自身，便于继续以闭包形式组合
     * @return 本闭包的副本
     */
    constexpr auto operator()() const { return *this; }

    /**
     * @brief 取出范围内每个元组/键值对的第 N 项
     * @param range 支持 get<N> 类型范围的 Range
     * @return 第 N 项构成的 element_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return element_view<N, decltype(all(_NEFORCE forward<R>(range)))>{all(_NEFORCE forward<R>(range))};
    }
};

/**
 * @brief elements 适配器，产生闭包或直接提取第 N 项
 * @tparam N 要提取的元素下标
 */
template <size_t N>
struct elements_adaptor : range_adaptor_closure<elements_adaptor<N>> {
    /**
     * @brief 得到可管道闭包
     * @return elements_adaptor_closure<N>
     */
    constexpr auto operator()() const { return elements_adaptor_closure<N>{}; }

    /**
     * @brief 直接取出范围内每个元素的第 N 项
     * @tparam R 范围类型
     * @param range 支持 get<N> 类型范围的 Range
     * @return 第 N 项构成的 element_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return element_view<N, decltype(all(_NEFORCE forward<R>(range)))>{all(_NEFORCE forward<R>(range))};
    }
};

/** @brief 可管道使用的 elements 适配器对象模板 */
template <size_t N>
NEFORCE_INLINE17 constexpr elements_adaptor<N> elements;

/** @brief 取键的适配器对象，等价于 elements<0> */
NEFORCE_INLINE17 constexpr elements_adaptor_closure<0> keys{};
/** @brief 取值的适配器对象，等价于 elements<1> */
NEFORCE_INLINE17 constexpr elements_adaptor_closure<1> values{};


/**
 * @brief counted 适配器，由迭代器与计数构造视图
 */
struct counted_adaptor {
    /**
     * @brief 以迭代器与计数构造视图
     * @tparam Iter 迭代器类型
     * @tparam N 整型计数类型
     * @param iter 起始迭代器
     * @param count 元素个数
     * @return 含 count 个元素的 counted_view
     */
    template <typename Iter, typename N>
        requires input_iterator<Iter> && is_integral_v<N>
    constexpr auto operator()(Iter iter, N count) const {
        return counted_view{iter, static_cast<iter_difference_t<Iter>>(count)};
    }
};

/** @brief 可管道使用的 counted 适配器对象 */
NEFORCE_INLINE17 constexpr counted_adaptor counted;


/**
 * @brief concat 适配器闭包，持有第二个范围
 * @tparam V2 第二个范围的类型
 */
template <typename V2>
struct concat_adaptor_closure : range_adaptor_closure<concat_adaptor_closure<V2>> {
public:
    using view_type = decltype(all(_NEFORCE declval<V2>()) | common()); ///< 第二范围视图类型

private:
    view_type view_;

public:
    concat_adaptor_closure() noexcept = default;

    /**
     * @brief 构造闭包并接管第二个范围
     * @param v2 第二个范围
     */
    constexpr explicit concat_adaptor_closure(V2&& v2) :
    view_(all(_NEFORCE forward<V2>(v2)) | common()) {}

    /**
     * @brief 把所持范围拼接在参数范围之后
     * @tparam V 前置范围类型
     * @param v 前置范围
     * @return 先 v 后所持范围的 concat_view
     */
    template <typename V>
        requires Range<V> && copy_constructible<view_type>
    constexpr auto operator()(V&& v) const {
        return concat_view{all(_NEFORCE forward<V>(v)) | common(), view_};
    }
};

/**
 * @brief concat 适配器，绑定第二个范围得到闭包或直接拼接两个范围
 */
struct concat_adaptor {
    /**
     * @brief 绑定被拼接的后置范围，得到可管道闭包
     * @tparam V2 第二个范围的类型
     * @param v2 第二个范围
     * @return concat_adaptor_closure<V2>
     */
    template <typename V2>
        requires Range<V2>
    constexpr auto operator()(V2&& v2) const {
        return concat_adaptor_closure<V2>{_NEFORCE forward<V2>(v2)};
    }

    /**
     * @brief 直接顺序拼接两个范围
     * @tparam V1 第一个范围类型
     * @tparam V2 第二个范围类型
     * @param v1 前置范围
     * @param v2 后置范围
     * @return 先 v1 后 v2 的 concat_view
     */
    template <typename V1, typename V2>
    constexpr auto operator()(V1&& v1, V2&& v2) const {
        return concat_view{all(_NEFORCE forward<V1>(v1)) | common(), all(_NEFORCE forward<V2>(v2)) | common()};
    }
};

/** @brief 可管道使用的 concat 适配器对象 */
NEFORCE_INLINE17 constexpr concat_adaptor concat;


/**
 * @brief split 适配器闭包，持有分隔元素
 * @tparam T 分隔元素类型
 */
template <typename T>
struct split_adaptor_closure : range_adaptor_closure<split_adaptor_closure<T>> {
private:
    T delim_;

public:
    split_adaptor_closure() noexcept = default;

    /**
     * @brief 构造闭包并保存分隔元素
     * @param d 分隔元素
     */
    constexpr explicit split_adaptor_closure(T d) :
    delim_(d) {}

    /**
     * @brief 按分隔元素切分范围
     * @param range 源 Range
     * @return 各子范围构成的 split_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return split_view{all(_NEFORCE forward<R>(range)), delim_};
    }
};

/**
 * @brief split 适配器，绑定分隔元素得到闭包或直接切分范围
 */
struct split_adaptor {
    /**
     * @brief 绑定分隔元素，得到可管道闭包
     * @tparam T 分隔元素类型
     * @param delim 分隔元素
     * @return split_adaptor_closure<T>
     */
    template <typename T>
    constexpr auto operator()(T delim) const {
        return split_adaptor_closure<T>{delim};
    }

    /**
     * @brief 直接按分隔元素切分范围
     * @tparam R 范围类型
     * @tparam T 分隔元素类型
     * @param range 源 Range
     * @param delim 分隔元素
     * @return 各子范围构成的 split_view
     */
    template <Range R, typename T>
    constexpr auto operator()(R&& range, T delim) const {
        return split_view{all(_NEFORCE forward<R>(range)), delim};
    }
};

/** @brief 可管道使用的 split 适配器对象 */
NEFORCE_INLINE17 constexpr split_adaptor split;


/**
 * @brief slice 适配器闭包，持有下标区间
 */
struct slice_adaptor_closure : range_adaptor_closure<slice_adaptor_closure> {
private:
    ptrdiff_t start_;
    ptrdiff_t end_;

public:
    slice_adaptor_closure() noexcept = default;

    /**
     * @brief 构造闭包并保存下标区间
     * @param s 起始下标
     * @param e 结束下标（不含）
     */
    constexpr explicit slice_adaptor_closure(ptrdiff_t s, ptrdiff_t e) :
    start_(s),
    end_(e) {}

    /**
     * @brief 按下标区间截取范围
     * @param range 源 Range
     * @return [start_, end_) 区间内元素构成的 slice_view
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return slice_view{all(_NEFORCE forward<R>(range)), start_, end_};
    }
};

/**
 * @brief slice 适配器，绑定下标区间得到闭包或直接截取范围
 */
struct slice_adaptor {
    /**
     * @brief 绑定下标区间，得到可管道闭包
     * @param start 起始下标
     * @param end 结束下标（不含）
     * @return slice_adaptor_closure
     */
    constexpr auto operator()(ptrdiff_t start, ptrdiff_t end) const { return slice_adaptor_closure{start, end}; }

    /**
     * @brief 直接按下标区间截取范围
     * @tparam R 范围类型
     * @param range 源 Range
     * @param start 起始下标
     * @param end 结束下标（不含）
     * @return [start, end) 区间内元素构成的 slice_view
     */
    template <Range R>
    constexpr auto operator()(R&& range, ptrdiff_t start, ptrdiff_t end) const {
        return slice_view{all(_NEFORCE forward<R>(range)), start, end};
    }
};

/** @brief 可管道使用的 slice 适配器对象 */
NEFORCE_INLINE17 constexpr slice_adaptor slice;


/**
 * @struct distinct_adaptor_closure
 * @brief 去重适配器闭包
 *
 * 按元素自身去重，见 distinct_view。
 */
struct distinct_adaptor_closure : range_adaptor_closure<distinct_adaptor_closure> {
public:
    /**
     * @brief 默认构造函数
     */
    distinct_adaptor_closure() noexcept = default;

    /**
     * @brief 对范围去重
     * @tparam R 范围类型
     * @param range 源范围
     * @return 去重视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using value_type = iter_value_t<decltype(view.begin())>;
        return distinct_view<view_type, identity<value_type>, set<value_type>>{_NEFORCE move(view),
                                                                               identity<value_type>{}};
    }
};

/**
 * @struct distinct_adaptor
 * @brief 去重适配器
 */
struct distinct_adaptor {
    /**
     * @brief 获取去重闭包
     * @return 去重闭包
     */
    constexpr auto operator()() const { return distinct_adaptor_closure{}; }

    /**
     * @brief 对范围去重
     * @tparam R 范围类型
     * @param range 源范围
     * @return 去重视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return distinct_adaptor_closure{}(_NEFORCE forward<R>(range));
    }
};

NEFORCE_INLINE17 constexpr distinct_adaptor distinct;


/**
 * @struct distinct_by_adaptor_closure
 * @brief 按键去重适配器闭包
 * @tparam Key 键函数类型
 */
template <typename Key>
struct distinct_by_adaptor_closure : range_adaptor_closure<distinct_by_adaptor_closure<Key>> {
    Key key; ///< 键函数

    /**
     * @brief 构造按键去重闭包
     * @param k 键函数
     */
    constexpr explicit distinct_by_adaptor_closure(Key k) :
    key(_NEFORCE move(k)) {}

    /**
     * @brief 对范围按键去重
     * @tparam R 范围类型
     * @param range 源范围
     * @return 去重视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using reference = decltype(*view.begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return distinct_view<view_type, Key, set<key_type>>{_NEFORCE move(view), key};
    }
};

/**
 * @struct distinct_by_adaptor
 * @brief 按键去重适配器
 */
struct distinct_by_adaptor {
    /**
     * @brief 获取按键去重闭包
     * @tparam Key 键函数类型
     * @param k 键函数
     * @return 按键去重闭包
     */
    template <typename Key>
    constexpr auto operator()(Key k) const {
        return distinct_by_adaptor_closure<Key>{_NEFORCE move(k)};
    }
};

NEFORCE_INLINE17 constexpr distinct_by_adaptor distinct_by;


/**
 * @struct order_by_adaptor_closure
 * @brief 按键升序排序适配器闭包
 * @tparam Key 键函数类型
 */
template <typename Key>
struct order_by_adaptor_closure : range_adaptor_closure<order_by_adaptor_closure<Key>> {
    Key key; ///< 键函数

    /**
     * @brief 构造按键排序闭包
     * @param k 键函数
     */
    constexpr explicit order_by_adaptor_closure(Key k) :
    key(_NEFORCE move(k)) {}

    /**
     * @brief 对范围按键升序排序
     * @tparam R 范围类型
     * @param range 源范围
     * @return 排序视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using reference = decltype(*view.begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return order_by_view<view_type, Key, less<key_type>>{_NEFORCE move(view), key, less<key_type>{}};
    }
};

/**
 * @struct order_by_adaptor
 * @brief 按键升序排序适配器
 */
struct order_by_adaptor {
    /**
     * @brief 获取按键排序闭包
     * @tparam Key 键函数类型
     * @param k 键函数
     * @return 按键排序闭包
     */
    template <typename Key>
    constexpr auto operator()(Key k) const {
        return order_by_adaptor_closure<Key>{_NEFORCE move(k)};
    }
};

NEFORCE_INLINE17 constexpr order_by_adaptor order_by;


/**
 * @struct order_by_descending_adaptor_closure
 * @brief 按键降序排序适配器闭包
 * @tparam Key 键函数类型
 */
template <typename Key>
struct order_by_descending_adaptor_closure : range_adaptor_closure<order_by_descending_adaptor_closure<Key>> {
    Key key; ///< 键函数

    /**
     * @brief 构造按键降序排序闭包
     * @param k 键函数
     */
    constexpr explicit order_by_descending_adaptor_closure(Key k) :
    key(_NEFORCE move(k)) {}

    /**
     * @brief 对范围按键降序排序
     * @tparam R 范围类型
     * @param range 源范围
     * @return 排序视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using reference = decltype(*view.begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return order_by_view<view_type, Key, descending_order<less<key_type>>>{
                _NEFORCE move(view), key, descending_order<less<key_type>>{less<key_type>{}}};
    }
};

/**
 * @struct order_by_descending_adaptor
 * @brief 按键降序排序适配器
 */
struct order_by_descending_adaptor {
    /**
     * @brief 获取按键降序排序闭包
     * @tparam Key 键函数类型
     * @param k 键函数
     * @return 按键降序排序闭包
     */
    template <typename Key>
    constexpr auto operator()(Key k) const {
        return order_by_descending_adaptor_closure<Key>{_NEFORCE move(k)};
    }
};

NEFORCE_INLINE17 constexpr order_by_descending_adaptor order_by_descending;


/**
 * @struct then_by_adaptor_closure
 * @brief 追加升序次级键的适配器闭包
 * @tparam Key 次级键函数类型
 */
template <typename Key>
struct then_by_adaptor_closure : range_adaptor_closure<then_by_adaptor_closure<Key>> {
    Key key; ///< 次级键函数

    /**
     * @brief 构造追加次级键闭包
     * @param k 次级键函数
     */
    constexpr explicit then_by_adaptor_closure(Key k) :
    key(_NEFORCE move(k)) {}

    /**
     * @brief 为排序视图追加一级升序排序键
     * @tparam R 范围类型，必须是 order_by_view
     * @param range 排序视图
     * @return 复合排序视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        using view_type = remove_cvref_t<decltype(all(_NEFORCE forward<R>(range)))>;
        static_assert(is_order_by_view_v<view_type>, "views::then_by requires an order_by view");
        auto view = all(_NEFORCE forward<R>(range));
        using reference = decltype(*view.begin());
        using first_cmp_t = typename view_type::compare_type;
        using second_key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        using key_t = composite_key<typename view_type::key_function_type, Key>;
        using compare_t = pair_compare<first_cmp_t, less<second_key_type>>;
        auto composite = key_t{view.key_function(), key};
        return order_by_view<view_type, key_t, compare_t>{_NEFORCE move(view), _NEFORCE move(composite),
                                                          compare_t{first_cmp_t{}, less<second_key_type>{}}};
    }
};

/**
 * @struct then_by_adaptor
 * @brief 追加升序次级键的适配器
 */
struct then_by_adaptor {
    /**
     * @brief 获取追加次级键闭包
     * @tparam Key 次级键函数类型
     * @param k 次级键函数
     * @return 闭包
     */
    template <typename Key>
    constexpr auto operator()(Key k) const {
        return then_by_adaptor_closure<Key>{_NEFORCE move(k)};
    }
};

NEFORCE_INLINE17 constexpr then_by_adaptor then_by;


/**
 * @struct then_by_descending_adaptor_closure
 * @brief 追加降序次级键的适配器闭包
 * @tparam Key 次级键函数类型
 */
template <typename Key>
struct then_by_descending_adaptor_closure : range_adaptor_closure<then_by_descending_adaptor_closure<Key>> {
    Key key; ///< 次级键函数

    /**
     * @brief 构造追加降序次级键闭包
     * @param k 次级键函数
     */
    constexpr explicit then_by_descending_adaptor_closure(Key k) :
    key(_NEFORCE move(k)) {}

    /**
     * @brief 为排序视图追加一级降序排序键
     * @tparam R 范围类型，必须是 order_by_view
     * @param range 排序视图
     * @return 复合排序视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        using view_type = remove_cvref_t<decltype(all(_NEFORCE forward<R>(range)))>;
        static_assert(is_order_by_view_v<view_type>, "views::then_by_descending requires an order_by view");
        auto view = all(_NEFORCE forward<R>(range));
        using reference = decltype(*view.begin());
        using first_cmp_t = typename view_type::compare_type;
        using second_key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        using key_t = composite_key<typename view_type::key_function_type, Key>;
        using compare_t = pair_compare<first_cmp_t, descending_order<less<second_key_type>>>;
        auto composite = key_t{view.key_function(), key};
        return order_by_view<view_type, key_t, compare_t>{
                _NEFORCE move(view), _NEFORCE move(composite),
                compare_t{first_cmp_t{}, descending_order<less<second_key_type>>{less<second_key_type>{}}}};
    }
};

/**
 * @struct then_by_descending_adaptor
 * @brief 追加降序次级键的适配器
 */
struct then_by_descending_adaptor {
    /**
     * @brief 获取追加降序次级键闭包
     * @tparam Key 次级键函数类型
     * @param k 次级键函数
     * @return 闭包
     */
    template <typename Key>
    constexpr auto operator()(Key k) const {
        return then_by_descending_adaptor_closure<Key>{_NEFORCE move(k)};
    }
};

NEFORCE_INLINE17 constexpr then_by_descending_adaptor then_by_descending;


/**
 * @struct group_by_adaptor_closure
 * @brief 按键分组适配器闭包
 * @tparam Key 键函数类型
 */
template <typename Key>
struct group_by_adaptor_closure : range_adaptor_closure<group_by_adaptor_closure<Key>> {
    Key key; ///< 键函数

    /**
     * @brief 构造按键分组闭包
     * @param k 键函数
     */
    constexpr explicit group_by_adaptor_closure(Key k) :
    key(_NEFORCE move(k)) {}

    /**
     * @brief 对范围按键分组
     * @tparam R 范围类型
     * @param range 源范围
     * @return 分组视图，元素为 grouping
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using reference = decltype(*view.begin());
        using key_type = remove_cvref_t<invoke_result_t<Key, reference>>;
        return group_by_view<view_type, Key, less<key_type>>{_NEFORCE move(view), key, less<key_type>{}};
    }
};

/**
 * @struct group_by_self_adaptor_closure
 * @brief 按元素自身分组的适配器闭包
 */
struct group_by_self_adaptor_closure : range_adaptor_closure<group_by_self_adaptor_closure> {
public:
    /**
     * @brief 默认构造函数
     */
    group_by_self_adaptor_closure() noexcept = default;

    /**
     * @brief 按元素自身分组
     * @tparam R 范围类型
     * @param range 源范围
     * @return 分组视图，元素为 grouping
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using value_type = iter_value_t<decltype(view.begin())>;
        return group_by_view<view_type, identity<value_type>, less<value_type>>{
                _NEFORCE move(view), identity<value_type>{}, less<value_type>{}};
    }
};

/**
 * @struct group_by_adaptor
 * @brief 按键分组适配器
 */
struct group_by_adaptor {
    /**
     * @brief 获取按元素自身分组的闭包
     * @return 闭包
     */
    constexpr auto operator()() const { return group_by_self_adaptor_closure{}; }

    /**
     * @brief 获取按键分组闭包
     * @tparam Key 键函数类型
     * @param k 键函数
     * @return 按键分组闭包
     */
    template <typename Key>
    constexpr auto operator()(Key k) const {
        return group_by_adaptor_closure<Key>{_NEFORCE move(k)};
    }

    /**
     * @brief 按元素自身分组
     * @tparam R 范围类型
     * @param range 源范围
     * @return 分组视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using value_type = iter_value_t<decltype(view.begin())>;
        return group_by_view<view_type, identity<value_type>, less<value_type>>{
                _NEFORCE move(view), identity<value_type>{}, less<value_type>{}};
    }
};

NEFORCE_INLINE17 constexpr group_by_adaptor group_by;


/**
 * @struct chunk_adaptor_closure
 * @brief 分块适配器闭包
 * @tparam DiffType 计数类型
 */
template <typename DiffType>
struct chunk_adaptor_closure : range_adaptor_closure<chunk_adaptor_closure<DiffType>> {
    DiffType count; ///< 每块长度

    /**
     * @brief 构造分块闭包
     * @param n 每块长度
     */
    constexpr explicit chunk_adaptor_closure(DiffType n) :
    count(n) {}

    /**
     * @brief 把范围切分为定长块
     * @tparam R 范围类型
     * @param range 源范围
     * @return 分块视图，元素为子范围
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using difference_type = iter_difference_t<decltype(view.begin())>;
        return chunk_view<view_type>{_NEFORCE move(view), static_cast<difference_type>(count)};
    }
};

/**
 * @struct chunk_adaptor
 * @brief 分块适配器
 */
struct chunk_adaptor {
    /**
     * @brief 获取分块闭包
     * @tparam N 计数类型
     * @param n 每块长度
     * @return 分块闭包
     */
    template <typename N>
        requires is_integral_v<N>
    constexpr auto operator()(N n) const {
        return chunk_adaptor_closure<N>{n};
    }
};

NEFORCE_INLINE17 constexpr chunk_adaptor chunk;


/**
 * @struct sliding_adaptor_closure
 * @brief 滑动窗口适配器闭包
 * @tparam DiffType 计数类型
 */
template <typename DiffType>
struct sliding_adaptor_closure : range_adaptor_closure<sliding_adaptor_closure<DiffType>> {
    DiffType window; ///< 窗口宽度

    /**
     * @brief 构造滑动窗口闭包
     * @param w 窗口宽度
     */
    constexpr explicit sliding_adaptor_closure(DiffType w) :
    window(w) {}

    /**
     * @brief 产出范围的所有定长滑动窗口
     * @tparam R 范围类型
     * @param range 源范围
     * @return 滑动窗口视图，元素为子范围
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        using difference_type = iter_difference_t<decltype(view.begin())>;
        return sliding_view<view_type>{_NEFORCE move(view), static_cast<difference_type>(window)};
    }
};

/**
 * @struct sliding_adaptor
 * @brief 滑动窗口适配器
 */
struct sliding_adaptor {
    /**
     * @brief 获取滑动窗口闭包
     * @tparam N 计数类型
     * @param w 窗口宽度
     * @return 滑动窗口闭包
     */
    template <typename N>
        requires is_integral_v<N>
    constexpr auto operator()(N w) const {
        return sliding_adaptor_closure<N>{w};
    }
};

NEFORCE_INLINE17 constexpr sliding_adaptor sliding;


/**
 * @struct zip_adaptor_closure
 * @brief 配对适配器闭包
 * @tparam V2 第二个范围包装后的视图类型
 */
template <typename V2>
struct zip_adaptor_closure : range_adaptor_closure<zip_adaptor_closure<V2>> {
public:
    using view_type = V2; ///< 第二个范围的视图类型

private:
    view_type view_;

public:
    zip_adaptor_closure() noexcept = default;

    /**
     * @brief 构造配对闭包
     * @tparam R2 第二个范围类型
     * @param range 第二个范围
     */
    template <typename R2>
    constexpr explicit zip_adaptor_closure(R2&& range) :
    view_(all(_NEFORCE forward<R2>(range))) {}

    /**
     * @brief 把范围与闭包持有的范围逐元素配对
     * @tparam R 第一个范围类型
     * @param range 第一个范围
     * @return 配对视图，元素为 pair
     */
    template <Range R>
        requires copy_constructible<view_type>
    constexpr auto operator()(R&& range) const {
        auto first_view = all(_NEFORCE forward<R>(range));
        using first_view_type = remove_cvref_t<decltype(first_view)>;
        return zip_view<first_view_type, view_type>{_NEFORCE move(first_view), view_};
    }
};

/**
 * @struct zip_adaptor
 * @brief 配对适配器
 */
struct zip_adaptor {
    /**
     * @brief 获取配对闭包
     * @tparam R2 第二个范围类型
     * @param range 第二个范围
     * @return 配对闭包
     */
    template <typename R2>
        requires Range<R2>
    constexpr auto operator()(R2&& range) const {
        return zip_adaptor_closure<decltype(all(_NEFORCE forward<R2>(range)))>{_NEFORCE forward<R2>(range)};
    }

    /**
     * @brief 逐元素配对两个范围
     * @tparam R1 第一个范围类型
     * @tparam R2 第二个范围类型
     * @param range1 第一个范围
     * @param range2 第二个范围
     * @return 配对视图
     */
    template <typename R1, typename R2>
    constexpr auto operator()(R1&& range1, R2&& range2) const {
        auto first_view = all(_NEFORCE forward<R1>(range1));
        auto second_view = all(_NEFORCE forward<R2>(range2));
        using first_view_type = remove_cvref_t<decltype(first_view)>;
        using second_view_type = remove_cvref_t<decltype(second_view)>;
        return zip_view<first_view_type, second_view_type>{_NEFORCE move(first_view), _NEFORCE move(second_view)};
    }
};

NEFORCE_INLINE17 constexpr zip_adaptor zip;


/**
 * @struct enumerate_adaptor_closure
 * @brief 附带下标适配器闭包
 */
struct enumerate_adaptor_closure : range_adaptor_closure<enumerate_adaptor_closure> {
public:
    enumerate_adaptor_closure() noexcept = default;

    /**
     * @brief 为范围元素附带从 0 开始的连续下标
     * @tparam R 范围类型
     * @param range 源范围
     * @return 附带下标的视图，元素为 pair
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        return enumerate_view<view_type>{_NEFORCE move(view)};
    }
};

/**
 * @struct enumerate_adaptor
 * @brief 附带下标适配器
 */
struct enumerate_adaptor {
    /**
     * @brief 获取附带下标闭包
     * @return 闭包
     */
    constexpr auto operator()() const { return enumerate_adaptor_closure{}; }

    /**
     * @brief 为范围元素附带下标
     * @tparam R 范围类型
     * @param range 源范围
     * @return 附带下标的视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return enumerate_adaptor_closure{}(_NEFORCE forward<R>(range));
    }
};

NEFORCE_INLINE17 constexpr enumerate_adaptor enumerate;


/**
 * @struct scan_adaptor_closure
 * @brief 前缀累计适配器闭包
 * @tparam T 累加值类型
 * @tparam Op 累加函数类型
 */
template <typename T, typename Op>
struct scan_adaptor_closure : range_adaptor_closure<scan_adaptor_closure<T, Op>> {
    T init; ///< 初值
    Op op;  ///< 累加函数

    /**
     * @brief 构造前缀累计闭包
     * @param initial 初值
     * @param operation 累加函数
     */
    constexpr scan_adaptor_closure(T initial, Op operation) :
    init(_NEFORCE move(initial)),
    op(_NEFORCE move(operation)) {}

    /**
     * @brief 对范围做包含式前缀累计
     * @tparam R 范围类型
     * @param range 源范围
     * @return 前缀累计视图
     */
    template <Range R>
    constexpr auto operator()(R&& range) const {
        auto view = all(_NEFORCE forward<R>(range));
        using view_type = remove_cvref_t<decltype(view)>;
        return scan_view<view_type, T, Op>{_NEFORCE move(view), init, op};
    }
};

/**
 * @struct scan_adaptor
 * @brief 前缀累计适配器
 */
struct scan_adaptor {
    /**
     * @brief 获取前缀累计闭包
     * @tparam T 累加值类型
     * @tparam Op 累加函数类型
     * @param initial 初值
     * @param operation 累加函数
     * @return 闭包
     */
    template <typename T, typename Op>
    constexpr auto operator()(T initial, Op operation) const {
        return scan_adaptor_closure<T, Op>{_NEFORCE move(initial), _NEFORCE move(operation)};
    }
};

NEFORCE_INLINE17 constexpr scan_adaptor scan;

NEFORCE_END_RANGES_VIEWS__


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_VIEWS_HPP__
