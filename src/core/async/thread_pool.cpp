#include <NeForce/core/async/thread_pool.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/string/string_builder.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    atomic<uint32_t> g_pool_thread_id{0};
    atomic<uint32_t> g_affinity_cursor{0};

    // Adaptive spin before parking: a worker that keeps finding work doubles its
    // spin budget (so a busy pool never generates a futex wake-up per task),
    // while a worker that burns a whole budget without finding work halves it and
    // parks, so an idle pool costs essentially no CPU.
    constexpr size_t idle_min_spin_rounds = 8;
    constexpr size_t idle_max_spin_rounds = 512;
    constexpr size_t idle_relax_cutoff = 128;
    constexpr size_t idle_steal_rounds = 4;

    // A worker that owns a deep local queue publishes one wake-up credit so a
    // parked sibling can come and steal; the owner withdraws the credit as soon
    // as its queue drops below the threshold again.
    constexpr uint32_t local_steal_signal_depth = 8;

    // A worker that finds a backlog far deeper than the pool itself behind the task
    // it just took pulls a whole batch out of the global queue and parks the surplus
    // in its own local queue, so a bulk submission is carried by the per-worker rings
    // instead of by the shared queue (one shared-queue round trip per batch instead of
    // per task). The depth threshold keeps coarse-grained jobs out of the batch path:
    // handing eight long tasks to one worker would leave the other workers idle at the
    // tail of the job, which costs far more than the shared-queue traffic it saves.
    constexpr size_t local_batch_size = 8;
    constexpr uint32_t local_batch_depth_ratio = 16;

    constexpr int64_t scaler_interval_ms = 1;

    constexpr int64_t liveness_interval_ms = 200;
    constexpr uint32_t scaler_slack = 1;

    bool cpu_mask_test(const uint64_t mask, const uint32_t cpu) noexcept {
        return (mask & (static_cast<uint64_t>(1) << cpu)) != 0ULL;
    }
} // namespace


uint32_t thread_pool::thread_pool_id_generator::get_new_id() noexcept {
    return g_pool_thread_id.fetch_add(1, memory_order_relaxed);
}

void thread_pool::thread_pool_id_generator::reset_id() noexcept { g_pool_thread_id.store(0, memory_order_relaxed); }

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
numa_node(other.numa_node),
attached(other.attached.load(memory_order_relaxed)),
published_size(other.published_size.load(memory_order_relaxed)),
retire(other.retire.load(memory_order_relaxed)),
completed(other.completed.load(memory_order_relaxed)),
stolen(other.stolen.load(memory_order_relaxed)),
failed(other.failed.load(memory_order_relaxed)),
steal_signalled(other.steal_signalled) {}

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
    attached.store(other.attached.load(memory_order_relaxed), memory_order_relaxed);
    published_size.store(other.published_size.load(memory_order_relaxed), memory_order_relaxed);
    retire.store(other.retire.load(memory_order_relaxed), memory_order_relaxed);
    completed.store(other.completed.load(memory_order_relaxed), memory_order_relaxed);
    stolen.store(other.stolen.load(memory_order_relaxed), memory_order_relaxed);
    failed.store(other.failed.load(memory_order_relaxed), memory_order_relaxed);
    steal_signalled = other.steal_signalled;
    spin_budget = other.spin_budget;
    idle_counted = other.idle_counted;
    return *this;
}

void worker_context::reset() noexcept {
    queue = local_queue();
    id = 0;
    is_stealing.store(false, memory_order_relaxed);
    consecutive_idle_count = 0;
    cpu_core = 0;
    numa_node = 0;
    attached.store(false, memory_order_relaxed);
    published_size.store(0, memory_order_relaxed);
    retire.store(false, memory_order_relaxed);
    completed.store(0, memory_order_relaxed);
    stolen.store(0, memory_order_relaxed);
    failed.store(0, memory_order_relaxed);
    steal_signalled = false;
    spin_budget = 64;
    idle_counted = false;
}

size_t thread_pool::max_thread_threshhold() noexcept {
    static size_t max_threshhold = sysinfo::instance().get_system_info().processor_numbers;
    return max_threshhold;
}

