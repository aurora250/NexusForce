#include <NeForce/core/system/share_memory.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#include <NeForce/core/utility/packages.hpp>
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
#    include <cerrno>
#    include <cstring>
#    include <fcntl.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
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

    constexpr share_memory::native_handle_type g_invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            nullptr;
#else
            -1;
#endif

    const auto granularity =
#ifdef NEFORCE_PLATFORM_WINDOWS
            sysinfo::instance().get_system_info().allocation_granularity;
#else
            sysinfo::instance().get_system_info().page_size;
#endif
} // namespace


share_memory::share_memory() noexcept :
handle_(g_invalid_handle) {}

share_memory::share_memory(const string& name, const size_t size, const open_mode mode, const access_mode access) :
handle_(g_invalid_handle) {
    open(name, size, mode, access);
}

share_memory::share_memory(share_memory&& other) noexcept :
handle_(other.handle_),
name_(move(other.name_)),
size_(other.size_),
internal_mapped_size_(other.internal_mapped_size_),
mapped_size_(other.mapped_size_),
original_mapped_addr_(other.original_mapped_addr_),
mapped_addr_(other.mapped_addr_),
access_mode_(other.access_mode_),
is_open_(other.is_open_) {
    other.handle_ = g_invalid_handle;
    other.size_ = 0;
    other.internal_mapped_size_ = 0;
    other.mapped_size_ = 0;
    other.original_mapped_addr_ = nullptr;
    other.mapped_addr_ = nullptr;
    other.is_open_ = false;
}

share_memory& share_memory::operator=(share_memory&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    close();

    handle_ = other.handle_;
    name_ = move(other.name_);
    size_ = other.size_;
    internal_mapped_size_ = other.internal_mapped_size_;
    mapped_size_ = other.mapped_size_;
    original_mapped_addr_ = other.original_mapped_addr_;
    mapped_addr_ = other.mapped_addr_;
    access_mode_ = other.access_mode_;
    is_open_ = other.is_open_;

    other.handle_ = g_invalid_handle;
    other.size_ = 0;
    other.internal_mapped_size_ = 0;
    other.mapped_size_ = 0;
    other.original_mapped_addr_ = nullptr;
    other.mapped_addr_ = nullptr;
    other.is_open_ = false;

    return *this;
}

share_memory::~share_memory() { close(); }

void share_memory::open(const string& name, size_t size, open_mode mode, access_mode access) {
    if (is_open_) {
        close();
    }

    if (name.empty()) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory name cannot be empty"));
    }

    name_ = name;
    access_mode_ = access;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD protect = (access == access_mode::read_only) ? PAGE_READONLY : PAGE_READWRITE;
    const ::DWORD access_flags = (access == access_mode::read_only) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

    if (handle_ != g_invalid_handle) {
        is_open_ = true;
        return;
    }

    auto get_real_size = [&]() -> size_t {
        void* temp_view = ::MapViewOfFile(handle_, FILE_MAP_READ, 0, 0, 0);
        if (temp_view == nullptr) {
            return 0;
        }
        MEMORY_BASIC_INFORMATION mbi{};
        size_t sz = 0;
        if (::VirtualQuery(temp_view, &mbi, sizeof(mbi)) != 0) {
            sz = mbi.RegionSize;
        }
        ::UnmapViewOfFile(temp_view);
        return sz;
    };

    if (mode == open_mode::create_only) {
        if (size == 0) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("Size must be greater than 0 for create_only mode"));
        }
#    ifdef NEFORCE_ARCH_BITS_64
        const auto size_high = static_cast<::DWORD>(size >> 32);
        const auto size_low = static_cast<::DWORD>(size & 0xFFFFFFFF);
#    else
        constexpr ::DWORD size_high = 0;
        const auto size_low = static_cast<::DWORD>(size);
