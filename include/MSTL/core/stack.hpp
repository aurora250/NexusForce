#ifndef MSTL_STACK_HPP__
#define MSTL_STACK_HPP__
#include "deque.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename Sequence = deque<T>>
class stack : public icollector<stack<T, Sequence>> {
    using self = stack<T, Sequence>;
    using super = icollector<self>;

public:
    using value_type        = typename Sequence::value_type;
    using difference_type   = typename Sequence::difference_type;
    using size_type         = typename Sequence::size_type;
    using reference         = typename Sequence::reference;
    using const_reference   = typename Sequence::const_reference;

    static_assert(is_object_v<T>, "stack only contains object types.");
    static_assert(is_same_v<T, value_type>, "stack require consistent types.");

private:
    Sequence seq_;

public:
    stack() = default;
    explicit stack(const Sequence& seq) : seq_(seq) {}
    explicit stack(Sequence&& seq) noexcept(is_nothrow_move_constructible_v<Sequence>) 
        : seq_(_MSTL move(seq)) {}
    ~stack() = default;

    MSTL_NODISCARD size_type size() const noexcept(noexcept(seq_.size())) { return seq_.size(); }
    MSTL_NODISCARD bool empty() const noexcept(noexcept(seq_.empty())) { return seq_.empty(); }

    MSTL_NODISCARD reference top() noexcept(noexcept(seq_.back())) { return seq_.back(); }
    MSTL_NODISCARD const_reference top() const noexcept(noexcept(seq_.back())) { return seq_.back(); }

    template <typename... Args>
    decltype(auto) emplace(Args&&... args) { return seq_.emplace(_MSTL forward<Args>(args)...); }

    void push(const value_type& x) { seq_.push_back(x); }
    void push(value_type&& x) { seq_.push_back(_MSTL move(x)); }

    void pop() noexcept(noexcept(seq_.pop_back())) { seq_.pop_back(); }

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
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Sequence>
stack(Sequence) -> stack<typename Sequence::value_type, Sequence>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_STACK_HPP__
