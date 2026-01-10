#ifndef MSTL_CORE_CONTAINER_PRIORITY_QUEUE_HPP__
#define MSTL_CORE_CONTAINER_PRIORITY_QUEUE_HPP__
#include "../algorithm/heap.hpp"
#include "vector.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename Sequence = vector<T>,
    typename Compare = less<typename Sequence::value_type>>
class priority_queue : public icollector<priority_queue<T, Sequence, Compare>> {

public:
    using value_type        = typename Sequence::value_type;
    using size_type         = typename Sequence::size_type;
    using reference         = typename Sequence::reference;
    using const_reference   = typename Sequence::const_reference;

    static_assert(is_object_v<T>, "priority queue only contains object types.");
    static_assert(is_same_v<T, value_type>, "priority queue require consistent types.");

private:
    compressed_pair<Compare, Sequence> pair_{ default_construct_tag{} };

    void make_heap_inside() {
        _MSTL make_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

public:
    priority_queue() = default;

    explicit priority_queue(const Compare& comp)
        noexcept(is_nothrow_default_constructible_v<Sequence> && is_nothrow_copy_constructible_v<Compare>)
        : pair_(exact_arg_construct_tag{}, comp) {}

    priority_queue(const Compare& comp, const Sequence& seq)
        : pair_(exact_arg_construct_tag{}, comp, seq) {
        make_heap_inside();
    }

    priority_queue(const Compare& comp, Sequence&& seq)
        noexcept(is_nothrow_move_constructible_v<Sequence> && is_nothrow_copy_constructible_v<Compare>)
        : pair_(exact_arg_construct_tag{}, comp, _MSTL move(seq)) {
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Sequence& seq)
        : pair_(default_construct_tag{}, seq) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last)
        : pair_(default_construct_tag{}, first, last) {
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Compare& comp)
        : pair_(exact_arg_construct_tag{}, comp, first, last) {
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Compare& comp, const Sequence& seq)
        : pair_(exact_arg_construct_tag{}, comp, seq) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    template <typename Iterator>
    priority_queue(Iterator first, Iterator last, const Compare& comp, Sequence&& seq)
        : pair_(exact_arg_construct_tag{}, comp, _MSTL move(seq)) {
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

    void swap(priority_queue& x) noexcept(is_nothrow_swappable_v<Sequence> && is_nothrow_swappable_v<Compare>) {
        pair_.swap(x.pair_);
    }

    MSTL_NODISCARD bool operator ==(const priority_queue& rhs) const
    noexcept(noexcept(pair_.value == rhs.pair_.value)) {
        return pair_.value == rhs.pair_.value;
    }
    MSTL_NODISCARD bool operator <(const priority_queue& rhs) const
    noexcept(noexcept(pair_.value < rhs.pair_.value)) {
        return pair_.value < rhs.pair_.value;
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
#endif // MSTL_CORE_CONTAINER_PRIORITY_QUEUE_HPP__
