#include <MSTL/core/file/path.hpp>
#include <MSTL/core/container/vector.hpp>
#include <MSTL/core/system/environment.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#endif
MSTL_BEGIN_NAMESPACE__

path path::parent_path() const {
    if (path_.empty()) return path{};

    const size_t last_sep = path_.find_last_of(FILE_SPLITER);
    if (last_sep == string::npos) return path{};
    if (last_sep == 0) return path{"/"};

    return path{path_.substr(0, last_sep)};
}

string_view path::filename() const noexcept {
    if (path_.empty()) return {};

    const size_t last_sep = path_.find_last_of(FILE_SPLITER);
    if (last_sep == string::npos) return path_.view();
    return path_.view().substr(last_sep + 1);
}

string_view path::stem() const noexcept {
    const string_view fname = filename();
    if (fname.empty()) return {};

    const size_t last_dot = fname.find_last_of('.');
    if (last_dot == string::npos || last_dot == 0) return fname;

    return fname.substr(0, last_dot);
}

string_view path::extension() const noexcept {
    return path::extension(path_.view());
}

string_view path::extension(const string_view path) noexcept {
    const size_t last_sep = path.find_last_of(FILE_SPLITER);
    const string_view filename = last_sep == string::npos ? path : path.substr(last_sep + 1);

    const size_t last_dot = filename.find_last_of('.');
    if (last_dot == string::npos || last_dot == 0 || last_dot == filename.size() - 1) {
        return {};
    }
    return filename.substr(last_dot + 1);
}

path path::lexically_normal() const noexcept {
    if (path_.empty()) return path(".");

    string result;
    vector<string_view> parts;

    size_t start = 0;
    const size_t length = path_.size();

    bool is_absolute = false;
#ifdef MSTL_PLATFORM_WINDOWS__
    if (length >= 2 && path_[1] == ':') {
        result += path_.substr(0, 2);
        start = 2;
        if (length > 2 && (path_[2] == '\\' || path_[2] == '/')) {
            is_absolute = true;
            start = 3;
        }
    } else if (length >= 2 && (path_[0] == '\\' && path_[1] == '\\')) {
        is_absolute = true;
        result += "\\\\";
        start = 2;
    } else if (!path_.empty() && (path_[0] == '\\' || path_[0] == '/')) {
        is_absolute = true;
        start = 1;
    }
#else
    if (!path_.empty() && path_[0] == '/') {
        is_absolute = true;
        start = 1;
    }
#endif

    size_t pos = start;
    while (pos < length) {
        size_t next_sep = pos;
        while (next_sep < length && path_[next_sep] != '/' && path_[next_sep] != '\\') {
            ++next_sep;
        }

        if (next_sep > pos) {
            string_view part = path_.view().substr(pos, next_sep - pos);
            parts.push_back(part);
        }

        pos = (next_sep < length) ? next_sep + 1 : length;
    }

    vector<string_view> normalized;
    for (const auto& part : parts) {
        if (part.empty() || part == ".") {
            continue;
        } else if (part == "..") {
            if (!normalized.empty() && normalized.back() != "..") {
                normalized.pop_back();
            } else if (!is_absolute) {
                normalized.push_back(part);
            }
        } else {
            normalized.push_back(part);
        }
    }

    if (is_absolute) {
#ifdef MSTL_PLATFORM_WINDOWS__
        if (result.empty() ||
            (result.size() == 2 && result[1] == ':') ||
            result == "\\\\") {
            result += PREFERRED_SEPARATOR;
        }
#else
        result += '/';
#endif
    }

    for (size_t i = 0; i < normalized.size(); ++i) {
        if (i > 0) result += PREFERRED_SEPARATOR;
        result += normalized[i];
    }

    if (result.empty()) {
        if (is_absolute) {
#ifdef MSTL_PLATFORM_WINDOWS__
            return path("C:\\");
#else
            return path("/");
#endif
        } else {
            return path(".");
        }
    }

    return path(_MSTL move(result));
}

