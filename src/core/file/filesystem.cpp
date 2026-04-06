#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/filesystem.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <dirent.h>
#    include <errno.h>
#    include <stdio.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

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
        subdir = ps.substr(0, pos);
        if (!subdir.empty() && !path(subdir).is_directory()) {
            if (!::CreateDirectoryA(subdir.data(), nullptr) && ::GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }
    }
    return ::CreateDirectoryA(ps.data(), nullptr) || ::GetLastError() == ERROR_ALREADY_EXISTS;

#else
    while ((pos = ps.find_first_of(path::spliter, pos + 1)) != string::npos) {
        subdir = ps.substr(0, pos);
        if (::mkdir(subdir.data(), 0755) == -1 && errno != EEXIST) {
            return false;
        }
    }
    return ::mkdir(ps.data(), 0755) == 0 || errno == EEXIST;
#endif
}

bool filesystem::remove(const path& p) noexcept {
    if (!p.is_file()) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::DeleteFileA(p.data()) != 0;
#else
    return ::unlink(p.data()) == 0;
#endif
}

bool filesystem::remove_directory(const path& p) noexcept {
    if (!p.is_directory()) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::RemoveDirectoryA(p.data()) != 0;
#else
    return ::rmdir(p.data()) == 0;
#endif
}

bool filesystem::remove_all_in_directory(const path& p, const bool recursive) {
    if (!p.is_directory()) {
        return false;
    }
    bool success = true;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string pattern = p.str() + "\\*";
    ::WIN32_FIND_DATAA fd{};
    const ::HANDLE hFind = ::FindFirstFileA(pattern.data(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        const string name = fd.cFileName;
        if (name == "." || name == "..") {
            continue;
        }
        const path full = p / path{name};

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                if (!remove_all_in_directory(full, true)) {
                    success = false;
                }
                if (!::RemoveDirectoryA(full.data())) {
                    success = false;
                }
            }
        } else {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                ::SetFileAttributesA(full.data(), fd.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
            }
            if (!::DeleteFileA(full.data())) {
                ::SetFileAttributesA(full.data(), FILE_ATTRIBUTE_NORMAL);
                if (!::DeleteFileA(full.data())) {
                    success = false;
                }
            }
        }
    } while (::FindNextFileA(hFind, &fd) != 0);
    ::FindClose(hFind);

#else
    ::DIR* dir = ::opendir(p.data());
    if (!dir) {
        return false;
    }

    ::dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        const string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        const path full = p / path{name};

        if (full.is_directory()) {
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

#endif

    return success;
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
    if (!::RemoveDirectoryA(p.data())) {
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
    if (!overwrite && to.exists()) {
        return false;
    }

    const path dest_parent = to.parent_path();
    if (!dest_parent.empty() && !dest_parent.exists()) {
        if (!create_directories(dest_parent)) {
            return false;
        }
    }

    path actual_to = to;
    if (to.is_directory()) {
        actual_to = to / path{string{from.filename()}};
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (overwrite && actual_to.exists()) {
        const ::DWORD attrs = ::GetFileAttributesA(actual_to.data());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
            ::SetFileAttributesA(actual_to.data(), attrs & ~FILE_ATTRIBUTE_READONLY);
        }
    }
    return ::CopyFileA(from.data(), actual_to.data(), !overwrite) != 0;

#else
    const int src_fd = ::open(from.data(), O_RDONLY);
    if (src_fd == -1) {
        return false;
    }

    struct ::stat st{};
    if (::fstat(src_fd, &st) == -1) {
        ::close(src_fd);
        return false;
    }

    const bool is_new_file = !actual_to.exists();
    const int flags = O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL);
    const int dst_fd = ::open(actual_to.data(), flags, 0644);
    if (dst_fd == -1) {
        ::close(src_fd);
        return false;
    }

    char buf[8192];
    bool ok = true;
    ::ssize_t r;
    while ((r = ::read(src_fd, buf, sizeof(buf))) > 0) {
        if (::write(dst_fd, buf, static_cast<size_t>(r)) != r) {
            ok = false;
            break;
        }
    }
    if (r < 0) {
        ok = false;
    }

    if (ok) {
        ::fchmod(dst_fd, st.st_mode & 0777);
        struct ::timespec times[2] = {st.st_atim, st.st_mtim};
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
    if (!dest.exists() && !create_directories(dest)) {
        return false;
    }

    bool success = true;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string pattern = src.str() + "\\*";
    ::WIN32_FIND_DATAA fd{};

    const ::HANDLE hFind = ::FindFirstFileA(pattern.data(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return ::GetLastError() == ERROR_FILE_NOT_FOUND;
    }

    do {
        const string item = fd.cFileName;
        if (item == "." || item == "..") {
            continue;
        }
        const path sp = src / path{item};
        const path dp = dest / path{item};
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!copy_directory(sp, dp, overwrite)) {
                success = false;
            }
        } else {
            if (!copy(sp, dp, overwrite)) {
                success = false;
            }
        }
    } while (::FindNextFileA(hFind, &fd) != 0);
    ::FindClose(hFind);

#else
    ::DIR* dir = ::opendir(src.data());
    if (!dir) {
        return false;
    }
    ::dirent* entry;

    while ((entry = ::readdir(dir)) != nullptr) {
        const string item = entry->d_name;
        if (item == "." || item == "..") {
            continue;
        }

        const path sp = src / path{item};
        const path dp = dest / path{item};

        if (sp.is_directory()) {
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

#endif
    return success;
}

bool filesystem::move(const path& from, const path& to, const bool overwrite) {
    if (!from.exists()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD flags = MOVEFILE_COPY_ALLOWED;
    if (overwrite) {
        flags |= MOVEFILE_REPLACE_EXISTING;
    }

    if (::MoveFileExA(from.data(), to.data(), flags)) {
        return true;
    }

    if (::GetLastError() == ERROR_NOT_SAME_DEVICE) {
        if (from.is_directory()) {
            return copy_directory(from, to, overwrite) && remove_all_in_directory(from, true) && remove_directory(from);
        }
        return copy(from, to, overwrite) && remove(from);
    }
    return false;

#else
    if (overwrite && to.exists()) {
        if (to.is_directory()) {
            if (!remove_all_in_directory(to, true) || !remove_directory(to)) {
                return false;
            }
        } else {
            if (!remove(to)) {
                return false;
            }
        }
    }

    if (::rename(from.data(), to.data()) == 0) {
        return true;
    }

    if (errno == EXDEV) {
        if (from.is_directory()) {
            return copy_directory(from, to, overwrite) && remove_all_in_directory(from, true) && remove_directory(from);
        }
        return copy(from, to, overwrite) && remove(from);
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
    if (!f.open(p, append, append ? file_access::APPEND : file_access::WRITE, file_shared::NO_SHARE,
                file_creation::OPEN_FORCE, file_attri::NORMAL)) {
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

    const ::ssize_t written = ::write(fd, content.data(), content.size());
    ::close(fd);
    return written == static_cast<::ssize_t>(content.size());
#endif
}

size_t filesystem::size(const path& p) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!::GetFileAttributesExA(p.data(), GetFileExInfoStandard, &data)) {
        return 0;
    }
    ::ULARGE_INTEGER ul{};
    ul.LowPart = data.nFileSizeLow;
    ul.HighPart = data.nFileSizeHigh;
    return static_cast<size_t>(ul.QuadPart);
#else
    struct ::stat64 st{};
    if (::stat64(p.data(), &st) == -1) {
        return 0;
    }
    return static_cast<size_t>(st.st_size);
#endif
}

NEFORCE_END_NAMESPACE__
