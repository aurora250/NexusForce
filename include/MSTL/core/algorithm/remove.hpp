#ifndef MSTL_CORE_ALGORITHM_ERASE_HPP__
#define MSTL_CORE_ALGORITHM_ERASE_HPP__
#include "search.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator1, typename Iterator2, typename T,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 remove_copy(Iterator1 first, Iterator1 last, Iterator2 result, const T& value) {
	for (; first != last; ++first) {
		if (*first != value) {
			*result = *first;
			++result;
		}
	}
	return result;
}

template <typename Iterator1, typename Iterator2, typename Predicate,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 remove_copy_if(Iterator1 first, Iterator1 last, Iterator2 result, Predicate pred) {
	for (; first != last; ++first) {
		if (!pred(*first)) {
			*result = *first;
			++result;
		}
	}
	return result;
}

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator remove(Iterator first, Iterator last, const T& value) {
	first = _MSTL find(first, last, value);
	Iterator next = first;
	return first == last ? first : _MSTL remove_copy(++next, last, first, value);
}

template <typename Iterator, typename Predicate, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator remove_if(Iterator first, Iterator last, Predicate pred) {
	first = _MSTL find_if(first, last, pred);
	Iterator next = first;
	return first == last ? first : _MSTL remove_copy_if(++next, last, first, pred);
}

template <typename Container, typename U,
	enable_if_t<is_same_v<typename Container::value_type, U>, int> = 0>
constexpr size_t erase(Container& cont, const U& value) {
	const auto old_size = cont.size();
	const auto end = cont.end();
	auto removed = _MSTL remove_if(cont.begin(), end,
		[&value](const auto& iter) { return *iter == value; });
	cont.erase(removed, end);
	return old_size - cont.size();
}

template <typename Container, typename Predicate>
constexpr size_t erase_if(Container& cont, Predicate pred) {
	const auto old_size = cont.size();
	const auto end = cont.end();
	auto removed = _MSTL remove_if(cont.begin(), end,
		[ref_pred = _MSTL ref(pred)](const auto& iter) { return ref_pred(*iter); });
	cont.erase(removed, end);
	return old_size - cont.size();
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_ERASE_HPP__
