#include <NeForce/core/file/file_locker.hpp>
NEFORCE_BEGIN_NAMESPACE__

file_locker::file_locker(const native_handle_type handle) noexcept :
handle_(handle) {}

bool file_locker::lock(const difference_type offset, const difference_type length,
                       const file_lock mode) const noexcept {
    if (offset < 0 || length < 0) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::OVERLAPPED ov{};
    const ::ULARGE_INTEGER off_ul = {static_cast<::DWORD>(offset & 0xFFFFFFFF),
                                     static_cast<::DWORD>(static_cast<uint64_t>(offset) >> 32)};
    ov.Offset = off_ul.LowPart;
    ov.OffsetHigh = off_ul.HighPart;

    ::DWORD len_lo = 0xFFFFFFFF, len_hi = 0xFFFFFFFF;
    if (length != 0) {
        const ::ULARGE_INTEGER len_ul = {static_cast<::DWORD>(length & 0xFFFFFFFF),
                                         static_cast<::DWORD>(static_cast<uint64_t>(length) >> 32)};
        len_lo = len_ul.LowPart;
        len_hi = len_ul.HighPart;
    }

    ::DWORD flags = 0;
    if ((static_cast<fud_t>(mode) & LOCKFILE_EXCLUSIVE_LOCK) != 0) {
        flags |= LOCKFILE_EXCLUSIVE_LOCK;
    }
    if ((static_cast<fud_t>(mode) & LOCKFILE_FAIL_IMMEDIATELY) != 0) {
        flags |= LOCKFILE_FAIL_IMMEDIATELY;
    }

    return ::LockFileEx(handle_, flags, 0, len_lo, len_hi, &ov) != 0;

#else
    struct ::flock fl{};
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;

    if ((static_cast<fud_t>(mode) & LOCK_EX) != 0) {
        fl.l_type = F_WRLCK;
    } else {
        fl.l_type = F_RDLCK;
    }

    const int cmd = ((static_cast<fud_t>(mode) & LOCK_NB) != 0) ? F_SETLK : F_SETLKW;
    return ::fcntl(handle_, cmd, &fl) == 0;
#endif
}

bool file_locker::unlock(const difference_type offset, const difference_type length) const noexcept {
    if (offset < 0 || length < 0) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::OVERLAPPED ov{};
    const ::ULARGE_INTEGER off_ul = {static_cast<::DWORD>(offset & 0xFFFFFFFF),
                                     static_cast<::DWORD>(static_cast<uint64_t>(offset) >> 32)};
    ov.Offset = off_ul.LowPart;
    ov.OffsetHigh = off_ul.HighPart;

    ::DWORD len_lo = 0xFFFFFFFF, len_hi = 0xFFFFFFFF;
    if (length != 0) {
        const ::ULARGE_INTEGER len_ul = {static_cast<::DWORD>(length & 0xFFFFFFFF),
                                         static_cast<::DWORD>(static_cast<uint64_t>(length) >> 32)};
        len_lo = len_ul.LowPart;
        len_hi = len_ul.HighPart;
    }
    return ::UnlockFileEx(handle_, 0, len_lo, len_hi, &ov) != 0;

#else
    struct ::flock fl{};
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;
    return ::fcntl(handle_, F_SETLK, &fl) == 0;
#endif
}

bool file_locker::try_lock(const difference_type offset, const difference_type length,
                           const file_lock mode) const noexcept {
    return lock(offset, length, mode | file_lock::FAIL_IMMEDIATELY);
}

bool file_locker::is_locked(const difference_type offset, const difference_type length,
                            file_lock* lock_out) const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    // Windows cannot directly check the lock status;
    // it can only detect it by attempting a non-blocking shared lock
    // This operation will temporarily lock the file area
    if (try_lock(offset, length, file_lock::SHARED)) {
        ignore = unlock(offset, length);
        if (lock_out != nullptr) {
            *lock_out = file_lock::SHARED;
        }
        return false;
    }
    if (lock_out != nullptr) {
        *lock_out = file_lock::EXCLUSIVE;
    }
    return true;

#else
    struct ::flock fl{};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;

    if (::fcntl(handle_, F_GETLK, &fl) == -1) {
        return false;
    }

    if (fl.l_type == F_UNLCK) {
        if (lock_out) {
            *lock_out = file_lock::SHARED;
        }
        return false;
    }
    if (lock_out) {
        *lock_out = (fl.l_type == F_RDLCK) ? file_lock::SHARED : file_lock::EXCLUSIVE;
    }
    return true;
#endif
}

bool file_locker::lock_whole(const file_lock mode) const noexcept { return lock(0, 0, mode); }

bool file_locker::unlock_whole() const noexcept { return unlock(0, 0); }

file_lock_guard::file_lock_guard(file_locker& locker, const difference_type offset, const difference_type length,
                                 const file_lock mode) :
locker_(locker),
offset_(offset),
length_(length),
locked_(locker_.lock(offset, length, mode)) {}

file_lock_guard::~file_lock_guard() {
    if (locked_) {
        ignore = locker_.unlock(offset_, length_);
    }
}

bool file_lock_guard::unlock() noexcept {
    if (!locked_) {
        return false;
    }
    if (locker_.unlock(offset_, length_)) {
        locked_ = false;
        return true;
    }
    return false;
}

NEFORCE_END_NAMESPACE__
