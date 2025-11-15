#ifndef MSTL_RANGES_HPP__
#define MSTL_RANGES_HPP__
#include "algobase.hpp"
MSTL_BEGIN_NAMESPACE__
#ifdef MSTL_STANDARD_20__

MSTL_BEGIN_RANGES__

template <typename R>
concept Range = requires(R& r) {
    { r.begin() } -> input_or_output_iterator;
    { r.end() } -> sentinel_for<decltype(r.begin())>;
};

template <typename R>
concept common_range = Range<R> && same_as<
    decltype(_MSTL declval<R>().begin()),
    decltype(_MSTL declval<R>().end())
>;

template <typename V>
concept View = Range<V> && move_constructible<V>;

template <typename F, typename T>
concept Predicate = predicate<F, T>;

template <typename F, typename T>
concept UnaryFunction = invocable<F, T>;


template <Range R, typename Adaptor>
constexpr auto operator |(R&& range, Adaptor&& adaptor) {
    return _MSTL forward<Adaptor>(adaptor)(_MSTL forward<R>(range));
}


template <Range R> requires(!View<remove_cvref_t<R>>)
class ref_view : public view_base<ref_view<R>> {
public:
    ref_view() = default;

    constexpr explicit ref_view(R& range) noexcept
    : ptr_(_MSTL addressof(range)) {}

    constexpr auto begin() const { return ptr_->begin(); }
    constexpr auto end() const { return ptr_->end(); }
    constexpr auto begin() { return ptr_->begin(); }
    constexpr auto end() { return ptr_->end(); }

    constexpr R& base() const { return *ptr_; }

private:
    R* ptr_ = nullptr;
};


template <Range R>
class owning_view : public view_base<owning_view<R>> {
private:
    R obj_;

public:
    owning_view() = default;

    constexpr explicit owning_view(R&& range)
    noexcept(is_nothrow_move_constructible_v<R>)
    : obj_(_MSTL move(range)) {}

    owning_view(owning_view&&) = delete;
    owning_view& operator =(owning_view&&) = delete;

    owning_view(const owning_view&) = delete;
    owning_view& operator= (const owning_view&) = delete;

    constexpr auto begin() const { return obj_.begin(); }
    constexpr auto end() const { return obj_.end(); }
    constexpr auto begin() { return obj_.begin(); }
    constexpr auto end() { return obj_.end(); }

    constexpr R& base() & { return obj_; }
    constexpr const R& base() const & { return obj_; }
    constexpr R&& base() && { return _MSTL move(obj_); }
    constexpr const R&& base() const && { return _MSTL move(obj_); }
};


struct all_view_factory {
    template <typename V> requires View<remove_cvref_t<V>>
    constexpr auto operator()(V&& v) const noexcept {
        return _MSTL forward<V>(v);
    }

    template <typename R>
        requires Range<R> && (!View<remove_cvref_t<R>>)
    constexpr auto operator()(R& r) const noexcept {
        return ref_view<remove_cvref_t<R>>{r};
    }

    template <typename R>
        requires Range<R> && (!View<remove_cvref_t<R>>)
    constexpr auto operator()(R&& r) const
    noexcept(is_nothrow_move_constructible_v<remove_cvref_t<R>>) {
        return owning_view<remove_cvref_t<R>>{_MSTL move(r)};
    }
};

MSTL_INLINE17 constexpr all_view_factory all{};


template <input_iterator BaseIter, sentinel_for<BaseIter> Sentinel, typename Pred>
class filter_iterator {
public:
    using base_category = iter_category_t<BaseIter>;
    using iterator_category = conditional_t<
        is_base_of_v<bidirectional_iterator_tag, base_category>,
        bidirectional_iterator_tag,
        forward_iterator_tag
    >;

    using value_type = iter_value_t<BaseIter>;
    using difference_type = iter_difference_t<BaseIter>;
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>;
    using reference = iter_reference_t<BaseIter>;

private:
    BaseIter base_begin_{};
    BaseIter current_{};
    Sentinel end_{};
    const Pred* pred_ = nullptr;

    constexpr void satisfy_predicate_forward() {
        while (current_ != end_ && !(*pred_)(*current_)) {
            ++current_;
        }
    }

    constexpr void satisfy_predicate_backward() {
        while (current_ != base_begin_) {
            if ((*pred_)(*current_)) {
                break;
            }
            --current_;
        }
    }

public:
    constexpr filter_iterator() = default;

    constexpr filter_iterator(BaseIter base_begin, BaseIter current, Sentinel end, const Pred* pred)
    : base_begin_(base_begin), current_(current), end_(end), pred_(pred) {
        satisfy_predicate_forward();
    }

    constexpr reference operator*() const { return *current_; }
    constexpr pointer operator->() const { return _MSTL addressof(*current_); }

    constexpr filter_iterator& operator++() {
        ++current_;
        satisfy_predicate_forward();
        return *this;
    }

    constexpr filter_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr filter_iterator& operator--()
        requires bidirectional_iterator<BaseIter> {
        do {
            --current_;
        } while (current_ != base_begin_ && !(*pred_)(*current_));
        return *this;
    }

    constexpr filter_iterator operator--(int)
        requires bidirectional_iterator<BaseIter> {
        auto tmp = *this;
        --(*this);
        return tmp;
    }

    constexpr bool operator==(const filter_iterator& other) const {
        return current_ == other.current_;
    }
    constexpr bool operator!=(const filter_iterator& other) const {
        return !(*this == other);
    }

    template <typename S>
    requires (!same_as<S, filter_iterator>)
    constexpr bool operator ==(const S& s) const {
        return current_ == s;
    }

    template <typename S>
    requires (!same_as<S, filter_iterator>)
    constexpr bool operator !=(const S& s) const {
        return current_ != s;
    }

    friend constexpr bool operator==(Sentinel s, const filter_iterator& it) {
        return it.current_ == s;
    }
    friend constexpr bool operator!=(Sentinel s, const filter_iterator& it) {
        return it.current_ != s;
    }

    constexpr bool operator<(const filter_iterator& other) const
        requires totally_ordered<BaseIter> {
        return current_ < other.current_;
    }
    constexpr bool operator>(const filter_iterator& other) const
        requires totally_ordered<BaseIter> {
        return current_ > other.current_;
    }
    constexpr bool operator<=(const filter_iterator& other) const
        requires totally_ordered<BaseIter> {
        return current_ <= other.current_;
    }
    constexpr bool operator>=(const filter_iterator& other) const
        requires totally_ordered<BaseIter> {
        return current_ >= other.current_;
    }

