#ifndef MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__
#define MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__
#include "../typeinfo/type_traits.hpp"
#include "../typeinfo/tags.hpp"
MSTL_BEGIN_NAMESPACE__

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
    static_assert(is_object<T>::value, "iterator traits requires object types.");

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
using iter_category_t   = typename iterator_traits<Iterator>::iterator_category;
template <typename Iterator>
using iter_value_t      = typename iterator_traits<Iterator>::value_type;
template <typename Iterator>
using iter_difference_t = typename iterator_traits<Iterator>::difference_type;
template <typename Iterator>
using iter_pointer_t    = typename iterator_traits<Iterator>::pointer;
template <typename Iterator>
using iter_reference_t  = typename iterator_traits<Iterator>::reference;


template <typename Iterator>
using get_iter_key_t = remove_const_t<typename iterator_traits<Iterator>::value_type::first_type>;
template <typename Iterator>
using get_iter_val_t = typename iterator_traits<Iterator>::value_type::second_type;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__
