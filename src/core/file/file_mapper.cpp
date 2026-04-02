#include <NeForce/core/file/file_mapper.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/system/sysinfo.hpp>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    const file_mapper::native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            INVALID_HANDLE_VALUE;
#else
            -1;
#endif

    void do_unmap(void*& ptr, file_mapper::size_type& size, file_mapper::size_type& offset
#ifdef NEFORCE_PLATFORM_WINDOWS
                  ,
                  file_mapper::native_handle_type& mapping_handle, const file_mapper::native_handle_type invalid
#endif
                  ) noexcept {
        if (!ptr) {
            return;
        }

#ifdef NEFORCE_PLATFORM_WINDOWS
        const uint32_t granularity = sysinfo::instance().get_system_info().allocation_granularity;
        const uintptr_t delta = static_cast<uintptr_t>(offset) & ~(static_cast<uintptr_t>(granularity) - 1u);
        const uintptr_t base_addr = reinterpret_cast<uintptr_t>(ptr) -
                                    (static_cast<uintptr_t>(offset) & (static_cast<uintptr_t>(granularity) - 1u));
        ::UnmapViewOfFile(reinterpret_cast<::LPVOID>(base_addr));

        if (mapping_handle != invalid) {
            ::CloseHandle(mapping_handle);
            mapping_handle = invalid;
        }
#else
        const long page_size = ::sysconf(_SC_PAGESIZE);
        if (page_size > 0) {
            const size_t page_mask = static_cast<size_t>(page_size) - 1;
            const uintptr_t base_addr = reinterpret_cast<uintptr_t>(ptr) - (static_cast<uintptr_t>(offset) & page_mask);
            const size_t total = size + (static_cast<size_t>(offset) & page_mask);
            ::munmap(reinterpret_cast<void*>(base_addr), total);
        }
#endif
        ptr = nullptr;
        size = 0;
        offset = 0;
    }
} // namespace


file_mapper::file_mapper(const native_handle_type file_handle) :
file_handle_(file_handle) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    mapping_handle_ = invalid_handle;
#endif
}

file_mapper::~file_mapper() { unmap(); }

file_mapper::file_mapper(file_mapper&& other) noexcept :
file_handle_(other.file_handle_),
ptr_(other.ptr_),
size_(other.size_),
offset_(other.offset_),
access_(other.access_)
#ifdef NEFORCE_PLATFORM_WINDOWS
,
mapping_handle_(other.mapping_handle_)
#endif
{
    other.ptr_ = nullptr;
    other.size_ = 0;
    other.offset_ = 0;
    other.file_handle_ = invalid_handle;
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.mapping_handle_ = invalid_handle;
#endif
}

file_mapper& file_mapper::operator=(file_mapper&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }

    unmap();
    file_handle_ = other.file_handle_;
    ptr_ = other.ptr_;
    size_ = other.size_;
    offset_ = other.offset_;
    access_ = other.access_;
    other.file_handle_ = invalid_handle;
#ifdef NEFORCE_PLATFORM_WINDOWS
    mapping_handle_ = other.mapping_handle_;
    other.mapping_handle_ = invalid_handle;
#endif
    other.ptr_ = nullptr;
    other.size_ = 0;
    other.offset_ = 0;

    return *this;
}

