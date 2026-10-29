#include <NeForce/core/async/heap_timer.hpp>
#include <NeForce/core/algorithm/heap.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool heap_timer::later(const entry& lhs, const entry& rhs) noexcept {
    if (lhs.deadline != rhs.deadline) {
        return lhs.deadline > rhs.deadline;
    }
    return lhs.id > rhs.id;
}

heap_timer::token heap_timer::insert(const steady_clock::time_point& deadline, const uintptr_t payload) {
    lock<mutex> guard{mutex_};
    const token id = next_id_++;
    heap_.push_back(entry{deadline, id, payload});
    push_heap(heap_.begin(), heap_.end(), later);
    states_.emplace(id, static_cast<uint8_t>(0));
    ++live_;

    // The hint only needs to be early enough to wake a parked thread;
    // a later deadline must not overwrite an earlier one.
    const int64_t ns = deadline.since_epoch().count();
    const int64_t hint = next_deadline_ns_.load(memory_order_relaxed);
    if (hint == 0 || ns < hint) {
        next_deadline_ns_.store(ns, memory_order_release);
    }
    return id;
}

bool heap_timer::cancel(const token id) {
    lock<mutex> guard{mutex_};
    const auto it = states_.find(id);
    if (it == states_.end() || it->second != 0) {
        return false;
    }
    it->second = 1;
    --live_;

    if (live_ == 0) {
        next_deadline_ns_.store(0, memory_order_release);
        return true;
    }
    // The cancelled entry may still sit at the top, which only makes the hint earlier than necessary;
    // compaction keeps the heap from growing without bound.
    if (heap_.size() > live_ * 2 + 64) {
        drop_cancelled_locked();
        refresh_hint_locked();
    }
    return true;
}

bool heap_timer::next_deadline(steady_clock::time_point& out) const {
    lock<mutex> guard{mutex_};
    auto* self = const_cast<heap_timer*>(this);
    // A precise query is also the natural place to refresh the lock-free hint:
    // dropping cancelled tops here keeps later parks from waking on an already dead deadline.
    self->refresh_hint_locked();
    if (heap_.empty()) {
        return false;
    }
    out = heap_.front().deadline;
    return true;
}

int64_t heap_timer::next_deadline_ns() const noexcept { return next_deadline_ns_.load(memory_order_acquire); }

size_t heap_timer::drain_expired(const steady_clock::time_point& now, expired_entry* out, const size_t max_count) {
    lock<mutex> guard{mutex_};
    if (out == nullptr || max_count == 0) {
        return 0;
    }

    drop_cancelled_locked();

    size_t count = 0;
    while (count < max_count && !heap_.empty() && heap_.front().deadline <= now) {
        pop_heap(heap_.begin(), heap_.end(), later);
        const entry top = heap_.back();
        heap_.pop_back();
        states_.erase(top.id);
        --live_;
        out[count].id = top.id;
        out[count].payload = top.payload;
        ++count;
    }

    refresh_hint_locked();
    return count;
}

bool heap_timer::contains(const token id) const {
    lock<mutex> guard{mutex_};
    const auto it = states_.find(id);
    return it != states_.end() && it->second == 0;
}

size_t heap_timer::size() const {
    lock<mutex> guard{mutex_};
    return live_;
}

bool heap_timer::empty() const {
    lock<mutex> guard{mutex_};
    return live_ == 0;
}

void heap_timer::clear() {
    lock<mutex> guard{mutex_};
    heap_.clear();
    states_.clear();
    live_ = 0;
    next_deadline_ns_.store(0, memory_order_release);
}

void heap_timer::drop_cancelled_locked() {
    while (!heap_.empty()) {
        const auto it = states_.find(heap_.front().id);
        if (it == states_.end() || it->second == 0) {
            break;
        }
        pop_heap(heap_.begin(), heap_.end(), later);
        states_.erase(heap_.back().id);
        heap_.pop_back();
    }

    if (heap_.size() <= live_ * 2 + 64) {
        return;
    }
    size_t write = 0;
    for (size_t i = 0; i < heap_.size(); ++i) {
        const auto it = states_.find(heap_[i].id);
        if (it != states_.end() && it->second == 0) {
            if (write != i) {
                heap_[write] = heap_[i];
            }
            ++write;
        } else {
            states_.erase(heap_[i].id);
        }
    }
    heap_.resize(write);
    make_heap(heap_.begin(), heap_.end(), later);
}

void heap_timer::refresh_hint_locked() {
    drop_cancelled_locked();
    const int64_t ns = heap_.empty() ? 0 : heap_.front().deadline.since_epoch().count();
    next_deadline_ns_.store(ns, memory_order_release);
}

NEFORCE_END_NAMESPACE__
