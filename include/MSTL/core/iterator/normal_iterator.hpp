#ifndef MSTL_CORE_ITERATOR_NORMAL_ITERATOR_HPP__
#define MSTL_CORE_ITERATOR_NORMAL_ITERATOR_HPP__
#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename Container>
class normal_iterator {
    using traits_type = iterator_traits<Iterator>;
    
public:
    using iterator_type = Iterator;
    using iterator_category = typename traits_type::iterator_category;
    using value_type = typename traits_type::value_type;
    using difference_type = typename traits_type::difference_type;
    using reference = typename traits_type::reference;
    using pointer = typename traits_type::pointer;
    
private:
    Iterator current_;
    
    template <typename Iter>
    using convertible_from = enable_if_t<is_convertible_v<Iter, Iterator>>;
    
public:
    constexpr normal_iterator() noexcept : current_(Iterator()) {}

    explicit constexpr normal_iterator(const Iterator& iter) noexcept 
    : current_(iter) {}

    template <typename Iter, typename = convertible_from<Iter>>
    constexpr normal_iterator(const normal_iterator<Iter, Container>& other) noexcept 
    : current_(other.base()) {}

    constexpr reference operator*() const noexcept { return *current_; }
    constexpr pointer operator->() const noexcept { return current_; }
    
    constexpr normal_iterator& operator++() noexcept {
        ++current_;
        return *this;
    }

    constexpr normal_iterator operator++(int) noexcept { 
        return normal_iterator(current_++);
    }

    constexpr normal_iterator& operator--() noexcept {
        --current_;
        return *this;
    }

    constexpr normal_iterator operator--(int) noexcept { 
        return normal_iterator(current_--);
    }

    constexpr reference operator [](difference_type n) const noexcept {
        return current_[n];
    }

    constexpr normal_iterator& operator+=(difference_type n) noexcept { 
        current_ += n;
        return *this;
    }

    constexpr normal_iterator operator+(difference_type n) const noexcept { 
        return normal_iterator(current_ + n);
    }

    constexpr normal_iterator& operator-=(difference_type n) noexcept { 
        current_ -= n;
        return *this;
    }

    constexpr normal_iterator operator-(difference_type n) const noexcept { 
        return normal_iterator(current_ - n);
    }

    constexpr const Iterator& base() const noexcept { 
        return current_;
    }
};

template <typename LeftIter, typename RightIter, typename Container>
MSTL_NODISCARD constexpr bool operator ==(
    const normal_iterator<LeftIter, Container>& lhs,
    const normal_iterator<RightIter, Container>& rhs) noexcept { 
    return lhs.base() == rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr bool operator ==(
    const normal_iterator<Iterator, Container>& lhs,
    const normal_iterator<Iterator, Container>& rhs) noexcept { 
    return lhs.base() == rhs.base(); 
}

template <typename LeftIter, typename RightIter, typename Container>
MSTL_NODISCARD constexpr bool operator !=(
    const normal_iterator<LeftIter, Container>& lhs,
    const normal_iterator<RightIter, Container>& rhs) noexcept { 
    return lhs.base() != rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr bool operator !=(
    const normal_iterator<Iterator, Container>& lhs,
    const normal_iterator<Iterator, Container>& rhs) noexcept { 
    return lhs.base() != rhs.base(); 
}

template <typename LeftIter, typename RightIter, typename Container>
MSTL_NODISCARD constexpr bool operator <(
    const normal_iterator<LeftIter, Container>& lhs,
    const normal_iterator<RightIter, Container>& rhs) noexcept { 
    return lhs.base() < rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr bool operator <(
    const normal_iterator<Iterator, Container>& lhs,
    const normal_iterator<Iterator, Container>& rhs) noexcept { 
    return lhs.base() < rhs.base(); 
}

template <typename LeftIter, typename RightIter, typename Container>
MSTL_NODISCARD constexpr bool operator >(
    const normal_iterator<LeftIter, Container>& lhs,
    const normal_iterator<RightIter, Container>& rhs) noexcept { 
    return lhs.base() > rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr bool operator >(
    const normal_iterator<Iterator, Container>& lhs,
    const normal_iterator<Iterator, Container>& rhs) noexcept { 
    return lhs.base() > rhs.base(); 
}

template <typename LeftIter, typename RightIter, typename Container>
MSTL_NODISCARD constexpr bool operator <=(
    const normal_iterator<LeftIter, Container>& lhs,
    const normal_iterator<RightIter, Container>& rhs) noexcept { 
    return lhs.base() <= rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr bool operator <=(
    const normal_iterator<Iterator, Container>& lhs,
    const normal_iterator<Iterator, Container>& rhs) noexcept { 
    return lhs.base() <= rhs.base(); 
}

template <typename LeftIter, typename RightIter, typename Container>
MSTL_NODISCARD constexpr bool operator >=(
    const normal_iterator<LeftIter, Container>& lhs,
    const normal_iterator<RightIter, Container>& rhs) noexcept { 
    return lhs.base() >= rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr bool operator >=(
    const normal_iterator<Iterator, Container>& lhs,
    const normal_iterator<Iterator, Container>& rhs) noexcept { 
    return lhs.base() >= rhs.base(); 
}

template <typename LeftIter, typename RightIter, typename Container>
MSTL_NODISCARD constexpr auto operator -(
    const normal_iterator<LeftIter, Container>& lhs,
    const normal_iterator<RightIter, Container>& rhs) noexcept
    -> decltype(lhs.base() - rhs.base()) { 
    return lhs.base() - rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr typename normal_iterator<Iterator, Container>::difference_type
operator -(const normal_iterator<Iterator, Container>& lhs,
           const normal_iterator<Iterator, Container>& rhs) noexcept {
    return lhs.base() - rhs.base(); 
}

template <typename Iterator, typename Container>
MSTL_NODISCARD constexpr normal_iterator<Iterator, Container>
operator +(iter_difference_t<normal_iterator<Iterator, Container>> n,
           const normal_iterator<Iterator, Container>& iter) noexcept {
    return normal_iterator<Iterator, Container>(iter.base() + n); 
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_NORMAL_ITERATOR_HPP__
