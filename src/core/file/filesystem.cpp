#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/system/environment.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <dirent.h>
#    include <cerrno>
#    include <cstdio>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    bool wide_dot_name(const wchar_t* name) {
        return name[0] == L'.' && (name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0'));
    }

    bool remove_all_in_directory_w(const wstring& dir, const bool recursive) {
        bool success = true;
        wstring pattern = dir;
        pattern += L"\\*";
        ::WIN32_FIND_DATAW fd{};
        const ::HANDLE h_find = ::FindFirstFileW(pattern.data(), &fd);
        if (h_find == INVALID_HANDLE_VALUE) {
            return false;
        }
        do {
            if (wide_dot_name(fd.cFileName)) {
                continue;
            }
            wstring full = dir;
            full += L'\\';
            full += fd.cFileName;
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
                if (recursive) {
                    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0U) {
                        if (!remove_all_in_directory_w(full, true)) {
                            success = false;
                        }
                    }
                    if (::RemoveDirectoryW(full.data()) == FALSE) {
                        success = false;
                    }
                }
            } else {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0U) {
                    ::SetFileAttributesW(full.data(), fd.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
                }
                if (::DeleteFileW(full.data()) == FALSE) {
                    ::SetFileAttributesW(full.data(), FILE_ATTRIBUTE_NORMAL);
                    if (::DeleteFileW(full.data()) == FALSE) {
                        success = false;
                    }
                }
            }
        } while (::FindNextFileW(h_find, &fd) == TRUE);
        ::FindClose(h_find);
        return success;
    }

    bool copy_file_w(const wstring& from_w, const wstring& to_w, const bool overwrite) {
        if (overwrite) {
            const ::DWORD attrs = ::GetFileAttributesW(to_w.data());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY) != 0U) {
                ::SetFileAttributesW(to_w.data(), attrs & ~FILE_ATTRIBUTE_READONLY);
            }
        }
        if (::CopyFileW(from_w.data(), to_w.data(), static_cast<::BOOL>(!overwrite)) == FALSE) {
            return false;
        }

        const ::HANDLE h_src = ::CreateFileW(from_w.data(), FILE_READ_ATTRIBUTES,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                             OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        const ::HANDLE h_dst = ::CreateFileW(to_w.data(), FILE_WRITE_ATTRIBUTES,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                             OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (h_src != INVALID_HANDLE_VALUE && h_dst != INVALID_HANDLE_VALUE) {
            ::FILETIME at{};
            ::FILETIME wt{};
            if (::GetFileTime(h_src, nullptr, &at, &wt) != FALSE) {
                ::SetFileTime(h_dst, nullptr, &at, &wt);
            }
        }
        if (h_src != INVALID_HANDLE_VALUE) {
            ::CloseHandle(h_src);
        }
        if (h_dst != INVALID_HANDLE_VALUE) {
            ::CloseHandle(h_dst);
        }
        return true;
    }

    bool copy_directory_w(const wstring& src_w, const wstring& dest_w, const bool overwrite) {
        bool success = true;
        wstring pattern = src_w;
        pattern += L"\\*";
        ::WIN32_FIND_DATAW fd{};
        const ::HANDLE h_find = ::FindFirstFileW(pattern.data(), &fd);
        if (h_find == INVALID_HANDLE_VALUE) {
            return ::GetLastError() == ERROR_FILE_NOT_FOUND;
        }
        do {
            if (wide_dot_name(fd.cFileName)) {
                continue;
            }
            wstring sp = src_w;
            sp += L'\\';
            sp += fd.cFileName;
            wstring dp = dest_w;
            dp += L'\\';
            dp += fd.cFileName;
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U) {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
                    success = false;
                    continue;
                }
                if (::CreateDirectoryW(dp.data(), nullptr) == FALSE && ::GetLastError() != ERROR_ALREADY_EXISTS) {
                    success = false;
                    continue;
                }
                if (!copy_directory_w(sp, dp, overwrite)) {
                    success = false;
                }
            } else {
                if (!copy_file_w(sp, dp, overwrite)) {
                    success = false;
                }
            }
        } while (::FindNextFileW(h_find, &fd) == TRUE);
        ::FindClose(h_find);
        return success;
    }