path path::absolute(const path& base) const {
    if (this->empty()) return path{};

#ifdef MSTL_PLATFORM_WINDOWS__
    char buffer[MAX_PATH];
    if (::GetFullPathNameA(path_.c_str(), MAX_PATH, buffer, nullptr) == 0) {
        return *this;
    }
    return path(string(buffer));

#elif defined(MSTL_PLATFORM_LINUX__)
    char buf[PATH_MAX];
    if (::realpath(path_.c_str(), buf) != nullptr) {
        return path(string(buf));
    } else {
        if (path_.empty()) {
            return base;
        }
        if (path_[0] == '/') {
            return *this;
        }
        const path joined = base / *this;
        return joined.lexically_normal();
    }
#endif
}

path path::relative(const path& base) const {
    const path abs_path = this->absolute();
    const path abs_base = base.absolute();

    vector<string_view> this_parts;
    for (auto it = abs_path.begin(); it != abs_path.end(); ++it) this_parts.push_back(*it);

    vector<string_view> base_parts;
    for (auto it = abs_base.begin(); it != abs_base.end(); ++it) base_parts.push_back(*it);

    size_t i = 0;
    const size_t n = _MSTL min(this_parts.size(), base_parts.size());
    while (i < n && this_parts[i] == base_parts[i]) ++i;

    string result;
    for (size_t j = i; j < base_parts.size(); ++j) {
        if (!result.empty()) result += PREFERRED_SEPARATOR;
        result += "..";
    }
    for (size_t j = i; j < this_parts.size(); ++j) {
        if (!result.empty()) result += PREFERRED_SEPARATOR;
        result += this_parts[j];
    }
    if (result.empty()) result = ".";

    return path(_MSTL move(result));
}


path path::current_path() {
    return path(environment::current_directory());
}

path path::temp_directory_path() {
    string temp(environment::temp_directory());
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!temp.empty() && (temp.back() == '\\' || temp.back() == '/')) {
        temp.pop_back();
    }
#endif
    return path(_MSTL move(temp));
}

path& path::operator /=(const path& p) {
    if (p.empty()) return *this;
    if (path_.empty()) {
        path_ = p.path_;
        return *this;
    }
    if (path_.back() == PREFERRED_SEPARATOR && !p.path_.empty() &&
        p.path_.front() == PREFERRED_SEPARATOR) {
        path_ += p.path_.substr(1);
    } else if (path_.back() != PREFERRED_SEPARATOR && !p.path_.empty() &&
        p.path_.front() != PREFERRED_SEPARATOR) {
        path_ += PREFERRED_SEPARATOR;
        path_ += p.path_;
    } else {
        path_ += p.path_;
    }
    return *this;
}

path& path::operator /=(const string_view p) {
    if (p.empty()) return *this;
    if (path_.empty()) {
        path_ = p;
        return *this;
    }
    if (path_.back() == PREFERRED_SEPARATOR && !p.empty() &&
        p.front() == PREFERRED_SEPARATOR) {
        path_ += p.substr(1);
    } else if (path_.back() != PREFERRED_SEPARATOR && !p.empty() &&
        p.front() != PREFERRED_SEPARATOR) {
        path_ += PREFERRED_SEPARATOR;
        path_ += p;
    } else {
        path_ += p;
    }
    return *this;
}

path path::operator /(const path& p) const {
    path result = *this;
    result /= p;
    return result;
}

path path::operator /(const string_view p) const {
    path result = *this;
    result /= p;
    return result;
}

bool path::exists() const noexcept {
    return path::exists(path_);
}

bool path::exists(const string& path) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::GetFileAttributesA(path.data()) != INVALID_FILE_ATTRIBUTES;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    return ::stat64(path.data(), &st) != -1;
#endif
}

bool path::is_directory() const noexcept {
    return path::is_directory(path_);
}

bool path::is_directory(const string& path) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::DWORD attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::stat64(path.data(), &st) == -1) return false;
    return S_ISDIR(st.st_mode);
#endif
}

bool path::is_file() const noexcept {
    return path::is_file(path_);
}

bool path::is_file(const string& path) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::DWORD attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY);
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::stat64 st{};
    if (::stat64(path.data(), &st) == -1) return false;
    return S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
