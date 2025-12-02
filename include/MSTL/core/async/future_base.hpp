#ifndef MSTL_CORE_ASYNC_FUTURE_BASE_HPP__
#define MSTL_CORE_ASYNC_FUTURE_BASE_HPP__
#include "../memory/aligned_buffer.hpp"
#include "../memory/weak_ptr.hpp"
#include "../memory/allocated_ptr.hpp"
#include "../exception/exception_ptr.hpp"
#include "../functional/function.hpp"
#include "atomic_futex.hpp"
#include "call_once.hpp"
#include "at_thread_exit.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(FutureError, exception, "Future Operation Failed.")


template <typename Res>
class future;

template <typename Res>
class shared_future;

template <typename Sign>
class packaged_task;

template <typename Res>
class promise;


enum class future_errc {
    future_already_retrieved = 1,
    promise_already_satisfied,
    no_state,
    broken_promise
};

constexpr const char* future_errc_to_string(const future_errc code) {
    switch (code) {
        case future_errc::future_already_retrieved:
            return "future_already_retrieved";
        case future_errc::promise_already_satisfied:
            return "promise_already_satisfied";
        case future_errc::no_state:
            return "no_state";
        case future_errc::broken_promise:
            return "broken_promise";
        default:
            throw_exception(value_exception("Invalid Func ErrorCode"));
    }
    return "";
}


enum class future_status {
    ready,
    timeout,
    deferred
};


enum class launch {
    async = 1,
    deferred = 2
};

constexpr launch operator &(const launch left, const launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) & static_cast<int>(right));
}
constexpr launch operator |(const launch left, const launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) | static_cast<int>(right));
}
constexpr launch operator ^(const launch left, const launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) ^ static_cast<int>(right));
}
constexpr launch operator ~(const launch value) noexcept {
    return static_cast<launch>(~static_cast<int>(value));
}

constexpr launch& operator &=(launch& left, const launch right) noexcept {
    return left = left & right;
}
constexpr launch& operator |=(launch& left, const launch right) noexcept {
    return left = left | right;
}
constexpr launch& operator ^=(launch& left, const launch right) noexcept {
    return left = left ^ right;
}

template <typename Func, typename... Args>
using async_result_t = invoke_result_t<decay_t<Func>, decay_t<Args>...>;

template <typename Func, typename... Args>
future<async_result_t<Func, Args...>>
async(launch policy, Func&& function, Args&&... args);

template <typename Func, typename... Args>
future<async_result_t<Func, Args...>>
async(Func&& function, Args&&... args);


struct __future_base {
    struct result_base {
        exception_ptr error_ptr;

    protected:
        result_base() : error_ptr(nullptr) {}
        virtual ~result_base() = default;

    public:
        result_base(const result_base&) = delete;
        result_base& operator=(const result_base&) = delete;

        virtual void destroy() = 0;

        struct Deleter {
            void operator()(result_base* result) const { result->destroy(); }
        };
    };

    template <typename Res>
    using Ptr = unique_ptr<Res, result_base::Deleter>;

    template <typename Res>
    struct basic_result : result_base {
    private:
        aligned_buffer<Res> storage;
        bool initialized;

        void destroy() override { delete this; }

    public:
        using result_type = Res;

        basic_result() noexcept : initialized() {}

        ~basic_result() override {
            if (initialized) {
                value().~Res();
            }
        }

        Res& value() noexcept {
            return *storage.ptr();
        }

        void set(const Res& result) {
            new (storage.addr()) Res(result);
            initialized = true;
        }

        void set(Res&& result) {
            new (storage.addr()) Res(_MSTL move(result));
            initialized = true;
        }
    };

    template <typename Res, typename Alloc>
    struct allocated_result final : basic_result<Res>, Alloc {
        using allocator_type = __allocator_traits_base::alloc_rebind_t<Alloc, allocated_result>;

        explicit allocated_result(const Alloc& alloc) :
            basic_result<Res>(), Alloc(alloc) { }

    private:
        void destroy() override {
            allocator_type alloc(*this);
            allocated_ptr<allocator_type> guard_ptr{ alloc, this };
            this->~AllocatedResult();
        }
    };

    template <typename Res, typename Alloc>
    static Ptr<allocated_result<Res, Alloc>>
    allocate_result(const Alloc& alloc) {
        using result_type = allocated_result<Res, Alloc>;
        typename result_type::allocator_type alloc2(alloc);
        auto guard = _MSTL allocate_guarded(alloc2);
        result_type* ptr = new(static_cast<void*>(guard.get())) result_type{alloc};
        guard = nullptr;
        return Ptr<result_type>(ptr);
    }

    template <typename Res, typename T>
    static Ptr<basic_result<Res>>
    allocate_result(const _MSTL allocator<T>&) {
        return Ptr<basic_result<Res>>(new basic_result<Res>);
    }

    class state_base {
    public:
        using PtrType = Ptr<result_base>;

    private:
        enum status : unsigned {
            not_ready,
            ready
        };

