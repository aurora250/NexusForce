#ifndef MSTL_CORE_ASYNC_FUTURE_BASE_HPP__
#define MSTL_CORE_ASYNC_FUTURE_BASE_HPP__
#include "../memory/aligned_buffer.hpp"
#include "../memory/weak_ptr.hpp"
#include "../memory/allocated_ptr.hpp"
#include "../memory/exception_ptr.hpp"
#include "../functional/function.hpp"
#include "atomic_futex.hpp"
#include "call_once.hpp"
#include "at_thread_exit.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(FutureError, exception, "Future Operation Failed.")


template <typename Result>
class future;

template <typename Result>
class shared_future;

template <typename Signature>
class packaged_task;

template <typename Result>
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
            throw_exception(value_exception("Invalid Function ErrorCode"));
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

constexpr launch operator&(launch left, launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) & static_cast<int>(right));
}

constexpr launch operator|(launch left, launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) | static_cast<int>(right));
}

constexpr launch operator^(launch left, launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) ^ static_cast<int>(right));
}

constexpr launch operator~(launch value) noexcept {
    return static_cast<launch>(~static_cast<int>(value));
}

constexpr launch& operator&=(launch& left, launch right) noexcept {
    return left = left & right;
}

constexpr launch& operator|=(launch& left, launch right) noexcept {
    return left = left | right;
}

constexpr launch& operator^=(launch& left, launch right) noexcept {
    return left = left ^ right;
}

template <typename Function, typename... Args>
using async_result_t = invoke_result_t<decay_t<Function>, decay_t<Args>...>;

template <typename Function, typename... Args>
future<async_result_t<Function, Args...>>
async(launch policy, Function&& function, Args&&... args);

template <typename Function, typename... Args>
future<async_result_t<Function, Args...>>
async(Function&& function, Args&&... args);


struct __future_base {
    struct result_base {
        exception_ptr error_ptr;

        result_base(const result_base&) = delete;
        result_base& operator=(const result_base&) = delete;

        virtual void destroy() = 0;

        struct Deleter {
            void operator()(result_base* result) const { result->destroy(); }
        };

    protected:
        result_base() : error_ptr(nullptr) {}
        virtual ~result_base() = default;
    };

    template <typename Result>
    using Ptr = unique_ptr<Result, result_base::Deleter>;

    template <typename Result>
    struct basic_result : result_base {
    private:
        aligned_buffer<Result> storage;
        bool initialized;

    public:
        typedef Result result_type;

        basic_result() noexcept : initialized() {}

        ~basic_result() override {
            if (initialized) {
                value().~Result();
            }
        }

        Result& value() noexcept {
            return *storage.ptr();
        }

        void set(const Result& result) {
            new (storage.addr()) Result(result);
            initialized = true;
        }

        void set(Result&& result) {
            new (storage.addr()) Result(_MSTL move(result));
            initialized = true;
        }

    private:
        void destroy() override { delete this; }
    };

    template <typename Result, typename Alloc>
    struct allocated_result final : basic_result<Result>, Alloc {
        using allocator_type = __allocator_traits_base::alloc_rebind_t<Alloc, allocated_result>;

        explicit allocated_result(const Alloc& alloc) :
            basic_result<Result>(), Alloc(alloc) { }

    private:
        void destroy() override {
            allocator_type alloc(*this);
            allocated_ptr<allocator_type> guard_ptr{ alloc, this };
            this->~AllocatedResult();
        }
    };

    template <typename Result, typename Allocator>
    static Ptr<allocated_result<Result, Allocator>>
    allocate_result(const Allocator& alloc) {
        using result_type = allocated_result<Result, Allocator>;
        typename result_type::allocator_type alloc2(alloc);
        auto guard = _MSTL allocate_guarded(alloc2);
        result_type* ptr = new(static_cast<void*>(guard.get())) result_type{alloc};
        guard = nullptr;
        return Ptr<result_type>(ptr);
    }

    template <typename Result, typename Type>
    static Ptr<basic_result<Result>>
    allocate_result(const _MSTL allocator<Type>&) {
        return Ptr<basic_result<Result>>(new basic_result<Result>);
    }

