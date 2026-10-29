#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/time/clocks.hpp>
#ifdef NEFORCE_PLATFORM_POSIX
#    include <cerrno>
#    include <unistd.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/epoll.h>
#    include <sys/eventfd.h>
#endif
#ifdef NEFORCE_PLATFORM_FREEBSD
#    include <NeForce/core/system/pipe.hpp>
#    include <fcntl.h>
#    include <poll.h>
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
    long to_wsa_events(const epoll_event e) {
        long wsa = FD_CLOSE;
        if ((e & epoll_event::in) != epoll_event::none) {
            wsa |= FD_READ | FD_ACCEPT;
        }
        if ((e & epoll_event::out) != epoll_event::none) {
            wsa |= FD_WRITE | FD_CONNECT;
        }
        return wsa;
    }

    epoll_event from_wsa_events(const long network_events) {
        auto out = epoll_event::none;
        if ((network_events & (FD_READ | FD_ACCEPT)) != 0) {
            out = out | epoll_event::in;
        }
        if ((network_events & (FD_WRITE | FD_CONNECT | FD_CLOSE)) != 0) {
            out = out | epoll_event::out;
        }
        return out;
    }
#endif
} // namespace


struct io_context::monitor {
#ifdef NEFORCE_PLATFORM_WINDOWS
    using file_completion_cb = function<void(error_code, size_t, void* overlapped)>;

    void* iocp_handle_ = nullptr;
    void* wake_event_ = nullptr;
    thread monitor_thread_;
    atomic<bool> monitor_running_{false};

    flat_unordered_map<native_handle_type, void*> fd_events_;
    vector<void*> pending_close_;

    flat_unordered_map<uintptr_t, file_completion_cb> file_completions_;

    monitor() {
        iocp_handle_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
        wake_event_ = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    }

    ~monitor() {
        bool expected = true;
        if (monitor_running_.compare_exchange_strong(expected, false, memory_order_acq_rel)) {
            ::SetEvent(wake_event_);
            if (monitor_thread_.joinable()) {
                try {
                    monitor_thread_.join();
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (const exception& e) {
                    NEFORCE_REPORT_EXCEPTION(e);
                }
            }
        }

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
        for (void* handle: pending_close_) {
            ::CloseHandle(handle);
        }
        pending_close_.clear();
    }

    void monitor_loop();

    void register_file_completion(uintptr_t key, file_completion_cb cb) { file_completions_[key] = move(cb); }

    void unregister_file_completion(uintptr_t key) { file_completions_.erase(key); }
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    pipe wake_pipe_;

    monitor() { wake_pipe_ = pipe(false, true); }
#else
    int epoll_fd_ = -1;
    int wake_fd_ = -1;

    monitor() {
        epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
        wake_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
    }

    ~monitor() {
        if (epoll_fd_ >= 0) {
            ::close(epoll_fd_);
        }
        if (wake_fd_ >= 0) {
            ::close(wake_fd_);
        }
    }
#endif
};

#ifdef NEFORCE_PLATFORM_WINDOWS
void io_context::monitor::monitor_loop() {
    while (monitor_running_) {
        unique_lock<mutex> lk(fd_mutex_);

        // Close handles retired by remove_fd(). They are swapped out under the lock and
        // closed only after the wait set below has been rebuilt, so a handle is never
        // closed while WSAWaitForMultipleEvents may still be waiting on it.
        vector<void*> to_close;
        to_close.swap(pending_close_);

        vector<::HANDLE> handles;
        vector<native_handle_type> fds;
        handles.push_back(wake_event_);
        fds.push_back(-1);

        for (const auto& pair: fd_events_) {
            handles.push_back(pair.second);
            fds.push_back(pair.first);
        }
        lk.unlock_quiet();

        for (void* h: to_close) {
            ::CloseHandle(h);
        }

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
                    const epoll_event events = from_wsa_events(net_ev.lNetworkEvents);
                    if (events != epoll_event::none) {
                        ::PostQueuedCompletionStatus(iocp_handle_, static_cast<uint32_t>(events), fd, nullptr);
                    }
                }
            }
        }
    }
}
#endif

uint64_t io_context::next_timer_deadline() {
    steady_clock::time_point deadline;
    if (!timers_.next_deadline(deadline)) {
        return numeric_traits<uint64_t>::max();
    }
    return static_cast<uint64_t>(time_cast<milliseconds>(deadline.since_epoch()).count());
}