    constexpr bool operator<(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel> {
        return current_ < s;
    }
    constexpr bool operator>(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel> {
        return current_ > s;
    }
    constexpr bool operator<=(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel> {
        return current_ <= s;
    }
    constexpr bool operator>=(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel> {
        return current_ >= s;
    }

    friend constexpr bool operator<(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel> {
        return s < it.current_;
    }
    friend constexpr bool operator>(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel> {
        return s > it.current_;
    }
    friend constexpr bool operator<=(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel> {
        return s <= it.current_;
    }
    friend constexpr bool operator>=(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel> {
        return s >= it.current_;
    }

    constexpr difference_type operator -(const filter_iterator& other) const noexcept
        requires sized_sentinel_for<BaseIter, BaseIter> {
        return static_cast<difference_type>(current_ - other.current_);
    }

    constexpr BaseIter base() const { return current_; }
};


template <View V, typename Pred>
class filter_view : public view_base<filter_view<V, Pred>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using base_sentinel = decltype(_MSTL declval<V>().end());
    using iterator = filter_iterator<base_iterator, base_sentinel, Pred>;
    using sentinel = base_sentinel;

private:
    V base_;
    Pred pred_;

public:
    constexpr filter_view() = default;

    constexpr filter_view(const filter_view& other)
    requires copy_constructible<V> && copy_constructible<Pred>
    : base_(other.base_), pred_(other.pred_) {}

    constexpr filter_view& operator =(const filter_view& other)
    requires copyable<V> && copyable<Pred> {
        if (this != &other) {
            base_ = other.base_;
            pred_ = other.pred_;
        }
        return *this;
    }

    constexpr filter_view(filter_view&&) = default;

    constexpr filter_view& operator =(filter_view&& other)
    noexcept(is_nothrow_move_assignable_v<V> && is_nothrow_move_assignable_v<Pred>)
    requires is_move_assignable_v<V> && is_move_assignable_v<Pred> {
        base_ = _MSTL move(other.base_);
        pred_ = _MSTL move(other.pred_);
        return *this;
    }

    constexpr filter_view(V base, Pred pred)
    : base_(_MSTL move(base)), pred_(_MSTL move(pred)) {}

    constexpr iterator begin() {
        return iterator(base_.begin(), base_.begin(), base_.end(), &pred_);
    }
    constexpr sentinel end() {
        return base_.end();
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.begin(), base_.begin(), base_.end(), &pred_);
    }
    constexpr sentinel end() const
    requires Range<const V> {
        return base_.end();
    }

    constexpr iterator end() requires common_range<V> {
        return iterator(base_.begin(), base_.end(), base_.end(), &pred_);
    }
    constexpr iterator end() const requires common_range<const V> {
        return iterator(base_.begin(), base_.end(), base_.end(), &pred_);
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }
};


template <input_iterator BaseIter, typename Func>
class transform_iterator {
public:
    using iterator_category = forward_iterator_tag;
    using base_reference = iter_reference_t<BaseIter>;
    using value_type = remove_cvref_t<invoke_result_t<Func, base_reference>>;
    using difference_type = iter_difference_t<BaseIter>;
    using reference = invoke_result_t<Func, base_reference>;
    using pointer = void;

    constexpr transform_iterator() = default;

    constexpr transform_iterator(BaseIter current, const Func* func)
    : current_(current), func_(func) {}

    constexpr reference operator *() const { return (*func_)(*current_); }

    constexpr transform_iterator& operator++() {
        ++current_;
        return *this;
    }

    constexpr transform_iterator operator++(int) {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    constexpr bool operator==(const transform_iterator& other) const {
        return current_ == other.current_;
    }
    constexpr bool operator!=(const transform_iterator& other) const {
        return !(*this == other);
    }

    template <typename S>
    requires (!same_as<S, transform_iterator>) && sentinel_for<S, BaseIter>
    constexpr bool operator==(const S& s) const {
        return current_ == s;
    }
    template <typename S>
    requires (!same_as<S, transform_iterator>) && sentinel_for<S, BaseIter>
    constexpr bool operator!=(const S& s) const {
        return !(current_ == s);
    }

    template <typename Sentinel>
    friend constexpr bool operator==(const Sentinel& s, const transform_iterator& it) {
        return s == it.current_;
    }
    template <typename Sentinel>
    friend constexpr bool operator!=(const Sentinel& s, const transform_iterator& it) {
        return !(s == it.current_);
    }

private:
    BaseIter current_{};
    const Func* func_ = nullptr;
};


template <View V, typename Func>
class transform_view : public view_base<transform_view<V, Func>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using iterator = transform_iterator<base_iterator, Func>;
    using sentinel = decltype(_MSTL declval<V>().end());

private:
    V base_;
    Func func_;

public:
    constexpr transform_view() = default;

    constexpr transform_view(V base, Func func)
    : base_(_MSTL move(base)), func_(_MSTL move(func)) {}

    constexpr transform_view(transform_view&&)
    requires movable<V> && movable<Func> = default;

    constexpr transform_view& operator =(transform_view&& other) noexcept
    requires movable<V> && movable<Func> {
        base_ = _MSTL move(other.base_);
        func_ = _MSTL move(other.func_);
        return *this;
    }

    constexpr transform_view(const transform_view& other)
    requires copy_constructible<V> && copy_constructible<Func>
    : base_(other.base_), func_(other.func_) {}

    constexpr transform_view& operator =(const transform_view& other)
    requires copyable<V> && copyable<Func> {
        if (this != &other) {
            base_ = other.base_;
            func_ = other.func_;
        }
        return *this;
    }

    constexpr iterator begin() {
        return iterator(base_.begin(), &func_);
    }

    constexpr sentinel end() {
        return base_.end();
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.begin(), &func_);
    }

    constexpr sentinel end() const
    requires Range<const V> {
        return base_.end();
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }
};


template <input_iterator BaseIter>
class take_iterator {
public:
    using iterator_category = forward_iterator_tag;
    using value_type = iter_value_t<BaseIter>;
    using difference_type = iter_difference_t<BaseIter>;
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>;
    using reference = iter_reference_t<BaseIter>;

    constexpr take_iterator() = default;

    constexpr take_iterator(BaseIter current, difference_type count)
    : current_(current), count_(count) {}

    constexpr reference operator*() const {
        return *current_;
    }

    constexpr pointer operator->() const {
        return _MSTL addressof(*current_);
    }

    constexpr take_iterator& operator++() {
        ++current_;
        --count_;
        return *this;
    }

    constexpr take_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr bool operator==(const take_iterator& other) const {
        return current_ == other.current_ || count_ == other.count_;
    }

    constexpr bool operator!=(const take_iterator& other) const {
        return !(*this == other);
    }

    template <typename S>
    requires (!same_as<S, take_iterator>)
    constexpr bool operator==(const S& s) const {
        return current_ == s || count_ == 0;
    }

    template <typename S>
    requires (!same_as<S, take_iterator>)
    constexpr bool operator!=(const S& s) const {
        return !(*this == s);
    }

private:
    BaseIter current_{};
    difference_type count_ = 0;
};


template <View V>
class take_view : public view_base<take_view<V>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using iterator = take_iterator<base_iterator>;
    using difference_type = iter_difference_t<base_iterator>;

private:
    V base_;
    difference_type count_ = 0;

public:
    constexpr take_view() = default;

    constexpr take_view(V base, difference_type count)
    : base_(_MSTL move(base)), count_(count) {}

    constexpr take_view(take_view&&) requires movable<V> = default;
    constexpr take_view& operator =(take_view&&) requires movable<V> = default;

    constexpr take_view(const take_view&) requires copy_constructible<V> = default;
    constexpr take_view& operator =(const take_view&) requires copyable<V> = default;

    constexpr iterator begin() {
        return iterator(base_.begin(), count_);
    }

    constexpr iterator end() {
        auto it = base_.begin();
        _MSTL advance(it, _MSTL min(count_, _MSTL distance(base_.begin(), base_.end())));
        return iterator(it, 0);
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.begin(), count_);
    }

    constexpr iterator end() const
    requires Range<const V> {
        auto it = base_.begin();
        _MSTL advance(it, _MSTL min(count_, _MSTL distance(base_.begin(), base_.end())));
        return iterator(it, 0);
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }
};

template <typename V>
take_view(V&&, iter_difference_t<decltype(_MSTL declval<V>().begin())>) -> take_view<V>;


template <input_iterator BaseIter, sentinel_for<BaseIter> Sentinel, typename Pred>
class take_while_iterator {
public:
    using iterator_category = forward_iterator_tag;
    using value_type = iter_value_t<BaseIter>;
    using difference_type = iter_difference_t<BaseIter>;
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>;
    using reference = iter_reference_t<BaseIter>;

    constexpr take_while_iterator() = default;

    constexpr take_while_iterator(BaseIter current, Sentinel end, const Pred* pred)
        : current_(current), end_(end), pred_(pred), done_(current == end || !(*pred)(*current)) {}

    constexpr reference operator*() const {
        return *current_;
    }

    constexpr pointer operator->() const {
        return _MSTL addressof(*current_);
    }

    constexpr take_while_iterator& operator++() {
        ++current_;
        if (current_ == end_ || !(*pred_)(*current_)) done_ = true;
        return *this;
    }

    constexpr take_while_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr bool operator==(const take_while_iterator& other) const {
        return current_ == other.current_ && done_ == other.done_;
    }
    constexpr bool operator!=(const take_while_iterator& other) const {
        return !(*this == other);
    }

    template <typename S>
    requires (!same_as<S, take_while_iterator>)
    constexpr bool operator==(const S& s) const {
        return done_ || current_ == s;
    }
    template <typename S>
    requires (!same_as<S, take_while_iterator>)
    constexpr bool operator!=(const S& s) const {
        return !(*this == s);
    }

private:
    BaseIter current_{};
    Sentinel end_{};
    const Pred* pred_ = nullptr;
    bool done_ = false;
};

template <View V, typename Pred>
class take_while_view : public view_base<take_while_view<V, Pred>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using base_sentinel = decltype(_MSTL declval<V>().end());
    using iterator = take_while_iterator<base_iterator, base_sentinel, Pred>;
    using sentinel = base_sentinel;

private:
    V base_;
    Pred pred_;

public:
    constexpr take_while_view() = default;

    constexpr take_while_view(V base, Pred pred)
        : base_(_MSTL move(base)), pred_(_MSTL move(pred)) {}

    constexpr iterator begin() {
        return iterator(base_.begin(), base_.end(), &pred_);
    }
    constexpr sentinel end() {
        return base_.end();
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.begin(), base_.end(), &pred_);
    }
    constexpr sentinel end() const
    requires Range<const V> {
        return base_.end();
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }
};

template <typename V, typename Pred>
take_while_view(V&&, Pred) -> take_while_view<V, Pred>;


template <View V>
class drop_view : public view_base<drop_view<V>> {
public:
    using iterator = decltype(_MSTL declval<V>().begin());
    using sentinel = decltype(_MSTL declval<V>().end());
    using difference_type = iter_difference_t<iterator>;

private:
    V base_;
    difference_type count_ = 0;

public:
    constexpr drop_view() = default;