    class state_base {
    public:
        typedef Ptr<result_base> PtrType;

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
        future_status wait_for(const chrono::duration<Rep, Period>& relative_time) {
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

        template <typename Clock, typename Duration>
        future_status wait_until(const chrono::time_point<Clock, Duration>& absolute_time) {
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

        template <typename Result, typename Arg>
        struct setter;

        template <typename Result, typename Arg>
        struct setter<Result, Arg&> {
            static_assert(is_same_v<Result, Arg&> || is_same_v<const Result, Arg>, "Invalid specialisation");

            typename promise<Result>::PtrType operator()() const {
                promise_ptr->storage->set(*arg_ptr);
                return _MSTL move(promise_ptr->storage);
            }
            promise<Result>* promise_ptr;
            Arg* arg_ptr;
        };

        template <typename Result>
        struct setter<Result, Result&&> {
            typename promise<Result>::PtrType operator()() const {
                promise_ptr->storage->set(_MSTL move(*arg_ptr));
                return _MSTL move(promise_ptr->storage);
            }
            promise<Result>* promise_ptr;
            Result* arg_ptr;
        };

        template <typename Result>
        struct setter<Result, void> {
            static_assert(is_void_v<Result>, "Only used for promise<void>");

            typename promise<Result>::ptr_type operator()() const {
                return _MSTL move(promise_ptr->storage);
            }
            promise<Result>* promise_ptr;
        };

        struct exception_ptr_tag {};

        template <typename Result>
        struct setter<Result, exception_ptr_tag> {
            typename promise<Result>::ptr_type operator()() const {
                promise_ptr->storage->error_ptr = *exp_ptr;
                return _MSTL move(promise_ptr->storage);
            }
            promise<Result>* promise_ptr;
            exception_ptr* exp_ptr;
        };

        template <typename Result, typename Arg>
        MSTL_ALWAYS_INLINE static setter<Result, Arg&&>
        create_setter(promise<Result>* promise_ptr, Arg&& arg) noexcept {
            return setter<Result, Arg&&>{ promise_ptr, _MSTL addressof(arg) };
        }

        template <typename Result>
        MSTL_ALWAYS_INLINE static setter<Result, exception_ptr_tag>
        create_setter(exception_ptr& exception, promise<Result>* promise_ptr) noexcept {
            return setter<Result, exception_ptr_tag>{ promise_ptr, &exception };
        }

        template <typename Result>
        MSTL_ALWAYS_INLINE static setter<Result, void>
        create_setter(promise<Result>* promise_ptr) noexcept {
            return setter<Result, void>{ promise_ptr };
        }

        template <typename Type>
        static void check(const shared_ptr<Type>& ptr) {
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

    template <typename BoundFunction, typename Result = decltype(_MSTL declval<BoundFunction&>()())>
    class deferred_state;

    template <typename BoundFunction, typename Result = decltype(_MSTL declval<BoundFunction&>()())>
    class async_state_impl;

    template <typename Signature>
    class task_state_base;

    template <typename Function, typename Alloc, typename Signature>
    class task_state;

    template <typename ResultPtr, typename Function,
         typename Result = typename ResultPtr::element_type::result_type>
    struct task_setter;

    template <typename ResultPtr, typename BoundFunction>
    static task_setter<ResultPtr, BoundFunction>
    create_task_setter(ResultPtr& ptr, BoundFunction& call) {
        return { _MSTL addressof(ptr), _MSTL addressof(call) };
    }
};

template <typename Res>
struct __future_base::basic_result<Res&> : __future_base::result_base {
    typedef Res& result_type;

    basic_result() noexcept : value_ptr() { }

    void set(Res& result) noexcept {
        value_ptr = _MSTL addressof(result);
    }

    Res& get() noexcept {
        return *value_ptr;
    }

private:
    Res* value_ptr;
    void destroy() override { delete this; }
};

template<>
struct __future_base::basic_result<void> : __future_base::result_base {
    typedef void result_type;

private:
    void destroy() override { delete this; }
};


template <typename Result, typename Arg>
struct is_location_invariant<__future_base::state_base::setter<Result, Arg>> : true_type {};

template <typename ResultPtr, typename Function, typename Result>
struct is_location_invariant<__future_base::task_setter<ResultPtr, Function, Result>> : true_type {};


template <typename Result>
class __basic_future : public __future_base {
protected:
    typedef shared_ptr<state_base> state_type;
    typedef __future_base::basic_result<Result>& result_type;

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

    template<typename Rep, typename Period>
    future_status wait_for(const chrono::duration<Rep, Period>& relative_time) const {
        state_base::check(state_ptr);
        return state_ptr->wait_for(relative_time);
    }

    template<typename Clock, typename Duration>
    future_status wait_until(const chrono::time_point<Clock, Duration>& absolute_time) const {
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

    explicit __basic_future(const shared_future<Result>&) noexcept;
    explicit __basic_future(shared_future<Result>&&) noexcept;
    explicit __basic_future(future<Result>&&) noexcept;

    constexpr __basic_future() noexcept {}

    struct reset {
        explicit reset(__basic_future& future) noexcept : future_ref(future) { }
        ~reset() { future_ref.state_ptr.reset(); }
        __basic_future& future_ref;
    };
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_FUTURE_BASE_HPP__
