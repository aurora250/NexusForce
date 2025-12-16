#include <MSTL/core/system/console.hpp>
#include <MSTL/logging/log_sink.hpp>
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

MSTL_END_NAMESPACE__