bool io_context::fire_expired_timer() {
    // The lock-free hint keeps the common "no timer due" case out of the heap lock.
    const int64_t hint = timers_.next_deadline_ns();
    if (hint == 0 || hint > steady_clock::now().since_epoch().count()) {
        return false;
    }

    heap_timer::expired_entry expired[1];
    if (timers_.drain_expired(steady_clock::now(), expired, 1) == 0) {
        return false;
    }

    timer_callback callback;
    {
        lock<mutex> lk(timer_mutex_);
        const auto it = timer_callbacks_.find(expired[0].id);
        if (it != timer_callbacks_.end()) {
            callback = move(it->second);
            timer_callbacks_.erase(it);
        }
    }
    if (callback) {
        callback();
    }
    return true;
}

void io_context::process_timers(const size_t max_count) {
    size_t count = 0;
    while (count < max_count && fire_expired_timer()) {
        ++count;
    }
}

size_t io_context::schedule_timer(const uint64_t delay_ms, timer_callback handler) {
    const auto deadline = steady_clock::now() + milliseconds(delay_ms);
    size_t id = 0;
    {
        lock<mutex> lk(timer_mutex_);
        const auto token = timers_.insert(deadline);
        timer_callbacks_.emplace(token, move(handler));
        id = static_cast<size_t>(token);
    }
    wake();
    return id;
}

bool io_context::cancel_timer(const size_t timer_id) {
    const auto token = static_cast<heap_timer::token>(timer_id);
    const bool cancelled = timers_.cancel(token);
    if (cancelled) {
        lock<mutex> lk(timer_mutex_);
        timer_callbacks_.erase(token);
    }
    return cancelled;
}

size_t io_context::drain_handlers(const size_t max_count) {
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

void io_context::work_started() noexcept { outstanding_work_.fetch_add(1, memory_order_relaxed); }

void io_context::work_finished() noexcept { outstanding_work_.fetch_sub(1, memory_order_relaxed); }

bool io_context::has_outstanding_work() const noexcept {
    if (outstanding_work_.load(memory_order_acquire) != 0) {
        return true;
    }
    if (external_queue_count_.load(memory_order_acquire) != 0) {
        return true;
    }
    return !timers_.empty();
}

bool io_context::has_pending_work() const noexcept {
    if (has_outstanding_work()) {
        return true;
    }
    return registered_fds_.load(memory_order_acquire) != 0;
}

io_context::io_context() :
monitor_(make_unique<monitor>()) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::epoll_event ev{};
    ev.events = static_cast<uint32_t>(epoll_event::in);
    ev.data.fd = monitor_->wake_fd_;
    ::epoll_ctl(monitor_->epoll_fd_, EPOLL_CTL_ADD, monitor_->wake_fd_, &ev);

    fd_info wake_info{};
    wake_info.fd = monitor_->wake_fd_;
    wake_info.events = epoll_event::in;
    fd_map_[monitor_->wake_fd_] = move(wake_info);
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    fd_info wake_info{};
    wake_info.fd = monitor_->wake_pipe_.native_read_handle();
    wake_info.events = epoll_event::in;
    fd_map_[wake_info.fd] = move(wake_info);
#endif
}

io_context::~io_context() {
    try {
        stop();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
    }
    // monitor_ stops the Windows monitor thread and closes the platform handles.
}

void io_context::add_fd(native_handle_type fd, epoll_event events, fd_callback cb, bool edge_triggered) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::epoll_event ev{};
    ev.events = static_cast<uint32_t>(events | (edge_triggered ? epoll_event::et : epoll_event::none));
    ev.data.fd = fd;

    lock<mutex> lk(fd_mutex_);

    // Re-registering a fd must MOD its existing epoll entry. EPOLL_CTL_ADD fails with
    // EEXIST once the fd is registered, and ignoring that leaves the kernel watching the
    // *old* event mask while fd_map_ already holds the new callback, so the new callback
    // never receives anything. Components register the same fd once per direction
    // (connect: epoll_event::out, then read: epoll_event::in), so this is a routine transition, not an
    // error case.
    const int ctl_op = (fd_map_.contains(fd)) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    if (::epoll_ctl(monitor_->epoll_fd_, ctl_op, fd, &ev) != 0) {
        // Silently dropping the registration would make the callback unreachable, which
        // shows up much later as an operation that never completes.
        terminate();
    }

    fd_info info{};
    info.fd = fd;
    info.events = events;
    info.callback = move(cb);
    if (!fd_map_.contains(fd)) {
        registered_fds_.fetch_add(1, memory_order_relaxed);
    }
    fd_map_[fd] = move(info);
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    lock<mutex> lk(fd_mutex_);
    fd_info info{};
    info.fd = fd;
    info.events = events;
    info.callback = move(cb);
    if (!fd_map_.contains(fd)) {
        registered_fds_.fetch_add(1, memory_order_relaxed);
    }
    fd_map_[fd] = move(info);
    wake();
