#include <NeForce/core/file/path.hpp>
#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/system/environment.hpp>
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
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

void path::split_iterator::find_next() {
    const size_t sz = path_->size();
    const size_t pos = start_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (pos == 0 && sz > 1 && (*path_)[1] == ':') {
        current_part_ = path_->substr(0, 2);
        start_ = 2;
        while (start_ < sz && ((*path_)[start_] == '/' || (*path_)[start_] == '\\')) {
            ++start_;
        }
        end_ = start_ - 1;
        done_ = false;
        return;
    }
#endif

    const size_t sep_pos = path_->find_first_of(spliter, pos);
    if (sep_pos == string::npos) {
        current_part_ = path_->substr(pos);
        end_ = sz;
    } else {
        current_part_ = path_->substr(pos, sep_pos - pos);
        end_ = sep_pos;
    }
}

path path::parent_path() const {
    if (path_.empty()) return path{};

    const size_t last_sep = path_.find_last_of(spliter);
    if (last_sep == string::npos) return path{};
    if (last_sep == 0) return path{"/"};

    return path{path_.substr(0, last_sep)};
}

string_view path::filename() const noexcept {
    if (path_.empty()) return {};

    const size_t last_sep = path_.find_last_of(spliter);
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
    const size_t last_sep = path.find_last_of(spliter);
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
#ifdef NEFORCE_PLATFORM_WINDOWS
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
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (result.empty() ||
            (result.size() == 2 && result[1] == ':') ||
            result == "\\\\") {
            result += preferred_separator;
        }
#else
        result += '/';
#endif
    }

    for (size_t i = 0; i < normalized.size(); ++i) {
        if (i > 0) result += preferred_separator;
        result += normalized[i];
    }

    if (result.empty()) {
        if (is_absolute) {
#ifdef NEFORCE_PLATFORM_WINDOWS
            return path("C:\\");
#else
            return path("/");
#endif
        } else {
            return path(".");
        }
    }

    return path(_NEFORCE move(result));
}

path path::absolute(const path& base) const {
    if (this->empty()) return path{};

#ifdef NEFORCE_PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    if (::GetFullPathNameA(path_.data(), MAX_PATH, buffer, nullptr) == 0) {
        return *this;
    }
    return path(string(buffer));

#elif defined(NEFORCE_PLATFORM_LINUX)
    char buf[PATH_MAX];
    if (::realpath(path_.data(), buf) != nullptr) {
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
    const size_t n = _NEFORCE min(this_parts.size(), base_parts.size());
    while (i < n && this_parts[i] == base_parts[i]) ++i;

    string result;
    for (size_t j = i; j < base_parts.size(); ++j) {
        if (!result.empty()) result += preferred_separator;
        result += "..";
    }
    for (size_t j = i; j < this_parts.size(); ++j) {
        if (!result.empty()) result += preferred_separator;
        result += this_parts[j];
    }
    if (result.empty()) result = ".";

    return path(_NEFORCE move(result));
}


path path::current_path() {
    return path(environment::current_directory());
}

path path::temp_directory_path() {
    string temp(environment::temp_directory());
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!temp.empty() && (temp.back() == '\\' || temp.back() == '/')) {
        temp.pop_back();
    }
#endif
    return path(_NEFORCE move(temp));
}

path& path::operator /=(const path& other) {
    if (other.empty()) return *this;
    if (path_.empty()) {
        path_ = other.path_;
        return *this;
    }
    if (path_.back() == preferred_separator && !other.path_.empty() &&
        other.path_.front() == preferred_separator) {
        path_ += other.path_.substr(1);
    } else if (path_.back() != preferred_separator && !other.path_.empty() &&
        other.path_.front() != preferred_separator) {
        path_ += preferred_separator;
        path_ += other.path_;
    } else {
        path_ += other.path_;
    }
    return *this;
}

path& path::operator /=(const string_view path) {
    if (path.empty()) return *this;
    if (path_.empty()) {
        path_ = path;
        return *this;
    }
    if (path_.back() == preferred_separator && !path.empty() &&
        path.front() == preferred_separator) {
        path_ += path.substr(1);
    } else if (path_.back() != preferred_separator && !path.empty() &&
        path.front() != preferred_separator) {
        path_ += preferred_separator;
        path_ += path;
    } else {
        path_ += path;
    }
    return *this;
}