    constexpr drop_view(V base, difference_type count)
    : base_(_MSTL move(base)), count_(count) {}

    constexpr drop_view(drop_view&&) requires movable<V> = default;
    constexpr drop_view& operator =(drop_view&&) requires movable<V> = default;

    constexpr drop_view(const drop_view&) requires copy_constructible<V> = default;
    constexpr drop_view& operator =(const drop_view&) requires copyable<V> = default;

    constexpr iterator begin() {
        auto it = base_.begin();
        auto end = base_.end();
        auto n = count_;
        while (n > 0 && it != end) {
            ++it;
            --n;
        }
        return it;
    }

    constexpr sentinel end() {
        return base_.end();
    }

    constexpr iterator begin() const
    requires Range<const V> {
        auto it = base_.begin();
        auto end = base_.end();
        auto n = count_;
        while (n > 0 && it != end) {
            ++it;
            --n;
        }
        return it;
    }

    constexpr sentinel end() const
    requires Range<const V> {
        return base_.end();
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }
};

template <typename V>
drop_view(V&&, iter_difference_t<decltype(_MSTL declval<V>().begin())>) -> drop_view<V>;


template <input_iterator BaseIter, sentinel_for<BaseIter> Sentinel, typename Pred>
class drop_while_iterator {
public:
    using iterator_category = forward_iterator_tag;
    using value_type = iter_value_t<BaseIter>;
    using difference_type = iter_difference_t<BaseIter>;
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>;
    using reference = iter_reference_t<BaseIter>;

    constexpr drop_while_iterator() = default;

    constexpr drop_while_iterator(BaseIter current, Sentinel end, const Pred* pred)
        : current_(current), end_(end), pred_(pred), started_(false) {
        satisfy_predicate_forward();
    }

    constexpr reference operator*() const {
        return *current_;
    }