#    endif
        handle_ = ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, protect, size_high, size_low, name.data());
        if (handle_ == g_invalid_handle) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("CreateFileMapping failed"));
        }
        if (::GetLastError() == ERROR_ALREADY_EXISTS) {
            ::CloseHandle(handle_);
            handle_ = g_invalid_handle;
            NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory already exists"));
        }
        size_ = size;
    } else if (mode == open_mode::open_only) {
        handle_ = ::OpenFileMappingA(access_flags, FALSE, name.data());
        if (handle_ == g_invalid_handle) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("OpenFileMapping failed"));
        }
        size_ = get_real_size();
        if (size_ == 0) {
            ::CloseHandle(handle_);
            handle_ = g_invalid_handle;
            NEFORCE_THROW_EXCEPTION(share_memory_exception("Failed to query shared memory size"));
        }
    } else {
        handle_ = ::OpenFileMappingA(access_flags, FALSE, name.data());
        if (handle_ != g_invalid_handle) {
            size_ = get_real_size();
            if (size_ == 0) {
                ::CloseHandle(handle_);
                handle_ = g_invalid_handle;
                NEFORCE_THROW_EXCEPTION(share_memory_exception("Failed to query shared memory size"));
            }
        } else {
            if (size == 0) {
                NEFORCE_THROW_EXCEPTION(
                        share_memory_exception("Size must be greater than 0 when creating new shared memory"));
            }
#    ifdef NEFORCE_ARCH_BITS_64
            const auto size_high = static_cast<::DWORD>(size >> 32);
            const auto size_low = static_cast<::DWORD>(size & 0xFFFFFFFF);
#    else
            constexpr ::DWORD size_high = 0;
            const auto size_low = static_cast<::DWORD>(size);
#    endif
            handle_ = ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, protect, size_high, size_low, name.data());
            if (handle_ == g_invalid_handle) {
                NEFORCE_THROW_EXCEPTION(share_memory_exception("CreateFileMapping failed"));
            }

            if (::GetLastError() == ERROR_ALREADY_EXISTS) {
                size_ = get_real_size();
                if (size_ == 0) {
                    ::CloseHandle(handle_);
                    handle_ = g_invalid_handle;
                    NEFORCE_THROW_EXCEPTION(share_memory_exception("Failed to query existing shared memory size"));
                }
            } else {
                size_ = size;
            }
        }
    }

    is_open_ = true;

#else
    const string shm_name = normalize_name(name);

    int flags = (access == access_mode::read_only) ? O_RDONLY : O_RDWR;

    if (mode == open_mode::create_only) {
        if (size == 0) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("Size must be greater than 0 for create_only mode"));
        }

        flags |= O_CREAT | O_EXCL;
        handle_ = ::shm_open(shm_name.data(), flags, 0666);
        if (handle_ == g_invalid_handle) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception(_NEFORCE last_error().message().data()));
        }

        if (::ftruncate(handle_, static_cast<::off_t>(size)) == -1) {
            const auto error = last_error();
            ::close(handle_);
            handle_ = g_invalid_handle;
            ::shm_unlink(shm_name.data());
            NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
        }
        size_ = size;
    } else if (mode == open_mode::open_only) {
        handle_ = ::shm_open(shm_name.data(), flags, 0666);
        if (handle_ == g_invalid_handle) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
        }

        struct ::stat stat_buf;
        if (::fstat(handle_, &stat_buf) == -1) {
            const auto error = last_error();
            ::close(handle_);
            handle_ = g_invalid_handle;
            NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
        }

        if (stat_buf.st_size == 0) {
            ::close(handle_);
            handle_ = g_invalid_handle;
            NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory exists but has zero size"));
        }

        size_ = static_cast<size_t>(stat_buf.st_size);
    } else {
        if (size == 0) {
            handle_ = ::shm_open(shm_name.data(), flags, 0666);
            if (handle_ == g_invalid_handle) {
                const auto error = last_error();
                NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
            }

            struct ::stat stat_buf;
            if (::fstat(handle_, &stat_buf) == -1) {
                const auto error = last_error();
                ::close(handle_);
                handle_ = g_invalid_handle;
                NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
            }

            if (stat_buf.st_size == 0) {
                ::close(handle_);
                handle_ = g_invalid_handle;
                NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory exists but has zero size"));
            }

            size_ = static_cast<size_t>(stat_buf.st_size);
        } else {
            handle_ = ::shm_open(shm_name.data(), flags | O_CREAT, 0666);
            if (handle_ == g_invalid_handle) {
                const auto error = last_error();
                NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
            }

            struct ::stat stat_buf;
            if (::fstat(handle_, &stat_buf) == -1) {
                const auto error = last_error();
                ::close(handle_);
                handle_ = g_invalid_handle;
                NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
            }

            if (stat_buf.st_size == 0) {
                if (::ftruncate(handle_, static_cast<::off_t>(size)) == -1) {
                    const auto error = last_error();
                    ::close(handle_);
                    handle_ = g_invalid_handle;
                    ::shm_unlink(shm_name.data());
                    NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
                }
                size_ = size;
            } else {
                size_ = static_cast<size_t>(stat_buf.st_size);
            }
        }
    }

    is_open_ = true;
