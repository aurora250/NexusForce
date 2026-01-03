#ifndef MSTL_CORE_ASYNC_THREAD_HPP__
#define MSTL_CORE_ASYNC_THREAD_HPP__
#include "../functional/apply.hpp"
#include "../exception/terminate.hpp"
#include "../exception/exception.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include "../config/undef_cmacro.hpp"
#elif defined(MSTL_PLATFORM_LINUX__)
#include <pthread.h>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(thread_exception, memory_exception, "Thread Operation Failed.")


class MSTL_API thread {
public:
    class id : public ihashable<id> {
    private:
#ifdef MSTL_PLATFORM_WINDOWS__
        using native_id_type = ::DWORD;
#else
        using native_id_type = ::pthread_t;
#endif

        native_id_type id_{};

        friend class thread;

    public:
        id() noexcept = default;
        explicit id(const native_id_type id) noexcept : id_(id) {}

        MSTL_NODISCARD native_id_type native() const noexcept { return id_; }

        MSTL_NODISCARD size_t to_hash() const noexcept {
            return _MSTL FNV_hash(reinterpret_cast<const byte_t*>(&id_), sizeof(id));
        }

        MSTL_NODISCARD bool operator ==(const id& rhs) const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
            return id_ == rhs.id_;
#else
            return ::pthread_equal(id_, rhs.id_) != 0;
#endif
        }

        MSTL_NODISCARD bool operator !=(const id& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

private:
    enum STATE {
        NOT_A_THREAD,
        CREATED,
        JOINED,
        DETACHED
    };

    struct data_base {
        virtual ~data_base() = default;
        virtual void run() = 0;
    };

    template <typename Callable>
    struct thread_data final : data_base {
        Callable func_;

        template <typename F>
        explicit thread_data(F&& f) : func_(_MSTL forward<F>(f)) {}

        void run() override { func_(); }
    };

public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_handle_type = ::HANDLE;
#else
    using native_handle_type = ::pthread_t;
#endif

private:
    native_handle_type handle_{};
    id id_{};
    STATE state_ = NOT_A_THREAD;

#ifdef MSTL_PLATFORM_WINDOWS__
    static unsigned int WINAPI thread_entry(void* arg);
#else
    static void* thread_entry(void* arg);
#endif

    void start_thread_impl(void* args);

    template <typename F>
    void start_thread(F&& f) {
        using data_type = thread_data<decay_t<F>>;
        unique_ptr<data_type> data = _MSTL make_unique<data_type>(_MSTL forward<F>(f));
        this->start_thread_impl(data.get());
        data.release();
        state_ = CREATED;
    }

public:
    thread() noexcept = default;

    template <typename F, typename... Args, typename = enable_if_t<!is_same_v<decay_t<F>, thread>>>
    explicit thread(F&& f, Args&&... args) {
        auto func = [func = _MSTL move(f), args = _MSTL make_tuple(_MSTL forward<Args>(args)...)]() mutable {
            return _MSTL apply(_MSTL move(func), _MSTL move(args));
        };
        thread::start_thread(_MSTL move(func));
    }

    thread(const thread&) = delete;
    thread& operator =(const thread&) = delete;

    thread(thread&& other) noexcept;
    thread& operator =(thread&& other) noexcept;

    ~thread();

    MSTL_NODISCARD id get_id() const noexcept { return id_; }
    MSTL_NODISCARD native_handle_type native_handle() const noexcept { return handle_; }

    MSTL_NODISCARD bool joinable() const noexcept { return state_ == CREATED; }

    void join();
    void detach();

    void swap(thread& other) noexcept;

    static uint32_t hardware_concurrency() noexcept;
};


MSTL_BEGIN_THIS_THREAD__

MSTL_ALWAYS_INLINE_INLINE thread::id get_id() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return thread::id(::GetCurrentThreadId());
#else
    return thread::id(::pthread_self());
#endif
}

MSTL_ALWAYS_INLINE_INLINE void yield() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SwitchToThread();
#else
    ::sched_yield();
#endif
}

MSTL_ALWAYS_INLINE_INLINE void sleep_for_ms(uint32_t milliseconds) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::Sleep(milliseconds);
#else
    ::usleep(milliseconds * 1000);
#endif
}

MSTL_END_THIS_THREAD__

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_THREAD_HPP__
