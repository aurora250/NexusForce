#ifndef MSTL_LOGGING_LOG_FORMATTER_HPP__
#define MSTL_LOGGING_LOG_FORMATTER_HPP__
#include "MSTL/core/container/vector.hpp"
MSTL_BEGIN_NAMESPACE__

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

MSTL_END_NAMESPACE__
#endif // MSTL_LOGGING_LOG_FORMATTER_HPP__