bool file_mapper::map(const size_type offset, size_type size, const file_access access, const file_map_hint hint) {
    lock<mutex> lk(mutex_);

#ifdef NEFORCE_PLATFORM_WINDOWS
    do_unmap(ptr_, size_, offset_, mapping_handle_, invalid_handle);
#else
    do_unmap(ptr_, size_, offset_);
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
    const uint32_t granularity = sysinfo::instance().get_system_info().allocation_granularity;
    const uint64_t aligned_offset = static_cast<uint64_t>(offset) & ~static_cast<uint64_t>(granularity - 1);
    const uint64_t offset_delta = static_cast<uint64_t>(offset) - aligned_offset;
    const uint64_t aligned_size = (size == 0) ? 0 : static_cast<uint64_t>(size) + offset_delta;

    ::DWORD protect, map_access;
    if (static_cast<fud_t>(access) & static_cast<fud_t>(file_access::WRITE)) {
        protect = PAGE_READWRITE;
        map_access = FILE_MAP_WRITE | FILE_MAP_READ;
    } else {
        protect = PAGE_READONLY;
        map_access = FILE_MAP_READ;
    }

    mapping_handle_ = ::CreateFileMappingW(file_handle_, nullptr, protect, static_cast<::DWORD>(aligned_size >> 32),
                                           static_cast<::DWORD>(aligned_size & 0xFFFFFFFF), nullptr);

    if (!mapping_handle_ || mapping_handle_ == invalid_handle) {
        mapping_handle_ = invalid_handle;
        return false;
    }

    void* base =
            ::MapViewOfFile(mapping_handle_, map_access, static_cast<::DWORD>(aligned_offset >> 32),
                            static_cast<::DWORD>(aligned_offset & 0xFFFFFFFF), static_cast<::SIZE_T>(aligned_size));

    if (!base) {
        ::CloseHandle(mapping_handle_);
        mapping_handle_ = invalid_handle;
        return false;
    }

    ptr_ = static_cast<char*>(base) + static_cast<ptrdiff_t>(offset_delta);

    if (size == 0) {
        ::MEMORY_BASIC_INFORMATION mbi{};
        if (::VirtualQuery(ptr_, &mbi, sizeof(mbi))) {
            size = static_cast<size_type>(mbi.RegionSize);
        }
    }

    const ::HMODULE hK32 = ::GetModuleHandleA("kernel32.dll");
    if (hK32) {
        using PFN = ::BOOL(__stdcall*)(::HANDLE, ::ULONG_PTR, ::PWIN32_MEMORY_RANGE_ENTRY, ::ULONG);
        static auto pfn = reinterpret_cast<PFN>(::GetProcAddress(hK32, "PrefetchVirtualMemory"));
        if (pfn && hint == file_map_hint::SEQUENTIAL) {
            ::WIN32_MEMORY_RANGE_ENTRY range{ptr_, size};
            pfn(::GetCurrentProcess(), 1, &range, 0);
        }
    }

#else
    const long page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return false;
    }

    if (size == 0) {
        struct ::stat64 st{};
        if (::fstat64(file_handle_, &st) == -1) {
            return false;
        }
        const uint64_t file_size = static_cast<uint64_t>(st.st_size);
        if (file_size <= static_cast<uint64_t>(offset)) {
            return false;
        }
        size = static_cast<size_type>(file_size - static_cast<uint64_t>(offset));
    }

    const size_t page_mask = static_cast<size_t>(page_size) - 1;
    const size_t aligned_off = static_cast<size_t>(offset) & ~page_mask;
    const size_t offset_delta = static_cast<size_t>(offset) - aligned_off;
    const size_t aligned_size = static_cast<size_t>(size) + offset_delta;

    int prot = PROT_READ;
    const auto af = static_cast<fud_t>(access);
    if (af & O_RDWR) {
        prot = PROT_READ | PROT_WRITE;
    } else if (af & O_WRONLY) {
        prot = PROT_WRITE;
    }

    void* base = ::mmap(nullptr, aligned_size, prot, MAP_SHARED, file_handle_, static_cast<::off_t>(aligned_off));
    if (base == MAP_FAILED) {
        return false;
    }

    int advice = MADV_NORMAL;
    switch (hint) {
        case file_map_hint::SEQUENTIAL: {
            advice = MADV_SEQUENTIAL;
            break;
        }
        case file_map_hint::RANDOM: {
            advice = MADV_RANDOM;
            break;
        }
        default: {
            break;
        }
    }
    ::madvise(base, aligned_size, advice);
    ptr_ = static_cast<char*>(base) + offset_delta;

#endif

    offset_ = offset;
    size_ = size;
    access_ = access;
    return true;
}

void file_mapper::unmap() noexcept {
    lock<mutex> lk(mutex_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    do_unmap(ptr_, size_, offset_, mapping_handle_, invalid_handle);
#else
    do_unmap(ptr_, size_, offset_);
#endif
}

bool file_mapper::remap(const size_type new_offset, const size_type new_size) {
    const file_access current_access = access_;
    unmap();
    return map(new_offset, new_size, current_access);
}

bool file_mapper::flush(const bool async) noexcept {
    lock<mutex> lk(mutex_);

    if (!ptr_) {
        return false;
    }
    if (!(static_cast<fud_t>(access_) & static_cast<fud_t>(file_access::WRITE))) {
        return true;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!::FlushViewOfFile(ptr_, size_)) {
        return false;
    }
    if (!async && mapping_handle_ != invalid_handle) {
        return ::FlushFileBuffers(file_handle_) != 0;
    }
    return true;

#else
    const long page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return false;
    }

    const size_t page_mask = static_cast<size_t>(page_size) - 1;
    const uintptr_t base_addr = reinterpret_cast<uintptr_t>(ptr_) - (static_cast<uintptr_t>(offset_) & page_mask);
    const size_t total = size_ + (static_cast<size_t>(offset_) & page_mask);

    return ::msync(reinterpret_cast<void*>(base_addr), total, async ? MS_ASYNC : MS_SYNC) == 0;
#endif
}

bool file_mapper::lock_pages(const bool lock_in_memory) const noexcept {
    if (!ptr_) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return lock_in_memory ? ::VirtualLock(ptr_, size_) != 0 : ::VirtualUnlock(ptr_, size_) != 0;

#else
    const long page_size = ::sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return false;
    }

    const size_t page_mask = static_cast<size_t>(page_size) - 1;
    const uintptr_t base_addr = reinterpret_cast<uintptr_t>(ptr_) - (static_cast<uintptr_t>(offset_) & page_mask);
    const size_t total = size_ + (static_cast<size_t>(offset_) & page_mask);

    return lock_in_memory ? ::mlock(reinterpret_cast<void*>(base_addr), total) == 0
                          : ::munlock(reinterpret_cast<void*>(base_addr), total) == 0;
#endif
}

file_mapper::map_info file_mapper::info() const noexcept {
    lock<mutex> lk(mutex_);
    map_info mi{};
    mi.address = ptr_;
    mi.size = size_;
    mi.offset = offset_;
    mi.access = access_;
    mi.is_mapped = (ptr_ != nullptr);
    return mi;
}

NEFORCE_END_NAMESPACE__
