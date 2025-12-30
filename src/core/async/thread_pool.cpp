#include <MSTL/core/async/thread_pool.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

struct thread_pool_id_generator {
    static uint32_t& get_id() noexcept {
        static uint32_t pool_thread_id = 0;
        return pool_thread_id;
    }
    static uint32_t get_new_id() noexcept { return get_id()++; }
    static void reset_id() noexcept { get_id() = 0; }
};

MSTL_BEGIN_INNER__

manual_thread::manual_thread(thread_func&& func) noexcept
    : func_(_MSTL move(func)),
    thread_id_(thread_pool_id_generator::get_new_id()) {}

void manual_thread::start() {
    _MSTL thread t(_MSTL move(func_), thread_id_);
    t.detach();
}

MSTL_END_INNER__

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
    _INNER worker_context ctx;
    ctx.id = thread_id;

    {
        _MSTL lock_guard<_MSTL mutex> lock(worker_contexts_mtx_);
        worker_contexts_.emplace(thread_id, _MSTL move(ctx));
        if (thread_id < worker_contexts_ptr_.size()) {
            worker_contexts_ptr_[thread_id].store(
                &worker_contexts_[thread_id], _MSTL memory_order_release
            );
        }
    }

    _INNER t_worker_ctx = &worker_contexts_[thread_id];
    auto last = high_resolution_clock::now();

    for (;;) {
        _MSTL optional<Task> task = _MSTL nullopt;

        // 从本地队列获取任务
        if (!_INNER t_worker_ctx->queue.empty()) {
            task = _INNER t_worker_ctx->queue.try_pop();
        }

        // 从全局队列获取任务
        if (!task) {
            _MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);
            if (!task_queue_.empty()) {
                task = task_queue_.top().task;
                task_queue_.pop();
                --task_size_;
                not_full_.notify_all();
            }
        }

        // 尝试窃取任务
        if (!task) {
            task = try_steal_task(*_INNER t_worker_ctx);
        }

        // 执行任务或等待
        if (task) {
            --idle_thread_size_;
            try {
                (*task)();
            } catch (...) {
                // 错误处理
            }
            ++total_completed_tasks_;
            ++idle_thread_size_;
            last = high_resolution_clock::now();
        } else {
			// 没有任务，进入等待
            _MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);

            // 检查退出条件
            if (!is_running_) {
                if (thread_id < worker_contexts_ptr_.size()) {
                    worker_contexts_ptr_[thread_id].store(nullptr, _MSTL memory_order_release);
                }
                {
                    _MSTL lock_guard<_MSTL mutex> ctx_lock(worker_contexts_mtx_);
                    worker_contexts_.erase(thread_id);
                }
                threads_map_.erase(thread_id);
                exit_cond_.notify_all();
                _INNER t_worker_ctx = nullptr;
                return;
            }

            // 动态线程模式下的超时退出
            if (pool_mode_ == THREAD_POOL_MODE::MODE_CACHED) {
                if (_MSTL cv_status::timeout == not_empty_.wait_for(lock, seconds(1))) {
                    auto now = high_resolution_clock::now();
                    const auto sub = duration_cast<seconds>(now - last);
                    if (sub.count() >= THREAD_POOL_MAX_IDLE_SECONDS
                        && threads_map_.size() > init_thread_size_) {
                        // 清理 WorkerContext
                        if (thread_id < worker_contexts_ptr_.size()) {
                            worker_contexts_ptr_[thread_id].store(nullptr, _MSTL memory_order_release);
                        }
                        {
                            _MSTL lock_guard<_MSTL mutex> ctx_lock(worker_contexts_mtx_);
                            worker_contexts_.erase(thread_id);
                        }
                        threads_map_.erase(thread_id);
                        --idle_thread_size_;
                        _INNER t_worker_ctx = nullptr;
                        return;
                        }
                }
            } else {
                // 固定线程模式，无限等待
                not_empty_.wait(lock);
            }
        }
    }
}

_MSTL optional<thread_pool::Task> thread_pool::try_steal_task(_INNER worker_context& ctx) {
    // 限制同时窃取的线程数
    if (steal_worker_count_.load(_MSTL memory_order_acquire) >=
        worker_contexts_ptr_.size() / 2) {
        return _MSTL nullopt;
        }

    steal_worker_count_.fetch_add(1, _MSTL memory_order_release);
    ctx.is_stealing.store(true, _MSTL memory_order_release);

    // 寻找任务最多的工作线程
    size_t max_size = 0;
    _INNER worker_context* target = nullptr;

    for (size_t i = 0; i < worker_contexts_ptr_.size(); ++i) {
        _INNER worker_context* other = worker_contexts_ptr_[i].load(_MSTL memory_order_acquire);
        if (other == nullptr || other->id == ctx.id) continue;
        if (other->is_stealing.load(_MSTL memory_order_acquire)) continue;

        size_t other_size = other->queue.size();
        if (other_size > max_size) {
            max_size = other_size;
            target = other;
        }
    }

    _MSTL optional<Task> result = _MSTL nullopt;
    if (target != nullptr && max_size > 0) {
        result = target->queue.be_stolen_by(ctx.queue);
    }

    steal_worker_count_.fetch_sub(1, _MSTL memory_order_release);
    ctx.is_stealing.store(false, _MSTL memory_order_release);

    return result;
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
        auto ptr = _MSTL make_unique<_INNER manual_thread>(
            [this](const int id) {
                thread_function(id);
            });
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

    {
        _MSTL lock_guard<_MSTL mutex> ctx_lock(worker_contexts_mtx_);
        worker_contexts_.clear();
    }
    for (auto& ptr : worker_contexts_ptr_) {
        ptr.store(nullptr, _MSTL memory_order_release);
    }

    task_size_ = 0;
    total_submitted_tasks_ = 0;
    total_completed_tasks_ = 0;
    steal_worker_count_ = 0;
    thread_pool_id_generator::reset_id();
}

MSTL_END_NAMESPACE__
