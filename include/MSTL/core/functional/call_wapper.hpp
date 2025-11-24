#ifndef MSTL_CORE_FUNCTIONAL_CALL_WAPPER_HPP__
#define MSTL_CORE_FUNCTIONAL_CALL_WAPPER_HPP__
#include "../utility/integer_sequence.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Tuple>
struct invoker {
private:
    template <typename>
    struct result_t;

    template <typename _Fn, typename... _Args>
    struct result_t<tuple<_Fn, _Args...>>
        : _INNER __invoke_result_aux<_Fn, _Args...> {};

    Tuple tup_;

public:
    template <typename... Args>
    explicit invoker(Args&&... args)
    : tup_(_MSTL forward<Args>(args)...) {}

    template <size_t... Index>
    typename result_t<Tuple>::type invoke(index_tuple<Index...>) {
        return _MSTL invoke(_MSTL get<Index>(_MSTL move(tup_))...);
    }

    typename result_t<Tuple>::type operator ()() {
        using Indices = build_index_tuple_t<tuple_size_v<Tuple>>;
        return this->invoke(Indices());
    }
};

template <typename... T>
using call_wrapper = invoker<tuple<decay_t<T>...>>;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_CALL_WAPPER_HPP__
