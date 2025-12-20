#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/async/thread_pool.hpp>
MSTL_BEGIN_NAMESPACE__

struct pool_id_generator {
    static uint32_t& get_id() noexcept {
        static uint32_t pool_thread_id = 0;
        return pool_thread_id;
    }
    static uint32_t get_new_id() noexcept { return get_id()++; }
    static void reset_id() noexcept { get_id() = 0; }
};


manual_thread::manual_thread(thread_func&& func) noexcept
    : func_(_MSTL move(func)),
    thread_id_(pool_id_generator::get_new_id()) {}

void manual_thread::start() {
    _MSTL thread t(_MSTL move(func_), thread_id_);
    t.detach();
}

string thread_pool::pool_statistics::to_string() const {
    string result;
    result += _MSTL to_string("total_threads:   ", total_threads,   "\n");
    result += _MSTL to_string("idle_threads:    ", idle_threads,    "\n");
    result += _MSTL to_string("busy_threads:    ", busy_threads,    "\n");
    result += _MSTL to_string("queue_size:      ", queue_size,      "\n");
    result += _MSTL to_string("total_submitted: ", total_submitted, "\n");
    result += _MSTL to_string("total_completed: ", total_completed);
    return result;
}

void thread_pool::thread_function(const id_type thread_id) {
    auto last = high_resolution_clock::now();

    for (;;) {
        Task task{};
        {
            _MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);
            while (task_queue_.empty()) {
                if (!is_running_) {
                    threads_map_.erase(thread_id);
                    exit_cond_.notify_all();
                    return;
                }
                if (pool_mode_ == THREAD_POOL_MODE::MODE_CACHED) {
                    if (_MSTL cv_status::timeout == not_empty_.wait_for(lock, seconds(1))) {
                        auto now = high_resolution_clock::now();
                        const auto sub = duration_cast<seconds>(now - last);
                        if (sub.count() >= THREAD_POOL_MAX_IDLE_SECONDS
                            && threads_map_.size() > init_thread_size_) {
                            threads_map_.erase(thread_id);
                            --idle_thread_size_;
                            return;
                        }
                    }
                }
                else {
                    not_empty_.wait(lock);
                }
            }

            --idle_thread_size_;
            task = task_queue_.top().task;
            task_queue_.pop();
            --task_size_;
            if (!task_queue_.empty()) not_empty_.notify_all();
            not_full_.notify_all();
        }
        if (task != nullptr) {
            try {
                task();
            } catch (...) {
                // 错误处理
            }
            ++total_completed_tasks_;
        }
        ++idle_thread_size_;
        last = high_resolution_clock::now();
    }
}

bool thread_pool::set_mode(const THREAD_POOL_MODE mode) noexcept {
    if (is_running_) return false;
    pool_mode_ = mode;
    return true;
}

bool thread_pool::set_task_threshhold(const size_t threshhold) noexcept {
    if (is_running_) return false;
    task_threshhold_ = threshhold;
    return true;
}

bool thread_pool::set_thread_threshhold(const size_t threshhold) noexcept {
    if (is_running_ || pool_mode_ == THREAD_POOL_MODE::MODE_FIXED) return false;
    thread_threshhold_ = threshhold > THREAD_POOL_THREAD_MAX_THRESHHOLD
        ? THREAD_POOL_THREAD_MAX_THRESHHOLD : threshhold;
    return true;
}

thread_pool::pool_statistics thread_pool::statistics() const {
    _MSTL unique_lock<_MSTL mutex> lock(const_cast<_MSTL mutex&>(task_queue_mtx_));
    pool_statistics stats{};
    stats.total_threads = threads_map_.size();
    stats.idle_threads = idle_thread_size_.load();
    stats.busy_threads = stats.total_threads > stats.idle_threads ?
        stats.total_threads - stats.idle_threads : 0;
    stats.queue_size = task_size_.load();
    stats.total_submitted = total_submitted_tasks_.load();
    stats.total_completed = total_completed_tasks_.load();
    return stats;
}

bool thread_pool::start(const size_t init_thread_size) {
    if(is_running_) return false;
    is_running_ = true;
    init_thread_size_ = init_thread_size;
    for (id_type i = 0; i < init_thread_size_; i++) {
        auto ptr = _MSTL make_unique<manual_thread>([this](const int id) { thread_function(id); });
        threads_map_.emplace(ptr->id(), _MSTL move(ptr));
    }
    for (id_type i = 0; i < init_thread_size_; i++) {
        threads_map_[i]->start();
        ++idle_thread_size_;
    }
    return true;
}

void thread_pool::stop() {
    if (!is_running_) return;
    is_running_ = false;

    _MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);
    not_empty_.notify_all();
    exit_cond_.wait(lock, [&] { return threads_map_.empty(); });

    while (!task_queue_.empty()) task_queue_.pop();
    task_size_ = 0;
    total_submitted_tasks_ = 0;
    total_completed_tasks_ = 0;
    pool_id_generator::reset_id();
}

MSTL_END_NAMESPACE__