#endif
}

void share_memory::close() noexcept {
    unmap();

    if (handle_ != g_invalid_handle) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CloseHandle(handle_);
#else
        ::close(handle_);
#endif
        handle_ = g_invalid_handle;
    }

    is_open_ = false;
    name_.clear();
    size_ = 0;
}

void* share_memory::map(size_t offset, const size_t length) {
    if (!is_open_) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory not open"));
    }

    if (mapped_addr_ != nullptr) {
        unmap();
    }

    if (offset > size_) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception(
                ("Offset " + to_string(offset) + " exceeds shared memory size " + to_string(size_)).data()));
    }

    size_t map_length = (length == 0) ? (size_ - offset) : length;

    if (length != 0 && (map_length > size_ || offset > size_ - map_length)) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception(("Map region [offset=" + to_string(offset) +
                                                        ", length=" + to_string(map_length) +
                                                        "] exceeds shared memory size (" + to_string(size_) + ")")
                                                               .data()));
    }

    const size_t aligned_offset = (offset / granularity) * granularity;
    const size_t padding = offset - aligned_offset;
    map_length += padding;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD access = (access_mode_ == access_mode::read_only) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

#    ifdef NEFORCE_ARCH_BITS_64
    const auto offset_high = static_cast<::DWORD>(aligned_offset >> 32);
    const auto offset_low = static_cast<::DWORD>(aligned_offset & 0xFFFFFFFF);
#    else
    constexpr ::DWORD offset_high = 0;
    const auto offset_low = static_cast<::DWORD>(aligned_offset);
#    endif

    original_mapped_addr_ = ::MapViewOfFile(handle_, access, offset_high, offset_low, map_length);
    if (original_mapped_addr_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("MapViewOfFile failed"));
    }
#else
    int prot = (access_mode_ == access_mode::read_only) ? PROT_READ : (PROT_READ | PROT_WRITE);

    original_mapped_addr_ =
            ::mmap(nullptr, map_length, prot, MAP_SHARED, handle_, static_cast<::off_t>(aligned_offset));

    if (original_mapped_addr_ == MAP_FAILED) {
        original_mapped_addr_ = nullptr;
        const auto error = _NEFORCE last_error();
        NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
    }
#endif

    mapped_addr_ = static_cast<char*>(original_mapped_addr_) + padding;
    internal_mapped_size_ = map_length;
    mapped_size_ = (length == 0) ? (size_ - offset) : length;
    return mapped_addr_;
}

void share_memory::unmap() noexcept {
    if (original_mapped_addr_ == nullptr) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::UnmapViewOfFile(original_mapped_addr_);
#else
    ::munmap(original_mapped_addr_, internal_mapped_size_);
#endif

    original_mapped_addr_ = nullptr;
    mapped_addr_ = nullptr;
    mapped_size_ = 0;
    internal_mapped_size_ = 0;
}

bool share_memory::flush(bool async) {
    if (original_mapped_addr_ == nullptr) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    (void) async;
    return ::FlushViewOfFile(original_mapped_addr_, internal_mapped_size_) != 0;
#else
    const int flags = async ? MS_ASYNC : MS_SYNC;
    return ::msync(original_mapped_addr_, internal_mapped_size_, flags) == 0;
#endif
}

bool share_memory::remove(const string& name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    native_handle_type h = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.data());
    if (h != nullptr) {
        ::CloseHandle(h);
        h = ::OpenFileMappingA(FILE_MAP_READ, FALSE, name.data());
        if (h != nullptr) {
            ::CloseHandle(h);
            return false;
        }
        return true;
    }
    return true;
#else
    const string shm_name = normalize_name(name);
    return ::shm_unlink(shm_name.data()) == 0;
#endif
}

bool share_memory::exists(const string& name) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const native_handle_type h = ::OpenFileMappingA(FILE_MAP_READ, FALSE, name.data());
    if (h != g_invalid_handle) {
        ::CloseHandle(h);
        return true;
    }
    return false;
#else
    const string shm_name = normalize_name(name);
    const int fd = ::shm_open(shm_name.data(), O_RDONLY, 0666);
    if (fd != g_invalid_handle) {
        ::close(fd);
        return true;
    }
    return false;
#endif
}

NEFORCE_END_NAMESPACE__