string thread_pool::pool_statistics::to_string() const {
    string_builder result;
    result.append(_NEFORCE concatenate("total_threads:   ", total_threads, "\n"));
    result.append(_NEFORCE concatenate("idle_threads:    ", idle_threads, "\n"));
    result.append(_NEFORCE concatenate("busy_threads:    ", busy_threads, "\n"));
    result.append(_NEFORCE concatenate("queue_size:      ", queue_size, "\n"));
    result.append(_NEFORCE concatenate("total_submitted: ", total_submitted, "\n"));
    result.append(_NEFORCE concatenate("total_stolen:    ", total_stolen, "\n"));
    result.append(_NEFORCE concatenate("total_completed: ", total_completed));
    return result.build();
}

optional<thread_pool::task_type> thread_pool::take_task(worker_context& self) {
    auto task = try_take_priority();
    if (task) {
        return task;
    }

    if (!self.queue.empty()) {
        task = self.queue.try_pop();
        publish_local_size(self);
        if (task) {
            return task;
        }
    }

    if (global_queue_) {
        task_type item;
        if (global_queue_->try_dequeue(item)) {
            pending_tasks_.fetch_sub(1, memory_order_acq_rel);
            // Only a backlog deeper than local_batch_depth_ratio rounds of all attached workers is worth batching;
            // see the threshold rationale next to local_batch_depth_ratio.
            const uint32_t backlog = pending_tasks_.load(memory_order_relaxed);
            if (backlog > local_batch_depth_ratio * attached_workers_.load(memory_order_acquire)) {
                take_batch(self);
            }
            return optional<task_type>{_NEFORCE move(item)};
        }
    }

    return try_steal_task(self);
}

void thread_pool::execute_task(worker_context& self, task_type& task) {
    try {
        task();
        self.completed.fetch_add(1, memory_order_relaxed);
    } catch (...) {
        // A task must never be able to kill its worker thread.
        self.failed.fetch_add(1, memory_order_relaxed);
    }

    if (helpers_waiting_.load(memory_order_relaxed) != 0U) {
        wake_word_.fetch_add(1, memory_order_release);
        wake_word_.notify_all();
    }
}

void thread_pool::help_until_ready(const function<bool()>& ready) {
    auto* ctx = get_worker_context();
    if (ctx == nullptr || !is_running_.load(memory_order_acquire)) {
        return;
    }

    for (;;) {
        if (ready()) {
            return;
        }

        ctx->consecutive_idle_count = idle_steal_rounds;
        auto task = take_task(*ctx);
        if (task) {
            execute_task(*ctx, *task);
            continue;
        }

        // Nothing to advance right now: park on the wake word. The readiness predicate is
        // published before the completing worker inspects helpers_waiting_, so a completion
        // that races with the park either becomes visible to the re-check below or bumps the
        // word, and no wake-up can be lost.
        helpers_waiting_.fetch_add(1, memory_order_acq_rel);
        const uint32_t observed = wake_word_.load(memory_order_acquire);
        if (ready()) {
            helpers_waiting_.fetch_sub(1, memory_order_acq_rel);
            continue;
        }

        parked_workers_.fetch_add(1, memory_order_relaxed);
        if (wake_word_.load(memory_order_acquire) == observed) {
            wake_word_.wait(observed, memory_order_acquire);
        }
        parked_workers_.fetch_sub(1, memory_order_relaxed);
        helpers_waiting_.fetch_sub(1, memory_order_acq_rel);
    }
}

size_t thread_pool::take_batch(worker_context& ctx) {
    const size_t room = ctx.queue.remain_size();
    if (room == 0U) {
        return 0;
    }

    const size_t want = room < local_batch_size ? room : local_batch_size;
    array<task_type, local_batch_size> batch;
    const size_t got = global_queue_->try_dequeue_bulk(addressof(batch[0]), want);
    if (got == 0U) {
        return 0;
    }

    for (size_t i = 0; i < got; ++i) {
        ctx.queue.push_back(_NEFORCE move(batch[i]));
    }
    pending_tasks_.fetch_sub(static_cast<uint32_t>(got), memory_order_acq_rel);
    publish_local_size(ctx);
    return got;
}

void thread_pool::publish_local_size(worker_context& ctx) {
    const auto depth = static_cast<uint32_t>(ctx.queue.size());
    ctx.published_size.store(depth, memory_order_release);

    if (depth >= local_steal_signal_depth) {
        if (!ctx.steal_signalled && parked_workers_.load(memory_order_relaxed) != 0U &&
            idle_workers_.load(memory_order_relaxed) == 0U) {
            ctx.steal_signalled = true;
            wake_word_.fetch_add(1, memory_order_release);
            wake_word_.notify_one();
        }
    } else {
        ctx.steal_signalled = false;
    }
}