    constexpr pointer operator->() const {
        return _MSTL addressof(*current_);
    }

    constexpr drop_while_iterator& operator++() {
        ++current_;
        return *this;
    }

    constexpr drop_while_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr bool operator==(const drop_while_iterator& other) const {
        return current_ == other.current_;
    }
    constexpr bool operator!=(const drop_while_iterator& other) const {
        return !(*this == other);
    }

    template <typename S>
    requires (!same_as<S, drop_while_iterator>)
    constexpr bool operator==(const S& s) const {
        return current_ == s;
    }
    template <typename S>
    requires (!same_as<S, drop_while_iterator>)
    constexpr bool operator!=(const S& s) const {
        return !(*this == s);
    }

private:
    constexpr void satisfy_predicate_forward() {
        if (started_) return;
        started_ = true;
        while (current_ != end_ && (*pred_)(*current_)) {
            ++current_;
        }
    }

    BaseIter current_{};
    Sentinel end_{};
    const Pred* pred_ = nullptr;
    bool started_ = false;
};

template <View V, typename Pred>
class drop_while_view : public view_base<drop_while_view<V, Pred>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using base_sentinel = decltype(_MSTL declval<V>().end());
    using iterator = drop_while_iterator<base_iterator, base_sentinel, Pred>;
    using sentinel = base_sentinel;

private:
    V base_;
    Pred pred_;

public:
    constexpr drop_while_view() = default;

    constexpr drop_while_view(V base, Pred pred)
        : base_(_MSTL move(base)), pred_(_MSTL move(pred)) {}

    constexpr iterator begin() {
        return iterator(base_.begin(), base_.end(), &pred_);
    }
    constexpr sentinel end() {
        return base_.end();
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.begin(), base_.end(), &pred_);
    }
    constexpr sentinel end() const
    requires Range<const V> {
        return base_.end();
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }
};

template <typename V, typename Pred>
drop_while_view(V&&, Pred) -> drop_while_view<V, Pred>;


template <bidirectional_iterator Iter>
class reverse_iterator {
public:
    using iterator_category = bidirectional_iterator_tag;
    using value_type = iter_value_t<Iter>;
    using difference_type = iter_difference_t<Iter>;
    using pointer = add_pointer_t<iter_reference_t<Iter>>;
    using reference = iter_reference_t<Iter>;

    constexpr reverse_iterator() = default;
    constexpr explicit reverse_iterator(Iter current) : current_(current) {}

    constexpr reference operator*() const {
        auto tmp = current_;
        return *--tmp;
    }

    constexpr pointer operator->() const {
        auto tmp = current_;
        return _MSTL addressof(*--tmp);
    }

    constexpr reverse_iterator& operator++() {
        --current_;
        return *this;
    }

    constexpr reverse_iterator operator++(int) {
        auto tmp = *this;
        --current_;
        return tmp;
    }

    constexpr reverse_iterator& operator--() {
        ++current_;
        return *this;
    }

    constexpr reverse_iterator operator--(int) {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    constexpr bool operator==(const reverse_iterator& other) const {
        return current_ == other.current_;
    }

    constexpr bool operator!=(const reverse_iterator& other) const {
        return !(*this == other);
    }

private:
    Iter current_{};
};

template <View V>
    requires bidirectional_iterator<decltype(_MSTL declval<V>().begin())> && common_range<V>
class reverse_view : public view_base<reverse_view<V>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using iterator = reverse_iterator<base_iterator>;

private:
    V base_;

public:
    constexpr reverse_view() = default;
    constexpr explicit reverse_view(V base) : base_(_MSTL move(base)) {}

    constexpr reverse_view(reverse_view&&) requires movable<V> = default;
    constexpr reverse_view& operator=(reverse_view&&) requires movable<V> = default;

    constexpr reverse_view(const reverse_view&) requires copy_constructible<V> = default;
    constexpr reverse_view& operator=(const reverse_view&) requires copyable<V> = default;

    constexpr iterator begin() {
        return iterator(base_.end());
    }
    constexpr iterator end() {
        return iterator(base_.begin());
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.end());
    }
    constexpr iterator end() const
    requires Range<const V> {
        return iterator(base_.begin());
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }
};

template <typename V>
reverse_view(V&&) -> reverse_view<V>;


template <typename T>
class iota_iterator {
public:
    using iterator_category = forward_iterator_tag;
    using value_type = T;
    using difference_type = conditional_t<
        is_integral_v<T>,
        conditional_t<sizeof(T) < sizeof(int), int, T>,
        ptrdiff_t
    >;
    using pointer = const T*;
    using reference = T;

    constexpr iota_iterator() = default;
    constexpr explicit iota_iterator(T value) : value_(value) {}

    constexpr reference operator*() const { return value_; }
    constexpr pointer operator->() const { return &value_; }

    constexpr iota_iterator& operator++() {
        ++value_;
        return *this;
    }

    constexpr iota_iterator operator++(int) {
        auto tmp = *this;
        ++value_;
        return tmp;
    }

    constexpr bool operator==(const iota_iterator& other) const {
        return value_ == other.value_;
    }
    constexpr bool operator!=(const iota_iterator& other) const {
        return value_ != other.value_;
    }

    constexpr bool operator<(const iota_iterator& other) const
    requires totally_ordered<T> {
        return value_ < other.value_;
    }

private:
    T value_{};
};

template <typename T>
class iota_view : public view_base<iota_view<T>> {
public:
    using iterator = iota_iterator<T>;

    constexpr iota_view() = default;
    constexpr explicit iota_view(T start) : start_(start), has_bound_(false) {}
    constexpr iota_view(T start, T bound) : start_(start), bound_(bound), has_bound_(true) {}

    constexpr iterator begin() const { return iterator(start_); }
    constexpr iterator end() const {
        return has_bound_ ? iterator(bound_) : iterator(start_);
    }

private:
    T start_{};
    T bound_{};
    bool has_bound_ = false;
};

template <typename T>
iota_view(T) -> iota_view<T>;

template <typename T>
iota_view(T, T) -> iota_view<T>;


template <typename T>
class repeat_iterator {
public:
    using iterator_category = forward_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    constexpr repeat_iterator() = default;
    constexpr repeat_iterator(const T* value, difference_type count)
    : value_(value), count_(count) {}

    constexpr reference operator*() const { return *value_; }
    constexpr pointer operator->() const { return value_; }

    constexpr repeat_iterator& operator++() {
        if (count_ > 0) --count_;
        return *this;
    }