        PtrType result_ptr;
        atomic_futex<> status;
        atomic_flag retrieved;
        once_flag flag;

    public:
        state_base() noexcept : status(status::not_ready), retrieved(false) {
            retrieved.clear();
        }

        state_base(const state_base&) = delete;
        state_base& operator=(const state_base&) = delete;
        virtual ~state_base() = default;

        result_base& wait() {
            complete_async();
            status.load_when_equal(status::ready, memory_order_acquire);
            return *result_ptr;
        }

        template <typename Rep, typename Period>
        future_status wait_for(const _MSTL_CHRONO duration<Rep, Period>& relative_time) {
            if (status.load(memory_order_acquire) == status::ready) {
                return future_status::ready;
            }

            if (is_deferred_future()) {
                return future_status::deferred;
            }

            if (relative_time > relative_time.zero() &&
                status.load_when_equal_for(status::ready, memory_order_acquire, relative_time)) {
                complete_async();
                return future_status::ready;
            }
            return future_status::timeout;
        }

        template <typename Clock, typename Dur>
        future_status wait_until(const _MSTL_CHRONO time_point<Clock, Dur>& absolute_time) {
            static_assert(is_clock_v<Clock>);
            if (status.load(memory_order_acquire) == status::ready) {
                return future_status::ready;
            }

            if (is_deferred_future()) {
                return future_status::deferred;
            }

            if (status.load_when_equal_until(status::ready, memory_order_acquire, absolute_time)) {
                complete_async();
                return future_status::ready;
            }
            return future_status::timeout;
        }

        template <typename Callable>
        void set_result(Callable&& result_func, const bool ignore_failure = false) {
            bool did_set = false;
            function<PtrType()> func = _MSTL forward<Callable>(result_func);
            call_once(flag, &state_base::do_set, this,
                _MSTL addressof(func), _MSTL addressof(did_set));
            if (did_set) {
                status.store_notify_all(status::ready, memory_order_release);
            } else if (!ignore_failure) {
                throw_exception(FutureError(future_errc_to_string(future_errc::promise_already_satisfied)));
            }
        }

        template <typename Callable>
        void set_delayed_result(Callable&& result_func, weak_ptr<state_base> self) {
            bool did_set = false;
            unique_ptr<make_ready> mr = make_unique<make_ready>();
            function<PtrType()> func = _MSTL forward<Callable>(result_func);
            call_once(flag, &state_base::do_set, this,
                _MSTL addressof(func), _MSTL addressof(did_set));
            if (!did_set) {
                throw_exception(FutureError(future_errc_to_string(future_errc::promise_already_satisfied)));
            }
            mr->shared_state = _MSTL move(self);
            mr->set();
            mr.release();
        }

        void break_promise(PtrType result) {
            if (static_cast<bool>(result)) {
                result->error_ptr = make_exception_ptr(
                    FutureError(future_errc_to_string(future_errc::broken_promise)));
                result_ptr.swap(result);
                status.store_notify_all(status::ready, memory_order_release);
            }
        }

        void set_retrieved_flag() {
            if (retrieved.test_and_set(memory_order_acquire)) {
                throw_exception(FutureError(future_errc_to_string(future_errc::future_already_retrieved)));
            }
        }

        template <typename Res, typename Arg>
        struct setter;

        template <typename Res, typename Arg>
        struct setter<Res, Arg&> {
            static_assert(is_same_v<Res, Arg&> || is_same_v<const Res, Arg>, "Invalid specialisation");

            typename promise<Res>::PtrType operator()() const {
                promise_ptr->storage->set(*arg_ptr);
                return _MSTL move(promise_ptr->storage);
            }
            promise<Res>* promise_ptr;
            Arg* arg_ptr;
        };

        template <typename Res>
        struct setter<Res, Res&&> {
            typename promise<Res>::PtrType operator()() const {
                promise_ptr->storage->set(_MSTL move(*arg_ptr));
                return _MSTL move(promise_ptr->storage);
            }
            promise<Res>* promise_ptr;
            Res* arg_ptr;
        };

        template <typename Res>
        struct setter<Res, void> {
            static_assert(is_void_v<Res>, "Only used for promise<void>");

            typename promise<Res>::ptr_type operator()() const {
                return _MSTL move(promise_ptr->storage);
            }
            promise<Res>* promise_ptr;
        };

        struct exception_ptr_tag {};

        template <typename Res>
        struct setter<Res, exception_ptr_tag> {
            typename promise<Res>::ptr_type operator()() const {
                promise_ptr->storage->error_ptr = *exp_ptr;
                return _MSTL move(promise_ptr->storage);
            }
            promise<Res>* promise_ptr;
            exception_ptr* exp_ptr;
        };

        template <typename Res, typename Arg>
        MSTL_ALWAYS_INLINE static setter<Res, Arg&&>
        create_setter(promise<Res>* promise_ptr, Arg&& arg) noexcept {
            return setter<Res, Arg&&>{ promise_ptr, _MSTL addressof(arg) };
        }

