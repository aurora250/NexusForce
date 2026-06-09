#include <NeForce/core/async/event_loop.hpp>
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
            wsa |= FD_READ | FD_ACCEPT | FD_CLOSE;
        }
        if ((e & epoll_out) != 0U) {
            wsa |= FD_WRITE;
        }
        return wsa;
    }

    uint32_t from_wsa_events(const long network_events) {
        uint32_t out = 0;
        if ((network_events & (FD_READ | FD_ACCEPT | FD_CLOSE)) != 0) {
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
void event_loop::monitor_loop() {
    while (monitor_running_) {
        unique_lock<mutex> lk(fd_mutex_);
        vector<::HANDLE> handles;
        vector<int> fds;
        handles.push_back(wake_event_);
        fds.push_back(-1);

        for (const auto& pair: fd_events_) {
            handles.push_back(pair.second);
            fds.push_back(pair.first);
        }
        lk.unlock_quiet();

        ::DWORD count = (handles.size() > WSA_MAXIMUM_WAIT_EVENTS) ? WSA_MAXIMUM_WAIT_EVENTS
                                                                   : static_cast<DWORD>(handles.size());

        const ::DWORD result = ::WSAWaitForMultipleEvents(count, handles.data(), FALSE, 50, FALSE);

        if (result >= WSA_WAIT_EVENT_0 && result < WSA_WAIT_EVENT_0 + count) {
            const size_t idx = result - WSA_WAIT_EVENT_0;
            if (idx == 0) {
                ::ResetEvent(wake_event_);
            } else if (idx < fds.size()) {
                int fd = fds[idx];
                ::WSANETWORKEVENTS net_ev{};
                if (::WSAEnumNetworkEvents(fd, handles[idx], &net_ev) == 0) {
                    uint32_t events = from_wsa_events(net_ev.lNetworkEvents);
                    if (events != 0) {
                        ::PostQueuedCompletionStatus(iocp_handle_, (DWORD) events, (ULONG_PTR) (intptr_t) fd, nullptr);
                    }
                }
            }
        }
    }
}
#endif

uint64_t event_loop::next_timer_deadline() const {
    lock<mutex> lk(timer_mutex_);
    if (timer_heap_.empty()) {
        return numeric_traits<uint64_t>::max();
    }
    return timer_heap_.front().deadline_ms;
}

void event_loop::process_timers() {
    unique_lock<mutex> lk(timer_mutex_);
    const uint64_t n_ms = now_ms();
    while (!timer_heap_.empty() && timer_heap_.front().deadline_ms <= n_ms) {
        pop_heap(timer_heap_.begin(), timer_heap_.end(), greater<timer_entry>());
        auto entry = move(timer_heap_.back());
        timer_heap_.pop_back();
        lk.unlock_quiet();
        if (entry.callback) {
            entry.callback();
        }
        lk.lock_quiet();
    }
}

size_t event_loop::schedule_timer(uint64_t delay_ms, timer_callback cb) {
    lock<mutex> lk(timer_mutex_);
    const size_t id = next_timer_id_++;
    timer_entry entry;
    entry.id = id;
    entry.deadline_ms = now_ms() + delay_ms;
    entry.callback = move(cb);
    timer_heap_.push_back(entry);
    push_heap(timer_heap_.begin(), timer_heap_.end(), greater<timer_entry>());
    wake();
    return id;
}

bool event_loop::cancel_timer(size_t timer_id) {
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

event_loop::event_loop() {
#ifdef NEFORCE_PLATFORM_LINUX
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    wake_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

    ::epoll_event ev{};
    ev.events = epoll_in | epoll_et;
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

event_loop::~event_loop() {
    stop();

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
    for (auto& pair: fd_events_) {
        if (pair.second != nullptr) {
            ::CloseHandle(pair.second);
        }
    }
#endif
}

void event_loop::add_fd(int fd, uint32_t events, fd_callback cb) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::epoll_event ev{};
    ev.events = events | epoll_et;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);

    fd_info info{};
    info.fd = fd;
    info.events = events;
    info.callback = move(cb);
    fd_map_[fd] = move(info);
#else
    ::CreateIoCompletionPort(reinterpret_cast<::HANDLE>(static_cast<::SOCKET>(fd)), iocp_handle_,
                             static_cast<::ULONG_PTR>(fd), 0);

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

void event_loop::mod_fd(int fd, uint32_t events) {
#ifdef NEFORCE_PLATFORM_LINUX
    auto it = fd_map_.find(fd);
    if (it == fd_map_.end()) {
        return;
    }
    it->second.events = events;

    ::epoll_event ev{};
    ev.events = events | epoll_et;
    ev.data.fd = fd;
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
#else
    auto it = fd_map_.find(fd);
    if (it == fd_map_.end()) {
        return;
    }
    it->second.events = events;

    lock<mutex> lk(fd_mutex_);
    auto eit = fd_events_.find(fd);
    if (eit != fd_events_.end() && eit->second != nullptr) {
        ::WSAEventSelect(fd, eit->second, to_wsa_events(events));
    }
#endif
}

void event_loop::remove_fd(int fd) {
#ifdef NEFORCE_PLATFORM_LINUX
    fd_map_.erase(fd);
    ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
#else
    fd_map_.erase(fd);

    lock<mutex> lk(fd_mutex_);
    auto eit = fd_events_.find(fd);
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

void event_loop::wake() {
#ifdef NEFORCE_PLATFORM_LINUX
    constexpr uint64_t val = 1;
    ::write(wake_fd_, &val, sizeof(val));
#else
    ::PostQueuedCompletionStatus(iocp_handle_, 0, 0, nullptr);
#endif
}

void event_loop::run() {
    running_ = true;

#ifdef NEFORCE_PLATFORM_LINUX
    while (running_) {
        const uint64_t deadline = next_timer_deadline();
        int timeout = -1;
        if (deadline != numeric_traits<uint64_t>::max()) {
            const uint64_t n_ms = now_ms();
            if (deadline > n_ms) {
                timeout = static_cast<int>(deadline - n_ms);
            } else {
                timeout = 0;
            }
        }
        run_once(timeout);
    }

#else
    monitor_running_ = true;
    monitor_thread_.start(&event_loop::monitor_loop, this);

    while (running_) {
        const uint64_t deadline = next_timer_deadline();
        ::DWORD timeout = numeric_traits<::DWORD>::max();
        if (deadline != numeric_traits<uint64_t>::max()) {
            const uint64_t n_ms = now_ms();
            if (deadline > n_ms) {
                timeout = static_cast<::DWORD>(deadline - n_ms);
            } else {
                timeout = 0;
            }
        }
        run_once(static_cast<int>(timeout));
    }

    monitor_running_ = false;
    ::SetEvent(wake_event_);
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
#endif
}

void event_loop::run_once(int timeout_ms) {
#ifdef NEFORCE_PLATFORM_LINUX
    constexpr int max_events = 128;
    ::epoll_event events[max_events];

    const int n = ::epoll_wait(epoll_fd_, events, max_events, timeout_ms);
    if (n < 0) {
        return;
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
            it->second.callback(fd, events[i].events);
        }
    }

#else
    ::DWORD bytes = 0;
    ::ULONG_PTR key = 0;
    ::LPOVERLAPPED overlapped = nullptr;
    const ::DWORD t = (timeout_ms < 0) ? numeric_traits<::DWORD>::max() : static_cast<::DWORD>(timeout_ms);
    const ::BOOL ok = ::GetQueuedCompletionStatus(iocp_handle_, &bytes, &key, &overlapped, t);

    if (key == 0) {
        process_timers();
        return;
    }

    if (ok == TRUE && overlapped == nullptr && ::GetLastError() == WAIT_TIMEOUT) {
        process_timers();
        return;
    }

    int fd = static_cast<int>(key);
    auto events = static_cast<uint32_t>(bytes);

    auto it = fd_map_.find(fd);
    if (it != fd_map_.end() && it->second.callback) {
        it->second.callback(fd, events);
    }
#endif

    process_timers();
}

void event_loop::stop() {
    running_ = false;
    wake();
}

NEFORCE_END_NAMESPACE__
