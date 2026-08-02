#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/time/clocks.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/epoll.h>
#    include <sys/eventfd.h>
#    include <unistd.h>
#endif
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <WinSock2.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    uint64_t now_ms() {
        return static_cast<uint64_t>(time_cast<milliseconds>(steady_clock::now().since_epoch()).count());
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    long to_wsa_events(const uint32_t e) {
        long wsa = 0;
        if ((e & epoll_in) != 0U) {
            wsa |= FD_READ | FD_ACCEPT;
        }
        if ((e & epoll_out) != 0U) {
            wsa |= FD_WRITE;
        }
        return wsa;
    }

    uint32_t from_wsa_events(const long network_events) {
        uint32_t out = 0;
        if ((network_events & (FD_READ | FD_ACCEPT)) != 0) {
            out |= epoll_in;
        }
        if ((network_events & FD_WRITE) != 0) {
            out |= epoll_out;
        }
        return out;
    }
#endif
} // namespace


#ifdef NEFORCE_PLATFORM_WINDOWS
void io_context::monitor_loop() {
    while (monitor_running_) {
        unique_lock<mutex> lk(fd_mutex_);
        vector<::HANDLE> handles;
        vector<native_handle_type> fds;
        handles.push_back(wake_event_);
        fds.push_back(-1);

        for (const auto& pair: fd_events_) {
            handles.push_back(pair.second);
            fds.push_back(pair.first);
        }
        lk.unlock_quiet();

        const ::DWORD count = (handles.size() > WSA_MAXIMUM_WAIT_EVENTS) ? WSA_MAXIMUM_WAIT_EVENTS
                                                                         : static_cast<::DWORD>(handles.size());

        const ::DWORD result = ::WSAWaitForMultipleEvents(count, handles.data(), FALSE, 50, FALSE);

        if (result >= WSA_WAIT_EVENT_0 && result < WSA_WAIT_EVENT_0 + count) {
            const size_t idx = result - WSA_WAIT_EVENT_0;
            if (idx == 0) {
                ::ResetEvent(wake_event_);
            } else if (idx < fds.size()) {
                const native_handle_type fd = fds[idx];
                ::WSANETWORKEVENTS net_ev{};
                if (::WSAEnumNetworkEvents(fd, handles[idx], &net_ev) == 0) {
                    const uint32_t events = from_wsa_events(net_ev.lNetworkEvents);
                    if (events != 0) {
                        ::PostQueuedCompletionStatus(iocp_handle_, events, fd, nullptr);
                    }
                }
            }
        }
    }
}
#endif

uint64_t io_context::next_timer_deadline() const {
    lock<mutex> lk(timer_mutex_);
    if (timer_heap_.empty()) {
        return numeric_traits<uint64_t>::max();
    }
    return timer_heap_.front().deadline_ms;
}

void io_context::process_timers(size_t max_count) {
    unique_lock<mutex> lk(timer_mutex_);
    const uint64_t n_ms = now_ms();
    size_t count = 0;
    while (count < max_count && !timer_heap_.empty() && timer_heap_.front().deadline_ms <= n_ms) {
        pop_heap(timer_heap_.begin(), timer_heap_.end(), greater<timer_entry>());
        auto entry = move(timer_heap_.back());
        timer_heap_.pop_back();
        lk.unlock_quiet();
        if (entry.callback) {
            entry.callback();
        }
        ++count;
        lk.lock_quiet();
    }
}

size_t io_context::schedule_timer(const uint64_t delay_ms, timer_callback handler) {
    lock<mutex> lk(timer_mutex_);
    const size_t id = next_timer_id_++;
    timer_entry entry;
    entry.id = id;
    entry.deadline_ms = now_ms() + delay_ms;
    entry.callback = move(handler);
    timer_heap_.push_back(entry);
    push_heap(timer_heap_.begin(), timer_heap_.end(), greater<timer_entry>());
    wake();
    return id;
}