path path::operator /(const path& other) const {
    path result = *this;
    result /= other;
    return result;
}

path path::operator /(const string_view path) const {
    _NEFORCE path result = *this;
    result /= path;
    return result;
}

bool path::exists() const noexcept {
    return path::exists(path_);
}

bool path::exists(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetFileAttributesA(path.data()) != INVALID_FILE_ATTRIBUTES;
#elif defined(NEFORCE_PLATFORM_LINUX)
    struct ::stat64 st{};
    return ::stat64(path.data(), &st) != -1;
#endif
}

bool path::is_directory() const noexcept {
    return path::is_directory(path_);
}

bool path::is_directory(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#elif defined(NEFORCE_PLATFORM_LINUX)
    struct ::stat64 st{};
    if (::stat64(path.data(), &st) == -1) return false;
    return S_ISDIR(st.st_mode);
#endif
}

bool path::is_file() const noexcept {
    return path::is_file(path_);
}

bool path::is_file(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY);
#elif defined(NEFORCE_PLATFORM_LINUX)
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

#ifdef NEFORCE_PLATFORM_WINDOWS
    while ((pos = path.find_first_of(spliter, pos + 1)) != string::npos) {
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
#elif defined(NEFORCE_PLATFORM_LINUX)
    while ((pos = path.find_first_of(spliter, pos + 1)) != string::npos) {
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
#ifdef NEFORCE_PLATFORM_WINDOWS
        return ::DeleteFileA(path.data()) != 0;
#elif defined(NEFORCE_PLATFORM_LINUX)
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
#ifdef NEFORCE_PLATFORM_WINDOWS
        return ::RemoveDirectoryA(path.data()) != 0;
#elif defined(NEFORCE_PLATFORM_LINUX)
        return ::rmdir(path.data()) == 0;
#endif
    }
    return false;
}

bool path::remove_all_in_directory(const bool recursive) const noexcept {
    return path::remove_all_in_directory(path_, recursive);
}

bool path::remove_all_in_directory(const string& target, const bool recursive) noexcept {
    if (!path::is_directory(target.view())) return false;
    bool success = true;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string search_pattern = target + "\\*";
    ::WIN32_FIND_DATAA find_data;
    const ::HANDLE find_handle = ::FindFirstFileA(search_pattern.data(), &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) return false;

    do {
        string item_name = find_data.cFileName;
        if (item_name == "." || item_name == "..") continue;
        string full_path = target + "\\" + item_name;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                if (!remove_all_in_directory(full_path, true)) {
                    success = false;
                }
                if (!::RemoveDirectoryA(full_path.data())) {
                    if (::GetLastError() != ERROR_DIR_NOT_EMPTY) {
                        success = false;
                    }
                }
            }
        } else {
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                ::SetFileAttributesA(full_path.data(),
                    find_data.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
            }
            if (!::DeleteFileA(full_path.data())) {
                const ::DWORD error = ::GetLastError();
                if (error == ERROR_ACCESS_DENIED) {
                    ::SetFileAttributesA(full_path.data(), FILE_ATTRIBUTE_NORMAL);
                    if (!::DeleteFileA(full_path.data())) {
                        success = false;
                    }
                } else {
                    success = false;
                }
            }
        }
    } while (::FindNextFileA(find_handle, &find_data) != 0);
    ::FindClose(find_handle);

