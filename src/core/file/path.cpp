#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/file/path_tree.hpp>
#include <NeForce/core/system/environment.hpp>
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
#    include <cstdlib>
#    include <dirent.h>
#    include <fcntl.h>
#    include <sys/stat.h>
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
    if (path_.empty()) {
        return path{};
    }

    const size_t last_sep = path_.find_last_of(spliter);
    if (last_sep == string::npos) {
        return path{};
    }
    if (last_sep == 0) {
        return path{"/"};
    }

    return path{path_.substr(0, last_sep)};
}

string_view path::filename() const noexcept {
    if (path_.empty()) {
        return {};
    }

    const size_t last_sep = path_.find_last_of(spliter);
    if (last_sep == string::npos) {
        return path_.view();
    }
    return path_.view().substr(last_sep + 1);
}

string_view path::stem() const noexcept {
    const string_view fname = filename();
    if (fname.empty()) {
        return {};
    }

    const size_t last_dot = fname.find_last_of('.');
    if (last_dot == string::npos || last_dot == 0) {
        return fname;
    }

    return fname.substr(0, last_dot);
}

string_view path::extension() const noexcept { return path::extension(path_.view()); }

string_view path::extension(const string_view path) noexcept {
    const size_t last_sep = path.find_last_of(spliter);
    const string_view filename = last_sep == string::npos ? path : path.substr(last_sep + 1);

    const size_t last_dot = filename.find_last_of('.');
    if (last_dot == string::npos || last_dot == 0 || last_dot == filename.size() - 1) {
        return {};
    }
    return filename.substr(last_dot + 1);
}