bool io_context::cancel_timer(const size_t timer_id) {
    lock<mutex> lk(timer_mutex_);
    for (auto it = timer_heap_.begin(); it != timer_heap_.end(); ++it) {
        if (it->id == timer_id) {
            timer_heap_.erase(it);
            make_heap(timer_heap_.begin(), timer_heap_.end(), greater<timer_entry>());
            return true;
        }
    }
    return false;
}

size_t io_context::drain_handlers(size_t max_count) {
    size_t count = 0;
    for (size_t i = 0; i < max_count; ++i) {
        auto handler = external_queue_.try_pop();
        if (!handler) {
            break;
        }
        external_queue_count_.fetch_sub(1, memory_order_relaxed);
        (*handler)();
        outstanding_work_.fetch_sub(1, memory_order_relaxed);
        ++count;
    }
    return count;
}

void io_context::post(handler_type handler) {
    outstanding_work_.fetch_add(1, memory_order_relaxed);
    external_queue_.push(move(handler));
    external_queue_count_.fetch_add(1, memory_order_relaxed);
    wake();
}

void io_context::dispatch(handler_type handler) {
    if (running_.load(memory_order_acquire) != 0) {
        post(move(handler));
    } else {
        handler();
    }
}

io_context::io_context() {
#ifdef NEFORCE_PLATFORM_LINUX
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    wake_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);

    ::epoll_event ev{};
    ev.events = epoll_in;
    ev.data.fd = wake_fd_;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wake_fd_, &ev);

    fd_info wake_info{};
    wake_info.fd = wake_fd_;
    wake_info.events = epoll_in;
    fd_map_[wake_fd_] = move(wake_info);
#else
    iocp_handle_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    wake_event_ = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
#endif
}

io_context::~io_context() {
    try {
        stop();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    // Stop and join the monitor thread (may have been started lazily by run_one).
    // Must happen before closing fd events and before ~thread() (which terminate()s if joinable).
    bool expected_monitor = true;
    if (monitor_running_.compare_exchange_strong(expected_monitor, false, memory_order_acq_rel)) {
        ::SetEvent(wake_event_);
        try {
            if (monitor_thread_.joinable()) {
                monitor_thread_.join();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }
#endif

#ifdef NEFORCE_PLATFORM_LINUX
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
    }
    if (wake_fd_ >= 0) {
        ::close(wake_fd_);
    }
#else
    if (iocp_handle_ != nullptr) {
        ::CloseHandle(iocp_handle_);
    }
    if (wake_event_ != nullptr) {
        ::CloseHandle(wake_event_);
    }
    for (const auto& pair: fd_events_) {
        if (pair.second != nullptr) {
            ::CloseHandle(pair.second);
        }
    }
#endif
}

void io_context::add_fd(native_handle_type fd, uint32_t events, fd_callback cb, bool edge_triggered) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::epoll_event ev{};
    ev.events = events | (edge_triggered ? epoll_et : 0U);
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);

    fd_info info{};
    info.fd = fd;
    info.events = events;
    info.callback = move(cb);
    fd_map_[fd] = move(info);
#else
    ::CreateIoCompletionPort(reinterpret_cast<::HANDLE>(fd), iocp_handle_, fd, 0);

    const ::WSAEVENT wevent = ::WSACreateEvent();
    ::WSAEventSelect(fd, wevent, to_wsa_events(events));

    {
        lock<mutex> lk(fd_mutex_);
        fd_events_[fd] = wevent;
    }

    fd_info info{};
    info.fd = fd;
    info.events = events;
    info.callback = move(cb);
    fd_map_[fd] = move(info);

    ::SetEvent(wake_event_);
#endif
}

void io_context::mod_fd(native_handle_type fd, uint32_t events, bool edge_triggered) {
#ifdef NEFORCE_PLATFORM_LINUX
    auto it = fd_map_.find(fd);
    if (it == fd_map_.end()) {
        return;
    }
    it->second.events = events;

    ::epoll_event ev{};
    ev.events = events | (edge_triggered ? epoll_et : 0U);
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
#else
    auto it = fd_map_.find(fd);
    if (it == fd_map_.end()) {
        return;
    }
    it->second.events = events;

    lock<mutex> lk(fd_mutex_);
    const auto eit = fd_events_.find(fd);
    if (eit != fd_events_.end() && eit->second != nullptr) {
        ::WSAEventSelect(fd, eit->second, to_wsa_events(events));
    }
#endif
}

