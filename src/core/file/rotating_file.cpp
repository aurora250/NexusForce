#include <NeForce/core/algorithm/sort.hpp>
#include <NeForce/core/exception/system_exception.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/rotating_file.hpp>
#include <NeForce/core/string/format.hpp>
NEFORCE_BEGIN_NAMESPACE__

path rotating_file::path_for(const size_t index) const {
    if (index == 0) {
        return base_path_;
    }
    return path(format("{}.{}", base_path_.str(), index));
}

vector<size_t> rotating_file::rotated_indices() const {
    vector<size_t> indices;
    const path parent = base_path_.parent_path().empty() ? path(".") : base_path_.parent_path();
    if (!parent.is_directory()) {
        return indices;
    }

    const string_view base_name = base_path_.filename();
    for (const auto& entry: parent.children()) {
        const string_view name = entry.filename();
        if (name.size() <= base_name.size() + 1 || !name.starts_with(base_name)) {
            continue;
        }
        if (name[base_name.size()] != '.') {
            continue;
        }

        const string_view suffix = name.substr(base_name.size() + 1);
        uint64_t value = 0;
        bool valid = !suffix.empty();
        for (const char c: suffix) {
            if (c < '0' || c > '9') {
                valid = false;
                break;
            }
            if (value > 1'000'000'000ULL) {
                valid = false;
                break;
            }
            value = value * 10 + static_cast<uint64_t>(c - '0');
        }
        if (valid && value > 0) {
            indices.push_back(static_cast<size_t>(value));
        }
    }

    sort(indices.begin(), indices.end());
    return indices;
}

vector<path> rotating_file::history_paths() const {
    vector<path> history;
    if (current_index_ != 0 && base_path_.exists()) {
        history.push_back(base_path_);
    }
    for (const size_t index: rotated_indices()) {
        if (index != current_index_) {
            history.push_back(path_for(index));
        }
    }
    return history;
}

bool rotating_file::open_at(const size_t index, const bool truncate) {
    const path target = path_for(index);
    const path parent = target.parent_path();
    if (!parent.empty() && !filesystem::create_directories(parent)) {
        return false;
    }

    const bool append = options_.append && index == 0 && !truncate;
    if (!file_.open(target, append, file_access::write, file_shared::share_read_write,
                    truncate ? file_creation::create_force : file_creation::open_force)) {
        return false;
    }

    current_path_ = target;
    current_index_ = index;
    size_ = truncate ? 0 : file_.size64();
    return true;
}

void rotating_file::cleanup_old_files() {
    if (options_.max_files == 0) {
        return;
    }

    const auto history = history_paths();
    if (history.size() <= options_.max_files) {
        return;
    }

    for (size_t index = 0; index + options_.max_files < history.size(); ++index) {
        static_cast<void>(filesystem::remove(history[index]));
    }
}

rotating_file::rotating_file(path file, rotating_file::options opts) :
base_path_(move(file)),
current_path_(base_path_),
options_(move(opts)) {
    if (!open_at(0, !options_.append)) {
        NEFORCE_THROW_EXCEPTION(file_exception("Failed to open rotating file"));
    }
    cleanup_old_files();
}

bool rotating_file::write(const string_view data) {
    if (data.empty()) {
        return true;
    }
    if (!file_.is_opened() && !open_at(current_index_, false)) {
        return false;
    }

    if (options_.max_bytes != 0 && size_ != 0 &&
        (size_ >= options_.max_bytes || data.size() > options_.max_bytes - size_)) {
        if (!rotate()) {
            return false;
        }
    }

    const auto written = file_.write(data.data(), static_cast<file::size_type>(data.size()));
    if (written != data.size()) {
        return false;
    }
    size_ += written;
    return true;
}

bool rotating_file::flush() { return file_.is_opened() && file_.flush(); }

bool rotating_file::rotate() {
    try {
        return options_.naming == rotating_file::naming::indexed ? rotate_indexed() : rotate_rollover();
    } catch (const exception&) {
        return false;
    }
}

bool rotating_file::rotate_rollover() {
    file_.close();
    const auto indices = rotated_indices();

    if (options_.max_files != 0) {
        for (const size_t index: indices) {
            if (index >= options_.max_files) {
                static_cast<void>(filesystem::remove(path_for(index)));
            }
        }
        for (size_t index = options_.max_files; index > 1; --index) {
            const path from = path_for(index - 1);
            const path to = path_for(index);
            if (!from.exists()) {
                continue;
            }
            if (to.exists()) {
                static_cast<void>(filesystem::remove(to));
            }
            if (!filesystem::rename(from, to)) {
                return false;
            }
        }
    } else {
        for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
            const path from = path_for(*it);
            const path to = path_for(*it + 1);
            if (!from.exists()) {
                continue;
            }
            if (to.exists()) {
                static_cast<void>(filesystem::remove(to));
            }
            if (!filesystem::rename(from, to)) {
                return false;
            }
        }
    }

    if (base_path_.exists()) {
        const path first = path_for(1);
        if (first.exists()) {
            static_cast<void>(filesystem::remove(first));
        }
        if (!filesystem::rename(base_path_, first)) {
            return false;
        }
    }

    if (!open_at(0, true)) {
        return false;
    }
    cleanup_old_files();
    return true;
}

bool rotating_file::rotate_indexed() {
    file_.close();
    const auto indices = rotated_indices();
    size_t next = 1;
    for (const size_t index: indices) {
        next = max(next, index + 1);
    }
    if (!open_at(next, true)) {
        return false;
    }
    cleanup_old_files();
    return true;
}

uint64_t rotating_file::size() const noexcept { return size_; }

const path& rotating_file::current_path() const noexcept { return current_path_; }

NEFORCE_END_NAMESPACE__
