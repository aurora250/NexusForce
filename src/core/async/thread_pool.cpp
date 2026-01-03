#include <MSTL/core/async/thread_pool.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

STEAL_STRATEGY local_queue::steal_strategy_ = STEAL_STRATEGY::HALF;
uint32_t local_queue::fixed_batch_size_ = 4;

uint32_t local_queue::be_stolen_by_impl(local_queue& dst, const uint32_t dst_tail) {
    uint64_t cur_src_head = head_.load(_MSTL memory_order_acquire);
    uint64_t next_src_head;
    uint32_t steal_num;

    while (true) {
        auto [cur_src_steal, cur_src_local_head] = unpack(cur_src_head);
        const auto cur_src_tail = tail_.load(_MSTL memory_order_acquire);
        const auto cur_src_size = cur_src_tail - cur_src_local_head;
        if (cur_src_size == 0) return 0;

        switch (steal_strategy_) {
            case STEAL_STRATEGY::HALF: {
                steal_num = cur_src_size / 2;
                break;
            }
            case STEAL_STRATEGY::SINGLE: {
                steal_num = 1;
                break;
            }
            case STEAL_STRATEGY::FIXED_BATCH: {
                steal_num = (cur_src_size >= fixed_batch_size_)
                    ? fixed_batch_size_
                    : _MSTL min(cur_src_size, static_cast<uint32_t>(1));
            }
            case STEAL_STRATEGY::ADAPTIVE: {
                if (cur_src_size <= 2) {
                    steal_num = 1;
                } else if (cur_src_size <= 8) {
                    steal_num = cur_src_size / 2;
                } else {
                    steal_num = _MSTL min(cur_src_size / 2, static_cast<uint32_t>(8));
                }
            }
            default: {
                MSTL_UNREACHABLE;
            }
        }

        if (steal_num == 0) return 0;

        const auto next_src_local_head = cur_src_local_head + steal_num;
        next_src_head = pack(cur_src_steal, next_src_local_head);

        if (head_.compare_exchange_weak(cur_src_head, next_src_head,
            _MSTL memory_order_acq_rel, _MSTL memory_order_acquire)) {
            break;
        }
    }

    auto [next_src_steal, next_src_local_head] = unpack(next_src_head);
    for (uint32_t i = 0; i < steal_num; i++) {
        const auto src_idx = static_cast<uint32_t>(next_src_steal + i) & mask_;
        const auto dst_idx = static_cast<uint32_t>(dst_tail + i) & mask_;
        dst.tasks_[dst_idx] = _MSTL move(tasks_[src_idx]);
    }

    cur_src_head = next_src_head;
    while (true) {
        auto [cur_src_steal, cur_src_local_head] = unpack(cur_src_head);
        next_src_head = pack(cur_src_local_head, cur_src_local_head);
        if (head_.compare_exchange_weak(cur_src_head, next_src_head,
            _MSTL memory_order_acq_rel, _MSTL memory_order_acquire)) {
            return steal_num;
        }
    }
}

local_queue::local_queue(local_queue&& other) noexcept
    : tasks_(_MSTL move(other.tasks_))
    , head_(other.head_.load(_MSTL memory_order_relaxed))
    , tail_(other.tail_.load(_MSTL memory_order_relaxed)) {}

local_queue& local_queue::operator =(local_queue&& other) noexcept {
    if (this != &other) {
        tasks_ = _MSTL move(other.tasks_);
        head_.store(other.head_.load(_MSTL memory_order_relaxed), _MSTL memory_order_relaxed);
        tail_.store(other.tail_.load(_MSTL memory_order_relaxed), _MSTL memory_order_relaxed);
    }
    return *this;
}

_MSTL optional<_MSTL function<void()>> local_queue::try_pop() {
    auto cur_head = head_.load(_MSTL memory_order_acquire);
    size_t index = 0;
    while (true) {
        auto [cur_steal, cur_local_head] = unpack(cur_head);
        const auto tail = tail_.load(_MSTL memory_order_acquire);
        if (cur_local_head == tail) {
            return _MSTL nullopt;
        }
        const auto next_local_head = cur_local_head + 1;
        const auto next_head = (cur_local_head == cur_steal)
            ? pack(next_local_head, next_local_head)
            : pack(cur_steal, next_local_head);

        if (head_.compare_exchange_weak(cur_head, next_head,
            _MSTL memory_order_acq_rel, _MSTL memory_order_acquire)) {
            index = static_cast<size_t>(cur_local_head) & mask_;
            break;
            }
    }
    auto task = _MSTL move(tasks_[index]);
    tasks_[index] = nullptr;
    return task;
}