void thread_pool::wake_workers(const uint32_t backlog) noexcept {
    const uint32_t parked = parked_workers_.load(memory_order_relaxed);
    if (parked == 0U) {
        return;
    }

    wake_word_.fetch_add(1, memory_order_release);
    if (backlog > parked) {
        wake_word_.notify_all();
    } else {
        wake_word_.notify_one();
    }
}

optional<thread_pool::task_type> thread_pool::try_take_priority() {
    if (priority_pending_.load(memory_order_acquire) == 0U) {
        return none;
    }

    lock<mutex> lk(priority_mtx_);
    if (priority_queue_.empty()) {
        return none;
    }

    auto task = priority_queue_.top().task;
    priority_queue_.pop();
    priority_pending_.fetch_sub(1, memory_order_acq_rel);
    pending_tasks_.fetch_sub(1, memory_order_acq_rel);
    return task;
}

optional<thread_pool::task_type> thread_pool::try_steal_task(worker_context& ctx) {
    if (ctx.consecutive_idle_count < idle_steal_rounds) {
        return none;
    }

    const uint32_t active = attached_workers_.load(memory_order_acquire);
    if (active < 2U) {
        return none;
    }
    if (steal_worker_count_.load(memory_order_acquire) >= (active + 1U) / 2U) {
        return none;
    }

    // A context is only read after its attached flag was observed true, and the
    // acquire path waits for steal_worker_count_ to drain before a context is
    // reused, so no global lock is needed to keep contexts alive.
    steal_worker_count_.fetch_add(1, memory_order_acq_rel);
    ctx.is_stealing.store(true, memory_order_relaxed);

    const uint32_t my_node = ctx.numa_node;
    const bool has_numa = (numa_nodes_ != nullptr && !numa_nodes_->empty());

    worker_context* best = nullptr;
    uint32_t best_size = 0;

    for (size_t i = 0; i < worker_index_.size(); ++i) {
        worker_context* target = worker_index_[i].load(memory_order_acquire);
        if (target == nullptr || target == &ctx) {
            continue;
        }
        if (!target->attached.load(memory_order_acquire)) {
            continue;
        }
        if (target->is_stealing.load(memory_order_relaxed)) {
            continue;
        }

        const uint32_t depth = target->published_size.load(memory_order_acquire);
        if (depth == 0U) {
            continue;
        }

        if (has_numa && target->numa_node != my_node) {
            if (depth > best_size + 4U) {
                best = target;
                best_size = depth;
            }
        } else if (depth > best_size) {
            best = target;
            best_size = depth;
        }
    }

    optional<task_type> result{none};
    if (best != nullptr) {
        result = best->queue.be_stolen_by(ctx.queue);
        if (result) {
            ctx.stolen.fetch_add(1, memory_order_relaxed);
            publish_local_size(ctx);
        }
    }

    ctx.is_stealing.store(false, memory_order_relaxed);
    steal_worker_count_.fetch_sub(1, memory_order_acq_rel);
    return result;
}

void thread_pool::apply_affinity(worker_context& ctx, const size_t slot_index) {
    auto core = static_cast<uint32_t>(slot_index);

    if (numa_nodes_ != nullptr && !numa_nodes_->empty()) {
        const auto& node = (*numa_nodes_)[slot_index % numa_nodes_->size()];
        ctx.numa_node = node.node_id;
        if (!node.core_list.empty()) {
            core = node.core_list[slot_index % node.core_list.size()];
        }
    }

    const uint64_t allowed = allowed_cpus_;
    if (allowed != 0ULL && !cpu_mask_test(allowed, core)) {
        // The computed core is outside the process CPU mask (taskset, cgroups):
        // fall back to a CPU of the allowed set, rotating across pools.
        const auto total = static_cast<uint32_t>(popcount64(allowed));
        if (total != 0U) {
            const auto wanted = g_affinity_cursor.fetch_add(1, memory_order_relaxed) % total;
            uint32_t seen = 0;
            for (uint32_t cpu = 0; cpu < 64U; ++cpu) {
                if (!cpu_mask_test(allowed, cpu)) {
                    continue;
                }
                if (seen == wanted) {
                    core = cpu;
                    break;
                }
                ++seen;
            }
        }
    }

    ctx.cpu_core = core;

    if (affinity_enabled_.load(memory_order_relaxed) && allowed != 0ULL && cpu_mask_test(allowed, core)) {
        this_thread::bind_core(core);
    }
}

