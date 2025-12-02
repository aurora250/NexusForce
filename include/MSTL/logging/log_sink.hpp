#ifndef MSTL_LOGGING_LOG_SINK_HPP__
#define MSTL_LOGGING_LOG_SINK_HPP__
#include "MSTL/core/memory/unique_ptr.hpp"
#include "log_event.hpp"
#include "log_formatter.hpp"
MSTL_BEGIN_NAMESPACE__

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

MSTL_END_NAMESPACE__
#endif // MSTL_LOGGING_LOG_SINK_HPP__