#endif // NEFORCE_PLATFORM_WINDOWS

    bool path_within(const string& src, const string& dest) {
        string s = src;
        string d = dest;
        while (!s.empty() && (s.back() == '/' || s.back() == '\\')) {
            s.pop_back();
        }
        while (!d.empty() && (d.back() == '/' || d.back() == '\\')) {
            d.pop_back();
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        for (char& c: s) {
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }
        for (char& c: d) {
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }
#endif
        if (d == s) {
            return true;
        }
        return d.size() > s.size() && d.starts_with(s) && (d[s.size()] == '/' || d[s.size()] == '\\');
    }
} // namespace


bool filesystem::create_directories(const path& p) {
    if (p.empty()) {
        return false;
    }
    if (p.is_directory()) {
        return true;
    }

    const string& ps = p.str();
    size_t pos = 0;
    string subdir;

#ifdef NEFORCE_PLATFORM_WINDOWS
    while ((pos = ps.find_first_of(path::spliter, pos + 1)) != string::npos) {
        subdir = ps.head(pos);
        if (!subdir.empty() && !path::is_directory(subdir)) {
            wstring wsubdir = character::to_wstring(subdir.view());
            if (::CreateDirectoryW(wsubdir.data(), nullptr) == FALSE && ::GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }
    }

    const wstring wps = character::to_wstring(ps.view());
    return ::CreateDirectoryW(wps.data(), nullptr) == TRUE || ::GetLastError() == ERROR_ALREADY_EXISTS;

#else
    while ((pos = ps.find_first_of(path::spliter, pos + 1)) != string::npos) {
        subdir = ps.head(pos);
        if (::mkdir(subdir.data(), 0755) == -1 && errno != EEXIST) {
            return false;
        }
    }
    return ::mkdir(ps.data(), 0755) == 0 || errno == EEXIST;
#endif
}

bool filesystem::remove(const path& p) {
    if (!p.is_file()) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    const wstring wp = character::to_wstring(p.view());
    return ::DeleteFileW(wp.data()) != 0;
#else
    return ::unlink(p.data()) == 0;
#endif
}

bool filesystem::remove_directory(const path& p) {
    if (!p.is_directory()) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    const wstring wp = character::to_wstring(p.view());
    return ::RemoveDirectoryW(wp.data()) != 0;
#else
    return ::rmdir(p.data()) == 0;
#endif
}

bool filesystem::remove_all_in_directory(const path& p, const bool recursive) {
    if (!p.is_directory()) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    const wstring wp = character::to_wstring(p.view());
    return remove_all_in_directory_w(wp, recursive);
#else
    ::DIR* dir = ::opendir(p.data());
    if (dir == nullptr) {
        return false;
    }

    bool success = true;
    const ::dirent* entry = nullptr;

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    while ((entry = ::readdir(dir)) != nullptr) {
        const string_view name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }
        const path full = p / path{name};

        bool is_dir = false;
#    ifdef _DIRENT_HAVE_D_TYPE
        if (entry->d_type == DT_DIR) {
            is_dir = true;
        } else if (entry->d_type == DT_UNKNOWN) {
            struct ::stat64 st{};
            // lstat: symbolic links are entries to unlink, never directories to descend into
            if (::lstat64(full.data(), &st) == 0 && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
                is_dir = true;
            }
        }
        // DT_LNK and any other type are unlinked as plain entries;
        // following a symlink here could delete the contents of the tree it points to.
#    else
        struct ::stat64 st{};
        if (::lstat64(full.data(), &st) == 0 && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
            is_dir = true;
        }
#    endif

        if (is_dir) {
            if (recursive) {
                if (!remove_all_in_directory(full, true)) {
                    success = false;
                }
                if (::rmdir(full.data()) != 0 && errno != ENOTEMPTY) {
                    success = false;
                }
            }
        } else {
            if (::unlink(full.data()) != 0) {
                if (errno == EACCES) {
                    ::chmod(full.data(), 0644);
                    if (::unlink(full.data()) != 0) {
                        success = false;
                    }
                } else {
                    success = false;
                }
            }
        }
    }

    ::closedir(dir);
    return success;
#endif
}

bool filesystem::remove_all(const path& p) {
    if (p.is_file()) {
        return remove(p);
    }
    if (!p.is_directory()) {
        return false;
    }

    bool ok = remove_all_in_directory(p, true);
#ifdef NEFORCE_PLATFORM_WINDOWS
    const wstring wp = character::to_wstring(p.view());
    if (::RemoveDirectoryW(wp.data()) == FALSE) {
        ok = false;
    }
#else
    if (::rmdir(p.data()) != 0) {
        ok = false;
    }
#endif
    return ok;
}

