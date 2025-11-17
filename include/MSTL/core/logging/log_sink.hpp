#ifndef MSTL_LOG_SINK_HPP__
#define MSTL_LOG_SINK_HPP__
#include "../async/thread.hpp"
#include "../container/unordered_map.hpp"
#include "../utility/file.hpp"
#include <mutex>
MSTL_BEGIN_NAMESPACE__

#ifdef ERROR
#undef ERROR
#endif

enum class LOG_LEVEL {
    TRACE=0, DEBUG, INFO, WARN, ERROR, FATAL
};

MSTL_CONSTEXPR20 string to_string(const LOG_LEVEL level) {
    switch(level) {
        case LOG_LEVEL::TRACE: return "TRACE";
        case LOG_LEVEL::DEBUG: return "DEBUG";
        case LOG_LEVEL::INFO:  return "INFO";
        case LOG_LEVEL::WARN:  return "WARN";
        case LOG_LEVEL::ERROR: return "ERROR";
        case LOG_LEVEL::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}


struct log_event {
    unordered_map<string, string> context;
    string file;
    string func;
    string message;
    datetime dt;
    int line;
    thread::id thread_id;
    LOG_LEVEL level;
};


class MSTL_API log_formatter {
private:
    struct part {
        bool is_placeholder;
        string text;

        part(const bool ph, string t) noexcept
        : is_placeholder(ph), text(_MSTL move(t)) {}
    };
    string pattern_;
    vector<part> parts_;

    void parse_pattern();
    string resolve_placeholder(string ph, const log_event& event) const;

public:
    explicit log_formatter(string pattern);

    MSTL_NODISCARD string format(const log_event& event);
};


class MSTL_API log_sink {
protected:
    unique_ptr<log_formatter> formatter_;

public:
    virtual ~log_sink() = default;
    virtual void log(const log_event& event) = 0;
    virtual void flush() = 0;

    void set_formatter(unique_ptr<log_formatter> formatter);
};


class MSTL_API console_sink final : public log_sink {
private:
    static string default_format(const log_event& ev);

public:
    void log(const log_event& event) override;
    void flush() override;
};

class MSTL_API file_sink final : public log_sink {
private:
    file file_;
    std::recursive_mutex mutex_;
    string base_filename_;
    string current_date_;
    size_t max_file_size_;
    size_t current_size_;
    int file_index_;
    bool enable_date_rotation_;

    void open_new_file();
    void rotate_file();
    void rotate_by_date(string today);
    static string default_format(log_event ev);

public:
    explicit file_sink(string filename,
        size_t max_file_size = 10 * 1024 * 1024,
        bool enable_date_rotation = true);

    void log(const log_event& event) override;
    void flush() override;
};

MSTL_END_NAMESPACE__
#endif // MSTL_LOG_SINK_HPP__
