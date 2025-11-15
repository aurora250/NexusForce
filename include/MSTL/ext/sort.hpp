#ifndef MSTL_SORT_HPP__
#define MSTL_SORT_HPP__
#include "MSTL/core/algo.hpp"
#include "leonardo_heap.hpp"
MSTL_BEGIN_NAMESPACE__

// bubble sort : Ot(N)~(N^2) Om(1) stable
template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void bubble_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    auto revend = _MSTL make_reverse_iterator(first);
    auto revstart = _MSTL make_reverse_iterator(--last);
    for (auto iter = revstart; iter != revend; ++iter) {
        bool not_finished = false;
        Iterator curend = iter.base();
        for (Iterator it = first; it != curend; ++it) {
            Iterator next = it;
            ++next;
            if (comp(*next, *it)) {
                _MSTL iter_swap(it, next);
                not_finished = true;
            }
        }
        if (!not_finished) break;
    }
}

template <typename Iterator>
MSTL_CONSTEXPR20 void bubble_sort(Iterator first, Iterator last) {
    return _MSTL bubble_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

// cocktail sort : Ot(N)~(N^2) Om(1) stable
template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void cocktail_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    bool swapped = true;
    Iterator left = first;
    Iterator right = last;
    --right;
    while (swapped) {
        swapped = false;
        for (Iterator i = left; i != right; ++i) {
            Iterator next = i;
            ++next;
            if (comp(*next, *i)) {
                _MSTL iter_swap(i, next);
                swapped = true;
            }
        }
        if (!swapped) break;
        --right;
        swapped = false;
        for (Iterator i = right; i != left; --i) {
            Iterator prev = i;
            --prev;
            if (comp(*i, *prev)) {
                _MSTL iter_swap(i, prev);
                swapped = true;
            }
        }
        ++left;
    }
}

template <typename Iterator>
MSTL_CONSTEXPR20 void cocktail_sort(Iterator first, Iterator last) {
    return _MSTL cocktail_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

// select sort : Ot(N^2) Om(1) unstable 
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void select_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    Iterator min;
    for (Iterator i = first; i != last; ++i) {
        min = i;
        for (Iterator j = i + 1; j != last; ++j) {
            if (comp(*j, *min)) {
                min = j;
            }
        }
        _MSTL iter_swap(i, min);
    }
}

template <typename Iterator>
MSTL_CONSTEXPR20 void select_sort(Iterator first, Iterator last) {
    return _MSTL select_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

// shell sort : Ot(NlogN)~(N^2) Om(1) unstable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void shell_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    using Distance = iter_difference_t<Iterator>;
    using T = iter_value_t<Iterator>;
    Distance dist = _MSTL distance(first, last);
    for (Distance gap = dist / 2; gap > 0; gap /= 2) {
        for (Iterator i = first + gap; i < last; ++i) {
            T temp = *i;
            Iterator j;
            for (j = i; j >= first + gap && comp(temp, *(j - gap)); j -= gap)
                *j = *(j - gap);
            *j = temp;
        }
    }
}

