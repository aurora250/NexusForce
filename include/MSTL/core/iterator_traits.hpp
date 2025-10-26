#ifndef MSTL_ITERATOR_TRAITS_HPP__
#define MSTL_ITERATOR_TRAITS_HPP__
#include "type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

struct input_iterator_tag {
    constexpr input_iterator_tag() = default;
};
struct output_iterator_tag {
    constexpr output_iterator_tag() = default;
};
struct forward_iterator_tag : input_iterator_tag {
    constexpr forward_iterator_tag() = default;
};
struct bidirectional_iterator_tag : forward_iterator_tag {
    constexpr bidirectional_iterator_tag() = default;
};
struct random_access_iterator_tag : bidirectional_iterator_tag {
    constexpr random_access_iterator_tag() = default;
};
#ifdef MSTL_STANDARD_20__
struct contiguous_iterator_tag : random_access_iterator_tag {
    constexpr contiguous_iterator_tag() = default;
};
#endif


MSTL_BEGIN_INNER__

template <typename, typename = void>
struct __iterator_traits_base {};

template <typename Iterator>
struct __iterator_traits_base<Iterator,
    void_t<typename Iterator::iterator_category, typename Iterator::value_type,
    typename Iterator::difference_type, typename Iterator::pointer, typename Iterator::reference>>
{
    using iterator_category = typename Iterator::iterator_category;
    using value_type        = typename Iterator::value_type;
    using difference_type   = typename Iterator::difference_type;
    using pointer           = typename Iterator::pointer;
    using reference         = typename Iterator::reference;
};

MSTL_END_INNER__


template <typename Iterator>
struct iterator_traits : _INNER __iterator_traits_base<Iterator> {};

template <typename T>
struct iterator_traits<T*> {
    static_assert(is_object_v<T>, "iterator traits requires object types.");

#ifdef MSTL_STANDARD_20__
    using iterator_category = contiguous_iterator_tag;
#else
    using iterator_category = random_access_iterator_tag;
#endif // MSTL_STANDARD_20__
    using value_type        = remove_cv_t<T>;
    using difference_type   = ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;
};

template <typename Iterator>
using iter_cat_t = typename iterator_traits<Iterator>::iterator_category;
template <typename Iterator>
using iter_val_t = typename iterator_traits<Iterator>::value_type;
template <typename Iterator>
using iter_dif_t = typename iterator_traits<Iterator>::difference_type;
template <typename Iterator>
using iter_ptr_t = typename iterator_traits<Iterator>::pointer;
template <typename Iterator>
using iter_ref_t = typename iterator_traits<Iterator>::reference;

MSTL_END_NAMESPACE__
#endif // MSTL_ITERATOR_TRAITS_HPP__
