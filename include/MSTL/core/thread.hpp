#ifndef MSTL_THREAD_HPP__
#define MSTL_THREAD_HPP__
#include "memory.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include <process.h>
#elif defined(MSTL_PLATFORM_LINUX__)
#include <pthread.h>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(ThreadOperationError, MemoryError, "Thread Operation Failed.")


class MSTL_API thread : public iswappable<thread> {
public:
    class id : public ihashable<id> {
    private:
#ifdef MSTL_PLATFORM_WINDOWS__
        using native_id_type = ::DWORD;
#else
        using native_id_type = ::pthread_t;
#endif

        native_id_type id_
#ifdef MSTL_PLATFORM_WINDOWS__
        = 0;
  #else
        {};
#endif

        friend class thread;

    public:
        id() noexcept = default;
        explicit id(const native_id_type id) noexcept : id_(id) {}

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

    struct thread_data_base {
        virtual ~thread_data_base() = default;
        virtual void run() = 0;
    };

    template <typename Callable>
    struct thread_data final : thread_data_base {
        Callable func_;

        template <typename F>
        explicit thread_data(F&& f) : func_(_MSTL forward<F>(f)) {}

        void run() override { func_(); }
    };

private:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_handle_type = ::HANDLE;
#else
    using native_handle_type = ::pthread_t;
#endif

    native_handle_type handle_{};
    id id_{};
    STATE state_ = NOT_A_THREAD;

private:
#ifdef MSTL_PLATFORM_WINDOWS__
    static unsigned int __stdcall thread_entry(void* arg) {
#else
    static void* thread_entry(void* arg) {
#endif
        const unique_ptr<thread_data_base> data(static_cast<thread_data_base*>(arg));
        try {
            data->run();
        } catch (...) {
            _MSTL terminate();
        }
#ifdef MSTL_PLATFORM_WINDOWS__
        return 0;
#else
        return nullptr;
#endif
    }

    template <typename F>
    void start_thread(F&& f) {
        using data_type = thread_data<decay_t<F>>;
        unique_ptr<data_type> data = _MSTL make_unique<data_type>(_MSTL forward<F>(f));

#ifdef MSTL_PLATFORM_WINDOWS__
        unsigned int thread_id;
        handle_ = reinterpret_cast<native_handle_type>(
            ::_beginthreadex(nullptr, 0, thread_entry, data.get(), 0, &thread_id)
        );
        if (handle_ == nullptr) {
            Exception(ThreadOperationError("Failed to create thread"));
        }
        id_ = id(thread_id);
#else
        native_handle_type tid;
        if (::pthread_create(&tid, nullptr, thread_entry, data.get()) != 0) {
            Exception(ThreadOperationError("Failed to create thread"));
        }
        handle_ = tid;
        id_ = id(tid);
#endif
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

    ~thread() {
        if (joinable()) {
            _MSTL terminate();
        }
    }

    MSTL_NODISCARD id get_id() const noexcept { return id_; }
    MSTL_NODISCARD native_handle_type native_handle() const noexcept { return handle_; }

    MSTL_NODISCARD bool joinable() const noexcept { return state_ == CREATED; }

    void join();
    void detach();

    void swap(thread& other) noexcept {
        _MSTL swap(handle_, other.handle_);
        _MSTL swap(id_, other.id_);
        _MSTL swap(state_, other.state_);
    }

    static uint32_t hardware_concurrency() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::SYSTEM_INFO sysinfo;
        ::GetSystemInfo(&sysinfo);
        return static_cast<uint32_t>(sysinfo.dwNumberOfProcessors);
#else
        const long nprocs = ::sysconf(_SC_NPROCESSORS_ONLN);
        return nprocs > 0 ? static_cast<uint32_t>(nprocs) : 0;
#endif
    }
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
#endif // MSTL_THREAD_HPP__
