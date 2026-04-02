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

    if (!file_.open(path(filename), true, file_access::WRITE, file_shared::SHARE_WRITE, file_creation::OPEN_FORCE)) {
        NEFORCE_THROW_EXCEPTION(file_exception("Failed to open log file"));
    }
    current_size_ = file_.size();
}

void file_sink::rotate_file() {
    file_.close();
    ++file_index_;
    open_new_file();
}

void file_sink::rotate_by_date(string today) {
    file_.close();
    current_date_ = _NEFORCE move(today);
    file_index_ = 0;
    current_size_ = 0;
    open_new_file();
}

static string default_format(log_event event) {
    string result;
    result += "["_s + to_string(event.level) + "] " + _NEFORCE move(event.message);
    return result;
}

file_sink::file_sink(path filename, const size_t max_file_size, const bool enable_date_rotation) :
base_filename_(_NEFORCE move(filename)),
max_file_size_(max_file_size),
current_size_(0),
file_index_(0),
enable_date_rotation_(enable_date_rotation) {
    if (enable_date_rotation_) {
        current_date_ = datetime::now().date().to_string();
    }
    open_new_file();
}

void file_sink::log(const log_event& event) {
    const string formatted = formatter_ ? formatter_->format(event) : default_format(event);
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
}

void file_sink::flush() {
    lock<mutex> lock(mutex_);
    file_.flush();
}

NEFORCE_END_NAMESPACE__