size_t thread_pool::acquire_slot() {
    lock<mutex> lk(workers_mtx_);

    for (size_t i = 0; i < worker_slots_.size(); ++i) {
        if (worker_slots_[i] && !worker_slots_[i]->used) {
            worker_slots_[i]->used = true;
            return i;
        }
    }

    if (worker_slots_.size() >= thread_threshhold_) {
        return numeric_traits<size_t>::max();
    }

    auto slot = make_unique<worker_slot>();
    slot->context = make_unique<worker_context>();
    slot->used = true;
    worker_slots_.emplace_back(_NEFORCE move(slot));
    return worker_slots_.size() - 1;
}

bool thread_pool::spawn_worker() {
    const size_t slot_index = acquire_slot();
    if (slot_index == numeric_traits<size_t>::max() || slot_index >= worker_index_.size()) {
        return false;
    }

    const auto& slot = worker_slots_[slot_index];

    // A context may only be reused once every thief that observed it is gone:
    // steal_worker_count_ is bumped before any context field is touched.
    while (steal_worker_count_.load(memory_order_acquire) != 0U) {
        this_thread::yield();
    }
    slot->context->reset();
    slot->id = thread_pool_id_generator::get_new_id();
    slot->context->id = slot->id;
    worker_index_[slot_index].store(slot->context.get(), memory_order_release);

    auto worker_func = [this, slot_index] { thread_function(slot_index); };
    slot->thread = make_unique<lazy_thread>(_NEFORCE move(worker_func));
    slot->thread->start();
    slot->thread->detach();
    return true;
}

bool thread_pool::dispatch_task(task_type&& job, const priority_type priority, const shared_ptr<task_info>& info) {
    if (static_cast<uint32_t>(priority) > 0) {
        {
            lock<mutex> lk(priority_mtx_);
            priority_queue_.emplace(_NEFORCE move(job), priority, info);
        }
        priority_pending_.fetch_add(1, memory_order_release);
        const uint32_t priority_before = pending_tasks_.fetch_add(1, memory_order_release);
        ++total_submitted_tasks_;
        arm_liveness_tick();
        wake_workers(priority_before + 1U);
        return true;
    }

    auto* ctx = get_worker_context();
    if (ctx != nullptr && ctx->queue.remain_size() > 0U) {
        ctx->queue.push_back(_NEFORCE move(job));
        publish_local_size(*ctx);
        ++total_submitted_tasks_;
        arm_liveness_tick();
        return true;
    }

    const uint32_t pending_before = pending_tasks_.fetch_add(1, memory_order_acq_rel);
    if (global_queue_ == nullptr || pending_before >= task_threshhold_ || !global_queue_->enqueue(_NEFORCE move(job))) {
        pending_tasks_.fetch_sub(1, memory_order_acq_rel);
        return false;
    }

    ++total_submitted_tasks_;
    arm_liveness_tick();
    wake_workers(pending_before + 1U);
    return true;
}

void thread_pool::arm_liveness_tick() {
    bool expected = false;
    if (!liveness_armed_.compare_exchange_strong(expected, true, memory_order_acq_rel, memory_order_relaxed)) {
        return;
    }
    if (timer_) {
        timer_->add_task(steady_clock::now() + milliseconds(liveness_interval_ms), [this] { liveness_tick(); });
    }
}

void thread_pool::liveness_tick() {
    const uint32_t pending = pending_tasks_.load(memory_order_acquire);

    size_t completed = 0;
    size_t local_depth = 0;
    for (size_t i = 0; i < worker_slots_.size(); ++i) {
        if (worker_slots_[i] && worker_slots_[i]->context) {
            completed += worker_slots_[i]->context->completed.load(memory_order_relaxed);
            local_depth += worker_slots_[i]->context->published_size.load(memory_order_acquire);
        }
    }
    const bool has_work = pending != 0U || local_depth != 0U;

    // Only a genuine stall (queued work plus zero progress during a whole tick) triggers the prod,
    // so the normal submit/drain pattern never sees an extra wake-up storm.
    if (has_work && completed == liveness_completed_) {
        wake_word_.fetch_add(1, memory_order_release);
        wake_word_.notify_all();
    }
    liveness_completed_ = completed;

    if (has_work && is_running_.load(memory_order_acquire) && timer_) {
        timer_->add_task(steady_clock::now() + milliseconds(liveness_interval_ms), [this] { liveness_tick(); });
    } else {
        liveness_armed_.store(false, memory_order_release);
    }
}

