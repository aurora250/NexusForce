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
#    include <NeForce/core/async/atomic_base.hpp>
#    include <NeForce/core/exception/error_code.hpp>
#    include <fcntl.h>
#    include <pthread.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_LINUX
    struct shm_header {
        pthread_mutex_t mutex;
        int ready; /**< 0=uninit, 1=initing, 2=ready */
    };

    constexpr size_t k_header_size = (sizeof(shm_header) + 63) & ~size_t{63};

    static_assert(k_header_size >= sizeof(shm_header), "Header size must accommodate shm_header");

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
is_open_(other.is_open_),
data_offset_(other.data_offset_),
mutex_owner_(other.mutex_owner_)
#ifdef NEFORCE_PLATFORM_WINDOWS
,
mutex_handle_(other.mutex_handle_)
#endif
{
    other.handle_ = g_invalid_handle;
    other.size_ = 0;
    other.internal_mapped_size_ = 0;
    other.mapped_size_ = 0;
    other.original_mapped_addr_ = nullptr;
    other.mapped_addr_ = nullptr;
    other.is_open_ = false;
    other.data_offset_ = 0;
    other.mutex_owner_ = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.mutex_handle_ = nullptr;
#endif
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
    data_offset_ = other.data_offset_;
    mutex_owner_ = other.mutex_owner_;
#ifdef NEFORCE_PLATFORM_WINDOWS
    mutex_handle_ = other.mutex_handle_;
#endif

    other.handle_ = g_invalid_handle;
    other.size_ = 0;
    other.internal_mapped_size_ = 0;
    other.mapped_size_ = 0;
    other.original_mapped_addr_ = nullptr;
    other.mapped_addr_ = nullptr;
    other.is_open_ = false;
    other.data_offset_ = 0;
    other.mutex_owner_ = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.mutex_handle_ = nullptr;
#endif

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

    auto get_real_size = [this]() -> size_t {
        const void* temp_view = ::MapViewOfFile(handle_, FILE_MAP_READ, 0, 0, 0);
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

    data_offset_ = k_header_size;

    int flags = (access == access_mode::read_only) ? O_RDONLY : O_RDWR;

    if (mode == open_mode::create_only) {
        if (size == 0) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("Size must be greater than 0 for create_only mode"));
        }

        flags |= O_CREAT | O_EXCL;
        handle_ = ::shm_open(shm_name.data(), flags, 0666);
        if (handle_ == g_invalid_handle) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception(last_error().message().data()));
        }

        mutex_owner_ = true;

        if (::ftruncate(handle_, static_cast<::off_t>(data_offset_ + size)) == -1) {
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

        if (stat_buf.st_size <= static_cast<::off_t>(data_offset_)) {
            ::close(handle_);
            handle_ = g_invalid_handle;
            NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory exists but has insufficient size"));
        }

        size_ = static_cast<size_t>(stat_buf.st_size) - data_offset_;
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

            if (stat_buf.st_size <= static_cast<::off_t>(data_offset_)) {
                ::close(handle_);
                handle_ = g_invalid_handle;
                NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory exists but has insufficient size"));
            }

            size_ = static_cast<size_t>(stat_buf.st_size) - data_offset_;
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
                mutex_owner_ = true;

                if (::ftruncate(handle_, static_cast<::off_t>(data_offset_ + size)) == -1) {
                    const auto error = last_error();
                    ::close(handle_);
                    handle_ = g_invalid_handle;
                    ::shm_unlink(shm_name.data());
                    NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
                }
                size_ = size;
            } else {
                size_ = static_cast<size_t>(stat_buf.st_size) - data_offset_;
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

void share_memory::grow(size_t new_size) {
    if (!is_open_) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory not open"));
    }
    if (new_size <= size_) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("New size must be greater than current size"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD protect = (access_mode_ == access_mode::read_only) ? PAGE_READONLY : PAGE_READWRITE;
    const ::DWORD access_flags = (access_mode_ == access_mode::read_only) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;

    const bool was_mapped = mapped_addr_ != nullptr;
    vector<char> saved_data;
    if (was_mapped) {
        saved_data.assign(static_cast<char*>(mapped_addr_), static_cast<char*>(mapped_addr_) + mapped_size_);
        unmap();
    }

    ::CloseHandle(handle_);

#    ifdef NEFORCE_ARCH_BITS_64
    const auto size_high = static_cast<::DWORD>(new_size >> 32);
    const auto size_low = static_cast<::DWORD>(new_size & 0xFFFFFFFF);
#    else
    constexpr ::DWORD size_high = 0;
    const auto size_low = static_cast<::DWORD>(new_size);
#    endif

    handle_ = ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, protect, size_high, size_low, name_.data());
    if (handle_ == g_invalid_handle) {
        is_open_ = false;
        NEFORCE_THROW_EXCEPTION(share_memory_exception("CreateFileMapping failed during grow"));
    }

    size_ = new_size;
    data_offset_ = 0;

    if (was_mapped) {
        constexpr auto offset_high = static_cast<::DWORD>(0);
        constexpr auto offset_low = static_cast<::DWORD>(0);
        original_mapped_addr_ = ::MapViewOfFile(handle_, access_flags, offset_high, offset_low, new_size);
        if (original_mapped_addr_ == nullptr) {
            NEFORCE_THROW_EXCEPTION(share_memory_exception("MapViewOfFile failed during grow"));
        }
        mapped_addr_ = original_mapped_addr_;
        internal_mapped_size_ = new_size;
        mapped_size_ = new_size;

        if (!saved_data.empty()) {
            memcpy(mapped_addr_, saved_data.data(), saved_data.size());
        }

        if (mutex_handle_ == nullptr) {
            const string mutex_name = "NeForce_shm_mutex_" + name_;
            mutex_handle_ = ::CreateMutexA(nullptr, FALSE, mutex_name.data());
        }
    }
#else
    const size_t fd_new_size = data_offset_ + new_size;
    if (::ftruncate(handle_, static_cast<::off_t>(fd_new_size)) == -1) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
    }

    size_ = new_size;

    if (mapped_addr_ != nullptr) {
        void* old_addr = original_mapped_addr_;
        size_t old_size = internal_mapped_size_;

        original_mapped_addr_ = ::mremap(old_addr, old_size, fd_new_size, MREMAP_MAYMOVE);
        if (original_mapped_addr_ == MAP_FAILED) {
            original_mapped_addr_ = nullptr;
            mapped_addr_ = nullptr;
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
        }
        mapped_addr_ = static_cast<char*>(original_mapped_addr_) + data_offset_;
        internal_mapped_size_ = fd_new_size;
        mapped_size_ = new_size;
    }
