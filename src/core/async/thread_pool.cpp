#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

local_queue::steal_strategy local_queue::steal_strategy_ = steal_strategy::adaptive;

uint32_t local_queue::fixed_batch_size_ = 4;


worker_context*& get_worker_context() noexcept {
    thread_local worker_context* t_worker_ctx{nullptr};
    return t_worker_ctx;
}

shared_ptr<task_group>& get_current_task_group() noexcept {
    thread_local shared_ptr<task_group> t_current_task_group{nullptr};
    return t_current_task_group;
}

uint32_t local_queue::be_stolen_by_impl(local_queue& dst, const uint32_t dst_tail) {
    uint64_t cur_src_head = head_.load(memory_order_acquire);
    uint64_t next_src_head = 0;
    uint32_t steal_num = 0;

    while (true) {
        const auto pir = unpack(cur_src_head);
        const auto cur_src_steal = pir.first;
        const auto cur_src_local_head = pir.second;
        const auto cur_src_tail = tail_.load(memory_order_acquire);
        const auto cur_src_size = cur_src_tail - cur_src_local_head;
        if (cur_src_size == 0) {
            return 0;
        }

        switch (steal_strategy_) {
            case steal_strategy::half: {
                steal_num = cur_src_size / 2;
                break;
            }
            case steal_strategy::single: {
                steal_num = 1;
                break;
            }
            case steal_strategy::fixed_batch: {
                steal_num = (cur_src_size >= fixed_batch_size_) ? fixed_batch_size_
                                                                : min(cur_src_size, static_cast<uint32_t>(1));
                break;
            }
            case steal_strategy::adaptive: {
                if (cur_src_size <= 2) {
                    steal_num = 1;
                } else if (cur_src_size <= 8) {
                    steal_num = cur_src_size / 2;
                } else {
                    steal_num = min(cur_src_size / 2, static_cast<uint32_t>(8));
                }
                break;
            }
            default: {
                unreachable();
            }
        }

        if (steal_num == 0) {
            return 0;
        }

        const auto next_src_local_head = cur_src_local_head + steal_num;
        next_src_head = pack(cur_src_steal, next_src_local_head);

        if (head_.compare_exchange_weak(cur_src_head, next_src_head, memory_order_acq_rel, memory_order_acquire)) {
            break;
        }
    }

    const auto next_src_steal = unpack(next_src_head).first;
    for (uint32_t i = 0; i < steal_num; i++) {
        const auto src_idx = static_cast<uint32_t>(next_src_steal + i) & mask_;
        const auto dst_idx = static_cast<uint32_t>(dst_tail + i) & mask_;
        dst.tasks_[dst_idx] = move(tasks_[src_idx]);
    }

    cur_src_head = next_src_head;
    while (true) {
        const auto cur_src_local_head = unpack(cur_src_head).second;
        next_src_head = pack(cur_src_local_head, cur_src_local_head);
        if (head_.compare_exchange_weak(cur_src_head, next_src_head, memory_order_acq_rel, memory_order_acquire)) {
            return steal_num;
        }
    }
}

local_queue::local_queue(local_queue&& other) noexcept :
tasks_(move(other.tasks_)),
head_(other.head_.load(memory_order_relaxed)),
tail_(other.tail_.load(memory_order_relaxed)) {}

local_queue& local_queue::operator=(local_queue&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    tasks_ = move(other.tasks_);
    head_.store(other.head_.load(memory_order_relaxed), memory_order_relaxed);
    tail_.store(other.tail_.load(memory_order_relaxed), memory_order_relaxed);
    return *this;
}

optional<function<void()>> local_queue::try_pop() {
    auto cur_head = head_.load(memory_order_acquire);
    size_t index = 0;
    while (true) {
        const auto pir = unpack(cur_head);
        const auto cur_steal = pir.first;
        const auto cur_local_head = pir.second;

        if (cur_local_head == tail_.load(memory_order_acquire)) {
            return none;
        }

        const auto next_local_head = cur_local_head + 1;
        const auto next_head = (cur_local_head == cur_steal) ? pack(next_local_head, next_local_head)
                                                             : pack(cur_steal, next_local_head);

        if (head_.compare_exchange_weak(cur_head, next_head, memory_order_acq_rel, memory_order_acquire)) {
            index = static_cast<size_t>(cur_local_head) & mask_;
            break;
        }
    }
    auto task = move(tasks_[index]);
    tasks_[index] = nullptr;
    return task;
}

