#include <NeForce/core/system/dynamic_library.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <libloaderapi.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <dlfcn.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

void dynamic_library::open() {
    if (handle_ != nullptr) {
        return;
    }
    if (path_.empty()) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("trying to open a empty dynamic library."));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    handle_ = ::LoadLibraryA(path_.data());
    if (handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("dynamic library load failed."));
    }
#else
    handle_ = ::dlopen(path_.data(), RTLD_LAZY | RTLD_LOCAL);
    if (handle_ == nullptr) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        const char* err = ::dlerror();
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception(err));
    }
#endif
}

void dynamic_library::close() {
    if (handle_ != nullptr) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::FreeLibrary(static_cast<::HMODULE>(handle_));
#else
        ::dlclose(handle_);
#endif
        handle_ = nullptr;
    }
}

dynamic_library::dynamic_library(string pth) :
path_(move(pth)) {
    open();
}

dynamic_library::dynamic_library(dynamic_library&& other) noexcept :
handle_(other.handle_),
path_(move(other.path_)) {
    other.handle_ = nullptr;
}

dynamic_library& dynamic_library::operator=(dynamic_library&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    close();
    handle_ = other.handle_;
    path_ = move(other.path_);
    other.handle_ = nullptr;
    return *this;
}

dynamic_library::~dynamic_library() { close(); }

void* dynamic_library::symbol(const string& name) const {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("Library not loaded"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::FARPROC proc = ::GetProcAddress(static_cast<::HMODULE>(handle_), name.data());
    if (proc == nullptr) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception("GetProcAddress failed"));
    }
    return reinterpret_cast<void*>(proc);
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    ::dlerror();
    void* sym = ::dlsym(handle_, name.data());
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* error = ::dlerror();
    if (error != nullptr) {
        NEFORCE_THROW_EXCEPTION(dynamic_library_exception(error));
    }
    return sym;
#endif
}

bool dynamic_library::has_symbol(const string& name) const noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetProcAddress(static_cast<::HMODULE>(handle_), name.data()) != nullptr;
#else
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    ::dlerror();
    ignore = ::dlsym(handle_, name.data());
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char* error = ::dlerror();
    return error == nullptr;
#endif
}

NEFORCE_END_NAMESPACE__