_MSTL optional<_MSTL function<void()>> local_queue::be_stolen_by(local_queue& dst_queue) {
    _MSTL optional<_MSTL function<void()>> result{_MSTL nullopt};

    auto [dst_steal, dst_local_head] =
        local_queue::unpack(dst_queue.head_.load(_MSTL memory_order_acquire));
    const auto dst_tail = dst_queue.tail_.load(_MSTL memory_order_acquire);
    if (dst_tail - dst_steal > static_cast<uint32_t>(capacity()) / 2) {
        return result;
    }

    auto steal_num = this->be_stolen_by_impl(dst_queue, dst_tail);
    if (steal_num == 0) return result;

    steal_num = steal_num - 1;
    const auto next_dst_tail = dst_tail + steal_num;
    const auto idx = static_cast<size_t>(next_dst_tail) & mask_;
    result.emplace(_MSTL move(dst_queue.tasks_[idx]));

    if (steal_num > 0) {
        dst_queue.tail_.store(next_dst_tail, _MSTL memory_order_release);
    }
    return result;
}

worker_context::worker_context(worker_context&& other) noexcept
: queue(_MSTL move(other.queue))
, id(other.id)
, is_stealing(other.is_stealing.load(_MSTL memory_order_relaxed))
, consecutive_idle_count(other.consecutive_idle_count) {}

worker_context& worker_context::operator =(worker_context&& other) noexcept {
    if (this != &other) {
        queue = _MSTL move(other.queue);
        id = other.id;
        is_stealing.store(other.is_stealing.load(
            _MSTL memory_order_relaxed), _MSTL memory_order_relaxed);
        consecutive_idle_count = other.consecutive_idle_count;
    }
    return *this;
}

MSTL_THREAD_LOCAL worker_context* t_worker_ctx = nullptr;
MSTL_THREAD_LOCAL _MSTL shared_ptr<task_group> t_current_task_group = nullptr;

worker_context*& get_worker_context() noexcept {
    return t_worker_ctx;
}

_MSTL shared_ptr<task_group>& get_current_task_group() noexcept {
    return t_current_task_group;
}

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
    result += _MSTL to_string("total_stolen:    ", total_stolen,    "\n");
    result += _MSTL to_string("total_completed: ", total_completed);
    return result;
}

void thread_pool::thread_function(const id_type thread_id) {
    worker_context ctx;
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

    t_worker_ctx = &worker_contexts_[thread_id];
    auto last = high_resolution_clock::now();

    constexpr size_t MIN_WAIT_MS = 1;      // 最小等待 1ms
    constexpr size_t MAX_WAIT_MS = 100;    // 最大等待 100ms
    constexpr size_t MAX_IDLE_SHIFT = 7;   // 最多 2^7 = 128 倍

    for (;;) {
        _MSTL optional<Task> task = _MSTL nullopt;

        // 从本地队列获取任务
        if (!t_worker_ctx->queue.empty()) {
            task = t_worker_ctx->queue.try_pop();
        }

        // 从全局队列获取任务
        if (!task) {
            _MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);
            if (!task_queue_.empty()) {
                task = task_queue_.top().task;
                task_queue_.pop();
                --task_size_;
            }
        }

        // 尝试窃取任务
        if (!task) {
            task = try_steal_task(*t_worker_ctx);
        }

        // 执行任务或等待
        if (task) {
            t_worker_ctx->consecutive_idle_count = 0;

            --idle_thread_size_;
            (*task)();
            ++total_completed_tasks_;
            ++idle_thread_size_;
            last = high_resolution_clock::now();
        } else {
            ++t_worker_ctx->consecutive_idle_count;

            // 指数退避
            const size_t shift = _MSTL min(t_worker_ctx->consecutive_idle_count, MAX_IDLE_SHIFT);
            size_t wait_ms = _MSTL min(MIN_WAIT_MS << shift, MAX_WAIT_MS);

            _MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);

            if (!is_running_) {
                --idle_thread_size_;

                if (thread_id < worker_contexts_ptr_.size()) {
                    worker_contexts_ptr_[thread_id].store(nullptr, _MSTL memory_order_release);
                }

                {
                    _MSTL lock_guard<_MSTL mutex> ctx_lock(worker_contexts_mtx_);
                    worker_contexts_.erase(thread_id);
                }

                threads_map_.erase(thread_id);
                exit_cond_.notify_all();
                t_worker_ctx = nullptr;
                return;
            }

            if (not_empty_.wait_for(lock, milliseconds(wait_ms), [this] {
                return !is_running_ || !task_queue_.empty();
            })) {
                last = high_resolution_clock::now();
            } else if (pool_mode_ == THREAD_POOL_MODE::MODE_CACHED) {
                if (_MSTL cv_status::timeout == not_empty_.wait_for(lock, seconds(1))) {
                    auto now = high_resolution_clock::now();
                    const auto sub = duration_cast<seconds>(now - last);

                    if (sub.count() >= THREAD_POOL_MAX_IDLE_SECONDS
                        && threads_map_.size() > init_thread_size_) {

                        if (thread_id < worker_contexts_ptr_.size()) {
                            worker_contexts_ptr_[thread_id].store(nullptr, _MSTL memory_order_release);
                        }
                        {
                            _MSTL lock_guard<_MSTL mutex> ctx_lock(worker_contexts_mtx_);
                            worker_contexts_.erase(thread_id);
                        }

                        threads_map_.erase(thread_id);
                        --idle_thread_size_;
                        t_worker_ctx = nullptr;
                        return;
                    }
                }
            }
        }
    }
}

