#ifndef MSTL_CORE_TIME_CLICK_HPP__
#define MSTL_CORE_TIME_CLICK_HPP__
#include "clocks.hpp"
MSTL_BEGIN_NAMESPACE__

struct click {
    using time_point = system_clock::time_point;

    time_point start_time{};
    time_point last_time{};
    bool started = false;
    bool stopped = false;

    void start() noexcept {
        start_time = system_clock::now();
        started = true;
        stopped = false;
    }
    void update() noexcept {
        last_time = system_clock::now();
    }
    void stop() noexcept {
        last_time = system_clock::now();
        stopped = true;
    }

    nanoseconds during() const {
        if (!started || !stopped) {
            throw_exception(value_exception("click not properly started/stopped"));
        }
        return duration_cast<nanoseconds>(last_time - start_time);
    }

    nanoseconds during_s() const noexcept {
        if (!started || !stopped) {
            return nanoseconds{0};
        }
        const auto diff = last_time - start_time;
        return diff.count() >= 0 ? duration_cast<nanoseconds>(diff) : nanoseconds{0};
    }

    void reset() noexcept {
        started = false;
        stopped = false;
        start_time = time_point{};
        last_time = time_point{};
    }
};


class scoped_click {
    click& clk_;

public:
    explicit scoped_click(click& clk) noexcept : clk_(clk) {
        clk_.start();
    }

    scoped_click(const scoped_click&) = delete;
    scoped_click& operator =(const scoped_click&) = delete;

    ~scoped_click() noexcept {
        clk_.stop();
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TIME_CLICK_HPP__