void io_context::remove_fd(native_handle_type fd) {
#ifdef NEFORCE_PLATFORM_LINUX
    fd_map_.erase(fd);
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
#else
    fd_map_.erase(fd);

    lock<mutex> lk(fd_mutex_);
    const auto eit = fd_events_.find(fd);
    if (eit != fd_events_.end()) {
        ::WSAEventSelect(fd, nullptr, 0);
        if (eit->second != nullptr) {
            ::CloseHandle(eit->second);
        }
        fd_events_.erase(eit);
    }
    ::SetEvent(wake_event_);
#endif
}

void io_context::wake() {
#ifdef NEFORCE_PLATFORM_LINUX
    constexpr uint64_t val = 1;
    ::write(wake_fd_, &val, sizeof(val));
#else
    ::PostQueuedCompletionStatus(iocp_handle_, 0, 0, nullptr);
#endif
}

size_t io_context::run() {
    running_.fetch_add(1, memory_order_relaxed);
    size_t total = 0;

#ifdef NEFORCE_PLATFORM_LINUX
    while (!stopped_.load(memory_order_acquire)) {
        const uint64_t deadline = next_timer_deadline();

        constexpr int max_poll_ms = 200;
        int timeout = max_poll_ms;
        if (deadline != numeric_traits<uint64_t>::max()) {
            const uint64_t n_ms = now_ms();
            if (deadline > n_ms) {
                const auto t = static_cast<int>(deadline - n_ms);
                timeout = t < max_poll_ms ? t : max_poll_ms;
            } else {
                timeout = 0;
            }
        }

        if (outstanding_work_.load(memory_order_acquire) == 0 &&
            external_queue_count_.load(memory_order_acquire) == 0 && timer_heap_.empty()) {
            break;
        }

        total += run_one(timeout);
    }

#else
    bool expected_monitor = false;
    if (monitor_running_.compare_exchange_strong(expected_monitor, true, memory_order_acq_rel)) {
        try {
            monitor_thread_.start(&io_context::monitor_loop, this);
        } catch (...) {
            monitor_running_.store(false, memory_order_release);
        }
    }

    while (!stopped_.load(memory_order_acquire)) {
        const uint64_t deadline = next_timer_deadline();
        constexpr ::DWORD max_poll_ms = 200;
        ::DWORD timeout = max_poll_ms;
        if (deadline != numeric_traits<uint64_t>::max()) {
            const uint64_t n_ms = now_ms();
            if (deadline > n_ms) {
                const auto t = static_cast<::DWORD>(deadline - n_ms);
                timeout = t < max_poll_ms ? t : max_poll_ms;
            } else {
                timeout = 0;
            }
        }

        if (outstanding_work_.load(memory_order_acquire) == 0 &&
            external_queue_count_.load(memory_order_acquire) == 0 && timer_heap_.empty()) {
            break;
        }

        total += run_one(static_cast<int>(timeout));
    }

    expected_monitor = true;
    if (monitor_running_.compare_exchange_strong(expected_monitor, false, memory_order_acq_rel)) {
        ::SetEvent(wake_event_);
        try {
            if (monitor_thread_.joinable()) {
                monitor_thread_.join();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }
#endif

    running_.fetch_sub(1, memory_order_relaxed);
    return total;
}

size_t io_context::run_one(int timeout_ms) {
    {
        auto handler = external_queue_.try_pop();
        if (handler) {
            external_queue_count_.fetch_sub(1, memory_order_relaxed);
            (*handler)();
            outstanding_work_.fetch_sub(1, memory_order_relaxed);
            return 1;
        }
    }

    {
        unique_lock<mutex> lk(timer_mutex_);
        if (!timer_heap_.empty()) {
            const uint64_t n_ms = now_ms();
            const uint64_t front_deadline = timer_heap_.front().deadline_ms;
            if (front_deadline <= n_ms) {
                pop_heap(timer_heap_.begin(), timer_heap_.end(), greater<timer_entry>());
                const auto entry = move(timer_heap_.back());
                timer_heap_.pop_back();
                lk.unlock_quiet();
                if (entry.callback) {
                    entry.callback();
                }
                return 1;
            }
        }
    }

    size_t count = 0;

#ifdef NEFORCE_PLATFORM_LINUX
    constexpr int max_events = 128;
    ::epoll_event events[max_events];

    const int n = ::epoll_wait(epoll_fd_, events, max_events, timeout_ms);
    if (n < 0) {
        return 0;
    }

    for (int i = 0; i < n; ++i) {
        const int fd = events[i].data.fd;
        if (fd == wake_fd_) {
            uint64_t dummy = 0;
            ::read(wake_fd_, &dummy, sizeof(dummy));
            continue;
        }
        auto it = fd_map_.find(fd);
        if (it != fd_map_.end() && it->second.callback) {
            it->second.callback(fd, events[i].events, error_code{});
            ++count;
        }
    }

#else
    // Ensure monitor thread is running to translate WSA events -> IOCP completions.
    // run() starts the monitor internally, but direct run_one() callers (e.g. dns_client)
    // also need it — start it lazily on first use.
    bool expected_monitor = false;
    if (monitor_running_.compare_exchange_strong(expected_monitor, true, memory_order_acq_rel)) {
        try {
            monitor_thread_.start(&io_context::monitor_loop, this);
        } catch (...) {
            monitor_running_.store(false, memory_order_release);
        }
    }

    ::DWORD bytes = 0;
    ::ULONG_PTR key = 0;
    ::LPOVERLAPPED overlapped = nullptr;
    const ::DWORD t = (timeout_ms < 0) ? numeric_traits<::DWORD>::max() : static_cast<::DWORD>(timeout_ms);
    const ::BOOL ok = ::GetQueuedCompletionStatus(iocp_handle_, &bytes, &key, &overlapped, t);

    if (overlapped != nullptr) {
        auto fit = file_completions_.find(key);
        if (fit != file_completions_.end()) {
            error_code ec;
            if (ok == FALSE && ::GetLastError() != ERROR_OPERATION_ABORTED) {
                ec = error_code(static_cast<int>(::GetLastError()), error_category::system());
            }
            fit->second(ec, bytes, overlapped);
            ++count;
        }
    } else if (key != 0 && ok == TRUE) {
        int fd = static_cast<int>(key);
        auto events = static_cast<uint32_t>(bytes);

        auto it = fd_map_.find(fd);
        if (it != fd_map_.end() && it->second.callback) {
            it->second.callback(fd, events, error_code{});
            ++count;
        }
    }
#endif

    return count;
}

size_t io_context::poll() {
    size_t total = 0;
    size_t n = 0;
    do {
        n = run_one(0);
        total += n;
    } while (n > 0);
    return total;
}

void io_context::run_pool(size_t n) {
    {
        lock<mutex> lk(pool_mutex_);
        for (size_t i = 0; i < n; ++i) {
            pool_threads_.emplace_back([this] {
                try {
                    run();
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (...) {
                    // Prevent terminate from uncaught exceptions
                }
            });
        }
    }
}

void io_context::stop() {
    if (stopped_.load(memory_order_acquire)) {
        return;
    }

    stopped_.store(true, memory_order_release);

    for (int i = 0; i < 256; ++i) {
        wake();
    }

    vector<thread> to_join;
    {
        lock<mutex> lk(pool_mutex_);
        to_join.swap(pool_threads_);
    }

    for (auto& t: to_join) {
        if (t.joinable()) {
            t.join();
        }
    }
}

io_context::executor io_context::get_executor() noexcept { return executor(*this); }

#ifdef NEFORCE_PLATFORM_WINDOWS

void io_context::register_file_completion(const uintptr_t key, file_completion_cb cb) {
    file_completions_[key] = move(cb);
}

void io_context::unregister_file_completion(const uintptr_t key) { file_completions_.erase(key); }

#endif

NEFORCE_END_NAMESPACE__