template <typename Iterator>
MSTL_CONSTEXPR20 void shell_sort(Iterator first, Iterator last) {
    return _MSTL shell_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

// counting sort : Ot(N + k) Om(k) stable
template <typename Iterator, typename Compare, typename IndexMapper, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void counting_sort(Iterator first, Iterator last, Compare comp, IndexMapper mapper) {
    if (first == last) return;
    using T = typename iterator_traits<Iterator>::value_type;
    auto min_max = _MSTL minmax_element(first, last, comp);
    auto min_val = mapper(*min_max.first);
    auto max_val = mapper(*min_max.second);
    const auto range = static_cast<size_t>(max_val - min_val + 1);
    vector<int> count(range, 0);

    for (Iterator it = first; it != last; ++it) {
        auto value = mapper(*it);
        if (value < min_val || value > max_val) {
            Exception(StopIterator("element out of range for counting sort."));
        }
        ++count[static_cast<size_t>(value - min_val)];
    }

    for (size_t i = 1; i < count.size(); ++i)
        count[i] += count[i - 1];

    vector<T> sorted(_MSTL distance(first, last));
    auto bound = _MSTL make_reverse_iterator(first);

    for (auto rit = _MSTL make_reverse_iterator(last); rit != bound; ++rit) {
        auto value = mapper(*rit);
        const auto index = static_cast<size_t>(value - min_val);
        size_t position = --count[index];
        sorted[position] = *rit;
    }
    _MSTL copy(sorted.begin(), sorted.end(), first);
}

template <typename Iterator>
MSTL_CONSTEXPR20 void counting_sort(Iterator first, Iterator last) {
    _MSTL counting_sort(first, last, 
        _MSTL less<iter_value_t<Iterator>>(), _MSTL identity<iter_value_t<Iterator>>());
}

template <typename Iterator, enable_if_t<
    is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void bucket_sort_less(Iterator first, Iterator last) {
    using T = iter_value_t<Iterator>;
    pair<Iterator, Iterator> min_max = _MSTL minmax_element(first, last);
    T min_val = *min_max.first;
    T max_val = *min_max.second;
    T range = max_val - min_val + 1;
    vector<size_t> bucket(range, 0);

    for (Iterator it = first; it != last; ++it) {
        ++bucket[*it - min_val];
    }

    Iterator index = first;

    for (size_t i = 0; i < bucket.size(); ++i) {
        while (bucket[i] > 0) {
            *index++ = static_cast<T>(i + min_val);
            bucket[i]--;
        }
    }
}

template <typename Iterator, enable_if_t<
    is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void bucket_sort_greater(Iterator first, Iterator last) {
    using T = iter_value_t<Iterator>;
    pair<Iterator, Iterator> min_max = _MSTL minmax_element(first, last);
    T min_val = *min_max.first;
    T max_val = *min_max.second;
    T range = max_val - min_val + 1;
    vector<size_t> bucket(range, 0);

    for (Iterator it = first; it != last; ++it) {
        ++bucket[*it - min_val];
    }

    Iterator index = first;

    for (size_t i = bucket.size(); i-- > 0; ) {
        while (bucket[i] > 0) {
            *index++ = static_cast<T>(i + min_val);
            bucket[i]--;
        }
    }
}

// bucket sort : Ot(N + k)~(N^2) Om(N + k) stable
template<typename Iterator>
MSTL_CONSTEXPR20 void bucket_sort(Iterator first, Iterator last) {
    _MSTL bucket_sort_less(first, last);
}

MSTL_BEGIN_INNER__

template <typename Iterator>
int __max_bit_aux(Iterator first, Iterator last) {
    auto max_num = *_MSTL max_element(first, last,
        [](const auto& a, const auto& b) -> bool { return a < b; });
    int p = 0;
    while (max_num > 0) {
        p++;
        max_num = max_num / 10;
    }
    return p;
}

template <typename T>
int __get_number_aux(T num, T d) {
    int p = 1;
    for (T i = 1; i < d; ++i)
        p *= 10;
    return num / p % 10;
}

MSTL_END_INNER__

template <typename Iterator, typename Mapper, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void radix_sort_less(Iterator first, Iterator last, Mapper mapper) {
    if (first == last) return;
    using Distance = typename iterator_traits<Iterator>::difference_type;
    using T = typename iterator_traits<Iterator>::value_type;
    using Mapped = remove_reference_t<decltype(mapper(*first))>;

    Distance length = _MSTL distance(first, last);
    vector<Mapped> mapped_values(length);
    vector<T> bucket(length);
    vector<int> count(10);
    Iterator it = first;

    FOR_EACH(value, mapped_values) {
        *value = mapper(*it++);
    }

    for (int d = 1; d <= _INNER __max_bit_aux(mapped_values.begin(), mapped_values.end()); ++d) {
        _MSTL fill(count.begin(), count.end(), 0);
        FOR_EACH(num, mapped_values) {
            ++count[_INNER __get_number_aux(*num, d)];
        }

        for (size_t i = 1; i < count.size(); ++i) {
            count[i] += count[i - 1];
        }
        for (auto iter = mapped_values.rbegin(); iter != mapped_values.rend(); ++iter) {
            const int k = _INNER __get_number_aux(*iter, d);
            bucket[--count[k]] = *(first + _MSTL distance(mapped_values.begin(), iter.base() - 1));
        }

        it = first;
        FOR_EACH(value, bucket) {
            *it++ = *value;
        }
        _MSTL transform(bucket.begin(), bucket.end(), mapped_values.begin(), mapper);
    }
}

template <typename Iterator, typename Mapper, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void radix_sort_greater(Iterator first, Iterator last, Mapper mapper) {
    if (first == last) return;
    using Mapped = remove_cvref_t<decltype(mapper(*first))>;

    iter_difference_t<Iterator> length = _MSTL distance(first, last);
    vector<Mapped> mapped_values(length);
    vector<iter_value_t<Iterator>> bucket(length);
    vector<int> count(10);
    Iterator it = first;

    FOR_EACH(value, mapped_values) {
        *value = mapper(*it++);
    }
    for (int d = 1; d <= _INNER __max_bit_aux(mapped_values.begin(), mapped_values.end()); ++d) {
        _MSTL fill(count.begin(), count.end(), 0);
        FOR_EACH(num, mapped_values) {
            ++count[_INNER __get_number_aux(*num, d)];
        }

        for (size_t i = count.size() - 1; i > 0; --i) {
            count[i - 1] += count[i];
        }

        for (auto iter = mapped_values.rbegin(); iter != mapped_values.rend(); ++iter) {
            const int k = _INNER __get_number_aux(*iter, d);
            bucket[--count[k]] = *(first + _MSTL distance(mapped_values.begin(), iter.base() - 1));
        }

        it = first;
        FOR_EACH(value, bucket) {
            *it++ = *value;
        }
        _MSTL transform(bucket.begin(), bucket.end(), mapped_values.begin(), mapper);
    }
}

// radix sort : Ot(d(n + k)) Om(N + k) stable
template <typename Iterator, typename Mapper = _MSTL identity<iter_value_t<Iterator>>>
MSTL_CONSTEXPR20 void radix_sort(Iterator first, Iterator last, Mapper mapper = Mapper()) {
    _MSTL radix_sort_less(first, last, mapper);
}

// smooth sort : Ot(NlogN) Om(1) unstable
template <typename Iterator>
MSTL_CONSTEXPR20 void smooth_sort(Iterator first, Iterator last) {
    _MSTL make_leonardo_heap(first, last);
    _MSTL sort_leonardo_heap(first, last);
}

// tim sort : Ot(NlogN) Om(N) stable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void tim_sort(Iterator first, Iterator last, Compare comp) {
    constexpr int MIN_MERGE = 32;
    iter_difference_t<Iterator> n = _MSTL distance(first, last);
    for (Iterator i = first; i < last; i += MIN_MERGE) {
        Iterator end = _MSTL min(i + MIN_MERGE, last);
        _MSTL insertion_sort(i, end, comp);
    }
    for (int size = MIN_MERGE; size < n; size *= 2) {
        for (Iterator left = first; left < last; left += 2 * size) {
            Iterator mid = left + size;
            Iterator right = _MSTL min(left + 2 * size, last);
            if (mid < right) {
                _MSTL inplace_merge(left, mid, right, comp);
            }
        }
    }
}

template <typename Iterator>
MSTL_CONSTEXPR20 void tim_sort(Iterator first, Iterator last) {
    return _MSTL tim_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

// monkey sort : Ot-avg((N + 1)!) Om(1) unstable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void monkey_sort(Iterator first, Iterator last, Compare comp) {
    while (!_MSTL is_sorted(first, last, comp)) {
        _MSTL shuffle(first, last);
    }
}

template <typename Iterator>
void monkey_sort(Iterator first, Iterator last) {
    return _MSTL monkey_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_SORT_HPP__