void thread_pool::prewarm_dispatch(const int64_t deadline_ns) {
    if (!is_running_.load(memory_order_acquire)) {
        return;
    }

    warm_deadline_ns_.store(deadline_ns, memory_order_release);
    wake_word_.fetch_add(1, memory_order_release);
    wake_word_.notify_one();
}

bool thread_pool::hold_warm_spin(worker_context& self) {
    const auto deadline = warm_deadline_ns_.load(memory_order_acquire);
    if (deadline == 0) {
        return false;
    }

    const auto now_ns = time_cast<nanoseconds>(steady_clock::now() - steady_clock::time_point{}).count();
    if (now_ns >= deadline) {
        if (warm_owner_.load(memory_order_acquire) == static_cast<int32_t>(self.id)) {
            warm_owner_.store(-1, memory_order_release);
            warm_deadline_ns_.store(0, memory_order_release);
        }
        return false;
    }

    int32_t expected = -1;
    if (warm_owner_.compare_exchange_strong(expected, static_cast<int32_t>(self.id), memory_order_acq_rel,
                                            memory_order_acquire) ||
        expected == static_cast<int32_t>(self.id)) {
        while (is_running_.load(memory_order_acquire)) {
            const auto tick = time_cast<nanoseconds>(steady_clock::now() - steady_clock::time_point{}).count();
            if (tick >= deadline) {
                break;
            }
            this_thread::relax();
        }
        warm_owner_.store(-1, memory_order_release);
        warm_deadline_ns_.store(0, memory_order_release);
        return true;
    }

    return false;
}

void thread_pool::worker_cleanup(const size_t slot_index) {
    const auto& slot = worker_slots_[slot_index];
    worker_context* const ctx = slot->context.get();

    if (ctx != nullptr) {
        ctx->attached.store(false, memory_order_release);

        if (ctx->idle_counted) {
            ctx->idle_counted = false;
            idle_workers_.fetch_sub(1, memory_order_relaxed);
        }

        // Finish the tasks this worker still owns instead of dropping them.
        for (;;) {
            auto remaining = ctx->queue.try_pop();
            if (!remaining) {
                break;
            }
            if (*remaining) {
                try {
                    (*remaining)();
                    ctx->completed.fetch_add(1, memory_order_relaxed);
                } catch (...) {
                    ctx->failed.fetch_add(1, memory_order_relaxed);
                }
            }
        }

        ctx->steal_signalled = false;
    }

    worker_index_[slot_index].store(nullptr, memory_order_release);
    get_worker_context() = nullptr;

    {
        lock<mutex> lk(workers_mtx_);
        slot->used = false;
    }

    attached_workers_.fetch_sub(1, memory_order_acq_rel);
    attached_workers_.notify_all();
}

