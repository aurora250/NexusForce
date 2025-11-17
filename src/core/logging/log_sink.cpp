#include <MSTL/core/logging/log_sink.hpp>
#include <MSTL/core/utilities/console.hpp>
#include <MSTL/core/utilities/vsprintf.hpp>
MSTL_BEGIN_NAMESPACE__

log_formatter::log_formatter(string pattern)
: pattern_(move(pattern)) {
    parse_pattern();
}

string log_formatter::format(const log_event& event) {
    string result;
    for (const auto& part : parts_) {
        if (part.is_placeholder) {
            result += resolve_placeholder(part.text, event);
        } else {
            result += part.text;
        }
    }
    return result;
}

void log_formatter::parse_pattern() {
    parts_.clear();
    size_t pos = 0;
    while(pos < pattern_.size()) {
        const size_t start = pattern_.find('{', pos);
        if (start == string::npos) {
            parts_.emplace_back(false, pattern_.substr(pos));
            break;
        }
        if (start > pos) {
            parts_.emplace_back(false, pattern_.substr(pos, start-pos));
        }
        const size_t end = pattern_.find('}', start);
        if (end == string::npos) {
            parts_.emplace_back(false, pattern_.substr(start));
            break;
        }
        const string placeholder = pattern_.substr(start+1, end - start -1);
        parts_.emplace_back(true, placeholder);
        pos = end + 1;
    }
}

string log_formatter::resolve_placeholder(string ph, const log_event& event) const {
    if (ph.size() > 8 && ph.view(0, 8) == "context.") {
        const auto it = event.context.find(ph.substr(8));
        if (it != event.context.end()) {
            return it->second;
        }
        return "";
    }

    if (ph == "time") {
        return event.dt.to_string();
    } else if (ph == "level") {
        return to_string(event.level);
    } else if (ph == "file") {
        return event.file;
    } else if (ph == "line") {
        return to_string(event.line);
    } else if (ph == "func") {
        return event.func;
    } else if (ph == "thread") {
        return to_string(event.thread_id.native());
    } else if (ph == "message") {
        return event.message;
    }
    return "{" + move(ph) + "}";
}

void log_sink::set_formatter(unique_ptr<log_formatter> formatter) {
    formatter_ = _MSTL move(formatter);
}

string console_sink::default_format(const log_event& ev) {
    string result;
    result += "["_s + to_string(ev.level) + "] " + ev.message;
    return result;
}

void console_sink::log(const log_event& event)  {
    const string formatted = formatter_ ? formatter_->format(event) : default_format(event);
    println(formatted);
}

void console_sink::flush() {
    console.flush();
}

void file_sink::open_new_file() {
    string filename = base_filename_;

    if (enable_date_rotation_ && !current_date_.empty()) {
        filename += "." + current_date_;
    }
    if (file_index_ > 0) {
        filename += "." + to_string(file_index_);
    }

    if (!file_.open(filename, true,
        FILE_ACCESS::WRITE,
        FILE_SHARED::SHARE_WRITE,
        FILE_CREATION::OPEN_FORCE)) {
        Exception(FileOperateError("Failed to open log file"));
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
    current_date_ = _MSTL move(today);
    file_index_ = 0;
    current_size_ = 0;
    open_new_file();
}

string file_sink::default_format(log_event ev) {
    string result;
    result += "["_s + to_string(ev.level) + "] " + move(ev.message);
    return result;
}

file_sink::file_sink(string filename, const size_t max_file_size, const bool enable_date_rotation)
: base_filename_(move(filename)), max_file_size_(max_file_size),
current_size_(0), file_index_(0), enable_date_rotation_(enable_date_rotation) {
    if (enable_date_rotation_) {
        current_date_ = datetime::now().dates().to_string();
    }
    open_new_file();
}

void file_sink::log(const log_event& event) {
    const string formatted = formatter_ ? formatter_->format(event) : default_format(event);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (enable_date_rotation_) {
        const string today = datetime::now().dates().to_string();
        if (today != current_date_) {
            rotate_by_date(today);
        }
    }

    if (!file_.opened()) {
        open_new_file();
    }
    if (current_size_ + formatted.size() + 1 > max_file_size_) {
        rotate_file();
    }
    file_.write(formatted + "\n");
    current_size_ += formatted.size() + 1;
}

void file_sink::flush() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    file_.flush();
}

MSTL_END_NAMESPACE__
