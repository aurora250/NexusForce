#ifndef MSTL_CORE_ASYNC_FUTURE_HPP__
#define MSTL_CORE_ASYNC_FUTURE_HPP__
#include "future_base.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Result>
class future : public __basic_future<Result> {
    static_assert(!is_array<Result>{}, "result type must not be an array");
    static_assert(!is_function<Result>{}, "result type must not be a function");
    static_assert(is_destructible<Result>{}, "result type must be destructible");

    friend class promise<Result>;

    template <typename> friend class packaged_task;

    template <typename Function, typename... Args>
    friend future<async_result_t<Function, Args...>>
    async(launch, Function&&, Args&&...);

    typedef __basic_future<Result> base_type;
    typedef typename base_type::state_type state_type;

    explicit future(const state_type& state) : base_type(state) {}

public:
    constexpr future() noexcept : base_type() {}
    future(future&& other) noexcept : base_type(_MSTL move(other)) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;

    future& operator=(future&& other) noexcept {
        future(_MSTL move(other)).swap(*this);
        return *this;
    }

    Result get() {
        typename base_type::reset reset_(*this);
        return _MSTL move(this->get_result().value());
    }

    shared_future<Result> share() noexcept;
};

template <typename Result>
class future<Result&> : public __basic_future<Result&> {
    friend class promise<Result&>;

    template <typename> friend class packaged_task;

    template <typename Function, typename... Args>
    friend future<async_result_t<Function, Args...>>
    async(launch, Function&&, Args&&...);

    typedef __basic_future<Result&> base_type;
    typedef typename base_type::state_type state_type;

    explicit future(const state_type& state) : base_type(state) {}

public:
    constexpr future() noexcept : base_type() {}
    future(future&& other) noexcept : base_type(_MSTL move(other)) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;

    future& operator=(future&& other) noexcept {
        future(_MSTL move(other)).swap(*this);
        return *this;
    }

    Result& get() {
        typename base_type::reset reset_(*this);
        return this->get_result().get();
    }

    shared_future<Result&> share() noexcept;
};

template <>
class future<void> : public __basic_future<void> {
    friend class promise<void>;

    template <typename> friend class packaged_task;

    template <typename Function, typename... Args>
    friend future<async_result_t<Function, Args...>>
    async(launch, Function&&, Args&&...);

    using base_type = __basic_future<void>;
    using state_type = base_type::state_type;

    explicit future(const state_type& state) : base_type(state) {}

public:
    constexpr future() noexcept {}
    future(future&& other) noexcept : base_type(_MSTL move(other)) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;

    future& operator=(future&& other) noexcept {
        future(_MSTL move(other)).swap(*this);
        return *this;
    }

    void get() {
        base_type::reset reset(*this);
        this->get_result();
    }

    shared_future<void> share() noexcept;
};

template <typename Result>
class shared_future : public __basic_future<Result> {
    static_assert(!is_array<Result>{}, "result type must not be an array");
    static_assert(!is_function<Result>{}, "result type must not be a function");
    static_assert(is_destructible<Result>{}, "result type must be destructible");

    typedef __basic_future<Result> base_type;

public:
    constexpr shared_future() noexcept : base_type() {}
    shared_future(const shared_future& other) noexcept : base_type(other) {}
    shared_future(future<Result>&& other) noexcept : base_type(_MSTL move(other)) {}
    shared_future(shared_future&& other) noexcept : base_type(_MSTL move(other)) {}

    shared_future& operator=(const shared_future& other) noexcept {
        shared_future(other).swap(*this);
        return *this;
    }

    shared_future& operator=(shared_future&& other) noexcept {
        shared_future(_MSTL move(other)).swap(*this);
        return *this;
    }

    const Result& get() const {
        return this->get_result().value();
    }
};

template <typename Result>
class shared_future<Result&> : public __basic_future<Result&> {
    typedef __basic_future<Result&> base_type;

public:
    constexpr shared_future() noexcept : base_type() {}
    shared_future(const shared_future& other) : base_type(other) {}
    shared_future(future<Result&>&& other) noexcept : base_type(_MSTL move(other)) {}
    shared_future(shared_future&& other) noexcept : base_type(_MSTL move(other)) {}

    shared_future& operator=(const shared_future& other) {
        shared_future(other).swap(*this);
        return *this;
    }

    shared_future& operator=(shared_future&& other) noexcept {
        shared_future(_MSTL move(other)).swap(*this);
        return *this;
    }

    Result& get() const {
        return this->get_result().get();
    }
};

template <>
class shared_future<void> : public __basic_future<void> {
    typedef __basic_future<void> base_type;

public:
    constexpr shared_future() noexcept {}
    shared_future(const shared_future& other) : base_type(other) {}
    shared_future(future<void>&& other) noexcept : base_type(_MSTL move(other)) {}
    shared_future(shared_future&& other) noexcept : base_type(_MSTL move(other)) {}

    shared_future& operator=(const shared_future& other) {
        shared_future(other).swap(*this);
        return *this;
    }

    shared_future& operator=(shared_future&& other) noexcept {
        shared_future(_MSTL move(other)).swap(*this);
        return *this;
    }

    void get() const {
        this->get_result();
    }
};


template <typename Result>
__basic_future<Result>::__basic_future(const shared_future<Result>& other) noexcept
    : state_ptr(other.state_ptr) {}

template <typename Result>
__basic_future<Result>::__basic_future(shared_future<Result>&& other) noexcept
    : state_ptr(_MSTL move(other.state_ptr)) {}

template <typename Result>
__basic_future<Result>::__basic_future(future<Result>&& other) noexcept
    : state_ptr(_MSTL move(other.state_ptr)) {}


template <typename Result>
shared_future<Result> future<Result>::share() noexcept {
    return shared_future<Result>(_MSTL move(*this));
}

template <typename Result>
shared_future<Result&> future<Result&>::share() noexcept {
    return shared_future<Result&>(_MSTL move(*this));
}

inline shared_future<void> future<void>::share() noexcept {
    return shared_future<void>(_MSTL move(*this));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_FUTURE_HPP__