#endif
}

void share_memory::lock() {
    if (mapped_addr_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory must be mapped before locking"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (mutex_handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("Mutex not initialized; call map() first"));
    }
    ::WaitForSingleObject(mutex_handle_, INFINITE);
#else
    auto* header = static_cast<shm_header*>(original_mapped_addr_);
    int ret = ::pthread_mutex_lock(&header->mutex);
    if (ret == EOWNERDEAD) {
        ::pthread_mutex_consistent(&header->mutex);
    }
#endif
}

void share_memory::unlock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (mutex_handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("Mutex not initialized; call map() first"));
    }
    ::ReleaseMutex(mutex_handle_);
#else
    if (original_mapped_addr_ != nullptr) {
        auto* header = static_cast<shm_header*>(original_mapped_addr_);
        ::pthread_mutex_unlock(&header->mutex);
    }
#endif
}

bool share_memory::try_lock() {
    if (mapped_addr_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(share_memory_exception("Shared memory must be mapped before locking"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (mutex_handle_ == nullptr) {
        return false;
    }
    return ::WaitForSingleObject(mutex_handle_, 0) == WAIT_OBJECT_0;
#else
    auto* header = static_cast<shm_header*>(original_mapped_addr_);
    int ret = ::pthread_mutex_trylock(&header->mutex);
    if (ret == EBUSY) {
        return false;
    }
    if (ret == EOWNERDEAD) {
        ::pthread_mutex_consistent(&header->mutex);
    }
    return (ret == 0 || ret == EOWNERDEAD);
#endif
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

    data_offset_ = 0;

    if (mutex_handle_ == nullptr) {
        const string mutex_name = "NeForce_shm_mutex_" + name_;
        mutex_handle_ = ::CreateMutexA(nullptr, FALSE, mutex_name.data());
    }
#else
    int prot = (access_mode_ == access_mode::read_only) ? PROT_READ : (PROT_READ | PROT_WRITE);

    // Always map from file offset 0 to include the shared-memory header
    const size_t total_map = data_offset_ + offset + map_length;
    original_mapped_addr_ = ::mmap(nullptr, total_map, prot, MAP_SHARED, handle_, 0);

    if (original_mapped_addr_ == MAP_FAILED) {
        original_mapped_addr_ = nullptr;
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(share_memory_exception(error.message().data()));
    }

    // Initialize the process-shared mutex using CAS to elect a single initializer
    {
        auto* header = static_cast<shm_header*>(original_mapped_addr_);
        if (access_mode_ == access_mode::read_only) {
            while (atomic_load(&header->ready, memory_order::acquire) != 2) {
                // spin-wait for the initializer to complete
            }
        } else {
            int expected = 0;
            if (atomic_cmpexch_strong(&header->ready, &expected, 1, memory_order::acquire, memory_order::relaxed)) {
                ::pthread_mutexattr_t attr;
                ::pthread_mutexattr_init(&attr);
                ::pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
                ::pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
                ::pthread_mutex_init(&header->mutex, &attr);
                ::pthread_mutexattr_destroy(&attr);
                mutex_owner_ = true;
                atomic_store(&header->ready, 2, memory_order::release);
            } else {
                while (atomic_load(&header->ready, memory_order::acquire) != 2) {
                    // spin-wait for the initializer to complete
                }
            }
        }
    }
#endif

    mapped_addr_ = static_cast<char*>(original_mapped_addr_) + data_offset_ + offset;
    internal_mapped_size_ =
#ifdef NEFORCE_PLATFORM_WINDOWS
            map_length;
#else
            total_map;
#endif
    mapped_size_ = (length == 0) ? (size_ - offset) : length;
    return mapped_addr_;
}

void share_memory::unmap() noexcept {
    if (original_mapped_addr_ == nullptr) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::UnmapViewOfFile(original_mapped_addr_);
    if (mutex_handle_ != nullptr) {
        ::CloseHandle(mutex_handle_);
        mutex_handle_ = nullptr;
    }
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