    constexpr repeat_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr bool operator==(const repeat_iterator& other) const {
        return count_ == other.count_;
    }
    constexpr bool operator!=(const repeat_iterator& other) const {
        return count_ != other.count_;
    }

private:
    const T* value_ = nullptr;
    difference_type count_ = 0;
};

template <typename T>
class repeat_view : public view_base<repeat_view<T>> {
public:
    using iterator = repeat_iterator<T>;
    using difference_type = ptrdiff_t;

    constexpr repeat_view() = default;

    constexpr explicit repeat_view(T value)
    : value_(_MSTL move(value)), has_bound_(false) {}

    constexpr repeat_view(T value, const difference_type count)
    : value_(_MSTL move(value)), count_(count), has_bound_(true) {}

    constexpr iterator begin() const {
        return has_bound_ ? iterator(&value_, count_) : iterator(&value_, -1);
    }
    constexpr iterator end() const {
        return iterator(&value_, 0);
    }

private:
    T value_{};
    difference_type count_ = 0;
    bool has_bound_ = false;
};

template <typename T>
repeat_view(T) -> repeat_view<T>;

template <typename T, typename N>
repeat_view(T, N) -> repeat_view<T>;


template <typename OuterIter, typename OuterSentinel>
class join_iterator {
private:
    using outer_reference = iter_reference_t<OuterIter>;
    using inner_range = remove_cvref_t<outer_reference>;
    using inner_iterator = decltype(_MSTL declval<inner_range>().begin());
    using inner_sentinel = decltype(_MSTL declval<inner_range>().end());

public:
    using iterator_category = forward_iterator_tag;
    using value_type = iter_value_t<inner_iterator>;
    using difference_type = iter_difference_t<inner_iterator>;
    using pointer = add_pointer_t<iter_reference_t<inner_iterator>>;
    using reference = iter_reference_t<inner_iterator>;

    constexpr join_iterator() = default;

    constexpr join_iterator(OuterIter outer_current, OuterSentinel outer_end)
        : outer_current_(outer_current), outer_end_(outer_end) {
        if (outer_current_ != outer_end_) {
            inner_current_ = (*outer_current_).begin();
            inner_end_ = (*outer_current_).end();
            satisfy();
        }
    }

    constexpr reference operator*() const { return *inner_current_; }
    constexpr pointer operator->() const { return _MSTL addressof(*inner_current_); }

    constexpr join_iterator& operator++() {
        ++inner_current_;
        satisfy();
        return *this;
    }

    constexpr join_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr bool operator==(const join_iterator& other) const {
        return outer_current_ == other.outer_current_ &&
               (outer_current_ == outer_end_ || inner_current_ == other.inner_current_);
    }
    constexpr bool operator!=(const join_iterator& other) const {
        return !(*this == other);
    }

private:
    constexpr void satisfy() {
        while (outer_current_ != outer_end_ && inner_current_ == inner_end_) {
            ++outer_current_;
            if (outer_current_ != outer_end_) {
                inner_current_ = (*outer_current_).begin();
                inner_end_ = (*outer_current_).end();
            }
        }
    }

    OuterIter outer_current_{};
    OuterSentinel outer_end_{};
    inner_iterator inner_current_{};
    inner_sentinel inner_end_{};
};

template <View V>
    requires Range<iter_reference_t<decltype(_MSTL declval<V>().begin())>>
class join_view : public view_base<join_view<V>> {
public:
    using outer_iterator = decltype(_MSTL declval<V>().begin());
    using outer_sentinel = decltype(_MSTL declval<V>().end());
    using iterator = join_iterator<outer_iterator, outer_sentinel>;

    constexpr join_view() = default;
    constexpr explicit join_view(V base) : base_(_MSTL move(base)) {}

    constexpr iterator begin() {
        return iterator(base_.begin(), base_.end());
    }
    constexpr iterator end() {
        return iterator(base_.end(), base_.end());
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.begin(), base_.end());
    }
    constexpr iterator end() const
    requires Range<const V> {
        return iterator(base_.end(), base_.end());
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }

private:
    V base_;
};

template <typename V>
join_view(V&&) -> join_view<V>;


template <size_t N, typename BaseIter>
class element_iterator {
public:
    using base_reference = iter_reference_t<BaseIter>;
    using element_type = decltype(_MSTL get<N>(_MSTL declval<base_reference>()));
    using iterator_category = forward_iterator_tag;
    using value_type = remove_cvref_t<element_type>;
    using difference_type = iter_difference_t<BaseIter>;
    using pointer = add_pointer_t<element_type>;
    using reference = element_type;

    constexpr element_iterator() = default;
    constexpr explicit element_iterator(BaseIter current) : current_(current) {}

    constexpr reference operator*() const {
        return _MSTL get<N>(*current_);
    }

    constexpr pointer operator->() const {
        return _MSTL addressof(_MSTL get<N>(*current_));
    }

    constexpr element_iterator& operator++() {
        ++current_;
        return *this;
    }

    constexpr element_iterator operator++(int) {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    constexpr bool operator==(const element_iterator& other) const {
        return current_ == other.current_;
    }
    constexpr bool operator!=(const element_iterator& other) const {
        return current_ != other.current_;
    }

    template <typename S>
    requires (!same_as<S, element_iterator>) && sentinel_for<S, BaseIter>
    constexpr bool operator==(const S& s) const {
        return current_ == s;
    }
    template <typename S>
    requires (!same_as<S, element_iterator>) && sentinel_for<S, BaseIter>
    constexpr bool operator!=(const S& s) const {
        return current_ != s;
    }

private:
    BaseIter current_{};
};

template <size_t N, View V>
class element_view : public view_base<element_view<N, V>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using base_sentinel = decltype(_MSTL declval<V>().end());
    using iterator = element_iterator<N, base_iterator>;
    using sentinel = base_sentinel;

    constexpr element_view() = default;
    constexpr explicit element_view(V base) : base_(_MSTL move(base)) {}

    constexpr iterator begin() {
        return iterator(base_.begin());
    }
    constexpr sentinel end() {
        return base_.end();
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return iterator(base_.begin());
    }
    constexpr sentinel end() const
    requires Range<const V> {
        return base_.end();
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }

private:
    V base_;
};


template <View V>
    requires (!common_range<V>)
class common_view : public view_base<common_view<V>> {
public:
    using base_iterator = decltype(_MSTL declval<V>().begin());
    using base_sentinel = decltype(_MSTL declval<V>().end());
    using iterator = base_iterator;
    using sentinel = base_iterator;

    constexpr common_view() = default;
    constexpr explicit common_view(V base) : base_(_MSTL move(base)) {}