optional<function<void()>> local_queue::be_stolen_by(local_queue& dst_queue) {
    optional<function<void()>> result{none};

    const auto dst_steal = local_queue::unpack(dst_queue.head_.load(memory_order_acquire)).first;
    const auto dst_tail = dst_queue.tail_.load(memory_order_acquire);
    if (dst_tail - dst_steal > static_cast<uint32_t>(capacity()) / 2) {
        return result;
    }

    auto steal_num = this->be_stolen_by_impl(dst_queue, dst_tail);
    if (steal_num == 0) {
        return result;
    }

    steal_num = steal_num - 1;
    const auto next_dst_tail = dst_tail + steal_num;
    const auto idx = static_cast<size_t>(next_dst_tail) & mask_;
    result.emplace(move(dst_queue.tasks_[idx]));

    if (steal_num > 0) {
        dst_queue.tail_.store(next_dst_tail, memory_order_release);
    }
    return result;
}

worker_context::worker_context(worker_context&& other) noexcept :
queue(move(other.queue)),
id(other.id),
is_stealing(other.is_stealing.load(memory_order_relaxed)),
consecutive_idle_count(other.consecutive_idle_count) {}

worker_context& worker_context::operator=(worker_context&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    queue = move(other.queue);
    id = other.id;
    is_stealing.store(other.is_stealing.load(memory_order_relaxed), memory_order_relaxed);
    consecutive_idle_count = other.consecutive_idle_count;
    return *this;
}

size_t thread_pool::max_thread_threshhold() noexcept {
    static size_t max_threshhold = sysinfo::instance().get_system_info().processor_numbers;
    return max_threshhold;
}

string thread_pool::pool_statistics::to_string() const {
    string result;
    result += _NEFORCE to_string("total_threads:   ", total_threads, "\n");
    result += _NEFORCE to_string("idle_threads:    ", idle_threads, "\n");
    result += _NEFORCE to_string("busy_threads:    ", busy_threads, "\n");
    result += _NEFORCE to_string("queue_size:      ", queue_size, "\n");
    result += _NEFORCE to_string("total_submitted: ", total_submitted, "\n");
    result += _NEFORCE to_string("total_stolen:    ", total_stolen, "\n");
    result += _NEFORCE to_string("total_completed: ", total_completed);
    return result;
}

