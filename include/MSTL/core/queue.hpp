#ifndef MSTL_QUEUE_HPP__
#define MSTL_QUEUE_HPP__
#include "deque.hpp"
#include "heap.hpp"
#include "vector.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename Sequence = deque<T>>
class queue : public icollector<queue<T, Sequence>> {
    using self = queue<T, Sequence>;
    using super = icollector<self>;

public:
    using value_type        = typename Sequence::value_type;
    using difference_type   = typename Sequence::difference_type;
    using size_type         = typename Sequence::size_type;
    using reference         = typename Sequence::reference;
    using const_reference   = typename Sequence::const_reference;

    using iterator          = typename Sequence::iterator;
    using const_iterator    = typename Sequence::const_iterator;

    static_assert(is_object_v<T>, "queue only contains object types.");
    static_assert(is_same_v<T, value_type>, "queue require consistent types.");

private:
    Sequence seq_;

public:

    queue() = default;
    explicit queue(const Sequence& x) : seq_(x) {}
    explicit queue(Sequence&& x) noexcept(is_nothrow_move_constructible_v<Sequence>)
        : seq_(_MSTL move(x)) {}
    ~queue() = default;

    MSTL_NODISCARD size_type size() const noexcept(noexcept(seq_.size())) { return seq_.size(); }
    MSTL_NODISCARD bool empty() const noexcept(noexcept(seq_.empty())) { return seq_.empty(); }

    MSTL_NODISCARD reference front() noexcept(noexcept(seq_.front())) { return seq_.front(); }
    MSTL_NODISCARD const_reference front() const noexcept(noexcept(seq_.front())) { return seq_.front(); }
    MSTL_NODISCARD reference back() noexcept(noexcept(seq_.back())) { return seq_.back(); }
    MSTL_NODISCARD const_reference back() const noexcept(noexcept(seq_.back())) { return seq_.back(); }

    MSTL_NODISCARD iterator begin() noexcept(noexcept(seq_.begin())) { return seq_.begin(); }
    MSTL_NODISCARD iterator end() noexcept(noexcept(seq_.end())) { return seq_.end(); }
    MSTL_NODISCARD const_iterator begin() const noexcept(noexcept(seq_.begin())) { return seq_.begin(); }
    MSTL_NODISCARD const_iterator end() const noexcept(noexcept(seq_.end())) { return seq_.end(); }
    MSTL_NODISCARD const_iterator cbegin() const noexcept(noexcept(seq_.cbegin())) { return seq_.cbegin(); }
    MSTL_NODISCARD const_iterator cend() const noexcept(noexcept(seq_.cend())) { return seq_.cend(); }

    void push(const T& x) { seq_.push_back(x); }
    void push(T&& x) { seq_.push_back(_MSTL move(x)); }

    void pop() noexcept(noexcept(seq_.pop_front())) { seq_.pop_front(); }

    template <typename... Args>
    decltype(auto) emplace(Args&&... args) { return seq_.emplace_back(_MSTL forward<Args>(args)...); }

    void swap(self& x) noexcept(is_nothrow_swappable_v<Sequence>) {
        _MSTL swap(seq_, x.seq_);
    }

    MSTL_NODISCARD bool operator ==(const self& rh) const
    noexcept(noexcept(seq_ == rh.seq_)) {
        return seq_ == rh.seq_;
    }
    MSTL_NODISCARD bool operator !=(const self& rh) const
    noexcept(noexcept(seq_ != rh.seq_)) {
        return seq_ != rh.seq_;
    }
    MSTL_NODISCARD bool operator <(const self& rh) const
    noexcept(noexcept(seq_ < rh.seq_)) {
        return seq_ < rh.seq_;
    }
    MSTL_NODISCARD bool operator >(const self& rh) const
    noexcept(noexcept(seq_ > rh.seq_)) {
        return seq_ > rh.seq_;
    }
    MSTL_NODISCARD bool operator <=(const self& rh) const
    noexcept(noexcept(seq_ <= rh.seq_)) {
        return seq_ <= rh.seq_;
    }
    MSTL_NODISCARD bool operator >=(const self& rh) const
    noexcept(noexcept(seq_ >= rh.seq_)) {
        return seq_ >= rh.seq_;
    }

    MSTL_NODISCARD size_type to_hash() const noexcept {
        return super::default_to_hash(seq_);
    }

    MSTL_NODISCARD string to_string() const {
        return super::default_to_string(seq_);
    }
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Sequence>
queue(Sequence) -> queue<typename Sequence::value_type, Sequence>;
#endif


template <typename T, typename Sequence = vector<T>,
    typename Compare = less<typename Sequence::value_type>>
class priority_queue : public icollector<priority_queue<T, Sequence, Compare>> {
    using self = priority_queue<T, Sequence, Compare>;
    using super = icollector<self>;

public:
    using value_type        = typename Sequence::value_type;
    using size_type         = typename Sequence::size_type;
    using reference         = typename Sequence::reference;
    using const_reference   = typename Sequence::const_reference;

