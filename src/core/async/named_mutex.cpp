#include <NeForce/core/async/named_mutex.hpp>
#include <NeForce/core/system/share_memory.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <NeForce/core/string/utf.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <NeForce/core/exception/error_code.hpp>
#    include <NeForce/core/hash/fnv.hpp>
#    include <NeForce/core/utility/packages.hpp>
#    include <fcntl.h>
#    include <pthread.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_POSIX
    string shm_name_for(const string& name) {
        return "/nf_mtx_" + to_string(fnv_hash_string64(name.data(), name.size()));
    }
#endif

    constexpr native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            nullptr;
#else
            -1;
#endif
} // namespace


struct named_mutex::impl {
    native_handle_type handle_ = invalid_handle;
    string name_;

#ifndef NEFORCE_PLATFORM_WINDOWS
    void* mapped_addr_ = nullptr;
    string shm_name_;
    bool owner_ = false;
#endif


    impl(const string& name, const bool create) :
    name_(name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        wstring wname = character::to_wstring(name.view());
        if (create) {
            handle_ = ::CreateMutexW(nullptr, FALSE, wname.data());
        } else {
            handle_ = ::OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, wname.data());
        }
        if (handle_ == nullptr) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: failed to create/open"));
        }
#else
        shm_name_ = shm_name_for(name);
        if (create) {
            handle_ = ::shm_open(shm_name_.data(), O_CREAT | O_RDWR, 0666);
            if (handle_ == invalid_handle) {
                NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: shm_open failed"));
            }
            owner_ = true;
            if (::ftruncate(handle_, sizeof(pthread_mutex_t)) < 0) {
                ::close(handle_);
                ::shm_unlink(shm_name_.data());
                NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: ftruncate failed"));
            }
        } else {
            handle_ = ::shm_open(shm_name_.data(), O_RDWR, 0666);
            if (handle_ == invalid_handle) {
                NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: shm_open existing failed"));
            }
        }

        mapped_addr_ = ::mmap(nullptr, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, handle_, 0);
        if (mapped_addr_ == MAP_FAILED) {
            ::close(handle_);
            if (owner_) {
                ::shm_unlink(shm_name_.data());
            }
            NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: mmap failed"));
        }

        if (owner_) {
            ::pthread_mutexattr_t attr;
            ::pthread_mutexattr_init(&attr);
            ::pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
#    ifndef NEFORCE_PLATFORM_MACOS
            ::pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
#    endif
            ::pthread_mutex_init(static_cast<pthread_mutex_t*>(mapped_addr_), &attr);
            ::pthread_mutexattr_destroy(&attr);
        }
#endif
    }

    ~impl() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (handle_ != nullptr) {
            ::CloseHandle(handle_);
        }
#else
        if (mapped_addr_ != nullptr && mapped_addr_ != MAP_FAILED) {
            if (owner_) {
                ::pthread_mutex_destroy(static_cast<pthread_mutex_t*>(mapped_addr_));
            }
            ::munmap(mapped_addr_, sizeof(pthread_mutex_t));
        }
        if (handle_ != invalid_handle) {
            ::close(handle_);
            if (owner_) {
                ::shm_unlink(shm_name_.data());
            }
        }
#endif
    }
};


named_mutex::named_mutex(const string& name, const bool create) :
impl_(make_unique<impl>(name, create)) {}

named_mutex::~named_mutex() = default;

named_mutex::named_mutex(named_mutex&& other) noexcept :
impl_(move(other.impl_)) {}

named_mutex& named_mutex::operator=(named_mutex&& other) noexcept {
    if (addressof(other) != this) {
        impl_ = move(other.impl_);
    }
    return *this;
}

void named_mutex::lock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (impl_ == nullptr || impl_->handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: not initialized"));
    }
    ::WaitForSingleObject(impl_->handle_, INFINITE);
#else
    if (impl_ == nullptr || impl_->mapped_addr_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: not initialized"));
    }
    const int ret = ::pthread_mutex_lock(static_cast<pthread_mutex_t*>(impl_->mapped_addr_));
#    ifndef NEFORCE_PLATFORM_MACOS
    if (ret == EOWNERDEAD) {
        ::pthread_mutex_consistent(static_cast<pthread_mutex_t*>(impl_->mapped_addr_));
    }
#    else
    static_cast<void>(ret);
#    endif
#endif
}

bool named_mutex::try_lock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (impl_ == nullptr || impl_->handle_ == nullptr) {
        return false;
    }
    return ::WaitForSingleObject(impl_->handle_, 0) == WAIT_OBJECT_0;
#else
    if (impl_ == nullptr || impl_->mapped_addr_ == nullptr) {
        return false;
    }
    const int ret = ::pthread_mutex_trylock(static_cast<pthread_mutex_t*>(impl_->mapped_addr_));
    if (ret == EBUSY) {
        return false;
    }
#    ifndef NEFORCE_PLATFORM_MACOS
    if (ret == EOWNERDEAD) {
        ::pthread_mutex_consistent(static_cast<pthread_mutex_t*>(impl_->mapped_addr_));
    }
#    endif
    return (ret == 0 || ret == EOWNERDEAD);
#endif
}

void named_mutex::unlock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (impl_ != nullptr && impl_->handle_ != nullptr) {
        ::ReleaseMutex(impl_->handle_);
    }
#else
    if (impl_ != nullptr && impl_->mapped_addr_ != nullptr) {
        ::pthread_mutex_unlock(static_cast<pthread_mutex_t*>(impl_->mapped_addr_));
    }
#endif
}

string_view named_mutex::name() const noexcept { return impl_ != nullptr ? impl_->name_.view() : ""; }

bool named_mutex::is_valid() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return impl_ != nullptr && impl_->handle_ != nullptr;
#else
    return impl_ != nullptr && impl_->mapped_addr_ != nullptr;
#endif
}

bool named_mutex::remove(const string& name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ignore = name;
    return true;
#else
    const string shm_name = shm_name_for(name);
    return ::shm_unlink(shm_name.data()) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
