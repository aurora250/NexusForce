#ifndef MSTL_CORE_CONTAINER_QUEUE_HPP__
#define MSTL_CORE_CONTAINER_QUEUE_HPP__
#include "deque.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename Sequence = deque<T>>
class queue : public icollector<queue<T, Sequence>> {
    using super = icollector<queue>;

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

    void swap(queue& x) noexcept(is_nothrow_swappable_v<Sequence>) {
        _MSTL swap(seq_, x.seq_);
    }

    MSTL_NODISCARD bool operator ==(const queue& rhs) const
    noexcept(noexcept(seq_ == rhs.seq_)) {
        return seq_ == rhs.seq_;
    }
    MSTL_NODISCARD bool operator <(const queue& rhs) const
    noexcept(noexcept(seq_ < rhs.seq_)) {
        return seq_ < rhs.seq_;
    }
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Sequence>
queue(Sequence) -> queue<typename Sequence::value_type, Sequence>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_QUEUE_HPP__
