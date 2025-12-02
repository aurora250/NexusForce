#ifndef MSTL_CORE_ALGORITHM_ASYNC_HPP__
#define MSTL_CORE_ALGORITHM_ASYNC_HPP__
#include "iterator.hpp"
#include "../async/thread.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename BinaryOperation, typename Result,
    size_t Threshhold = 10, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
void reduce(Iterator first, Iterator last, BinaryOperation op, Result& res) {
    const size_t dist = _MSTL distance(first, last);
    if (dist <= Threshhold) {
        for (Iterator it = first; it != last; ++it)
            res = op(res, *it);
    }
    else {
        Iterator mid = _MSTL next(first, dist / 2);
        Result l_res = res, r_res = res;
        _MSTL thread r_thd(reduce<Iterator, BinaryOperation, Result, Threshhold>, mid, last, op, _MSTL ref(r_res));
        _MSTL reduce(first, mid, op, l_res);
        r_thd.join();
        res = op(l_res, r_res);
    }
}

template <typename Iterator, typename UnaryOperation, typename BinaryOp, typename Result,
    size_t Threshhold = 10, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
void transform_reduce(Iterator first, Iterator last, UnaryOperation transform, BinaryOp reduce, Result& res) {
    const size_t dist = _MSTL distance(first, last);
    if (dist <= Threshhold) {
        for (Iterator it = first; it != last; ++it)
            res = reduce(res, transform(*it));
    }
    else {
        Iterator mid = _MSTL next(first, dist / 2);
        Result l_res = _MSTL initialize<Result>(), r_res = _MSTL initialize<Result>();
        _MSTL thread r_thd(transform_reduce<Iterator, UnaryOperation, BinaryOp, Result, Threshhold>,
            mid, last, transform, reduce, _MSTL ref(r_res));
        _MSTL transform_reduce(first, mid, transform, reduce, l_res);
        r_thd.join();
        res = reduce(res, reduce(l_res, r_res));
    }
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_ASYNC_HPP__