void thread_pool::thread_function(const size_t slot_index) {
    worker_context* const self_ptr = worker_index_[slot_index].load(memory_order_acquire);
    if (self_ptr == nullptr) {
        return;
    }
    worker_context& self = *self_ptr;

    get_worker_context() = &self;
    self.queue.set_steal_strategy(configured_steal_strategy_, configured_steal_batch_);
    apply_affinity(self, slot_index);

    self.attached.store(true, memory_order_release);
    attached_workers_.fetch_add(1, memory_order_acq_rel);
    attached_workers_.notify_all();

    size_t pending_stall_retries = 0;

    for (;;) {
        auto task = take_task(self);
        if (task) {
            if (self.idle_counted) {
                self.idle_counted = false;
                idle_workers_.fetch_sub(1, memory_order_relaxed);
            }
            if (self.spin_budget < idle_max_spin_rounds) {
                self.spin_budget <<= 1;
            }
            self.consecutive_idle_count = 0;
            wake_workers(pending_tasks_.load(memory_order_acquire));
            execute_task(self, *task);
            continue;
        }

        if (!is_running_.load(memory_order_acquire) || self.retire.load(memory_order_acquire)) {
            break;
        }

        if (hold_warm_spin(self)) {
            if (self.idle_counted) {
                self.idle_counted = false;
                idle_workers_.fetch_sub(1, memory_order_relaxed);
            }
            self.consecutive_idle_count = 0;
            continue;
        }

        ++self.consecutive_idle_count;
        if (self.consecutive_idle_count == 1 && !self.idle_counted) {
            self.idle_counted = true;
            idle_workers_.fetch_add(1, memory_order_relaxed);
        }
        if (self.consecutive_idle_count <= self.spin_budget) {
            if (self.consecutive_idle_count <= idle_relax_cutoff) {
                this_thread::relax();
            } else {
                this_thread::yield();
            }
            continue;
        }

        // Only shrink the spin budget when the pool really has nothing to do,
        // so a hot pool keeps its workers ready instead of parking and paying a
        // wake-up syscall per submission.
        if (pending_tasks_.load(memory_order_acquire) == 0U && self.spin_budget > idle_min_spin_rounds) {
            self.spin_budget >>= 1;
        }
        if (self.idle_counted) {
            self.idle_counted = false;
            idle_workers_.fetch_sub(1, memory_order_relaxed);
        }

        const uint32_t observed = wake_word_.load(memory_order_acquire);
        parked_workers_.fetch_add(1, memory_order_relaxed);

        task = take_task(self);
        if (task) {
            parked_workers_.fetch_sub(1, memory_order_relaxed);
            self.consecutive_idle_count = 0;
            pending_stall_retries = 0;
            wake_workers(pending_tasks_.load(memory_order_acquire));
            execute_task(self, *task);
            continue;
        }

        if (pending_tasks_.load(memory_order_acquire) != 0U && pending_stall_retries < 64U) {
            ++pending_stall_retries;
            wake_word_.fetch_add(1, memory_order_release);
            wake_word_.notify_one();
            parked_workers_.fetch_sub(1, memory_order_relaxed);
            this_thread::relax();
            continue;
        }
        pending_stall_retries = 0;

        if (wake_word_.load(memory_order_acquire) == observed) {
            self.consecutive_idle_count = 0;
            wake_word_.wait(observed, memory_order_acquire);
        }
        parked_workers_.fetch_sub(1, memory_order_relaxed);
        self.consecutive_idle_count = self.spin_budget;
    }

    worker_cleanup(slot_index);
}

void thread_pool::scaler_function() {
    while (!scaler_stop_.load(memory_order_acquire)) {
        this_thread::sleep_for_ms(scaler_interval_ms);
        if (scaler_stop_.load(memory_order_acquire) || !is_running_.load(memory_order_acquire)) {
            break;
        }
        if (pool_mode_.load(memory_order_relaxed) != pool_mode::cached) {
            continue;
        }

        const uint32_t attached = attached_workers_.load(memory_order_acquire);
        const uint32_t parked = parked_workers_.load(memory_order_relaxed);
        const uint32_t pending = pending_tasks_.load(memory_order_acquire);

        // Grow only when there is more runnable work than workers, shrink only
        // with a slack, so the pool does not oscillate around the balance point.
        // A large backlog ramps several workers per tick instead of one, which is
        // what keeps cached mode competitive with a fixed pool under a burst.
        if (pending > attached && attached < thread_threshhold_) {
            uint32_t step = pending / (attached == 0U ? 1U : attached);
            step = step == 0U ? 1U : min(step, static_cast<uint32_t>(4));
            for (uint32_t i = 0; i < step; ++i) {
                if (attached_workers_.load(memory_order_acquire) >= thread_threshhold_) {
                    break;
                }
                spawn_worker();
            }
            continue;
        }

        if (attached > init_thread_size_ && parked > pending + scaler_slack) {
            for (size_t i = 0; i < worker_index_.size(); ++i) {
                worker_context* target = worker_index_[i].load(memory_order_acquire);
                if (target == nullptr || !target->attached.load(memory_order_acquire)) {
                    continue;
                }
                if (target->published_size.load(memory_order_acquire) != 0U) {
                    continue;
                }

                target->retire.store(true, memory_order_release);
                wake_word_.fetch_add(1, memory_order_release);
                wake_word_.notify_all();
                break;
            }
        }
    }
}