    static_assert(is_object_v<T>, "priority queue only contains object types.");
    static_assert(is_same_v<T, value_type>, "priority queue require consistent types.");

private:
    compressed_pair<Compare, Sequence> pair_{ _MSTL_TAG default_construct_tag{} };

    void make_heap_inside() {
        _MSTL make_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

public:
    priority_queue() = default;

    explicit priority_queue(const Compare& comp) 
        noexcept(is_nothrow_default_constructible_v<Sequence> && is_nothrow_copy_constructible_v<Compare>) 
        : pair_(_MSTL_TAG exact_arg_construct_tag{}, comp) {}

    priority_queue(const Compare& comp, const Sequence& seq)
        : pair_(_MSTL_TAG exact_arg_construct_tag{}, comp, seq) {
        make_heap_inside();
    }

    priority_queue(const Compare& comp, Sequence&& seq)
        noexcept(is_nothrow_move_constructible_v<Sequence> && is_nothrow_copy_constructible_v<Compare>)
        : pair_(_MSTL_TAG exact_arg_construct_tag{}, comp, _MSTL move(seq)) {
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Sequence& seq)
        : pair_(_MSTL_TAG default_construct_tag{}, seq) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last) 
        : pair_(_MSTL_TAG default_construct_tag{}, first, last) {
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Compare& comp) 
        : pair_(_MSTL_TAG exact_arg_construct_tag{}, comp, first, last) {
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Compare& comp, const Sequence& seq)
        : pair_(_MSTL_TAG exact_arg_construct_tag{}, comp, seq) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Compare& comp, Sequence&& seq) 
        : pair_(_MSTL_TAG exact_arg_construct_tag{}, comp, _MSTL move(seq)) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    MSTL_NODISCARD bool empty() const noexcept(noexcept(_MSTL declval<Sequence>().empty())) { return pair_.value.empty(); }
    MSTL_NODISCARD size_type size() const noexcept(noexcept(_MSTL declval<Sequence>().size())) { return pair_.value.size(); }

    MSTL_NODISCARD const_reference top() const noexcept(noexcept(_MSTL declval<Sequence>().front())) { return pair_.value.front(); }

    void push(const value_type& x) {
        pair_.value.push_back(x);
        _MSTL push_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }
    void push(value_type&& x) {
        pair_.value.push_back(_MSTL move(x));
        _MSTL push_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

    void pop() {
        _MSTL pop_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
        pair_.value.pop_back();
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        pair_.value.emplace_back(_MSTL forward<Args>(args)...);
        _MSTL push_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

    void swap(self& x) noexcept(is_nothrow_swappable_v<Sequence> && is_nothrow_swappable_v<Compare>) {
        pair_.swap(x.pair_);
    }

    MSTL_NODISCARD bool operator ==(const self& rh) const
    noexcept(noexcept(pair_.value == rh.pair_.value)) {
        return pair_.value == rh.pair_.value;
    }
    MSTL_NODISCARD bool operator !=(const self& rh) const
    noexcept(noexcept(pair_.value != rh.pair_.value)) {
        return pair_.value != rh.pair_.value;
    }
    MSTL_NODISCARD bool operator <(const self& rh) const
    noexcept(noexcept(pair_.value < rh.pair_.value)) {
        return pair_.value < rh.pair_.value;
    }
    MSTL_NODISCARD bool operator >(const self& rh) const
    noexcept(noexcept(pair_.value > rh.pair_.value)) {
        return pair_.value > rh.pair_.value;
    }
    MSTL_NODISCARD bool operator <=(const self& rh) const
    noexcept(noexcept(pair_.value <= rh.pair_.value)) {
        return pair_.value <= rh.pair_.value;
    }
    MSTL_NODISCARD bool operator >=(const self& rh) const
    noexcept(noexcept(pair_.value >= rh.pair_.value)) {
        return pair_.value >= rh.pair_.value;
    }

    MSTL_NODISCARD size_type to_hash() const noexcept {
        return super::default_to_hash(pair_.value);
    }

    MSTL_NODISCARD string to_string() const {
        return super::default_to_string(pair_.value);
    }
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Compare, typename Sequence>
priority_queue(Compare, Sequence) -> priority_queue<typename Sequence::value_type, Sequence, Compare>;

template <typename Iterator, typename Compare = less<iter_value_t<Iterator>>,
    typename Sequence = vector<iter_value_t<Iterator>>>
priority_queue(Iterator, Iterator, Compare = Compare(), Sequence = Sequence())
-> priority_queue<iter_value_t<Iterator>, Sequence, Compare>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_QUEUE_HPP__