#endif
}

bool path::create_directories() const {
    return path::create_directories(path_);
}

bool path::create_directories(const string& path) {
    if (path.empty()) return false;
    if (path::is_directory(path.view())) return true;

    size_t pos = 0;
    string subdir;

#ifdef MSTL_PLATFORM_WINDOWS__
    while ((pos = path.find_first_of(FILE_SPLITER, pos + 1)) != string::npos) {
        subdir = path.substr(0, pos);
        if (!subdir.empty() && !path::is_directory(subdir)) {
            if (!::CreateDirectoryA(subdir.data(), nullptr)) {
                if (::GetLastError() != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            }
        }
    }
    return ::CreateDirectoryA(path.data(), nullptr) ||
        ::GetLastError() == ERROR_ALREADY_EXISTS;
#elif defined(MSTL_PLATFORM_LINUX__)
    while ((pos = path.find_first_of(FILE_SPLITER, pos + 1)) != string::npos) {
        subdir = path.substr(0, pos);
        if (::mkdir(subdir.data(), 0755) == -1 && errno != EEXIST) {
            return false;
        }
    }
    return ::mkdir(path.data(), 0755) == 0 || errno == EEXIST;
#endif
}

bool path::remove() const noexcept {
    return path::remove(path_);
}

bool path::remove(const string& path) noexcept {
    if (path::is_file(path.view())) {
#ifdef MSTL_PLATFORM_WINDOWS__
        return ::DeleteFileA(path.data()) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
        return ::unlink(path.data()) == 0;
#endif
    }
    return false;
}

bool path::remove_directory() const noexcept {
    return path::remove_directory(path_);
}

bool path::remove_directory(const string& path) noexcept {
    if (path::is_directory(path.view())) {
#ifdef MSTL_PLATFORM_WINDOWS__
        return ::RemoveDirectoryA(path.data()) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
        return ::rmdir(path.data()) == 0;
#endif
    }
    return false;
}

bool path::remove_all_in_directory(const bool recursive) const noexcept {
    return path::remove_all_in_directory(path_, recursive);
}

bool path::remove_all_in_directory(const string& directory_path, const bool recursive) noexcept {
    if (!path::is_directory(directory_path.view())) return false;
    bool success = true;

#ifdef MSTL_PLATFORM_WINDOWS__
    const string search_pattern = directory_path + "\\*";
    ::WIN32_FIND_DATAA find_data;
    const ::HANDLE find_handle = ::FindFirstFileA(search_pattern.c_str(), &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) return false;

    do {
        string item_name = find_data.cFileName;
        if (item_name == "." || item_name == "..") continue;
        string full_path = directory_path + "\\" + item_name;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                if (!remove_all_in_directory(full_path, true)) {
                    success = false;
                }
                if (!::RemoveDirectoryA(full_path.c_str())) {
                    if (::GetLastError() != ERROR_DIR_NOT_EMPTY) {
                        success = false;
                    }
                }
            }
        } else {
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                ::SetFileAttributesA(full_path.c_str(),
                    find_data.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
            }
            if (!::DeleteFileA(full_path.c_str())) {
                const ::DWORD error = ::GetLastError();
                if (error == ERROR_ACCESS_DENIED) {
                    ::SetFileAttributesA(full_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (!::DeleteFileA(full_path.c_str())) {
                        success = false;
                    }
                } else {
                    success = false;
                }
            }
        }
    } while (::FindNextFileA(find_handle, &find_data) != 0);
    ::FindClose(find_handle);

#elif defined(MSTL_PLATFORM_LINUX__)
    ::DIR* dir = ::opendir(directory_path.c_str());
    if (dir == nullptr) return false;

    ::dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        string item_name = entry->d_name;
        if (item_name == "." || item_name == "..") continue;

        string full_path = directory_path + "/" + item_name;

        if (path::is_directory(full_path.view())) {
            if (recursive) {
                if (!remove_all_in_directory(full_path, true)) {
                    success = false;
                }
                if (::rmdir(full_path.c_str()) != 0 && errno != ENOTEMPTY) {
                    success = false;
                }
            }
        } else {
            if (::unlink(full_path.c_str()) != 0) {
                if (errno == EACCES) {
                    ::chmod(full_path.c_str(), 0644);
                    if (::unlink(full_path.c_str()) != 0) {
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

bool path::remove_all() const noexcept {
    return remove_all(path_);
}

bool path::remove_all(const string& path) noexcept {
    if (path::is_file(path)) {
        return path::remove(path);
    }
    if (!path::is_directory(path)) {
        return false;
    }

    bool success = remove_all_in_directory(path, true);
#ifdef MSTL_PLATFORM_WINDOWS__
    if (!::RemoveDirectoryA(path.c_str())) {
        success = false;
    }
#elif defined(MSTL_PLATFORM_LINUX__)
    if (::rmdir(path.c_str()) != 0) {
        success = false;
    }
#endif
    return success;
}

bool path::copy(const path& to, const bool overwrite) const {
    return path::copy(*this, to, overwrite);
}

bool path::copy(const path& from, const path& to, const bool overwrite) {
    if (!from.exists()) return false;
    if (from.is_directory()) return false;

    if (!overwrite && to.exists()) return false;

    const path dest_parent = to.parent_path();
    if (!dest_parent.empty() && !dest_parent.exists()) {
        if (!dest_parent.create_directories()) return false;
    }

    path actual_to = to;
    if (to.exists() && to.is_directory()) {
        const string_view filename = from.filename();
        actual_to = to / path{string{filename}};
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    if (overwrite && actual_to.exists()) {
        const ::DWORD attrs = ::GetFileAttributesA(actual_to.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
            ::SetFileAttributesA(actual_to.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
        }
    }

    if (::CopyFileA(from.c_str(), actual_to.c_str(), !overwrite)) {
        return true;
    }

    return false;

#elif defined(MSTL_PLATFORM_LINUX__)
    const int source_fd = ::open(from.c_str(), O_RDONLY);
    if (source_fd == -1) return false;

    struct ::stat64 st{};
    if (::fstat64(source_fd, &st) == -1) {
        ::close(source_fd);
        return false;
    }

    int flags = O_WRONLY | O_CREAT;
    if (overwrite) {
        flags |= O_TRUNC;
    } else {
        flags |= O_EXCL;
    }

    const int dest_fd = ::open(actual_to.c_str(), flags, 0644);
    if (dest_fd == -1) {
        ::close(source_fd);
        return false;
    }

    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytes_read;
    bool success = true;

    while ((bytes_read = ::read(source_fd, buffer, FILE_BUFFER_SIZE)) > 0) {
        const ssize_t bytes_written = ::write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            success = false;
            break;
        }
    }

    if (bytes_read < 0) success = false;

    if (success) {
        ::fchmod(dest_fd, st.st_mode & 0777);

        ::timeval times[2];
        times[0].tv_sec = st.st_atime;
        times[0].tv_usec = 0;
        times[1].tv_sec = st.st_mtime;
        times[1].tv_usec = 0;
        ::futimes(dest_fd, times);
    }

    ::close(source_fd);
    ::close(dest_fd);

    if (!success) {
        ::unlink(actual_to.c_str());
    }

    return success;
#endif
}

bool path::copy_directory(const path& destination, const bool overwrite) const {
    return path::copy_directory(*this, destination, overwrite);
}

bool path::copy_directory(const path& source, const path& destination, const bool overwrite) {
    if (!source.is_directory()) return false;

    if (!destination.exists() && !destination.create_directories()) {
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    const string search_pattern = source.str() + "\\*";
    ::WIN32_FIND_DATAA find_data;
    const ::HANDLE hFind = ::FindFirstFileA(search_pattern.c_str(), &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        if (::GetLastError() == ERROR_FILE_NOT_FOUND) return true;
        return false;
    }

    bool success = true;

    do {
        string item = find_data.cFileName;
        if (item == "." || item == "..") continue;

        path src_path = source / path{item};
        path dst_path = destination / path{item};

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!copy_directory(src_path, dst_path, overwrite)) {
                success = false;
            }
        } else {
            if (!path::copy(src_path, dst_path, overwrite)) {
                success = false;
            }
        }
    } while (::FindNextFileA(hFind, &find_data) != 0);

    ::FindClose(hFind);
    return success;

#elif defined(MSTL_PLATFORM_LINUX__)
    ::DIR* dir = ::opendir(source.c_str());
    if (dir == nullptr) return false;

    bool success = true;
    ::dirent* entry;

    while ((entry = ::readdir(dir)) != nullptr) {
        string item = entry->d_name;
        if (item == "." || item == "..") continue;

        path src_path = source / path{item};
        path dst_path = destination / path{item};

        if (src_path.is_directory()) {
            if (!path::copy_directory(src_path, dst_path, overwrite)) {
                success = false;
            }
        } else {
            if (!path::copy(src_path, dst_path, overwrite)) {
                success = false;
            }
        }
    }

    ::closedir(dir);
    return success;
#endif
}

bool path::move(const path& to, const bool overwrite) const noexcept {
    return path::move(*this, to, overwrite);
}

bool path::move(const path& from, const path& to, const bool overwrite) noexcept {
    if (!from.exists()) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD flags = MOVEFILE_COPY_ALLOWED;
    if (overwrite) {
        flags |= MOVEFILE_REPLACE_EXISTING;
    }

    if (::MoveFileExA(from.c_str(), to.c_str(), flags)) {
        return true;
    }

    const ::DWORD error = ::GetLastError();

    if (error == ERROR_NOT_SAME_DEVICE) {
        if (from.is_directory()) {
            if (path::copy_directory(from, to, overwrite)) {
                return from.remove_all_in_directory(true) && from.remove_directory();
            }
        } else {
            if (path::copy(from, to, overwrite)) {
                return from.remove();
            }
        }
    }

    return false;

#elif defined(MSTL_PLATFORM_LINUX__)
    if (overwrite) {
        if (to.exists()) {
            if (to.is_directory()) {
                if (!to.remove_all_in_directory(true) || !to.remove_directory()) {
                    return false;
                }
            } else {
                if (!to.remove()) {
                    return false;
                }
            }
        }
    }

    if (::rename(from.c_str(), to.c_str()) == 0) {
        return true;
    }

    if (errno == EXDEV) {
        if (from.is_directory()) {
            if (path::copy_directory(from, to, overwrite)) {
                return from.remove_all_in_directory(true) && from.remove_directory();
            }
        } else {
            if (path::copy(from, to, overwrite)) {
                return from.remove();
            }
        }
    }

    return false;
#endif
}

bool path::rename(const path& new_name) const {
    return path::move(*this, new_name, true);
}

bool path::rename(const path& old_name, const path& new_name) {
    return path::move(old_name, new_name, true);
}

bool path::operator ==(const path& rh) const noexcept {
    const path lhs_norm = this->lexically_normal();
    const path rhs_norm = rh.lexically_normal();

#ifdef MSTL_PLATFORM_WINDOWS__
    return string_compare_ignore_case(lhs_norm.c_str(), rhs_norm.c_str()) == 0;
#else
    return lhs_norm.str() == rhs_norm.str();
#endif
}

bool path::operator <(const path& rh) const noexcept {
    const path lhs_norm = lexically_normal();
    const path rhs_norm = rh.lexically_normal();

#ifdef MSTL_PLATFORM_WINDOWS__
    return string_compare_ignore_case(lhs_norm.c_str(), rhs_norm.c_str()) < 0;
#else
    return lhs_norm.str() < rhs_norm.str();
#endif
}

size_t path::to_hash() const {
    const path norm_path = lexically_normal();

    if (norm_path.empty()) {
        return hash<const char*>()("");
    }
#ifdef MSTL_PLATFORM_WINDOWS__
    auto lower_str = norm_path.str();
    lower_str.lowercase();
    return hash<string>()(lower_str);
#else
    return hash<string>()(norm_path.str());
#endif
}

MSTL_END_NAMESPACE__
