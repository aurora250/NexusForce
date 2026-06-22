#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

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
    uint32_t claimed_base = 0;

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

        claimed_base = cur_src_local_head;

        const auto next_src_local_head = cur_src_local_head + steal_num;
        next_src_head = pack(cur_src_steal, next_src_local_head);

        if (head_.compare_exchange_weak(cur_src_head, next_src_head, memory_order_acq_rel, memory_order_acquire)) {
            break;
        }
    }

    for (uint32_t i = 0; i < steal_num; i++) {
        const auto src_idx = static_cast<uint32_t>(claimed_base + i) & mask_;
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
steal_strategy_(other.steal_strategy_),
fixed_batch_size_(other.fixed_batch_size_),
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
    steal_strategy_ = other.steal_strategy_;
    fixed_batch_size_ = other.fixed_batch_size_;
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
consecutive_idle_count(other.consecutive_idle_count),
cpu_core(other.cpu_core),
numa_node(other.numa_node) {}

worker_context& worker_context::operator=(worker_context&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    queue = move(other.queue);
    id = other.id;
    is_stealing.store(other.is_stealing.load(memory_order_relaxed), memory_order_relaxed);
    consecutive_idle_count = other.consecutive_idle_count;
    cpu_core = other.cpu_core;
    numa_node = other.numa_node;
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
        const auto& cpu_info = sysinfo::instance().get_CPU_info();
        const uint32_t cpu_count = cpu_info.logical_processors;
        uint32_t core = thread_id % (cpu_count > 0 ? cpu_count : 1);
        if (numa_nodes_ != nullptr && !numa_nodes_->empty()) {
            const auto& node = (*numa_nodes_)[thread_id % numa_nodes_->size()];
            ctx.numa_node = node.node_id;
            if (!node.core_list.empty()) {
                core = node.core_list[thread_id % node.core_list.size()];
            }
        }
        ctx.cpu_core = core;
    }

    {
        lock<mutex> lock(worker_contexts_mtx_);
        worker_contexts_.emplace(thread_id, move(ctx));
        if (thread_id < worker_contexts_ptr_.size()) {
            worker_contexts_ptr_[thread_id].store(&worker_contexts_[thread_id], memory_order_release);
        }
    }

    get_worker_context() = &worker_contexts_[thread_id];
    get_worker_context()->queue.set_steal_strategy(configured_steal_strategy_, configured_steal_batch_);
    ++idle_thread_size_;
    work_available_.notify_all();

    auto last = system_clock::now();

    for (;;) {
        optional<task_type> task{};
        worker_context& self = *get_worker_context();

        // L1: local queue (hot path, zero contention)
        if (!self.queue.empty()) {
            task = self.queue.try_pop();
        }

        // L2: lock-free global queue
        if (!task && global_queue_) {
            auto ptr = global_queue_->try_pop();
            if (ptr && *ptr) {
                task = move(**ptr);
                global_task_count_.fetch_sub(1, memory_order_relaxed);
            } else if (global_task_count_.load(memory_order_acquire) > 0) {
                // The counter indicates tasks are pending but try_pop
                // returned empty.  This occurs with the lock-free queue
                // when a producer has stored data in the tail node but
                // not yet advanced the tail pointer (transient empty
                // window).  Brief spin-and-retry before falling through
                // to the idle path so that we do not escalate to deep
                // sleep while tasks are actually available.
                for (int r = 0; r < 16 && !task; ++r) {
                    this_thread::relax();
                    ptr = global_queue_->try_pop();
                    if (ptr && *ptr) {
                        task = move(**ptr);
                        global_task_count_.fetch_sub(1, memory_order_relaxed);
                    }
                }
            }
        }

        // L3: work-stealing
        if (!task) {
            task = try_steal_task(self);
        }

        // L4: priority queue (rare path)
        if (!task) {
            lock<mutex> lk(priority_mtx_);
            if (!priority_queue_.empty()) {
                task = priority_queue_.top().task;
                priority_queue_.pop();
            }
        }

        if (task) {
            self.consecutive_idle_count = 0;
            --idle_thread_size_;
            (*task)();
            ++total_completed_tasks_;
            ++idle_thread_size_;
            last = system_clock::now();
        } else {
            ++self.consecutive_idle_count;

            // If the submission counter indicates pending tasks, do
            // not escalate the idle level — the lock-free queue may be
            // in a transient empty window (data stored before tail
            // advanced).  Yield and retry so that we do not drift into
            // deep sleep while work is available.
            //
            // Only applies while the pool is running; when !is_running
            // we must fall through to the CV-wait path where the exit
            // check lives, otherwise workers would spin forever during
            // shutdown.
            if (is_running_.load(memory_order_relaxed) && global_task_count_.load(memory_order_acquire) > 0) {
                this_thread::yield();
                continue;
            }

            if (self.consecutive_idle_count < 16) {
                this_thread::relax();
                if (is_running_.load(memory_order_relaxed)) {
                    continue;
                }
            } else if (self.consecutive_idle_count < 256) {
                this_thread::yield();
                if (is_running_.load(memory_order_relaxed)) {
                    continue;
                }
            }

            // Deep idle: CV wait
            unique_lock<mutex> lk(work_available_mtx_);

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
                lk.unlock_quiet();
                get_worker_context() = nullptr;
                return;
            }

            if (pool_mode_ == pool_mode::cached) {
                work_available_.wait_for(lk, seconds(1));
                auto now = system_clock::now();
                const auto sub = time_cast<seconds>(now - last);
                if (sub.count() >= static_cast<int64_t>(max_idle_seconds) && threads_map_.size() > init_thread_size_) {
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
                    lk.unlock_quiet();
                    get_worker_context() = nullptr;
                    return;
                }
            } else {
                work_available_.wait_for(lk, milliseconds(1));
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

    const uint32_t my_node = ctx.numa_node;
    const bool has_numa = (numa_nodes_ != nullptr && !numa_nodes_->empty());

    worker_context* same_node_target = nullptr;
    size_t same_node_max = 0;
    worker_context* cross_node_target = nullptr;
    size_t cross_node_max = 0;

    // Hold worker_contexts_mtx_ for the entire scan+steal to prevent
    // target workers from exiting and destroying their context mid-access.
    lock<mutex> lock(worker_contexts_mtx_);

    for (auto& atomic_ptr: worker_contexts_ptr_) {
        worker_context* ptr = atomic_ptr.load(memory_order_acquire);
        if (ptr == nullptr || ptr->id == ctx.id || ptr->is_stealing.load(memory_order_acquire)) {
            continue;
        }

        const size_t other_size = ptr->queue.size();
        if (other_size == 0) {
            continue;
        }

        if (has_numa && ptr->numa_node == my_node) {
            if (other_size > same_node_max) {
                same_node_max = other_size;
                same_node_target = ptr;
            }
        } else {
            if (other_size > cross_node_max) {
                cross_node_max = other_size;
                cross_node_target = ptr;
            }
        }
    }

    optional<task_type> result;

    // Prefer same-NUMA-node stealing
    if (same_node_target != nullptr && same_node_max > 0) {
        result = same_node_target->queue.be_stolen_by(ctx.queue);
        if (result) {
            total_stolen_tasks_.fetch_add(1, memory_order_relaxed);
        }
    }

    // Cross-NUMA fallback: only if remote queue is significantly larger
    if (!result && cross_node_target != nullptr && cross_node_max > 4) {
        result = cross_node_target->queue.be_stolen_by(ctx.queue);
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
    stats.queue_size = global_task_count_.load();
    stats.total_submitted = total_submitted_tasks_.load();
    stats.total_stolen = total_stolen_tasks_.load();
    stats.total_completed = total_completed_tasks_.load();
    return stats;
}

thread_pool::thread_pool() :
thread_threshhold_{max_thread_threshhold()} {
    worker_contexts_.reserve(thread_threshhold_);
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

bool thread_pool::set_steal_mode(const steal_strategy strategy, const uint32_t steal_batch) noexcept {
    if (is_running_) {
        return false;
    }
    configured_steal_strategy_ = strategy;
    configured_steal_batch_ = steal_batch;
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

thread_pool::pool_statistics thread_pool::statistics() const { return statistics_unsafe(); }

bool thread_pool::start(const size_t init_thread_size) {
    if (is_running_) {
        return false;
    }

    {
        const auto& numa_info = sysinfo::instance().get_numa_info();
        if (!numa_info.empty()) {
            numa_nodes_ = &numa_info;
        }
    }
    if (!global_queue_) {
        global_queue_ = make_unique<lock_free_queue<shared_ptr<task_type>>>();
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

    this_thread::sleep_for_ms(5);

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
        unique_lock<mutex> lk(work_available_mtx_);
        work_available_.notify_all();
    }
    {
        unique_lock<mutex> lk(work_available_mtx_);
        exit_cond_.wait(lk, [&] { return threads_map_.empty(); });
    }

    if (global_queue_) {
        global_queue_->clear();
    }

    {
        lock<mutex> lk(priority_mtx_);
        while (!priority_queue_.empty()) {
            priority_queue_.pop();
        }
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
    global_task_count_ = 0;
    steal_worker_count_ = 0;
    next_task_id_.store(0, memory_order_relaxed);
    thread_pool_id_generator::reset_id();

    threads_map_.clear();
    init_thread_size_ = 0;
    idle_thread_size_ = 0;

    return stat;
}

NEFORCE_END_NAMESPACE__
