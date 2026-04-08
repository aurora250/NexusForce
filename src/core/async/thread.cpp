#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/async/thread_tracker.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/string/utf.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/async/call_once.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    include <process.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS

    ::HRESULT(WINAPI* g_set_thread_description)(::HANDLE, ::PCWSTR) = nullptr;
    ::HRESULT(WINAPI* g_get_thread_description)(::HANDLE, ::PWSTR*) = nullptr;

#    pragma pack(push, 8)
    struct threadname_info {
        ::DWORD dw_type;
        ::LPCSTR sz_name;
        ::DWORD dw_thread_id;
        ::DWORD dw_flags;
    };
#    pragma pack(pop)

    void init_thread_name_funcs() {
        static once_flag init_module_flag;
        static auto init_thread_name = []() {
            auto* const kernel32 = ::GetModuleHandle("kernel32.dll");
            if (kernel32) {
                g_set_thread_description = reinterpret_cast<decltype(g_set_thread_description)>(
                        ::GetProcAddress(kernel32, "SetThreadDescription"));
                g_get_thread_description = reinterpret_cast<decltype(g_get_thread_description)>(
                        ::GetProcAddress(kernel32, "GetThreadDescription"));
            }
        };
        call_once(init_module_flag, init_thread_name);
    }

#    ifdef NEFORCE_COMPILER_MSVC
    void set_thread_name_by_exception(const char* name) {
        constexpr ::DWORD msvc_exception = 0x406D1388;
        threadname_info info{};
        info.dw_type = 0x1000;
        info.sz_name = name;
        info.dw_thread_id = ::GetCurrentThreadId();
        info.dw_flags = 0;
        __try {
            ::RaiseException(msvc_exception, 0, sizeof(info) / sizeof(::ULONG_PTR),
                             reinterpret_cast<::ULONG_PTR*>(&info));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
#    endif

#endif

    vector<thread::hook::callback_t>& thread_hook_hooks() {
        static vector<thread::hook::callback_t> hooks;
        return hooks;
    }

    mutex& thread_hook_mutex() {
        static mutex mtx;
        return mtx;
    }
} // namespace


atomic<int> thread_tracker::count_{0};


void thread::hook::add_hook(callback_t hook) {
    lock<mutex> lock(thread_hook_mutex());
    thread_hook_hooks().push_back(_NEFORCE move(hook));
}

void thread::hook::remove_hook(callback_t hook) {
    lock<mutex> lock(thread_hook_mutex());
    auto it = find(thread_hook_hooks().begin(), thread_hook_hooks().end(), _NEFORCE move(hook));
    if (it != thread_hook_hooks().end()) {
        thread_hook_hooks().erase(it);
    }
}

void thread::hook::invoke(const point point, const id thread_id) {
    lock<mutex> lock(thread_hook_mutex());
    for (auto& hook: thread_hook_hooks()) {
        hook(point, thread_id);
    }
}

thread::thread_monitor::thread_monitor(const id thread_id) :
thread_id_(thread_id) {
    thread_tracker::instance().on_thread_create();
    hook::invoke(hook::point::thread_start, thread_id_);
}

thread::thread_monitor::~thread_monitor() noexcept {
    try {
        hook::invoke(hook::point::thread_end, thread_id_);
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
    thread_tracker::instance().on_thread_destroy();
}

#ifdef NEFORCE_PLATFORM_WINDOWS
unsigned int __stdcall thread::thread_entry(void* arg) {
#else
void* thread::thread_entry(void* arg) {
#endif
    auto* args = static_cast<thread_startup_args*>(arg);
    const unique_ptr<data_base> data = _NEFORCE move(args->data);
    thread_monitor monitor(args->thread_id);

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
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
    hook::invoke(hook::point::before_create, id_);

#ifdef NEFORCE_PLATFORM_WINDOWS
    // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
    unsigned int thread_id;
    handle_ = reinterpret_cast<native_handle_type>(::_beginthreadex(nullptr, 0, thread_entry, args, 0, &thread_id));
    if (handle_ == nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete args;
        NEFORCE_THROW_EXCEPTION(thread_exception("Failed to create thread"));
    }
    id_ = id(thread_id);

#else
    native_handle_type tid = 0;
    if (::pthread_create(&tid, nullptr, thread_entry, args) != 0) {
        delete args;
        NEFORCE_THROW_EXCEPTION(thread_exception("Failed to create thread"));
    }
    handle_ = tid;
    id_ = id(tid);

#endif

    state_ = CREATED;
    hook::invoke(hook::point::after_create, id_);
}

thread::thread(thread&& other) noexcept :
handle_(other.handle_),
id_(other.id_),
state_(other.state_) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.handle_ = nullptr;
#else
    other.handle_ = native_handle_type{};
#endif
    other.id_ = id{};
    other.state_ = NOT_A_THREAD;
}

thread& thread::operator=(thread&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    if (joinable()) {
        try {
            hook::invoke(hook::point::before_destroy, id_);
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
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

    return *this;
}

thread::~thread() {
    try {
        hook::invoke(hook::point::before_destroy, id_);
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
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

bool thread::set_name(const char* name) {
    if (!joinable()) {
        NEFORCE_THROW_EXCEPTION(thread_exception("Thread not joinable, cannot set name"));
    }
    return set_name(handle_, name);
}

bool thread::name(char* buffer, const size_t size) const {
    if (!joinable()) {
        return false;
    }
    return name(handle_, buffer, size);
}

void thread::swap(thread& other) noexcept {
    _NEFORCE swap(handle_, other.handle_);
    _NEFORCE swap(id_, other.id_);
    _NEFORCE swap(state_, other.state_);
}

bool thread::set_name(native_handle_type handle, const char* name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    init_thread_name_funcs();
    if (g_set_thread_description != nullptr) {
        wstring wstr{to_wstring(name)};
        HRESULT hr = g_set_thread_description(handle, wstr.data());
        return SUCCEEDED(hr);
    }
#    ifdef NEFORCE_COMPILER_MSVC
    if (handle == ::GetCurrentThread()) {
        set_thread_name_by_exception(name);
        return true;
    }
#    endif
    return false;
#else
    return ::pthread_setname_np(handle, name) == 0;
#endif
}

bool thread::name(native_handle_type handle, char* buffer, size_t size) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    init_thread_name_funcs();
    if (g_get_thread_description != nullptr) {
        ::PWSTR wname = nullptr;
        if (SUCCEEDED(g_get_thread_description(handle, &wname))) {
            string name = to_string(wname);
            const size_t name_len = name.size();
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
    if (size < 16) {
        return false;
    }
    return ::pthread_getname_np(handle, buffer, size) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