    constexpr iterator begin() {
        return base_.begin();
    }

    constexpr iterator end() {
        auto it = base_.begin();
        auto sen = base_.end();
        while (it != sen) ++it;
        return it;
    }

    constexpr iterator begin() const
    requires Range<const V> {
        return base_.begin();
    }

    constexpr iterator end() const
    requires Range<const V> {
        auto it = base_.begin();
        auto sen = base_.end();
        while (it != sen) ++it;
        return it;
    }

    constexpr V base() const & requires copy_constructible<V> { return base_; }
    constexpr V base() && { return _MSTL move(base_); }

private:
    V base_;
};

template <typename V>
common_view(V&&) -> common_view<V>;


template <typename Iter>
class counted_iterator {
public:
    using iterator_category = forward_iterator_tag;
    using value_type = iter_value_t<Iter>;
    using difference_type = iter_difference_t<Iter>;
    using pointer = add_pointer_t<iter_reference_t<Iter>>;
    using reference = iter_reference_t<Iter>;

    constexpr counted_iterator() = default;
    constexpr counted_iterator(Iter current, difference_type count)
        : current_(current), count_(count) {}

    constexpr reference operator*() const { return *current_; }
    constexpr pointer operator->() const { return _MSTL addressof(*current_); }

    constexpr counted_iterator& operator++() {
        ++current_;
        --count_;
        return *this;
    }

    constexpr counted_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    constexpr bool operator==(const counted_iterator& other) const {
        return count_ == other.count_;
    }
    constexpr bool operator!=(const counted_iterator& other) const {
        return count_ != other.count_;
    }

    constexpr Iter base() const { return current_; }
    constexpr difference_type count() const { return count_; }

private:
    Iter current_{};
    difference_type count_ = 0;
};

template <typename Iter>
class counted_view : public view_base<counted_view<Iter>> {
public:
    using iterator = counted_iterator<Iter>;
    using difference_type = iter_difference_t<Iter>;

    constexpr counted_view() = default;
    constexpr counted_view(Iter iter, difference_type count)
        : iter_(iter), count_(count) {}

    constexpr iterator begin() const {
        return iterator(iter_, count_);
    }
    constexpr iterator end() const {
        return iterator(iter_, 0);
    }

private:
    Iter iter_;
    difference_type count_ = 0;
};

template <typename Iter, typename N>
counted_view(Iter, N) -> counted_view<Iter>;


template <View V1, View V2>
class concat_view : public view_base<concat_view<V1, V2>> {
    V1 v1_;
    V2 v2_;

public:
    concat_view() = default;
    concat_view(V1 v1, V2 v2) : v1_(_MSTL move(v1)), v2_(_MSTL move(v2)) {}

    struct iterator {
        using iter1_t = decltype(_MSTL declval<V1>().begin());
        using iter2_t = decltype(_MSTL declval<V2>().begin());

        using iterator_category = forward_iterator_tag;
        using value_type = common_type_t<
            iter_value_t<iter1_t>, iter_value_t<iter2_t>>;
        using difference_type = common_type_t<
            iter_difference_t<iter1_t>, iter_difference_t<iter2_t>>;
        using pointer = void;
        using reference = common_reference_t<
            iter_reference_t<iter1_t>, iter_reference_t<iter2_t>>;

        iter1_t current1_;
        iter1_t end1_;
        iter2_t current2_;
        iter2_t end2_;
        bool in_first_ = true;

        iterator() = default;

        iterator(iter1_t first1, iter1_t last1, iter2_t first2, iter2_t last2)
            : current1_(first1), end1_(last1), current2_(first2), end2_(last2) {
            if (current1_ == end1_) {
                in_first_ = false;
            }
        }

        reference operator*() const {
            return in_first_ ? *current1_ : *current2_;
        }

        iterator& operator++() {
            if (in_first_) {
                ++current1_;
                if (current1_ == end1_) {
                    in_first_ = false;
                }
            } else {
                ++current2_;
            }
            return *this;
        }

        iterator operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const iterator& other) const {
            if (in_first_ != other.in_first_) return false;
            if (in_first_) return current1_ == other.current1_;
            return current2_ == other.current2_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    auto begin() {
        return iterator(v1_.begin(), v1_.end(), v2_.begin(), v2_.end());
    }
    auto end() {
        return iterator(v1_.end(), v1_.end(), v2_.end(), v2_.end());
    }
    auto begin() const {
        return iterator(v1_.begin(), v1_.end(), v2_.begin(), v2_.end());
    }
    auto end() const {
        return iterator(v1_.end(), v1_.end(), v2_.end(), v2_.end());
    }
};


template <typename Iterator>
class subrange_view : public view_base<subrange_view<Iterator>> {
    Iterator first_;
    Iterator last_;

public:
    subrange_view() = default;
    subrange_view(Iterator first, Iterator last) : first_(first), last_(last) {}

    Iterator begin() const { return first_; }
    Iterator end() const { return last_; }
};

template <View V, typename T>
class split_view : public view_base<split_view<V, T>> {
    V base_;
    T delimiter_;

    using base_iterator = decltype(_MSTL declval<V>().begin());
    using base_sentinel = decltype(_MSTL declval<V>().end());

public:
    struct iterator {
        base_iterator current_;
        base_sentinel end_;
        T delimiter_;

        using iterator_category = forward_iterator_tag;
        using value_type = subrange_view<base_iterator>;
        using difference_type = iter_difference_t<base_iterator>;
        using pointer = void;
        using reference = value_type;

        iterator() = default;
        iterator(base_iterator cur, base_sentinel end, T delim)
        : current_(cur), end_(end), delimiter_(delim) {}

        value_type operator*() const {
            base_iterator start = current_;
            base_iterator iter = current_;
            while (iter != end_ && !(*iter == delimiter_)) ++iter;
            return value_type{start, iter};
        }

        iterator& operator++() {
            if (current_ == end_) return *this;
            while (current_ != end_ && !(*current_ == delimiter_)) ++current_;
            if (current_ != end_) ++current_;
            return *this;
        }

        iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return current_ == other.current_;
        }
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    split_view() = default;

    split_view(V base, T delimiter)
    : base_(_MSTL move(base)), delimiter_(delimiter) {}

    iterator begin() {
        return iterator{base_.begin(), base_.end(), delimiter_};
    }
    iterator end() {
        return iterator{base_.end(), base_.end(), delimiter_};
    }
};


template <View V>
class slice_view : public view_base<slice_view<V>> {
    V base_;
    iter_difference_t<decltype(_MSTL declval<V>().begin())> offset_;
    iter_difference_t<decltype(_MSTL declval<V>().begin())> length_;