thread_pool::pool_statistics thread_pool::statistics_unsafe() const {
    pool_statistics stats{};
    size_t completed = 0;
    size_t stolen = 0;
    size_t attached = 0;

    for (size_t i = 0; i < worker_slots_.size(); ++i) {
        if (!worker_slots_[i]) {
            continue;
        }
        const auto* ctx = worker_slots_[i]->context.get();
        if (ctx == nullptr) {
            continue;
        }
        completed += ctx->completed.load(memory_order_relaxed);
        stolen += ctx->stolen.load(memory_order_relaxed);
        if (ctx->attached.load(memory_order_acquire)) {
            ++attached;
        }
    }

    stats.total_threads = attached;
    stats.idle_threads = parked_workers_.load(memory_order_relaxed);
    stats.busy_threads = stats.total_threads > stats.idle_threads ? stats.total_threads - stats.idle_threads : 0;
    stats.queue_size = pending_tasks_.load(memory_order_acquire);
    stats.total_submitted = total_submitted_tasks_.load(memory_order_relaxed);
    stats.total_stolen = stolen;
    stats.total_completed = completed;
    return stats;
}

thread_pool::thread_pool() :
timer_(make_unique<timer_scheduler<steady_clock>>()),
thread_threshhold_{max_thread_threshhold()} {
    worker_slots_.reserve(thread_threshhold_);
    reset_worker_index(thread_threshhold_);
}

void thread_pool::reset_worker_index(const size_t capacity) {
    worker_index_.clear();
    worker_index_.reserve(capacity);
    for (size_t i = 0; i < capacity; ++i) {
        atomic<worker_context*> empty;
        empty.store(nullptr, memory_order_relaxed);
        worker_index_.emplace_back(_NEFORCE move(empty));
    }
}

thread_pool::~thread_pool() {
    if (!is_running_.load(memory_order_acquire)) {
        return;
    }
    try {
        stop();
    } catch (...) {
        terminate();
    }
}

bool thread_pool::set_mode(const pool_mode mode) noexcept {
    if (is_running_.load(memory_order_acquire)) {
        return false;
    }
    pool_mode_.store(mode, memory_order_relaxed);
    return true;
}

bool thread_pool::set_steal_mode(const steal_strategy strategy, const uint32_t steal_batch) noexcept {
    if (is_running_.load(memory_order_acquire)) {
        return false;
    }
    configured_steal_strategy_ = strategy;
    configured_steal_batch_ = steal_batch;
    return true;
}

bool thread_pool::set_warmup_window(const int64_t window_us) noexcept {
    if (is_running_.load(memory_order_acquire)) {
        return false;
    }
    warmup_window_us_ = window_us > 0 ? window_us : 0;
    return true;
}

void thread_pool::set_task_tracking(const bool enable) noexcept {
    task_tracking_enabled_.store(enable, memory_order_relaxed);
}

shared_ptr<task_info> thread_pool::make_task_info(const priority_type priority) {
    if (!task_tracking_enabled_.load(memory_order_relaxed)) {
        // One shared placeholder keeps submit_result::task_info non-null while
        // removing the per-task allocation from the hot path entirely.
        static shared_ptr<task_info> placeholder = make_shared<task_info>(0, priority);
        return placeholder;
    }
    return make_shared<task_info>(generate_task_id(), priority);
}

bool thread_pool::set_cpu_affinity(const bool enable) noexcept {
    if (is_running_.load(memory_order_acquire)) {
        return false;
    }
    affinity_enabled_.store(enable, memory_order_relaxed);
    return true;
}

bool thread_pool::set_task_threshhold(const size_t threshhold) noexcept {
    if (is_running_.load(memory_order_acquire)) {
        return false;
    }
    task_threshhold_ = threshhold;
    return true;
}

bool thread_pool::set_thread_threshhold(const size_t threshhold) {
    if (is_running_.load(memory_order_acquire) || pool_mode_.load(memory_order_relaxed) == pool_mode::fixed) {
        return false;
    }

    thread_threshhold_ = threshhold > max_thread_threshhold() ? max_thread_threshhold() : threshhold;
    if (thread_threshhold_ == 0) {
        thread_threshhold_ = 1;
    }

    lock<mutex> lk(workers_mtx_);
    worker_slots_.clear();
    reset_worker_index(thread_threshhold_);
    worker_slots_.reserve(thread_threshhold_);
    return true;
}

thread_pool::pool_statistics thread_pool::statistics() const { return statistics_unsafe(); }