#elif defined(NEFORCE_PLATFORM_LINUX)
    ::DIR* dir = ::opendir(target.data());
    if (dir == nullptr) return false;

    ::dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        string item_name = entry->d_name;
        if (item_name == "." || item_name == "..") continue;

        string full_path = target + "/" + item_name;

        if (path::is_directory(full_path.view())) {
            if (recursive) {
                if (!remove_all_in_directory(full_path, true)) {
                    success = false;
                }
                if (::rmdir(full_path.data()) != 0 && errno != ENOTEMPTY) {
                    success = false;
                }
            }
        } else {
            if (::unlink(full_path.data()) != 0) {
                if (errno == EACCES) {
                    ::chmod(full_path.data(), 0644);
                    if (::unlink(full_path.data()) != 0) {
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
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!::RemoveDirectoryA(path.data())) {
        success = false;
    }
#elif defined(NEFORCE_PLATFORM_LINUX)
    if (::rmdir(path.data()) != 0) {
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

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (overwrite && actual_to.exists()) {
        const ::DWORD attrs = ::GetFileAttributesA(actual_to.data());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
            ::SetFileAttributesA(actual_to.data(), attrs & ~FILE_ATTRIBUTE_READONLY);
        }
    }

    if (::CopyFileA(from.data(), actual_to.data(), !overwrite)) {
        return true;
    }

    return false;

#elif defined(NEFORCE_PLATFORM_LINUX)
    const int source_fd = ::open(from.data(), O_RDONLY);
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

    const int dest_fd = ::open(actual_to.data(), flags, 0644);
    if (dest_fd == -1) {
        ::close(source_fd);
        return false;
    }

    char buffer[buffer_size];
    ssize_t bytes_read;
    bool success = true;

    while ((bytes_read = ::read(source_fd, buffer, buffer_size)) > 0) {
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
        ::unlink(actual_to.data());
    }

    return success;
#endif
}

bool path::copy_directory(const path& dest, const bool overwrite) const {
    return path::copy_directory(*this, dest, overwrite);
}

bool path::copy_directory(const path& src, const path& dest, const bool overwrite) {
    if (!src.is_directory()) return false;

    if (!dest.exists() && !dest.create_directories()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string search_pattern = src.str() + "\\*";
    ::WIN32_FIND_DATAA find_data;
    const ::HANDLE hFind = ::FindFirstFileA(search_pattern.data(), &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        if (::GetLastError() == ERROR_FILE_NOT_FOUND) return true;
        return false;
    }

    bool success = true;

    do {
        string item = find_data.cFileName;
        if (item == "." || item == "..") continue;

        path src_path = src / path{item};
        path dst_path = dest / path{item};

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

#elif defined(NEFORCE_PLATFORM_LINUX)
    ::DIR* dir = ::opendir(src.data());
    if (dir == nullptr) return false;

    bool success = true;
    ::dirent* entry;

    while ((entry = ::readdir(dir)) != nullptr) {
        string item = entry->d_name;
        if (item == "." || item == "..") continue;

        path src_path = src / path{item};
        path dst_path = dest / path{item};

        if (src_path.is_directory()) {
            if (!copy_directory(src_path, dst_path, overwrite)) {
                success = false;
            }
        } else {
            if (!copy(src_path, dst_path, overwrite)) {
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

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD flags = MOVEFILE_COPY_ALLOWED;
    if (overwrite) {
        flags |= MOVEFILE_REPLACE_EXISTING;
    }

    if (::MoveFileExA(from.data(), to.data(), flags)) {
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

#elif defined(NEFORCE_PLATFORM_LINUX)
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

    if (::rename(from.data(), to.data()) == 0) {
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

bool path::rename(const path& name) const {
    return path::move(*this, name, true);
}

bool path::rename(const path& old_name, const path& new_name) {
    return path::move(old_name, new_name, true);
}

bool path::operator ==(const path& rhs) const noexcept {
    const path lhs_norm = this->lexically_normal();
    const path rhs_norm = rhs.lexically_normal();

#ifdef NEFORCE_PLATFORM_WINDOWS
    return string_compare_ignore_case(lhs_norm.data(), rhs_norm.data()) == 0;
#else
    return lhs_norm.str() == rhs_norm.str();
#endif
}

bool path::operator <(const path& rhs) const noexcept {
    const path lhs_norm = lexically_normal();
    const path rhs_norm = rhs.lexically_normal();

#ifdef NEFORCE_PLATFORM_WINDOWS
    return string_compare_ignore_case(lhs_norm.data(), rhs_norm.data()) < 0;
#else
    return lhs_norm.str() < rhs_norm.str();
#endif
}

size_t path::to_hash() const {
    const path norm_path = lexically_normal();

    if (norm_path.empty()) {
        return hash<const char*>()("");
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto lower_str = norm_path.str();
    lower_str.lowercase();
    return hash<string>()(lower_str);
#else
    return hash<string>()(norm_path.str());
#endif
}

NEFORCE_END_NAMESPACE__
