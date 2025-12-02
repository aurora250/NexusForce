#ifndef MSTL_CORE_ASYNC_ASYNC_HPP__
#define MSTL_CORE_ASYNC_ASYNC_HPP__
#include "../memory/shared_ptr.hpp"
#include "../functional/call_wrapper.hpp"
#include "packaged_task.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Func, typename... Args>
MSTL_NODISCARD future<async_result_t<Func, Args...>>
async(launch policy, Func&& function, Args&&... args) {
    using Wrapper = _MSTL call_wrapper<Func, Args...>;
    using AsyncState = __future_base::async_state_impl<Wrapper, async_result_t<Func, Args...>>;
    using DeferredState = __future_base::deferred_state<Wrapper, async_result_t<Func, Args...>>;

    shared_ptr<__future_base::state_base> state;
    if ((policy & launch::async) == launch::async) {
        state = _MSTL make_shared<AsyncState>(
            _MSTL forward<Func>(function), _MSTL forward<Args>(args)...);
    }
    if (!state) {
        state = _MSTL make_shared<DeferredState>(
            _MSTL forward<Func>(function), _MSTL forward<Args>(args)...);
    }
    return _MSTL future<async_result_t<Func, Args...>>(_MSTL move(state));
}

template <typename Func, typename... Args>
MSTL_NODISCARD future<async_result_t<Func, Args...>>
async(Func&& function, Args&&... args) {
    return _MSTL async(launch::async | launch::deferred,
        _MSTL forward<Func>(function), _MSTL forward<Args>(args)...
        );
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ASYNC_HPP__