bool thread_pool::start(const size_t init_thread_size) {
    if (is_running_.load(memory_order_acquire)) {
        return false;
    }

    const auto& numa_info = sysinfo::instance().get_numa_info();
    numa_nodes_ = numa_info.empty() ? nullptr : &numa_info;

    global_queue_ = make_unique<lock_free_queue<task_type>>();
    timer_ = make_unique<timer_scheduler<steady_clock>>();

    uint64_t allowed = 0;
    if (!this_thread::affinity(allowed)) {
        allowed = 0;
    }
    allowed_cpus_ = allowed;

    {
        lock<mutex> lk(workers_mtx_);
        worker_slots_.clear();
        reset_worker_index(thread_threshhold_);
    }

    pending_tasks_.store(0, memory_order_relaxed);
    priority_pending_.store(0, memory_order_relaxed);
    wake_word_.store(0, memory_order_relaxed);
    warm_deadline_ns_.store(0, memory_order_relaxed);
    warm_owner_.store(-1, memory_order_relaxed);
    parked_workers_.store(0, memory_order_relaxed);
    idle_workers_.store(0, memory_order_relaxed);
    helpers_waiting_.store(0, memory_order_relaxed);
    steal_worker_count_.store(0, memory_order_relaxed);
    attached_workers_.store(0, memory_order_relaxed);
    total_submitted_tasks_.store(0, memory_order_relaxed);
    next_task_id_.store(0, memory_order_relaxed);
    thread_pool_id_generator::reset_id();
    scaler_stop_.store(false, memory_order_relaxed);

    init_thread_size_ = init_thread_size;
    is_running_.store(true, memory_order_release);

    for (size_t i = 0; i < init_thread_size_; ++i) {
        if (!spawn_worker()) {
            break;
        }
    }

    // Ready barrier instead of a fixed sleep: wait until every worker registered.
    const auto deadline = steady_clock::now() + seconds(2);
    uint32_t attached = attached_workers_.load(memory_order_acquire);
    while (attached < init_thread_size_ && steady_clock::now() < deadline) {
        attached_workers_.wait(attached, memory_order_acquire);
        attached = attached_workers_.load(memory_order_acquire);
    }

    if (pool_mode_.load(memory_order_relaxed) == pool_mode::cached) {
        scaler_thread_ = make_unique<lazy_thread>([this] { scaler_function(); });
        scaler_thread_->start();
    }

    return true;
}

thread_pool::pool_statistics thread_pool::stop() {
    if (!is_running_.load(memory_order_acquire)) {
        return {};
    }

    const size_t saved_total_threads = attached_workers_.load(memory_order_acquire);

    is_running_.store(false, memory_order_release);

    scaler_stop_.store(true, memory_order_release);
    if (scaler_thread_) {
        if (scaler_thread_->joinable()) {
            scaler_thread_->join();
        }
        scaler_thread_.reset();
    }

    if (timer_) {
        timer_->stop();
    }

    // Wake every parked worker: the wait word has to change, shutdown must never
    // depend on a wake-up that only a producer would normally provide.
    wake_word_.fetch_add(attached_workers_.load(memory_order_acquire) + 1U, memory_order_acq_rel);
    wake_word_.notify_all();

    uint32_t attached = attached_workers_.load(memory_order_acquire);
    while (attached != 0U) {
        attached_workers_.wait(attached, memory_order_acquire);
        attached = attached_workers_.load(memory_order_acquire);
    }

    auto stat = statistics_unsafe();
    stat.total_threads = saved_total_threads;

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
        lock<mutex> lk(workers_mtx_);
        worker_slots_.clear();
        worker_index_.clear();
    }

    total_submitted_tasks_.store(0, memory_order_relaxed);
    next_task_id_.store(0, memory_order_relaxed);
    pending_tasks_.store(0, memory_order_relaxed);
    priority_pending_.store(0, memory_order_relaxed);
    wake_word_.store(0, memory_order_relaxed);
    warm_deadline_ns_.store(0, memory_order_relaxed);
    warm_owner_.store(-1, memory_order_relaxed);
    parked_workers_.store(0, memory_order_relaxed);
    idle_workers_.store(0, memory_order_relaxed);
    helpers_waiting_.store(0, memory_order_relaxed);
    steal_worker_count_.store(0, memory_order_relaxed);
    thread_pool_id_generator::reset_id();

    init_thread_size_ = 0;

    return stat;
}

NEFORCE_END_NAMESPACE__