    using base_iterator = decltype(_MSTL declval<V>().begin());

    struct cache_t {
        base_iterator begin_;
        base_iterator end_;
    };
    mutable _MSTL optional<cache_t> cache_;

    void ensure_cache() const {
        if (cache_) return;

        auto b = base_.begin();
        auto e = base_.end();

        for (auto i = 0; i < offset_ && b != e; ++i, ++b);
        auto begin_it = b;

        for (auto i = 0; i < length_ && b != e; ++i, ++b);
        auto end_it = b;

        cache_ = cache_t{begin_it, end_it};
    }

public:
    slice_view() = default;

    slice_view(V base, iter_difference_t<base_iterator> offset,
               iter_difference_t<base_iterator> length)
        : base_(_MSTL move(base)), offset_(offset), length_(length) {}

    base_iterator begin() {
        ensure_cache();
        return cache_->begin_;
    }

    base_iterator end() {
        ensure_cache();
        return cache_->end_;
    }

    base_iterator begin() const
    requires Range<const V> {
        ensure_cache();
        return cache_->begin_;
    }

    base_iterator end() const
    requires Range<const V> {
        ensure_cache();
        return cache_->end_;
    }
};


template <typename Derived>
struct range_adaptor_closure {
    template <typename OtherClosure>
    friend constexpr auto operator |(
        range_adaptor_closure<Derived> lhs,
        range_adaptor_closure<OtherClosure> rhs) {
        return [
            lhs = static_cast<const Derived&>(lhs),
            rhs = static_cast<const OtherClosure&>(rhs)
            ](auto&& range) {
            return _MSTL forward<decltype(range)>(range) | lhs | rhs;
        };
    }
};


MSTL_BEGIN_RANGES_VIEWS__

struct all_adaptor {
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return all(_MSTL forward<R>(range));
    }
};

MSTL_INLINE17 constexpr all_adaptor all;


template <typename Pred>
struct filter_adaptor_closure : range_adaptor_closure<filter_adaptor_closure<Pred>> {
    Pred pred;

    constexpr explicit filter_adaptor_closure(Pred p) : pred(_MSTL move(p)) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return filter_view{all(_MSTL forward<R>(range)), pred};
    }
};

struct filter_adaptor {
    template <typename Pred>
    constexpr auto operator()(Pred pred) const {
        return filter_adaptor_closure<Pred>{_MSTL move(pred)};
    }

    template <Range R, typename Pred>
    constexpr auto operator()(R&& range, Pred pred) const {
        return filter_view{all(_MSTL forward<R>(range)), _MSTL move(pred)};
    }
};

MSTL_INLINE17 constexpr filter_adaptor filter;


template <typename Func>
struct transform_adaptor_closure : range_adaptor_closure<transform_adaptor_closure<Func>> {
    Func func;

    constexpr explicit transform_adaptor_closure(Func f) : func(_MSTL move(f)) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return transform_view{all(_MSTL forward<R>(range)), func};
    }
};

struct transform_adaptor {
    template <typename Func>
    constexpr auto operator()(Func func) const {
        return transform_adaptor_closure<Func>{_MSTL move(func)};
    }

    template <Range R, typename Func>
    constexpr auto operator()(R&& range, Func func) const {
        return transform_view{all(_MSTL forward<R>(range)), _MSTL move(func)};
    }
};

MSTL_INLINE17 constexpr transform_adaptor transform;


template <typename DiffType>
struct take_adaptor_closure : range_adaptor_closure<take_adaptor_closure<DiffType>> {
    DiffType count;

    constexpr explicit take_adaptor_closure(DiffType n) : count(n) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return take_view{all(_MSTL forward<R>(range)), count};
    }
};

struct take_adaptor {
    template <integral N>
    constexpr auto operator()(N count) const {
        return take_adaptor_closure<N>{count};
    }

    template <Range R, integral N>
    constexpr auto operator()(R&& range, N count) const {
        return take_view{all(_MSTL forward<R>(range)), count};
    }
};

MSTL_INLINE17 constexpr take_adaptor take;


template <typename Pred>
struct take_while_adaptor_closure : range_adaptor_closure<take_while_adaptor_closure<Pred>> {
    Pred pred;

    constexpr explicit take_while_adaptor_closure(Pred p) : pred(_MSTL move(p)) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return take_while_view{all(_MSTL forward<R>(range)), pred};
    }
};

struct take_while_adaptor {
    template <typename Pred>
    constexpr auto operator()(Pred pred) const {
        return take_while_adaptor_closure<Pred>{_MSTL move(pred)};
    }

    template <Range R, typename Pred>
    constexpr auto operator()(R&& range, Pred pred) const {
        return take_while_view{all(_MSTL forward<R>(range)), _MSTL move(pred)};
    }
};

MSTL_INLINE17 constexpr take_while_adaptor take_while;


template <typename DiffType>
struct drop_adaptor_closure : range_adaptor_closure<drop_adaptor_closure<DiffType>> {
    DiffType count;

    constexpr explicit drop_adaptor_closure(DiffType n) : count(n) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return drop_view{all(_MSTL forward<R>(range)), count};
    }
};

struct drop_adaptor {
    template <integral N>
    constexpr auto operator()(N count) const {
        return drop_adaptor_closure<N>{count};
    }

    template <Range R, integral N>
    constexpr auto operator()(R&& range, N count) const {
        return drop_view{all(_MSTL forward<R>(range)), count};
    }
};

MSTL_INLINE17 constexpr drop_adaptor drop;


template <typename Pred>
struct drop_while_adaptor_closure : range_adaptor_closure<drop_while_adaptor_closure<Pred>> {
    Pred pred;

    constexpr explicit drop_while_adaptor_closure(Pred p) : pred(_MSTL move(p)) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return drop_while_view{all(_MSTL forward<R>(range)), pred};
    }
};

struct drop_while_adaptor {
    template <typename Pred>
    constexpr auto operator()(Pred pred) const {
        return drop_while_adaptor_closure<Pred>{_MSTL move(pred)};
    }

    template <Range R, typename Pred>
    constexpr auto operator()(R&& range, Pred pred) const {
        return drop_while_view{all(_MSTL forward<R>(range)), _MSTL move(pred)};
    }
};

MSTL_INLINE17 constexpr drop_while_adaptor drop_while;


struct reverse_adaptor_closure : range_adaptor_closure<reverse_adaptor_closure> {
    template <Range R>
        requires bidirectional_iterator<decltype(_MSTL declval<R>().begin())>
    constexpr auto operator()(R&& range) const {
        return reverse_view{all(_MSTL forward<R>(range))};
    }
};

