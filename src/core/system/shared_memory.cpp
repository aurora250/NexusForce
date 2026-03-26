#include <NeForce/core/system/shared_memory.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/config/windef.hpp>
#include <windef.h>
#include <WinBase.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_LINUX
    string normalize_name(const string& name) {
        if (name.empty()) {
            return "/neforce_shm_default";
        }
        if (name[0] != '/') {
            return "/" + name;
        }
        return name;
    }
#endif

    shared_memory::native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
        nullptr;
#else
        -1;
#endif
}


shared_memory::shared_memory() noexcept
: handle_(invalid_handle) {}

shared_memory::shared_memory(const string& name, size_t size,
                             open_mode mode, access_mode access)
: handle_(invalid_handle) {
    open(name, size, mode, access);
}

shared_memory::shared_memory(shared_memory&& other) noexcept
: handle_(other.handle_),
  name_(static_cast<string&&>(other.name_)),
  size_(other.size_),
  mapped_size_(other.mapped_size_),
  mapped_addr_(other.mapped_addr_),
  access_mode_(other.access_mode_),
  is_open_(other.is_open_) {
    other.handle_ = invalid_handle;
    other.size_ = 0;
    other.mapped_size_ = 0;
    other.mapped_addr_ = nullptr;
    other.is_open_ = false;
}

shared_memory& shared_memory::operator =(shared_memory&& other) noexcept {
    if (addressof(other) == this) return *this;

    close();

    handle_ = other.handle_;
    name_ = static_cast<string&&>(other.name_);
    size_ = other.size_;
    mapped_size_ = other.mapped_size_;
    mapped_addr_ = other.mapped_addr_;
    access_mode_ = other.access_mode_;
    is_open_ = other.is_open_;

    other.handle_ = invalid_handle;
    other.size_ = 0;
    other.mapped_size_ = 0;
    other.mapped_addr_ = nullptr;
    other.is_open_ = false;

    return *this;
}

shared_memory::~shared_memory() {
    close();
}

void shared_memory::open(const string& name, size_t size,
                         open_mode mode, access_mode access) {
    if (is_open_) {
        close();
    }

    name_ = name;
    size_ = size;
    access_mode_ = access;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD protect = (access == access_mode::read_only) ? PAGE_READONLY : PAGE_READWRITE;
    const ::DWORD access_flags = (access == access_mode::read_only) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

    if (mode == open_mode::create_only) {
        handle_ = ::CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            protect,
            static_cast<::DWORD>(size >> 32),
            static_cast<::DWORD>(size & 0xFFFFFFFF),
            name.data()
        );

        if (handle_ == invalid_handle) {
            NEFORCE_THROW_EXCEPTION(shared_memory_exception("CreateFileMapping failed"));
        }

        if (::GetLastError() == ERROR_ALREADY_EXISTS) {
            ::CloseHandle(handle_);
            handle_ = nullptr;
            NEFORCE_THROW_EXCEPTION(shared_memory_exception("Shared memory already exists"));
        }
    } else if (mode == open_mode::open_only) {
        handle_ = ::OpenFileMappingA(access_flags, FALSE, name.data());
        if (handle_ == invalid_handle) {
            NEFORCE_THROW_EXCEPTION(shared_memory_exception("OpenFileMapping failed"));
        }
    } else {
        handle_ = ::CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            protect,
            static_cast<::DWORD>(size >> 32),
            static_cast<::DWORD>(size & 0xFFFFFFFF),
            name.data()
        );

        if (handle_ == invalid_handle) {
            NEFORCE_THROW_EXCEPTION(shared_memory_exception("CreateFileMapping failed"));
        }
    }

    is_open_ = true;