path path::lexically_normal() const {
    if (path_.empty()) {
        return path(".");
    }

    string result;
    string drive_prefix;
    vector<string_view> parts;

    size_t start = 0;
    const size_t length = path_.size();
    bool is_absolute = false;

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (length >= 2 && path_[1] == ':') {
        drive_prefix = path_.view(0, 2);
        start = 2;
        if (length > 2 && (path_[2] == '\\' || path_[2] == '/')) {
            is_absolute = true;
            start = 3;
        }
    } else if (length >= 2 && path_[0] == '\\' && path_[1] == '\\') {
        is_absolute = true;
        drive_prefix = "\\\\";
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
    for (const auto& part: parts) {
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

#ifdef NEFORCE_PLATFORM_WINDOWS
    result = drive_prefix;
    if (is_absolute) {
        result += preferred_separator;
    }
#else
    if (is_absolute) {
        result += '/';
    }
#endif

    for (size_t i = 0; i < normalized.size(); ++i) {
        if (i > 0) {
            result += preferred_separator;
        }
        result += normalized[i];
    }

    if (result.empty()) {
        return path(".");
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (is_absolute && normalized.empty()) {
        return path(drive_prefix + preferred_separator);
    }
#else
    if (is_absolute && normalized.empty()) {
        return path("/");
    }
#endif

    return path(move(result));
}

path path::absolute(const path& base) const {
    if (this->empty()) {
        return path{};
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    if (::GetFullPathNameA(path_.data(), MAX_PATH, buffer, nullptr) == 0) {
        return *this;
    }
    return path(string(buffer));

#else
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
    for (auto it = abs_path.begin(); it != abs_path.end(); ++it) {
        this_parts.push_back(*it);
    }

    vector<string_view> base_parts;
    for (auto it = abs_base.begin(); it != abs_base.end(); ++it) {
        base_parts.push_back(*it);
    }

    size_t i = 0;
    const size_t n = min(this_parts.size(), base_parts.size());
    while (i < n && this_parts[i] == base_parts[i]) {
        ++i;
    }

    string result;
    for (size_t j = i; j < base_parts.size(); ++j) {
        if (!result.empty()) {
            result += preferred_separator;
        }
        result += "..";
    }
    for (size_t j = i; j < this_parts.size(); ++j) {
        if (!result.empty()) {
            result += preferred_separator;
        }
        result += this_parts[j];
    }
    if (result.empty()) {
        result = ".";
    }

    return path(move(result));
}


path path::current_path() { return path(environment::current_directory()); }

path path::temp_directory_path() {
    string temp(environment::temp_directory());
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (!temp.empty() && (temp.back() == '\\' || temp.back() == '/')) {
        temp.pop_back();
    }
#endif
    return path(move(temp));
}

path& path::operator/=(const path& other) {
    if (other.empty()) {
        return *this;
    }

    if (path_.empty()) {
        path_ = other.path_;
        return *this;
    }

    const char back = path_.back();
    const bool left_has_sep = (back == '/' || back == '\\');
    const char front = other.path_.front();
    const bool right_has_sep = (front == '/' || front == '\\');

    if (left_has_sep && right_has_sep) {
        path_ += other.path_.substr(1);
    } else if (!left_has_sep && !right_has_sep) {
        path_ += preferred_separator;
        path_ += other.path_;
    } else {
        path_ += other.path_;
    }

    return *this;
}

path& path::operator/=(const string_view other) {
    if (other.empty()) {
        return *this;
    }

    if (path_.empty()) {
        path_ = other;
        return *this;
    }

    const char back = path_.back();
    const bool left_has_sep = (back == '/' || back == '\\');
    const bool right_has_sep = (!other.empty() && (other.front() == '/' || other.front() == '\\'));

    if (left_has_sep && right_has_sep) {
        path_ += other.substr(1);
    } else if (!left_has_sep && !right_has_sep) {
        path_ += preferred_separator;
        path_ += other;
    } else {
        path_ += other;
    }

    return *this;
}

path path::operator/(const path& other) const {
    path result = *this;
    result /= other;
    return result;
}

path path::operator/(const string_view pth) const {
    path result = *this;
    result /= pth;
    return result;
}

path_tree path::to_tree() const { return path_tree::scan(*this, path_tree::scan_options{}); }

vector<path> path::children(const bool include_hidden) const {
    path_tree::scan_options opts;
    opts.max_depth = 1;
    opts.include_hidden = include_hidden;
    const path_tree tree = path_tree::scan(*this, opts);

    vector<path> result;
    if (!tree.empty() && tree.root()) {
        for (const auto& child: tree.root()->children()) {
            result.push_back(child->get_path());
        }
    }
    return result;
}

vector<path> path::child_files(const bool include_hidden) const {
    path_tree::scan_options opts;
    opts.max_depth = 1;
    opts.include_hidden = include_hidden;
    opts.files_only = true;
    const path_tree tree = path_tree::scan(*this, opts);

    vector<path> result;
    if (!tree.empty() && tree.root()) {
        for (const auto& child: tree.root()->children()) {
            result.push_back(child->get_path());
        }
    }
    return result;
}

vector<path> path::child_dirs(const bool include_hidden) const {
    path_tree::scan_options opts;
    opts.max_depth = 1;
    opts.include_hidden = include_hidden;
    opts.dirs_only = true;
    const path_tree tree = path_tree::scan(*this, opts);

    vector<path> result;
    if (!tree.empty() && tree.root()) {
        for (const auto& child: tree.root()->children()) {
            result.push_back(child->get_path());
        }
    }
    return result;
}

bool path::exists() const noexcept { return path::exists(path_); }

bool path::exists(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetFileAttributesA(path.data()) != INVALID_FILE_ATTRIBUTES;
#else
    struct ::stat64 st{};
    return ::stat64(path.data(), &st) != -1;
#endif
}

bool path::is_directory() const noexcept { return path::is_directory(path_); }

bool path::is_directory(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct ::stat64 st{};
    if (::stat64(path.data(), &st) == -1) {
        return false;
    }
    return S_ISDIR(st.st_mode);
#endif
}

bool path::is_file() const noexcept { return path::is_file(path_); }

bool path::is_file(const string& path) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD attrib = ::GetFileAttributesA(path.data());
    return attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct ::stat64 st{};
    if (::stat64(path.data(), &st) == -1) {
        return false;
    }
    return S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
#endif
}

bool path::operator==(const path& rhs) const {
    const path lhs_norm = this->lexically_normal();
    const path rhs_norm = rhs.lexically_normal();

#ifdef NEFORCE_PLATFORM_WINDOWS
    return string_compare_ignore_case(lhs_norm.data(), rhs_norm.data()) == 0;
#else
    return lhs_norm.str() == rhs_norm.str();
#endif
}

bool path::operator<(const path& rhs) const {
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
