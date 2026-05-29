#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/logging/file_sink.hpp>
NEFORCE_BEGIN_NAMESPACE__

void file_sink::open_new_file() {
    string filename = base_filename_.str();

    if (enable_date_rotation_ && !current_date_.empty()) {
        filename += "." + current_date_;
    }
    if (file_index_ > 0) {
        filename += "." + to_string(file_index_);
    }

    path p(filename);

    filesystem::create_directories(p.parent_path());

    if (!file_.open(move(p), true, file_access::WRITE, file_shared::SHARE_WRITE, file_creation::OPEN_FORCE)) {
        NEFORCE_THROW_EXCEPTION(file_exception("Failed to open log file"));
    }
    current_size_ = file_.size();
}

void file_sink::rotate_file() {
    file_.close();
    ++file_index_;
    open_new_file();

    if (max_files_ > 0 && file_index_ > 0) {
        cleanup_old_files();
    }
}

void file_sink::rotate_by_date(string today) {
    file_.close();

    if (max_files_ > 0 && !current_date_.empty()) {
        vector<path> old_files;
        const string base = base_filename_.str();
        for (size_t i = 0; i <= static_cast<size_t>(file_index_); ++i) {
            path p = i == 0 ? path(base + "." + current_date_) : path(base + "." + current_date_ + "." + to_string(i));
            if (p.exists()) {
                old_files.push_back(p);
            }
        }
        if (old_files.size() > max_files_) {
            const size_t to_remove = old_files.size() - max_files_;
            for (size_t i = 0; i < to_remove && i < old_files.size(); ++i) {
                filesystem::remove(old_files[i]);
            }
        }
    }

    current_date_ = move(today);
    file_index_ = 0;
    current_size_ = 0;
    open_new_file();
}

void file_sink::cleanup_old_files() {
    const string base = base_filename_.str();
    vector<path> existing;

    for (int i = 1; i <= file_index_ + 5; ++i) {
        path p;
        if (enable_date_rotation_ && !current_date_.empty()) {
            p = path(base + "." + current_date_ + "." + to_string(i));
        } else {
            p = path(base + "." + to_string(i));
        }
        if (p.exists()) {
            existing.push_back(p);
        }
    }

    if (existing.size() > max_files_) {
        const size_t to_remove = existing.size() - max_files_;
        for (size_t i = 0; i < to_remove && i < existing.size(); ++i) {
            filesystem::remove(existing[i]);
        }
    }
}

file_sink::file_sink(path filename, const size_t max_file_size, const bool enable_date_rotation,
                     const size_t max_files) :
base_filename_(move(filename)),
max_file_size_(max_file_size),
current_size_(0),
file_index_(0),
enable_date_rotation_(enable_date_rotation),
max_files_(max_files) {
    if (enable_date_rotation_) {
        current_date_ = datetime::now().date().to_string();
    }
    open_new_file();

    if (max_files_ > 0) {
        cleanup_old_files();
    }
}

void file_sink::log(const log_event& event) {
    const string formatted = formatter_ ? formatter_->format(event) : default_sink_format(event);
    lock<mutex> lock(mutex_);

    if (enable_date_rotation_) {
        const string today = datetime::now().date().to_string();
        if (today != current_date_) {
            rotate_by_date(today);
        }
    }

    if (!file_.is_opened()) {
        open_new_file();
    }

    if (current_size_ + formatted.size() + 1 > max_file_size_) {
        rotate_file();
    }

    file_.write(formatted + "\n");
    current_size_ += formatted.size() + 1;

    if (should_auto_flush()) {
        last_flush_ = timestamp::now();
        file_.flush();
    }
}

void file_sink::flush() {
    lock<mutex> lock(mutex_);
    file_.flush();
}

NEFORCE_END_NAMESPACE__
