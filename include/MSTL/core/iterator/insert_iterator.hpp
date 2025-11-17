#ifndef MSTL_CORE_ITERATOR_INSERT_ITERATOR_HPP__
#define MSTL_CORE_ITERATOR_INSERT_ITERATOR_HPP__
#include "../utility/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Container>
class back_insert_iterator {
public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = void;
    using pointer           = void;
    using reference         = void;
    using self              = back_insert_iterator<Container>;

    constexpr explicit back_insert_iterator(Container& x) noexcept
    : container(_MSTL addressof(x)) {}

    constexpr self& operator =(const typename Container::value_type& value) {
        container->push_back(value);
        return *this;
    }
    constexpr self& operator =(typename Container::value_type&& value) {
        container->push_back(_MSTL move(value));
        return *this;
    }

    MSTL_CONSTEXPR20 ~back_insert_iterator() noexcept = default;

    MSTL_NODISCARD constexpr self& operator *() noexcept { return *this; }
    constexpr self& operator ++() noexcept { return *this; }
    constexpr self& operator ++(int) noexcept { return *this; }

private:
    Container* container;
};
template <typename Container>
MSTL_NODISCARD constexpr back_insert_iterator<Container> back_inserter(Container& x) noexcept {
    return back_insert_iterator<Container>(x);
}

template <typename Container>
class front_insert_iterator {
public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = void;
    using pointer           = void;
    using reference         = void;
    using self              = front_insert_iterator<Container>;

    constexpr explicit front_insert_iterator(Container& x) noexcept
    : container(_MSTL addressof(x)) {}

    constexpr self& operator =(const typename Container::value_type& value) {
        container->push_front(value);
        return *this;
    }
    constexpr self& operator =(typename Container::value_type&& value) {
        container->push_front(_MSTL move(value));
        return *this;
    }

    MSTL_CONSTEXPR20 ~front_insert_iterator() noexcept = default;

    MSTL_NODISCARD constexpr self& operator *() noexcept { return *this; }
    constexpr self& operator ++() noexcept { return *this; }
    constexpr self& operator ++(int) noexcept { return *this; }

private:
    Container* container;
};

template <typename Container>
MSTL_NODISCARD constexpr front_insert_iterator<Container> front_inserter(Container& x) noexcept {
    return front_insert_iterator<Container>(x);
}

template <typename Container>
class insert_iterator {
public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = void;
    using pointer           = void;
    using reference         = void;
    using self              = insert_iterator<Container>;

    constexpr insert_iterator(Container& x, typename Container::iterator it) noexcept
    : container(_MSTL addressof(x)), iter(_MSTL move(it)) {}

    constexpr self& operator =(const typename Container::value_type& value) {
        iter = container->insert(iter, value);
        ++iter;
        return *this;
    }
    constexpr self& operator =(typename Container::value_type&& value) {
        iter = container->insert(iter, _MSTL move(value));
        ++iter;
        return *this;
    }

    MSTL_CONSTEXPR20 ~insert_iterator() noexcept = default;

    MSTL_NODISCARD constexpr self& operator *() noexcept { return *this; }
    constexpr self& operator ++() noexcept { return *this; }
    constexpr self& operator ++(int) noexcept { return *this; }

private:
    Container* container;
    typename Container::iterator iter;
};

template <typename Container>
MSTL_NODISCARD constexpr insert_iterator<Container>
inserter(Container& x, typename Container::iterator it) noexcept {
    return insert_iterator<Container>(x, it);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_INSERT_ITERATOR_HPP__
