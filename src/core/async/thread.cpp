#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/async/thread_hook.hpp>
#include <NeForce/core/async/thread_tracker.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/string/utf.hpp>
#include <NeForce/core/time/clocks.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/async/call_once.hpp>
#include <windef.h>
#include <process.h>
#include <WinBase.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_WINDOWS
namespace {
    ::HRESULT (WINAPI* pSetThreadDescription)(::HANDLE, ::PCWSTR) = nullptr;
    ::HRESULT (WINAPI* pGetThreadDescription)(::HANDLE, ::PWSTR*) = nullptr;

#pragma pack(push,8)
    struct THREADNAME_INFO {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    };
#pragma pack(pop)

    void init_thread_name_funcs() {
        static once_flag init_module_flag;
        static auto init_thread_name = []() {
            const auto kernel32 = ::GetModuleHandle("kernel32.dll");
            if (kernel32) {
                pSetThreadDescription = reinterpret_cast<decltype(pSetThreadDescription)>(
                    ::GetProcAddress(kernel32, "SetThreadDescription"));
                pGetThreadDescription = reinterpret_cast<decltype(pGetThreadDescription)>(
                    ::GetProcAddress(kernel32, "GetThreadDescription"));
            }
        };
        call_once(init_module_flag, init_thread_name);
    }

    void set_thread_name_by_exception(const char* name) {
        constexpr ::DWORD MSVC_EXCEPTION = 0x406D1388;
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = ::GetCurrentThreadId();
        info.dwFlags = 0;
        __try {
            ::RaiseException(
                MSVC_EXCEPTION,
                0,
                sizeof(info) / sizeof(::ULONG_PTR),
                reinterpret_cast<::ULONG_PTR*>(&info));
        } __except(EXCEPTION_EXECUTE_HANDLER) {}
    }
}
#endif


atomic<int> thread_tracker::count_{0};


thread::thread_monitor::thread_monitor(const thread* self) noexcept : self_(self) {
    thread_tracker::instance().on_thread_create();
    thread_hook::invoke(thread_hook::point::thread_start, self_);
}

thread::thread_monitor::~thread_monitor() {
    thread_hook::invoke(thread_hook::point::thread_end, self_);
    thread_tracker::instance().on_thread_destroy();
}

#ifdef NEFORCE_PLATFORM_WINDOWS
unsigned int __stdcall
#else
void*
#endif
thread::thread_entry(void* arg) {
    auto* args = static_cast<thread_startup_args*>(arg);
    unique_ptr<data_base> data = _NEFORCE move(args->data);
    thread_monitor monitor(args->self);
    delete args;

    try {
        data->run();
    } catch (...) {
        terminate();
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return 0;
#else
    return nullptr;
#endif
}

void thread::start_thread_impl(thread_startup_args* args) {
    thread_hook::invoke(thread_hook::point::before_create, this);

#ifdef NEFORCE_PLATFORM_WINDOWS
    unsigned int thread_id;
    handle_ = reinterpret_cast<native_handle_type>(
        ::_beginthreadex(nullptr, 0, thread_entry, args, 0, &thread_id)
    );
    if (handle_ == nullptr) {
        delete args;
        NEFORCE_THROW_EXCEPTION(thread_exception("Failed to create thread"));
    }
    id_ = id(thread_id);

#else
    native_handle_type tid;
    if (::pthread_create(&tid, nullptr, thread_entry, args) != 0) {
        delete args;
        NEFORCE_THROW_EXCEPTION(thread_exception("Failed to create thread"));
    }
    handle_ = tid;
    id_ = id(tid);

#endif

    state_ = CREATED;
    thread_hook::invoke(thread_hook::point::after_create, this);
}

thread::thread(thread&& other) noexcept
: handle_(other.handle_), id_(other.id_), state_(other.state_) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.handle_ = nullptr;
#else
    other.handle_ = native_handle_type{};
#endif
    other.id_ = id{};
    other.state_ = NOT_A_THREAD;
}

thread& thread::operator =(thread&& other) noexcept {
    if (this != &other) {
        if (joinable()) {
            thread_hook::invoke(thread_hook::point::before_destroy, this);
            terminate();
        }

        handle_ = other.handle_;
        id_ = other.id_;
        state_ = other.state_;
#ifdef NEFORCE_PLATFORM_WINDOWS
        other.handle_ = nullptr;
#else
        other.handle_ = native_handle_type{};
#endif
        other.id_ = id{};
        other.state_ = NOT_A_THREAD;
    }
    return *this;
}

thread::~thread() {
    thread_hook::invoke(thread_hook::point::before_destroy, this);
    if (joinable()) {
        terminate();
    }
}

void thread::join() {
    if (!joinable()) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread is not joinable"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::WaitForSingleObject(handle_, numeric_traits<::DWORD>::max()) != WAIT_OBJECT_0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Fail to join thread"));
    }
    ::CloseHandle(handle_);
    handle_ = nullptr;
#else
    if (::pthread_join(handle_, nullptr) != 0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread is not joinable"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = JOINED;
}

void thread::detach() {
    if (!joinable()) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread is not detachable"));
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::CloseHandle(handle_) == FALSE) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Fail to detach thread"));
    }
    handle_ = nullptr;
#else
    if (::pthread_detach(handle_) != 0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Fail to detach thread"));
    }
    handle_ = native_handle_type{};
#endif
    state_ = DETACHED;
}

void thread::set_name(const char* name) {
    if (!joinable()) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread not joinable, cannot set name"));
    }
    set_name(handle_, name);
}

bool thread::name(char* buffer, size_t size) const {
    if (!joinable()) return false;
    return name(handle_, buffer, size);
}

void thread::swap(thread& other) noexcept {
    _NEFORCE swap(handle_, other.handle_);
    _NEFORCE swap(id_, other.id_);
    _NEFORCE swap(state_, other.state_);
}

void thread::set_name(native_handle_type handle, const char* name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    init_thread_name_funcs();
    if (pSetThreadDescription) {
        wstring wstr{to_wstring(name)};
        pSetThreadDescription(handle, wstr.data());
    } else {
        if (handle == ::GetCurrentThread()) {
            set_thread_name_by_exception(name);
        } else {}
    }
#else
    if (::pthread_setname_np(handle, name) != 0) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Failed to set thread name"));
    }
#endif
}

bool thread::name(native_handle_type handle, char* buffer, size_t size) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    init_thread_name_funcs();
    if (pGetThreadDescription) {
        ::PWSTR wname = nullptr;
        if (SUCCEEDED(pGetThreadDescription(handle, &wname))) {
            string name = to_string(wname);
            size_t name_len = name.size();
            if (name_len < size) {
                memory_copy(buffer, name.data(), name_len);
                buffer[name_len] = '\0';
            } else {
                memory_copy(buffer, name.data(), size - 1);
                buffer[size - 1] = '\0';
            }
            ::LocalFree(wname);
            return true;
        }
    }
    return false;
#else
    if (size < 16) return false;
    return ::pthread_getname_np(handle, buffer, size) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
