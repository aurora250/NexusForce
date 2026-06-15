#include <NeForce/core/async/named_mutex.hpp>
#include <NeForce/core/system/share_memory.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/exception/error_code.hpp>
#    include <fcntl.h>
#    include <pthread.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

named_mutex::named_mutex(const string& name, bool create) :
name_(name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (create) {
        handle_ = ::CreateMutexA(nullptr, FALSE, name.data());
    } else {
        handle_ = ::OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, name.data());
    }
    if (handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: failed to create/open"));
    }
#else
    shm_name_ = "/neforce_named_mutex_" + name;
    if (create) {
        shm_fd_ = ::shm_open(shm_name_.data(), O_CREAT | O_RDWR, 0666);
        if (shm_fd_ < 0) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: shm_open failed"));
        }
        owner_ = true;
        if (::ftruncate(shm_fd_, sizeof(pthread_mutex_t)) < 0) {
            ::close(shm_fd_);
            ::shm_unlink(shm_name_.data());
            NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: ftruncate failed"));
        }
    } else {
        shm_fd_ = ::shm_open(shm_name_.data(), O_RDWR, 0666);
        if (shm_fd_ < 0) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: shm_open existing failed"));
        }
    }

    mapped_addr_ = ::mmap(nullptr, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
    if (mapped_addr_ == MAP_FAILED) {
        ::close(shm_fd_);
        if (owner_) {
            ::shm_unlink(shm_name_.data());
        }
        NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: mmap failed"));
    }

    if (owner_) {
        ::pthread_mutexattr_t attr;
        ::pthread_mutexattr_init(&attr);
        ::pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        ::pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
        ::pthread_mutex_init(static_cast<pthread_mutex_t*>(mapped_addr_), &attr);
        ::pthread_mutexattr_destroy(&attr);
    }
#endif
}

named_mutex::~named_mutex() {
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
    if (shm_fd_ >= 0) {
        ::close(shm_fd_);
        if (owner_) {
            ::shm_unlink(shm_name_.data());
        }
    }
#endif
}

named_mutex::named_mutex(named_mutex&& other) noexcept :
#ifdef NEFORCE_PLATFORM_WINDOWS
handle_(other.handle_),
#else
shm_fd_(other.shm_fd_),
mapped_addr_(other.mapped_addr_),
shm_name_(move(other.shm_name_)),
owner_(other.owner_),
#endif
name_(move(other.name_)) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.handle_ = nullptr;
#else
    other.shm_fd_ = -1;
    other.mapped_addr_ = nullptr;
    other.owner_ = false;
#endif
}

named_mutex& named_mutex::operator=(named_mutex&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    this->~named_mutex();
#ifdef NEFORCE_PLATFORM_WINDOWS
    handle_ = other.handle_;
    other.handle_ = nullptr;
#else
    shm_fd_ = other.shm_fd_;
    mapped_addr_ = other.mapped_addr_;
    shm_name_ = move(other.shm_name_);
    owner_ = other.owner_;
    other.shm_fd_ = -1;
    other.mapped_addr_ = nullptr;
    other.owner_ = false;
#endif
    name_ = move(other.name_);
    return *this;
}

void named_mutex::lock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: not initialized"));
    }
    ::WaitForSingleObject(handle_, INFINITE);
#else
    if (mapped_addr_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("named_mutex: not initialized"));
    }
    int ret = ::pthread_mutex_lock(static_cast<pthread_mutex_t*>(mapped_addr_));
    if (ret == EOWNERDEAD) {
        ::pthread_mutex_consistent(static_cast<pthread_mutex_t*>(mapped_addr_));
    }
#endif
}

bool named_mutex::try_lock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (handle_ == nullptr) {
        return false;
    }
    return ::WaitForSingleObject(handle_, 0) == WAIT_OBJECT_0;
#else
    if (mapped_addr_ == nullptr) {
        return false;
    }
    int ret = ::pthread_mutex_trylock(static_cast<pthread_mutex_t*>(mapped_addr_));
    if (ret == EBUSY) {
        return false;
    }
    if (ret == EOWNERDEAD) {
        ::pthread_mutex_consistent(static_cast<pthread_mutex_t*>(mapped_addr_));
    }
    return (ret == 0 || ret == EOWNERDEAD);
#endif
}

void named_mutex::unlock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (handle_ != nullptr) {
        ::ReleaseMutex(handle_);
    }
#else
    if (mapped_addr_ != nullptr) {
        ::pthread_mutex_unlock(static_cast<pthread_mutex_t*>(mapped_addr_));
    }
#endif
}

bool named_mutex::is_valid() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return handle_ != nullptr;
#else
    return mapped_addr_ != nullptr;
#endif
}

bool named_mutex::remove(const string& name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ignore = name;
    return true;
#else
    const string shm_name = "/neforce_named_mutex_" + name;
    return ::shm_unlink(shm_name.data()) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