struct reverse_adaptor {
    constexpr auto operator()() const {
        return reverse_adaptor_closure{};
    }

    template <Range R>
        requires bidirectional_iterator<decltype(_MSTL declval<R>().begin())>
    constexpr auto operator()(R&& range) const {
        return reverse_view{all(_MSTL forward<R>(range))};
    }
};

MSTL_INLINE17 constexpr reverse_adaptor reverse;


struct iota_adaptor {
    template <typename T>
    constexpr auto operator()(T start) const {
        return iota_view<T>{start};
    }

    template <typename T>
    constexpr auto operator()(T start, T bound) const {
        return iota_view<T>{start, bound};
    }
};

MSTL_INLINE17 constexpr iota_adaptor iota;


struct repeat_adaptor {
    template <typename T>
    constexpr auto operator()(T value) const {
        return repeat_view<T>{_MSTL move(value)};
    }

    template <typename T, integral N>
    constexpr auto operator()(T value, N count) const {
        return repeat_view<T>{_MSTL move(value), static_cast<ptrdiff_t>(count)};
    }
};

MSTL_INLINE17 constexpr repeat_adaptor repeat;


struct join_adaptor_closure : range_adaptor_closure<join_adaptor_closure> {
    template <Range R>
        requires Range<iter_reference_t<decltype(_MSTL declval<R>().begin())>>
    constexpr auto operator()(R&& range) const {
        return join_view{all(_MSTL forward<R>(range))};
    }
};

struct join_adaptor {
    constexpr auto operator()() const {
        return join_adaptor_closure{};
    }

    template <Range R>
        requires Range<iter_reference_t<decltype(_MSTL declval<R>().begin())>>
    constexpr auto operator()(R&& range) const {
        return join_view{all(_MSTL forward<R>(range))};
    }
};

MSTL_INLINE17 constexpr join_adaptor join;


template <size_t N>
struct elements_adaptor_closure : range_adaptor_closure<elements_adaptor_closure<N>> {
    template <Range R>
    constexpr auto operator()(R&& range) const {
        return element_view<N, decltype(all(_MSTL forward<R>(range)))>{
            all(_MSTL forward<R>(range))
        };
    }
};

template <size_t N>
struct elements_adaptor {
    constexpr auto operator()() const {
        return elements_adaptor_closure<N>{};
    }

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return element_view<N, decltype(all(_MSTL forward<R>(range)))>{
            all(_MSTL forward<R>(range))
        };
    }
};

template <size_t N>
MSTL_INLINE17 constexpr elements_adaptor<N> elements;

MSTL_INLINE17 constexpr auto keys = elements<0>;
MSTL_INLINE17 constexpr auto values = elements<1>;


struct common_adaptor_closure : range_adaptor_closure<common_adaptor_closure> {
    template <Range R>
    constexpr auto operator()(R&& range) const {
        if constexpr (common_range<R>) {
            return all(_MSTL forward<R>(range));
        } else {
            return common_view{all(_MSTL forward<R>(range))};
        }
    }
};

struct common_adaptor {
    constexpr auto operator()() const {
        return common_adaptor_closure{};
    }

    template <Range R>
    constexpr auto operator()(R&& range) const {
        if constexpr (common_range<R>) {
            return all(_MSTL forward<R>(range));
        } else {
            return common_view{all(_MSTL forward<R>(range))};
        }
    }
};

MSTL_INLINE17 constexpr common_adaptor common;


struct counted_adaptor {
    template <typename Iter, integral N>
    constexpr auto operator()(Iter iter, N count) const {
        return counted_view{iter, static_cast<iter_difference_t<Iter>>(count)};
    }
};

MSTL_INLINE17 constexpr counted_adaptor counted;


template <typename V2>
struct concat_adaptor_closure : range_adaptor_closure<concat_adaptor_closure<V2>> {
    using view_type = decltype(all(_MSTL declval<V2>()));
    view_type view_;

    constexpr explicit concat_adaptor_closure(V2&& v2)
    : view_(all(_MSTL forward<V2>(v2))) {}

    template <typename V>
    requires Range<V>
    constexpr auto operator()(V&& v) const {
        return concat_view{ all(_MSTL forward<V>(v)), view_ };
    }
};

struct concat_adaptor {
    template <typename V2>
    constexpr auto operator()(V2&& v2) const {
        return concat_adaptor_closure<V2>{_MSTL forward<V2>(v2)};
    }

    template <typename V1, typename V2>
    constexpr auto operator()(V1&& v1, V2&& v2) const {
        return concat_view{ all(_MSTL forward<V1>(v1)), all(_MSTL forward<V2>(v2)) };
    }
};

MSTL_INLINE17 constexpr concat_adaptor concat;


template <typename T>
struct split_adaptor_closure : range_adaptor_closure<split_adaptor_closure<T>> {
    T delim_;

    constexpr explicit split_adaptor_closure(T d) : delim_(d) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return split_view{all(_MSTL forward<R>(range)), delim_};
    }
};

struct split_adaptor {
    template <typename T>
    constexpr auto operator()(T delim) const {
        return split_adaptor_closure<T>{delim};
    }

    template <Range R, typename T>
    constexpr auto operator()(R&& range, T delim) const {
        return split_view{all(_MSTL forward<R>(range)), delim};
    }
};

MSTL_INLINE17 constexpr split_adaptor split;


struct slice_adaptor_closure : range_adaptor_closure<slice_adaptor_closure> {
    ptrdiff_t offset_, length_;

    constexpr explicit slice_adaptor_closure(ptrdiff_t o, ptrdiff_t l)
     : offset_(o), length_(l) {}

    template <Range R>
    constexpr auto operator()(R&& range) const {
        return slice_view{all(_MSTL forward<R>(range)), offset_, length_};
    }
};

struct slice_adaptor {
    constexpr auto operator()(ptrdiff_t offset, ptrdiff_t length) const {
        return slice_adaptor_closure{offset, length};
    }

    template <Range R>
    constexpr auto operator()(R&& range, ptrdiff_t offset, ptrdiff_t length) const {
        return slice_view{all(_MSTL forward<R>(range)), offset, length};
    }
};

MSTL_INLINE17 constexpr slice_adaptor slice;

MSTL_END_RANGES_VIEWS__

MSTL_END_RANGES__
#endif // MSTL_STANDARD_20__
MSTL_END_NAMESPACE__
#endif // MSTL_RANGES_HPP__
