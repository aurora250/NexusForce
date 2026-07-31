#include <NeForce/core/system/console.hpp>
#include <NeForce/logging/log_sink.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <syslog.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr color trace_clr(color::gray());
    constexpr color debug_clr(color::blue());
    constexpr color info_clr(color::green());
    constexpr color warn_clr(color::yellow());
    constexpr color error_clr(color::red());
    constexpr color fatal_clr(color::purple());
    constexpr color default_clr(color::white());

#ifdef NEFORCE_PLATFORM_LINUX
    int to_syslog_priority(const log_level level) {
        switch (level) {
            case log_level::TRACE:
            case log_level::DEBUG:
                return LOG_DEBUG;
            case log_level::INFO:
                return LOG_INFO;
            case log_level::WARN:
                return LOG_WARNING;
            case log_level::ERROR:
                return LOG_ERR;
            case log_level::FATAL:
                return LOG_CRIT;
            default:
                return LOG_INFO;
        }
    }
#endif

    string resolve_placeholder(const string& ph, const log_event& event) {
        if (ph.size() > 8 && ph.view(0, 8) == "context.") {
            if (event.context) {
                const auto it = event.context->find(ph.substr(8));
                if (it != event.context->end()) {
                    return it->second;
                }
            }
            return "";
        }

        if (ph == "time") {
            return event.dt.to_string();
        } else if (ph == "level") {
            return to_string(event.level);
        } else if (ph == "file") {
            const string_view f = event.loc.file_name();
            if (!f.empty()) {
                const char* last = f.data();
                for (const char* p = f.data(); (*p) != 0; ++p) {
                    if (*p == '/' || *p == '\\') {
                        last = p + 1;
                    }
                }
                return {last};
            }
            return "";
        } else if (ph == "filepath") {
            return event.loc.file_name();
        } else if (ph == "line") {
            return to_string(event.loc.line());
        } else if (ph == "func") {
            return event.loc.func_name();
        } else if (ph == "thread") {
            return to_string(event.thread_id.native_handle());
        } else if (ph == "name") {
            return event.logger_name;
        } else if (ph == "message") {
            return event.message;
        }
        return "{" + ph + "}";
    }

} // namespace

log_formatter::log_formatter(string pattern) :
pattern_(move(pattern)) {
    parse_pattern();
}

string log_formatter::format(const log_event& event) {
    string result;
    result.reserve(256);
    for (const auto& part: parts_) {
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
    while (pos < pattern_.size()) {
        const size_t start = pattern_.find('{', pos);
        if (start == string::npos) {
            parts_.emplace_back(false, pattern_.tail(pos));
            break;
        }
        if (start > pos) {
            parts_.emplace_back(false, pattern_.substr(pos, start - pos));
        }
        const size_t end = pattern_.find('}', start);
        if (end == string::npos) {
            parts_.emplace_back(false, pattern_.tail(start));
            break;
        }
        const string placeholder = pattern_.substr(start + 1, end - start - 1);
        parts_.emplace_back(true, placeholder);
        pos = end + 1;
    }
}

void log_sink::set_formatter(unique_ptr<log_formatter> formatter) { formatter_ = move(formatter); }

const color& level_color(const log_level level) {
    switch (level) {
        case log_level::TRACE:
            return trace_clr;
        case log_level::DEBUG:
            return debug_clr;
        case log_level::INFO:
            return info_clr;
        case log_level::WARN:
            return warn_clr;
        case log_level::ERROR:
            return error_clr;
        case log_level::FATAL:
            return fatal_clr;
        default:
            return default_clr;
    }
}

void console_sink::log(const log_event& event) {
    const string formatted = formatter_ ? formatter_->format(event) : default_sink_format(event);
    printcln(level_color(event.level), formatted);
}

void console_sink::flush() { console.flush(); }

#ifdef NEFORCE_PLATFORM_LINUX

syslog_sink::syslog_sink(string ident, const syslog_facility facility) :
ident_(move(ident)),
facility_(facility) {}

syslog_sink::~syslog_sink() {
    if (opened_) {
        ::closelog();
    }
}

void syslog_sink::ensure_open() {
    if (!opened_) {
        const char* ident_str = ident_.empty() ? nullptr : ident_.data();
        ::openlog(ident_str, LOG_PID | LOG_NDELAY, static_cast<int>(facility_));
        opened_ = true;
    }
}

void syslog_sink::log(const log_event& event) {
    ensure_open();
    const string formatted = formatter_ ? formatter_->format(event) : default_sink_format(event);
    const int priority = to_syslog_priority(event.level);
    ::syslog(priority, "%s", formatted.data());
}

void syslog_sink::flush() {
    // syslog 每次调用立即写入，无需显式 flush
}

#endif // NEFORCE_PLATFORM_LINUX

NEFORCE_END_NAMESPACE__