void thread_pool::thread_function(const id_type thread_id) {
    worker_context ctx;
    ctx.id = thread_id;

    {
        lock<mutex> lock(worker_contexts_mtx_);
        worker_contexts_.emplace(thread_id, move(ctx));
        if (thread_id < worker_contexts_ptr_.size()) {
            worker_contexts_ptr_[thread_id].store(&worker_contexts_[thread_id], memory_order_release);
        }
    }

    get_worker_context() = &worker_contexts_[thread_id];
    ++idle_thread_size_;
    auto last = system_clock::now();

    constexpr size_t MIN_WAIT_MS = 1;
    constexpr size_t MAX_WAIT_MS = 100;
    constexpr size_t MAX_IDLE_SHIFT = 7;

    for (;;) {
        optional<task_type> task{};

        if (!get_worker_context()->queue.empty()) {
            task = get_worker_context()->queue.try_pop();
        }

        if (!task) {
            lock<mutex> lock(task_queue_mtx_);
            if (!task_queue_.empty()) {
                task = task_queue_.top().task;
                task_queue_.pop();
                --task_size_;
                not_full_.notify_one();
            }
        }

        if (!task) {
            task = try_steal_task(*get_worker_context());
        }

        if (task) {
            get_worker_context()->consecutive_idle_count = 0;

            --idle_thread_size_;
            (*task)();
            ++total_completed_tasks_;
            ++idle_thread_size_;
            last = system_clock::now();
        } else {
            ++get_worker_context()->consecutive_idle_count;

            const size_t shift = min(get_worker_context()->consecutive_idle_count, MAX_IDLE_SHIFT);
            size_t wait_ms = min(MIN_WAIT_MS << shift, MAX_WAIT_MS);

            unique_lock<mutex> lk(task_queue_mtx_);

            if (!is_running_) {
                --idle_thread_size_;

                {
                    lock<mutex> ctx_lock(worker_contexts_mtx_);
                    if (thread_id < worker_contexts_ptr_.size()) {
                        worker_contexts_ptr_[thread_id].store(nullptr, memory_order_release);
                    }
                    worker_contexts_.erase(thread_id);
                }

                threads_map_.erase(thread_id);
                if (threads_map_.empty()) {
                    exit_cond_.notify_all();
                }

                get_worker_context() = nullptr;
                return;
            }

            if (not_empty_.wait_for(lk, milliseconds(wait_ms),
                                    [this] { return !is_running_ || !task_queue_.empty(); })) {
                last = system_clock::now();
            } else if (pool_mode_ == pool_mode::cached) {
                if (cv_status::timeout == not_empty_.wait_for(lk, seconds(1))) {
                    auto now = system_clock::now();
                    const auto sub = time_cast<seconds>(now - last);

                    if (sub.count() >= static_cast<int64_t>(max_idle_seconds) &&
                        threads_map_.size() > init_thread_size_) {
                        {
                            lock<mutex> ctx_lock(worker_contexts_mtx_);
                            if (thread_id < worker_contexts_ptr_.size()) {
                                worker_contexts_ptr_[thread_id].store(nullptr, memory_order_release);
                            }
                            worker_contexts_.erase(thread_id);
                        }

                        threads_map_.erase(thread_id);
                        --idle_thread_size_;
                        get_worker_context() = nullptr;
                        return;
                    }
                }
            }
        }
    }
}

optional<thread_pool::task_type> thread_pool::try_steal_task(worker_context& ctx) {
    if (ctx.consecutive_idle_count > 10 && ctx.consecutive_idle_count % 4 != 0) {
        return none;
    }
    if (steal_worker_count_.load(memory_order_acquire) >= worker_contexts_ptr_.size() / 2) {
        return none;
    }

    steal_worker_count_.fetch_add(1, memory_order_release);
    ctx.is_stealing.store(true, memory_order_release);

    size_t max_size = 0;
    worker_context* target = nullptr;

    thread_local vector<worker_context*> snapshot;
    snapshot.clear();

    {
        lock<mutex> lock(worker_contexts_mtx_);
        for (auto& atomic_ptr: worker_contexts_ptr_) {
            worker_context* ptr = atomic_ptr.load(memory_order_acquire);
            if (ptr != nullptr && ptr->id != ctx.id) {
                snapshot.push_back(ptr);
            }
        }
    }

    for (worker_context* other: snapshot) {
        if (other->is_stealing.load(memory_order_acquire)) {
            continue;
        }

        const size_t other_size = other->queue.size();
        if (other_size > max_size) {
            max_size = other_size;
            target = other;
        }
    }

    optional<task_type> result;

    if (target != nullptr && max_size > 0) {
        result = target->queue.be_stolen_by(ctx.queue);
        if (result) {
            total_stolen_tasks_.fetch_add(1, memory_order_relaxed);
        }
    }

    ctx.is_stealing.store(false, memory_order_release);
    steal_worker_count_.fetch_sub(1, memory_order_release);

    return result;
}

thread_pool::pool_statistics thread_pool::statistics_unsafe() const {
    pool_statistics stats{};
    stats.total_threads = threads_map_.size();
    stats.idle_threads = idle_thread_size_.load();
    stats.busy_threads = stats.total_threads > stats.idle_threads ? stats.total_threads - stats.idle_threads : 0;
    stats.queue_size = task_size_.load();
    stats.total_submitted = total_submitted_tasks_.load();
    stats.total_stolen = total_stolen_tasks_.load();
    stats.total_completed = total_completed_tasks_.load();
    return stats;
}

