#ifndef NEFORCE_CORE_ITERATOR_RANGES_REF_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_REF_VIEW_HPP__

/**
 * @file ref_view.hpp
 * @brief 引用视图、拥有视图与 all 视图工厂
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
 * @class ref_view
 * @brief 对左值范围的引用视图
 * @tparam R 被引用的范围类型
 * @warning 源范围的生存期必须长于本视图
 */
template <Range R>
    requires(!View<remove_cvref_t<R>>)
class ref_view : public view_base<ref_view<R>> {
public:
    /**
     * @brief 默认构造函数
     */
    ref_view() = default;

    /**
     * @brief 从范围构造
     * @param range 被引用的左值范围
     */
    constexpr explicit ref_view(R& range) noexcept :
    ptr_(_NEFORCE addressof(range)) {}

    /**
     * @brief 获取起始迭代器
     * @return 源范围的起始迭代器
     */
    constexpr auto begin() const { return ptr_->begin(); }
    /**
     * @brief 获取末尾 sentinel
     * @return 源范围的末尾 sentinel
     */
    constexpr auto end() const { return ptr_->end(); }
    /**
     * @brief 获取起始迭代器
     * @return 源范围的起始迭代器
     */
    constexpr auto begin() { return ptr_->begin(); }
    /**
     * @brief 获取末尾 sentinel
     * @return 源范围的末尾 sentinel
     */
    constexpr auto end() { return ptr_->end(); }

    /**
     * @brief 获取元素个数
     * @return 源范围的元素个数
     */
    constexpr auto size() const
        requires requires(const R& r) {
            { r.size() } -> convertible_to<size_t>;
        }
    {
        return static_cast<size_t>(ptr_->size());
    }

    /**
     * @brief 判断范围是否为空
     * @return 源范围是否为空
     */
    NEFORCE_NODISCARD constexpr bool empty() const
        requires requires(const R& r) { r.empty(); }
    {
        return ptr_->empty();
    }

    /**
     * @brief 获取底层数据指针
     * @return 源范围的 data()
     */
    constexpr auto data() const
        requires requires(const R& r) { r.data(); }
    {
        return ptr_->data();
    }

    /**
     * @brief 获取底层范围
     * @return 被引用的源范围的引用
     */
    constexpr R& base() const { return *ptr_; }

private:
    R* ptr_ = nullptr;
};


/**
 * @class owning_view
 * @brief 拥有右值范围的视图
 * @tparam R 被拥有的范围类型
 */
template <Range R>
class owning_view : public view_base<owning_view<R>> {
private:
    R obj_;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr owning_view()
        requires default_initializable<R>
    = default;

    /**
     * @brief 从右值范围构造
     * @param range 被移动进视图的范围
     */
    constexpr explicit owning_view(R&& range) noexcept(is_nothrow_move_constructible_v<R>) :
    obj_(_NEFORCE move(range)) {}

    /**
     * @brief 移动构造函数
     * @param other 被移动的视图
     */
    constexpr owning_view(owning_view&& other) noexcept(is_nothrow_move_constructible_v<R>) :
    obj_(_NEFORCE move(other.obj_)) {}

    /**
     * @brief 移动赋值操作符
     * @param other 被移动的视图
     * @return 自身引用
     */
    constexpr owning_view& operator=(owning_view&& other) noexcept(is_nothrow_move_assignable_v<R>) {
        obj_ = _NEFORCE move(other.obj_);
        return *this;
    }

    owning_view(const owning_view&) = delete;
    owning_view& operator=(const owning_view&) = delete;

    /**
     * @brief 获取起始迭代器
     * @return 底层范围的起始迭代器
     */
    constexpr auto begin() const { return obj_.begin(); }
    /**
     * @brief 获取末尾 sentinel
     * @return 底层范围的末尾 sentinel
     */
    constexpr auto end() const { return obj_.end(); }
    /**
     * @brief 获取起始迭代器
     * @return 底层范围的起始迭代器
     */
    constexpr auto begin() { return obj_.begin(); }
    /**
     * @brief 获取末尾 sentinel
     * @return 底层范围的末尾 sentinel
     */
    constexpr auto end() { return obj_.end(); }

    /**
     * @brief 获取元素个数
     * @return 底层范围的元素个数
     */
    constexpr auto size() const
        requires requires(const R& r) {
            { r.size() } -> convertible_to<size_t>;
        }
    {
        return static_cast<size_t>(obj_.size());
    }

    /**
     * @brief 判断范围是否为空
     * @return 底层范围是否为空
     */
    NEFORCE_NODISCARD constexpr bool empty() const
        requires requires(const R& r) { r.empty(); }
    {
        return obj_.empty();
    }

    /**
     * @brief 获取底层数据指针
     * @return 底层范围的 data()
     */
    constexpr auto data() const
        requires requires(const R& r) { r.data(); }
    {
        return obj_.data();
    }

    /**
     * @brief 获取底层范围
     * @return 底层范围的左值引用
     */
    constexpr R& base() & { return obj_; }
    /**
     * @brief 获取底层范围
     * @return 底层范围的 const 左值引用
     */
    constexpr const R& base() const& { return obj_; }
    /**
     * @brief 获取底层范围
     * @return 被移动的底层范围
     */
    constexpr R&& base() && { return _NEFORCE move(obj_); }
    /**
     * @brief 获取底层范围
     * @return 底层范围的 const 右值引用
     */
    constexpr const R&& base() const&& { return _NEFORCE move(obj_); }
};


/**
 * @struct all_view_factory
 * @brief all 视图工厂类型
 */
struct all_view_factory {
    /**
     * @brief 处理右值 view
     * @tparam V view 类型
     * @param v 被移动的 view
     * @return 移动构造的 view
     */
    template <typename V>
        requires View<remove_cvref_t<V>> && (!is_lvalue_reference_v<V>)
    constexpr auto operator()(V&& v) const noexcept(is_nothrow_move_constructible_v<remove_cvref_t<V>>) {
        return remove_cvref_t<V>(_NEFORCE forward<V>(v));
    }

    /**
     * @brief 处理左值 view
     * @tparam V view 类型
     * @param v 被拷贝的 view
     * @return view 的拷贝
     * @note 仅当 view 可拷贝构造时可用。
     */
    template <typename V>
        requires View<remove_cvref_t<V>> && is_lvalue_reference_v<V> && copy_constructible<remove_cvref_t<V>>
    constexpr auto operator()(V&& v) const noexcept(is_nothrow_copy_constructible_v<remove_cvref_t<V>>) {
        return remove_cvref_t<V>(v);
    }

    /**
     * @brief 处理左值 range
     * @tparam R 范围类型
     * @param r 被引用的左值范围
     * @return 包装 r 的 ref_view
     */
    template <typename R>
        requires Range<R> && (!View<remove_cvref_t<R>>)
    constexpr auto operator()(R& r) const noexcept {
        return ref_view<R>{r};
    }

    /**
     * @brief 处理右值 range
     * @tparam R 范围类型
     * @param r 被移动的右值范围
     * @return 拥有该范围的 owning_view
     */
    template <typename R>
        requires Range<R> && (!View<remove_cvref_t<R>>) && (!is_lvalue_reference_v<R>) && move_constructible<R>
    constexpr auto operator()(R&& r) const noexcept(is_nothrow_move_constructible_v<R>) {
        return owning_view<R>{_NEFORCE move(r)};
    }
};

/**
 * @brief 全局 all 视图工厂对象，用于把任意 range 统一转换为 view
 */
NEFORCE_INLINE17 constexpr all_view_factory all{};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_REF_VIEW_HPP__