bool filesystem::copy(const path& from, const path& to, const bool overwrite) {
    if (!from.exists() || from.is_directory()) {
        return false;
    }

    path actual_to = to;
    if (to.is_directory()) {
        actual_to = to / path{from.filename()};
    }

    if (!overwrite && actual_to.exists()) {
        return false;
    }
    if (actual_to.str() == from.str()) {
        return true;
    }

    const path dest_parent = actual_to.parent_path();
    if (!dest_parent.empty() && !dest_parent.exists()) {
        if (!create_directories(dest_parent)) {
            return false;
        }
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return copy_file_w(character::to_wstring(from.view()), character::to_wstring(actual_to.view()), overwrite);
#else
    const int src_fd = ::open(from.data(), O_RDONLY);
    if (src_fd == -1) {
        return false;
    }

    struct ::stat64 st{};
    if (::fstat64(src_fd, &st) == -1) {
        ::close(src_fd);
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        ::close(src_fd);
        return false; // refuse FIFOs/devices because opening them could block forever
    }

    // aliases (hard links / symlinks) of the destination
    // must never be truncated before the source is read.
    struct ::stat64 dst_st{};
    if (::stat64(actual_to.data(), &dst_st) == 0 && dst_st.st_dev == st.st_dev && dst_st.st_ino == st.st_ino) {
        ::close(src_fd);
        return true;
    }

    const bool is_new_file = !actual_to.exists();
    const int flags = O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL);
    const int dst_fd = ::open(actual_to.data(), flags, 0644);
    if (dst_fd == -1) {
        ::close(src_fd);
        return false;
    }

    bool ok = true;
    bool use_range = true;
    for (;;) {
        if (use_range) {
            // fall back to the user-space loop when unsupported.
            const ssize_t copied = ::copy_file_range(src_fd, nullptr, dst_fd, nullptr, 64U << 20, 0);
            if (copied > 0) {
                continue;
            }
            if (copied == 0) {
                break;
            }
            if (errno == EXDEV || errno == EINVAL || errno == ENOSYS || errno == EOPNOTSUPP || errno == EPERM) {
                use_range = false;
                continue;
            }
            ok = false;
            break;
        }

        char buf[256U << 10];
        for (;;) {
            const ssize_t r = ::read(src_fd, buf, sizeof(buf));
            if (r == -1) {
                if (errno == EINTR) {
                    continue;
                }
                ok = false;
                break;
            }
            if (r == 0) {
                break;
            }
            ssize_t off = 0;
            while (off < r) {
                const ssize_t w = ::write(dst_fd, buf + off, static_cast<size_t>(r - off));
                if (w == -1) {
                    if (errno == EINTR) {
                        continue;
                    }
                    ok = false;
                    break;
                }
                off += w;
            }
            if (!ok) {
                break;
            }
        }
        break;
    }

    if (ok) {
        ::fchmod(dst_fd, st.st_mode & 0777);
        const ::timespec times[2] = {st.st_atim, st.st_mtim};
        ::futimens(dst_fd, times);
    }

    ::close(src_fd);
    ::close(dst_fd);

    if (!ok && is_new_file) {
        ::unlink(actual_to.data());
    }
    return ok;
#endif
}

bool filesystem::copy_directory(const path& src, const path& dest, const bool overwrite) {
    if (!src.is_directory()) {
        return false;
    }
    // Never copy a directory into itself or its own subtree
    if (path_within(src.str(), dest.str())) {
        return false;
    }
    if (!dest.exists() && !create_directories(dest)) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return copy_directory_w(character::to_wstring(src.view()), character::to_wstring(dest.view()), overwrite);
#else
    ::DIR* dir = ::opendir(src.data());
    if (dir == nullptr) {
        return false;
    }

    bool success = true;
    const ::dirent* entry = nullptr;

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    while ((entry = ::readdir(dir)) != nullptr) {
        const string_view name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }
        const path sp = src / path{name};
        const path dp = dest / path{name};

        bool is_dir = false;
#    ifdef _DIRENT_HAVE_D_TYPE
        if (entry->d_type == DT_DIR) {
            is_dir = true;
        } else if (entry->d_type == DT_UNKNOWN) {
            struct ::stat64 st{};
            if (::lstat64(sp.data(), &st) == 0 && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
                is_dir = true;
            }
        }
#    else
        struct ::stat64 st{};
        if (::lstat64(sp.data(), &st) == 0 && S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
            is_dir = true;
        }
#    endif

        if (is_dir) {
            if (!copy_directory(sp, dp, overwrite)) {
                success = false;
            }
        } else {
            if (!copy(sp, dp, overwrite)) {
                success = false;
            }
        }
    }
    ::closedir(dir);
    return success;
