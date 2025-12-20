#ifndef MSTL_CORE_ASYNC_JTHREAD_HPP__
#define MSTL_CORE_ASYNC_JTHREAD_HPP__
#include "stop_token.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Callable, typename... Args>
constexpr bool pmf_expects_stop_token = false;

template <typename Callable, typename Object, typename... Args>
constexpr bool pmf_expects_stop_token<Callable, Object, Args...> = conjunction_v<
    is_member_function_pointer<remove_reference_t<Callable>>, 
    is_invocable<Callable, Object, stop_token, Args...>>;


class jthread {
public:
    using id = thread::id;
    using native_handle_type = thread::native_handle_type;

private:
    stop_source stop_source_{nostopstate};
    thread thread_{};

    template <typename Callable, typename Object, typename... Args,
    enable_if_t<pmf_expects_stop_token<Callable, Args...>, int> = 0>
    static thread create(stop_source& source, Callable func, Object&& object, Args&&... args) {
        return thread{func, forward<Object>(object), source.get_token(), forward<Args>(args)...};
    }

    template <typename Callable, typename... Args,
        enable_if_t<!pmf_expects_stop_token<Callable, Args...> &&
            is_invocable_v<decay_t<Callable>, stop_token, decay_t<Args>...>, int> = 0>
    static thread create(stop_source& source, Callable func, Args&&... args) {
        return thread{forward<Callable>(func), source.get_token(), forward<Args>(args)...};
    }

    template <typename Callable, typename... Args,
        enable_if_t<!pmf_expects_stop_token<Callable, Args...> &&
            !is_invocable_v<decay_t<Callable>, stop_token, decay_t<Args>...>, int> = 0>
    static thread create(stop_source& source, Callable func, Args&&... args) {
        static_assert(is_invocable_v<decay_t<Callable>, decay_t<Args>...>,
            "jthread arguments must be invocable after conversion to rvalues");
        return thread{forward<Callable>(func), forward<Args>(args)...};
    }

public:
    jthread() noexcept = default;

    template <typename Callable, typename... Args, typename = 
        enable_if_t<!is_same_v<remove_cvref_t<Callable>, jthread>>>
    explicit jthread(Callable&& func, Args&&... args)
    : thread_{this->create(stop_source_, _MSTL forward<Callable>(func), _MSTL forward<Args>(args)...)} {}

    jthread(const jthread&) = delete;
    jthread(jthread&&) noexcept = default;

    ~jthread() {
        if (joinable()) {
            request_stop();
            join();
        }
    }

    jthread& operator =(const jthread&) = delete;

    jthread& operator =(jthread&& other) noexcept {
        jthread(move(other)).swap(*this);
        return *this;
    }

    void swap(jthread& other) noexcept {
        _MSTL swap(stop_source_, other.stop_source_);
        _MSTL swap(thread_, other.thread_);
    }

    MSTL_NODISCARD bool joinable() const noexcept {
        return thread_.joinable();
    }

    void join() {
        thread_.join();
    }

    void detach() {
        thread_.detach();
    }

    MSTL_NODISCARD id get_id() const noexcept {
        return thread_.get_id();
    }

    MSTL_NODISCARD native_handle_type native_handle() const {
        return thread_.native_handle();
    }

    MSTL_NODISCARD static uint32_t hardware_concurrency() noexcept {
        return thread::hardware_concurrency();
    }

    MSTL_NODISCARD stop_source get_stop_source() noexcept {
        return stop_source_;
    }

    MSTL_NODISCARD stop_token get_stop_token() const noexcept {
        return stop_source_.get_token();
    }

    bool request_stop() noexcept {
        return stop_source_.request_stop();
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_JTHREAD_HPP__
