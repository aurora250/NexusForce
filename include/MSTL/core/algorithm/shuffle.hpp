#ifndef MSTL_CORE_ALGORITHM_SHUFFLE_HPP__
#define MSTL_CORE_ALGORITHM_SHUFFLE_HPP__
#include "../numeric/random.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void shuffle(Iterator first, Iterator last) {
	if (first == last) return;
    for (Iterator i = _MSTL next(first); i != last; ++i) {
        auto distance = _MSTL distance(first, i);
        Iterator j = _MSTL next(first, random_lcd::next_int(0, static_cast<int>(distance)));
        _MSTL iter_swap(i, j);
    }
}

template <typename Iterator, typename Generator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void shuffle(Iterator first, Iterator last, Generator& rand) {
	if (first == last) return;
	for (Iterator i = _MSTL next(first); i != last; ++i) {
		Iterator j = _MSTL next(first, rand(i - first + 1));
		_MSTL iter_swap(i, j);
	}
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_SHUFFLE_HPP__