#endif
}

bool filesystem::move(const path& from, const path& to, const bool overwrite) {
    if (!from.exists()) {
        return false;
    }

    if (!overwrite && to.exists()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD flags = MOVEFILE_COPY_ALLOWED;
    if (overwrite) {
        flags |= MOVEFILE_REPLACE_EXISTING;
    }

    const wstring wfrom = character::to_wstring(from.view());
    const wstring wto = character::to_wstring(to.view());

    if (::MoveFileExW(wfrom.data(), wto.data(), flags) == TRUE) {
        return true;
    }

    if (::GetLastError() == ERROR_NOT_SAME_DEVICE) {
        if (from.is_directory()) {
            return copy_directory(from, to, overwrite) && remove_all_in_directory(from, true) && remove_directory(from);
        }
        return copy(from, to, overwrite) && remove(from);
    }

    // MoveFileEx cannot replace a non-empty directory even with MOVEFILE_REPLACE_EXISTING;
    // delete the destination directory and retry once when overwrite was requested.
    if (overwrite && to.is_directory()) {
        if (remove_all_in_directory(to, true) && remove_directory(to)) {
            if (::MoveFileExW(wfrom.data(), wto.data(), flags) == TRUE) {
                return true;
            }
        }
    }
    return false;

#else
    // rename replaces an existing destination file (or empty directory) atomically,
    // so never delete the destination first, that would open a window in which a failed rename loses data.
    // Only a non-empty destination directory cannot be replaced by rename;
    // fall back to delete-then-rename for that single case.
    if (::rename(from.data(), to.data()) == 0) {
        return true;
    }

    if (errno == EXDEV) {
        if (from.is_directory()) {
            return copy_directory(from, to, overwrite) && remove_all_in_directory(from, true) && remove_directory(from);
        }
        return copy(from, to, overwrite) && remove(from);
    }

    if ((errno == ENOTEMPTY || errno == EEXIST) && overwrite && to.is_directory()) {
        if (!remove_all_in_directory(to, true) || !remove_directory(to)) {
            return false;
        }
        if (::rename(from.data(), to.data()) == 0) {
            return true;
        }
    }
    return false;
#endif
}

bool filesystem::rename(const path& old_name, const path& new_name) { return move(old_name, new_name, true); }

bool filesystem::create_and_write(const path& p, const string& content, const bool append) {
    const path parent = p.parent_path();
    if (!parent.empty() && !parent.exists()) {
        if (!create_directories(parent)) {
            return false;
        }
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    file f;
    if (!f.open(p, append, append ? file_access::append : file_access::write, file_shared::no_share,
                file_creation::open_force, file_attri::normal)) {
        return false;
    }
    const file::size_type written = f.write(content, content.size());
    return written == content.size();

#else
    int flags = O_WRONLY | O_CREAT;
    flags |= append ? O_APPEND : O_TRUNC;

    const int fd = ::open(p.data(), flags, 0644);
    if (fd == -1) {
        return false;
    }

    const ssize_t written = ::write(fd, content.data(), content.size());
    ::close(fd);
    return written == static_cast<ssize_t>(content.size());
#endif
}

byte_size filesystem::size(const path& p) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WIN32_FILE_ATTRIBUTE_DATA data{};
    const wstring wp = character::to_wstring(p.view());
    if (::GetFileAttributesExW(wp.data(), ::GetFileExInfoStandard, &data) == FALSE) {
        return byte_size{0};
    }
    ::ULARGE_INTEGER ul{};
    ul.LowPart = data.nFileSizeLow;
    ul.HighPart = data.nFileSizeHigh;
    return byte_size{ul.QuadPart};
#else
    struct ::stat64 st{};
    if (::stat64(p.data(), &st) == -1) {
        return byte_size{0};
    }
    if (S_ISDIR(st.st_mode)) {
        return byte_size{0};
    }
    return byte_size{static_cast<uint64_t>(st.st_size)};
#endif
}

path filesystem::temp_directory() { return path{environment::temp_directory()}; }

path filesystem::home_directory() { return path{environment::home_directory()}; }

path filesystem::app_data_directory() { return path{environment::app_data_directory()}; }

path filesystem::config_directory() { return path{environment::config_directory()}; }

NEFORCE_END_NAMESPACE__
