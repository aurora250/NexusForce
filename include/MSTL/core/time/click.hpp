#ifndef MSTL_CORE_TIME_CLICK_HPP__
#define MSTL_CORE_TIME_CLICK_HPP__
#include "clocks.hpp"
MSTL_BEGIN_NAMESPACE__

struct click {
    using time_point = system_clock::time_point;

    time_point start_time{};
    time_point end_time{};
    bool started = false;
    bool stopped = false;

    void start() noexcept {
        start_time = system_clock::now();
        started = true;
        stopped = false;
    }
    void stop() noexcept {
        end_time = system_clock::now();
        stopped = true;
    }

    nanoseconds during() const {
        if (!started || !stopped) {
            throw_exception(value_exception("click not properly started/stopped"));
        }
        return duration_cast<nanoseconds>(end_time - start_time);
    }

    nanoseconds during_s() const noexcept {
        if (!started || !stopped) {
            return nanoseconds{0};
        }
        const auto diff = end_time - start_time;
        return diff.count() >= 0 ? duration_cast<nanoseconds>(diff) : nanoseconds{0};
    }

    template <typename Func, typename... Args, typename Res = invoke_result_t<Func, Args...>>
    enable_if_t<is_void_v<Res>> run(Func&& func, Args&&... args) {
        start();
        func(_MSTL forward<Args>(args)...);
        stop();
        return;
    }

    template <typename Func, typename... Args, typename Res = invoke_result_t<Func, Args...>>
    enable_if_t<!is_void_v<Res>, Res> run(Func&& func, Args&&... args) {
        start();
        Res res = func(_MSTL forward<Args>(args)...);
        stop();
        return _MSTL move(res);
    }

    void reset() noexcept {
        started = false;
        stopped = false;
        start_time = time_point{};
        end_time = time_point{};
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