#else
    const string shm_name = normalize_name(name);

    int flags = (access == access_mode::read_only) ? O_RDONLY : O_RDWR;

    if (mode == open_mode::create_only) {
        flags |= O_CREAT | O_EXCL;
        handle_ = ::shm_open(shm_name.data(), flags, 0666);
        if (handle_ == invalid_handle) {
            NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
        }

        if (::ftruncate(handle_, static_cast<::off_t>(size)) == -1) {
            ::close(handle_);
            handle_ = invalid_handle;
            ::shm_unlink(shm_name.data());
            NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
        }
    } else if (mode == open_mode::open_only) {
        handle_ = ::shm_open(shm_name.data(), flags, 0666);
        if (handle_ == invalid_handle) {
            NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
        }

        struct ::stat stat_buf;
        if (::fstat(handle_, &stat_buf) == -1) {
            ::close(handle_);
            handle_ = invalid_handle;
            NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
        }
        size_ = static_cast<size_t>(stat_buf.st_size);
    } else {
        handle_ = ::shm_open(shm_name.data(), flags | O_CREAT, 0666);
        if (handle_ == invalid_handle) {
            NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
        }

        struct ::stat stat_buf;
        if (::fstat(handle_, &stat_buf) == -1) {
            ::close(handle_);
            handle_ = invalid_handle;
            NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
        }

        if (stat_buf.st_size == 0) {
            if (::ftruncate(handle_, static_cast<::off_t>(size)) == -1) {
                ::close(handle_);
                handle_ = invalid_handle;
                ::shm_unlink(shm_name.data());
                NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
            }
        } else {
            size_ = static_cast<size_t>(stat_buf.st_size);
        }
    }

    is_open_ = true;
#endif
}

void shared_memory::close() noexcept {
    unmap();

    if (handle_ != invalid_handle) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CloseHandle(handle_);
#else
        ::close(handle_);
#endif
        handle_ = invalid_handle;
    }

    is_open_ = false;
}

void* shared_memory::map(size_t offset, size_t length) {
    if (!is_open_) {
        NEFORCE_THROW_EXCEPTION(shared_memory_exception("Shared memory not open"));
    }

    if (mapped_addr_ != nullptr) {
        unmap();
    }

    const size_t map_length = (length == 0) ? size_ : length;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD access = (access_mode_ == access_mode::read_only) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

    mapped_addr_ = ::MapViewOfFile(
        handle_,
        access,
        static_cast<::DWORD>(offset >> 32),
        static_cast<::DWORD>(offset & 0xFFFFFFFF),
        map_length
    );

    if (mapped_addr_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(shared_memory_exception("MapViewOfFile failed"));
    }
#else
    int prot = (access_mode_ == access_mode::read_only) ? PROT_READ : (PROT_READ | PROT_WRITE);

    mapped_addr_ = ::mmap(
        nullptr,
        map_length,
        prot,
        MAP_SHARED,
        handle_,
        static_cast<::off_t>(offset)
    );

    if (mapped_addr_ == MAP_FAILED) {
        mapped_addr_ = nullptr;
        NEFORCE_THROW_EXCEPTION(shared_memory_exception(::strerror(errno)));
    }
#endif

    mapped_size_ = map_length;
    return mapped_addr_;
}

void shared_memory::unmap() noexcept {
    if (mapped_addr_ == nullptr) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::UnmapViewOfFile(mapped_addr_);
#else
    ::munmap(mapped_addr_, mapped_size_);
#endif

    mapped_addr_ = nullptr;
    mapped_size_ = 0;
}

bool shared_memory::flush(bool async) noexcept {
    if (mapped_addr_ == nullptr) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::FlushViewOfFile(mapped_addr_, mapped_size_) != 0;
#else
    const int flags = async ? MS_ASYNC : MS_SYNC;
    return ::msync(mapped_addr_, mapped_size_, flags) == 0;
#endif
}

bool shared_memory::remove(const string& name) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    // auto remove when handle is closed in Windows
    return true;
#else
    const string shm_name = normalize_name(name);
    return ::shm_unlink(shm_name.data()) == 0;
#endif
}

bool shared_memory::exists(const string& name) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE h = ::OpenFileMappingA(FILE_MAP_READ, FALSE, name.data());
    if (h != invalid_handle) {
        ::CloseHandle(h);
        return true;
    }
    return false;
#else
    const string shm_name = normalize_name(name);
    const int fd = ::shm_open(shm_name.data(), O_RDONLY, 0666);
    if (fd != invalid_handle) {
        ::close(fd);
        return true;
    }
    return false;
#endif
}

NEFORCE_END_NAMESPACE__