        template <typename Res>
        MSTL_ALWAYS_INLINE static setter<Res, exception_ptr_tag>
        create_setter(exception_ptr& exception, promise<Res>* promise_ptr) noexcept {
            return setter<Res, exception_ptr_tag>{ promise_ptr, &exception };
        }

        template <typename Res>
        MSTL_ALWAYS_INLINE static setter<Res, void>
        create_setter(promise<Res>* promise_ptr) noexcept {
            return setter<Res, void>{ promise_ptr };
        }

        template <typename T>
        static void check(const shared_ptr<T>& ptr) {
            if (!static_cast<bool>(ptr)) {
                throw_exception(FutureError(future_errc_to_string(future_errc::no_state)));
            }
        }

    private:
        void do_set(function<PtrType()>* func, bool* did_set) {
            PtrType result = (*func)();
            *did_set = true;
            result_ptr.swap(result);
        }

        virtual void complete_async() {}
        virtual bool is_deferred_future() const { return false; }

        struct make_ready final : at_thread_exit_elt {
            weak_ptr<state_base> shared_state;
            static void run(void* ptr) {
                const auto self = static_cast<make_ready*>(ptr);
                const auto state = self->shared_state.lock();
                if (state) {
                    state->status.store_notify_all(status::ready, memory_order_release);
                }
                delete self;
            }

            void set() {
                at_thread_exit_register(this, &make_ready::run);
            }
        };
    };

    class async_state_common;

    template <typename BoundFunc, typename Res = decltype(_MSTL declval<BoundFunc&>()())>
    class deferred_state;

    template <typename BoundFunc, typename Res = decltype(_MSTL declval<BoundFunc&>()())>
    class async_state_impl;

    template <typename Sign>
    class task_state_base;

    template <typename Func, typename Alloc, typename Sign>
    class task_state;

    template <typename ResPtr, typename Func,
         typename Res = typename ResPtr::element_type::result_type>
    struct task_setter;

    template <typename ResPtr, typename BoundFunc>
    static task_setter<ResPtr, BoundFunc>
    create_task_setter(ResPtr& ptr, BoundFunc& call) {
        return { _MSTL addressof(ptr), _MSTL addressof(call) };
    }
};

template <typename Res>
struct __future_base::basic_result<Res&> : __future_base::result_base {
private:
    Res* value_ptr;

    void destroy() override { delete this; }

public:
    using result_type = Res&;

    basic_result() noexcept : value_ptr() { }

    void set(Res& result) noexcept {
        value_ptr = _MSTL addressof(result);
    }

    Res& get() noexcept {
        return *value_ptr;
    }
};

template<>
struct __future_base::basic_result<void> : __future_base::result_base {
    using result_type = void;

private:
    void destroy() override { delete this; }
};


template <typename Res, typename Arg>
struct is_location_invariant<__future_base::state_base::setter<Res, Arg>> : true_type {};

template <typename ResPtr, typename Func, typename Res>
struct is_location_invariant<__future_base::task_setter<ResPtr, Func, Res>> : true_type {};


template <typename Res>
class __basic_future : public __future_base {
protected:
    using state_type = shared_ptr<state_base>;
    using result_type = __future_base::basic_result<Res>&;

private:
    state_type state_ptr;

public:
    __basic_future(const __basic_future&) = delete;
    __basic_future& operator=(const __basic_future&) = delete;

    bool valid() const noexcept {
        return static_cast<bool>(state_ptr);
    }

    void wait() const {
        state_base::check(state_ptr);
        state_ptr->wait();
    }

    template <typename Rep, typename Period>
    future_status wait_for(const _MSTL_CHRONO duration<Rep, Period>& relative_time) const {
        state_base::check(state_ptr);
        return state_ptr->wait_for(relative_time);
    }

    template <typename Clock, typename Dur>
    future_status wait_until(const _MSTL_CHRONO time_point<Clock, Dur>& absolute_time) const {
        state_base::check(state_ptr);
        return state_ptr->wait_until(absolute_time);
    }

protected:
    result_type get_result() const {
        state_base::check(state_ptr);
        result_base& result = state_ptr->wait();
        if (!(result.error_ptr == nullptr)) {
            rethrow_exception(result.error_ptr);
        }
        return static_cast<result_type>(result);
    }

    void swap(__basic_future& other) noexcept {
        state_ptr.swap(other.state_ptr);
    }

    explicit __basic_future(const state_type& state) : state_ptr(state) {
        state_base::check(state_ptr);
        state_ptr->set_retrieved_flag();
    }

    explicit __basic_future(const shared_future<Res>&) noexcept;
    explicit __basic_future(shared_future<Res>&&) noexcept;
    explicit __basic_future(future<Res>&&) noexcept;

    constexpr __basic_future() noexcept {}

    struct reset {
        explicit reset(__basic_future& future) noexcept : future_ref(future) { }
        ~reset() { future_ref.state_ptr.reset(); }
        __basic_future& future_ref;
    };
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_FUTURE_BASE_HPP__