#else
    ::CreateIoCompletionPort(reinterpret_cast<::HANDLE>(fd), monitor_->iocp_handle_, fd, 0);

    const ::WSAEVENT wevent = ::WSACreateEvent();
    ::WSAEventSelect(fd, wevent, to_wsa_events(events));

    {
        lock<mutex> lk(fd_mutex_);
        // Re-registering the same fd replaces the association;
        // retire the previous event handle so the monitor thread closes it after rebuilding its wait set
        // (it may still be waiting on the old handle right now).
        const auto prev = monitor_->fd_events_.find(fd);
        if (prev != monitor_->fd_events_.end()) {
            if (prev->second != nullptr && prev->second != wevent) {
                monitor_->pending_close_.push_back(prev->second);
            }
            prev->second = wevent;
        } else {
            monitor_->fd_events_[fd] = wevent;
        }

        fd_info info{};
        info.fd = fd;
        info.events = events;
        info.callback = move(cb);
        if (!fd_map_.contains(fd)) {
            registered_fds_.fetch_add(1, memory_order_relaxed);
        }
        fd_map_[fd] = move(info);
    }

    ::SetEvent(monitor_->wake_event_);
#endif
}

void io_context::mod_fd(native_handle_type fd, epoll_event events, bool edge_triggered) {
    lock<mutex> lk(fd_mutex_);

    const auto it = fd_map_.find(fd);
    if (it == fd_map_.end()) {
        return;
    }
    it->second.events = events;

#ifdef NEFORCE_PLATFORM_LINUX
    ::epoll_event ev{};
    ev.events = static_cast<uint32_t>(events | (edge_triggered ? epoll_event::et : epoll_event::none));
    ev.data.fd = fd;
    ::epoll_ctl(monitor_->epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    wake();
#else
    const auto eit = monitor_->fd_events_.find(fd);
    if (eit != monitor_->fd_events_.end() && eit->second != nullptr) {
        ::WSAEventSelect(fd, eit->second, to_wsa_events(events));
    }
#endif
}

void io_context::remove_fd(native_handle_type fd) {
#ifdef NEFORCE_PLATFORM_LINUX
    lock<mutex> lk(fd_mutex_);
    if (fd_map_.contains(fd)) {
        fd_map_.erase(fd);
        // The internal wake fd lives in fd_map_ but was never counted.
        if (fd != monitor_->wake_fd_) {
            registered_fds_.fetch_sub(1, memory_order_relaxed);
        }
    }
    ::epoll_ctl(monitor_->epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    lock<mutex> lk(fd_mutex_);
    if (fd_map_.contains(fd)) {
        fd_map_.erase(fd);
        if (fd != monitor_->wake_pipe_.native_read_handle()) {
            registered_fds_.fetch_sub(1, memory_order_relaxed);
        }
    }
    wake();
#else
    lock<mutex> lk(fd_mutex_);
    if (fd_map_.contains(fd)) {
        fd_map_.erase(fd);
        registered_fds_.fetch_sub(1, memory_order_relaxed);
    }

    const auto eit = monitor_->fd_events_.find(fd);
    if (eit != monitor_->fd_events_.end()) {
        ::WSAEventSelect(fd, nullptr, 0);
        if (eit->second != nullptr) {
            // Do not CloseHandle here, the monitor thread may be waiting on this event.
            // Hand it over; the monitor closes it after rebuilding its wait set.
            monitor_->pending_close_.push_back(eit->second);
        }
        monitor_->fd_events_.erase(eit);
    }
    ::SetEvent(monitor_->wake_event_);
#endif
}

void io_context::wake() noexcept {
#ifdef NEFORCE_PLATFORM_LINUX
    constexpr uint64_t val = 1;
    ::write(monitor_->wake_fd_, &val, sizeof(val));
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    constexpr uint8_t val = 1;
    monitor_->wake_pipe_.write(&val, sizeof(val));
#else
    ::PostQueuedCompletionStatus(monitor_->iocp_handle_, 0, 0, nullptr);
#endif
}

size_t io_context::run() {
    running_.fetch_add(1, memory_order_relaxed);
    size_t total = 0;

#if defined(NEFORCE_PLATFORM_LINUX) || defined(NEFORCE_PLATFORM_FREEBSD)
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

        if (!has_outstanding_work()) {
            break;
        }

        total += run_one(timeout);
    }

#else
    bool expected_monitor = false;
    if (monitor_->monitor_running_.compare_exchange_strong(expected_monitor, true, memory_order_acq_rel)) {
        try {
            monitor_->monitor_thread_.start(&monitor::monitor_loop, monitor_.get());
        } catch (...) {
            monitor_->monitor_running_.store(false, memory_order_release);
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

        if (!has_outstanding_work()) {
            break;
        }

        total += run_one(static_cast<int>(timeout));
    }

    expected_monitor = true;
    if (monitor_->monitor_running_.compare_exchange_strong(expected_monitor, false, memory_order_acq_rel)) {
        ::SetEvent(monitor_->wake_event_);
        try {
            if (monitor_->monitor_thread_.joinable()) {
                monitor_->monitor_thread_.join();
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (const exception& e) {
            NEFORCE_REPORT_EXCEPTION(e);
        }
    }
#endif

    running_.fetch_sub(1, memory_order_relaxed);
    return total;
}

size_t io_context::run_one(int timeout_ms) {
    // wait up to timeout_ms for ONE ready handler.
#ifdef NEFORCE_PLATFORM_WINDOWS
    // Ensure monitor thread is running to translate WSA events -> IOCP completions.
    // run() starts the monitor internally, but direct run_one() callers (e.g. dns_client)
    // also need it — start it lazily on first use.
    bool expected_monitor = false;
    if (monitor_->monitor_running_.compare_exchange_strong(expected_monitor, true, memory_order_acq_rel)) {
        try {
            monitor_->monitor_thread_.start(&monitor::monitor_loop, monitor_.get());
        } catch (...) {
            monitor_->monitor_running_.store(false, memory_order_release);
        }
    }
#endif

    const uint64_t begin_ms = now_ms();

    for (;;) {
        {
            auto handler = external_queue_.try_pop();
            if (handler) {
                external_queue_count_.fetch_sub(1, memory_order_relaxed);
                (*handler)();
                outstanding_work_.fetch_sub(1, memory_order_relaxed);
                return 1;
            }
        }

        if (fire_expired_timer()) {
            return 1;
        }

        // return immediately instead of sleeping the timeout.
        if (!has_pending_work()) {
            return 0;
        }

        // Remaining budget of this call (negative = wait forever).
        int wait_ms = timeout_ms;
        if (timeout_ms >= 0) {
            const uint64_t elapsed_ms = now_ms() - begin_ms;
            if (compare_greater_equal(elapsed_ms, timeout_ms)) {
                return 0;
            }
            wait_ms = static_cast<int>(static_cast<uint64_t>(timeout_ms) - elapsed_ms);
        }

        size_t count = 0;

#ifdef NEFORCE_PLATFORM_LINUX
        constexpr int max_events = 128;
        ::epoll_event events[max_events];

        const int n = ::epoll_wait(monitor_->epoll_fd_, events, max_events, wait_ms);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }

        for (int i = 0; i < n; ++i) {
            const int fd = events[i].data.fd;
            if (fd == monitor_->wake_fd_) {
                uint64_t dummy = 0;
                ::read(monitor_->wake_fd_, &dummy, sizeof(dummy));
                continue;
            }
            // Dispatch on a copy: the callback may call remove_fd() or re-register the same fd from inside the callback,
            // which erases/replaces the stored callback and would otherwise destroy the object currently executing.
            fd_callback cb;
            {
                lock<mutex> lk(fd_mutex_);
                const auto it = fd_map_.find(fd);
                if (it != fd_map_.end()) {
                    cb = it->second.callback;
                }
            }
            if (cb) {
                cb(fd, static_cast<epoll_event>(events[i].events), error_code{});
                ++count;
            }
        }

#elif defined(NEFORCE_PLATFORM_FREEBSD)
        vector<::pollfd> pfds;
        vector<native_handle_type> fds;
        {
            lock<mutex> lk(fd_mutex_);
            pfds.reserve(fd_map_.size());
            fds.reserve(fd_map_.size());
            for (const auto& pair: fd_map_) {
                ::pollfd pfd{};
                pfd.fd = pair.first;
                if ((pair.second.events & epoll_event::in) != epoll_event::none) {
                    pfd.events |= POLLIN;
                }
                if ((pair.second.events & epoll_event::out) != epoll_event::none) {
                    pfd.events |= POLLOUT;
                }
                pfds.push_back(pfd);
                fds.push_back(pair.first);
            }
        }

        const int n = ::poll(pfds.data(), static_cast<::nfds_t>(pfds.size()), wait_ms);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }

        for (size_t i = 0; i < pfds.size(); ++i) {
            if (pfds[i].revents == 0) {
                continue;
            }
            const int fd = fds[i];
            if (fd == monitor_->wake_pipe_.native_read_handle()) {
                char buf[64];
                while (monitor_->wake_pipe_.read(buf, sizeof(buf)) > 0) {
                }
                continue;
            }

            epoll_event events = epoll_event::none;
            if ((pfds[i].revents & POLLIN) != 0) {
                events = events | epoll_event::in;
            }
            if ((pfds[i].revents & POLLOUT) != 0) {
                events = events | epoll_event::out;
            }
            if ((pfds[i].revents & (POLLERR | POLLHUP)) != 0) {
                events = events | epoll_event::in | epoll_event::out;
            }

            fd_callback cb;
            {
                lock<mutex> lk(fd_mutex_);
                const auto it = fd_map_.find(fd);
                if (it != fd_map_.end()) {
                    cb = it->second.callback;
                }
            }
            if (cb) {
                cb(fd, events, error_code{});
                ++count;
            }
        }

#else
        ::DWORD bytes = 0;
        ::ULONG_PTR key = 0;
        ::LPOVERLAPPED overlapped = nullptr;
        const ::DWORD t = (wait_ms < 0) ? numeric_traits<::DWORD>::max() : static_cast<::DWORD>(wait_ms);
        const ::BOOL ok = ::GetQueuedCompletionStatus(monitor_->iocp_handle_, &bytes, &key, &overlapped, t);

        if (overlapped != nullptr) {
            const auto fit = monitor_->file_completions_.find(key);
            if (fit != monitor_->file_completions_.end()) {
                error_code ec;
                if (ok == FALSE && ::GetLastError() != ERROR_OPERATION_ABORTED) {
                    ec = error_code(static_cast<int>(::GetLastError()), error_category::system());
                }
                // Dispatch on a copy: the completion may unregister itself from inside.
                const auto cb = fit->second;
                cb(ec, bytes, overlapped);
                ++count;
            }
        } else if (key != 0 && ok == TRUE) {
            const int fd = static_cast<int>(key);
            const auto events = static_cast<epoll_event>(bytes);

            // Dispatch on a copy, taken under fd_mutex_ so a concurrent add_fd()/remove_fd()
            // cannot free the entry while it is being copied; see the Linux branch above.
            fd_callback cb;
            {
                lock<mutex> lk(fd_mutex_);
                const auto it = fd_map_.find(fd);
                if (it != fd_map_.end()) {
                    cb = it->second.callback;
                }
            }
            if (cb) {
                cb(fd, events, error_code{});
                ++count;
            }
        }
#endif

        if (count > 0) {
            return count;
        }
        // Spurious wakeup (e.g. the wake fd signalled by post() before the handler became visible):
        // retry until work is dispatched or the deadline expires.
    }
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

void io_context::run_pool(const size_t n) {
    {
        lock<mutex> lk(pool_mutex_);
        for (size_t i = 0; i < n; ++i) {
            pool_threads_.emplace_back([this] {
                try {
                    run();
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (const exception& e) {
                    NEFORCE_REPORT_EXCEPTION(e);
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


NEFORCE_END_NAMESPACE__
