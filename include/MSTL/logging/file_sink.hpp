#ifndef MSTL_LOGGING_FILE_SINK_HPP__
#define MSTL_LOGGING_FILE_SINK_HPP__
#include "MSTL/core/system/file.hpp"
#include "MSTL/core/async/mutex.hpp"
#include "log_sink.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API file_sink final : public log_sink {
private:
    file file_;
    _MSTL recursive_mutex mutex_;
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
#endif // MSTL_LOGGING_FILE_SINK_HPP__