thread_pool::thread_pool() :
thread_threshhold_{max_thread_threshhold()} {
    worker_contexts_ptr_.reserve(thread_threshhold_);
    for (size_t i = 0; i < thread_threshhold_; ++i) {
        atomic<worker_context*> tmp;
        tmp.store(nullptr, memory_order_relaxed);
        worker_contexts_ptr_.emplace_back(move(tmp));
    }
}

thread_pool::~thread_pool() {
    if (!is_running_) {
        return;
    }
    try {
        stop();
    } catch (...) {
        terminate();
    }
}

bool thread_pool::set_mode(const pool_mode mode) noexcept {
    if (is_running_) {
        return false;
    }
    pool_mode_ = mode;
    return true;
}

bool thread_pool::set_steal_mode(const steal_strategy strategy, uint32_t steal_batch) noexcept {
    if (is_running_) {
        return false;
    }
    local_queue::set_steal_strategy(strategy, steal_batch);
    return true;
}

bool thread_pool::set_task_threshhold(const size_t threshhold) noexcept {
    if (is_running_) {
        return false;
    }
    task_threshhold_ = threshhold;
    return true;
}

bool thread_pool::set_thread_threshhold(const size_t threshhold) noexcept {
    if (is_running_ || pool_mode_ == pool_mode::fixed) {
        return false;
    }
    thread_threshhold_ = threshhold > max_thread_threshhold() ? max_thread_threshhold() : threshhold;
    return true;
}

thread_pool::pool_statistics thread_pool::statistics() const {
    lock<mutex> lock(task_queue_mtx_);
    return statistics_unsafe();
}

bool thread_pool::start(const size_t init_thread_size) {
    if (is_running_) {
        return false;
    }

    {
        lock<mutex> ctx_lock(worker_contexts_mtx_);
        worker_contexts_.clear();
        for (auto& ptr: worker_contexts_ptr_) {
            ptr.store(nullptr, memory_order_release);
        }
    }

    is_running_ = true;
    init_thread_size_ = init_thread_size;
    idle_thread_size_ = 0;

    unique_lock<mutex> lk(task_queue_mtx_);

    for (id_type i = 0; i < init_thread_size_; i++) {
        id_type thread_id = thread_pool_id_generator::get_new_id();
        auto worker_func = [this, thread_id]() { thread_function(thread_id); };
        auto ptr = make_unique<lazy_thread>(move(worker_func));

        {
            lock<mutex> ctx_lock(worker_contexts_mtx_);
            if (thread_id >= worker_contexts_ptr_.size()) {
                worker_contexts_ptr_.reserve(thread_id + 1);
                for (size_t j = worker_contexts_ptr_.size(); j <= thread_id; ++j) {
                    atomic<worker_context*> tmp;
                    tmp.store(nullptr, memory_order_relaxed);
                    worker_contexts_ptr_.emplace_back(move(tmp));
                }
            }
        }

        threads_map_.emplace(thread_id, move(ptr));
        threads_map_[thread_id]->start();
        threads_map_[thread_id]->detach();
    }

    return true;
}

thread_pool::pool_statistics thread_pool::stop() {
    if (!is_running_) {
        return {};
    }
    const size_t saved_total_threads = threads_map_.size();

    is_running_ = false;

    timer_.stop();

    {
        unique_lock<mutex> lk(task_queue_mtx_);
        not_empty_.notify_all();

        exit_cond_.wait(lk, [&] { return threads_map_.empty(); });

        while (!task_queue_.empty()) {
            task_queue_.pop();
        }
        task_size_ = 0;
    }

    {
        lock<mutex> ctx_lock(worker_contexts_mtx_);
        for (auto& ptr: worker_contexts_ptr_) {
            ptr.store(nullptr, memory_order_release);
        }
        worker_contexts_.clear();
    }

    auto stat = statistics_unsafe();
    stat.total_threads = saved_total_threads;

    total_submitted_tasks_ = 0;
    total_completed_tasks_ = 0;
    total_stolen_tasks_ = 0;
    steal_worker_count_ = 0;
    next_task_id_.store(0, memory_order_relaxed);
    thread_pool_id_generator::reset_id();

    threads_map_.clear();
    init_thread_size_ = 0;
    idle_thread_size_ = 0;

    return stat;
}

NEFORCE_END_NAMESPACE__