optional<thread_pool::Task> thread_pool::try_steal_task(worker_context& ctx) {
    if (ctx.consecutive_idle_count > 10 && ctx.consecutive_idle_count % 4 != 0) {
        return nullopt;
    }
    if (steal_worker_count_.load(memory_order_acquire) >= worker_contexts_ptr_.size() / 2) {
        return nullopt;
    }

    steal_worker_count_.fetch_add(1, memory_order_release);
    ctx.is_stealing.store(true, memory_order_release);

    size_t max_size = 0;
    worker_context* target = nullptr;

    {
        lock_guard<mutex> lock(worker_contexts_mtx_);

        for (size_t i = 0; i < worker_contexts_ptr_.size(); ++i) {
            worker_context* other = worker_contexts_ptr_[i].load(memory_order_acquire);

            if (other == nullptr || other->id == ctx.id) {
                continue;
            }
            if (other->is_stealing.load(memory_order_acquire)) {
                continue;
            }

            const size_t other_size = other->queue.size();
            if (other_size > max_size) {
                max_size = other_size;
                target = other;
            }
        }
    }

    optional<Task> result = nullopt;

    if (target != nullptr && max_size > 0) {
        result = target->queue.be_stolen_by(ctx.queue);
        if (result) {
            total_stolen_tasks_.fetch_add(1, memory_order_relaxed);
        }
    }

    steal_worker_count_.fetch_sub(1, memory_order_release);
    ctx.is_stealing.store(false, memory_order_release);

    return result;
}

thread_pool::pool_statistics thread_pool::statistics_unsafe() const {
    pool_statistics stats{};
    stats.total_threads = threads_map_.size();
    stats.idle_threads = idle_thread_size_.load();
    stats.busy_threads = stats.total_threads > stats.idle_threads ?
        stats.total_threads - stats.idle_threads : 0;
    stats.queue_size = task_size_.load();
    stats.total_submitted = total_submitted_tasks_.load();
    stats.total_stolen = total_stolen_tasks_.load();
    stats.total_completed = total_completed_tasks_.load();
    return stats;
}

thread_pool::thread_pool() {
    worker_contexts_ptr_.reserve(THREAD_POOL_THREAD_MAX_THRESHHOLD);
    for (size_t i = 0; i < THREAD_POOL_THREAD_MAX_THRESHHOLD; ++i) {
        atomic<worker_context*> tmp;
        tmp.store(nullptr, memory_order_relaxed);
        worker_contexts_ptr_.emplace_back(move(tmp));
    }
}

thread_pool::~thread_pool() {
    if(is_running_) {
        stop();
    }
}

bool thread_pool::set_mode(const THREAD_POOL_MODE mode) noexcept {
    if (is_running_) return false;
    pool_mode_ = mode;
    return true;
}

bool thread_pool::set_steal_mode(const STEAL_STRATEGY strategy, uint32_t steal_batch) noexcept {
    if (is_running_) return false;
    local_queue::set_steal_strategy(strategy, steal_batch);
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
    return statistics_unsafe();
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

thread_pool::pool_statistics thread_pool::stop() {
    if (!is_running_) return {};
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

    const auto stat = statistics_unsafe();

    task_size_ = 0;
    total_submitted_tasks_ = 0;
    total_completed_tasks_ = 0;
    total_stolen_tasks_ = 0;
    steal_worker_count_ = 0;
    next_task_id_.store(0, memory_order_relaxed);
    thread_pool_id_generator::reset_id();

    return stat;
}

MSTL_END_NAMESPACE__
