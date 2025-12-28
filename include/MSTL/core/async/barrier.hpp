#ifndef MSTL_CORE_ASYNC_BARRIER_HPP__
#define MSTL_CORE_ASYNC_BARRIER_HPP__
#include "../memory/unique_ptr.hpp"
#include "../container/array.hpp"
#include "atomic_base.hpp"
#include "thread.hpp"
MSTL_BEGIN_NAMESPACE__

struct empty_completion {
    MSTL_ALWAYS_INLINE void operator ()() noexcept {}
};


template <typename CmplFunc>
class tree_barrier {
    using phase_ref_t = atomic_ref_base<byte_t>;
    using phase_cref_t = atomic_ref_base<const byte_t>;
    
    static constexpr auto phase_alignment = phase_ref_t::required_alignment;
    
    struct alignas(64) state_data {
        alignas(phase_alignment) array<byte_t, 64> tickets;
    };

    ptrdiff_t expected_count_;
    unique_ptr<state_data[]> state_array_;
    atomic_base<ptrdiff_t> expected_adjustment_;
    CmplFunc completion_function_;
    alignas(phase_alignment) byte_t current_phase_;

    bool do_arrive(const byte_t old_phase, size_t current_index) {
        const auto old_phase_value = old_phase;
        const auto half_step = static_cast<byte_t>(old_phase_value + 1);
        const auto full_step = static_cast<byte_t>(old_phase_value + 2);

        size_t current_expected = expected_count_;
        current_index %= ((expected_count_ + 1) >> 1);

        for (int round = 0; ; ++round) {
            if (current_expected <= 1)
                return true;
            
            size_t const end_node = ((current_expected + 1) >> 1),
                         last_node = end_node - 1;
            
            for ( ; ; ++current_index) {
                if (current_index == end_node)
                    current_index = 0;
                    
                auto expected_phase = old_phase;
                phase_ref_t phase_ref(state_array_[current_index].tickets[round]);
                
                if (current_index == last_node && (current_expected & 1)) {
                    if (phase_ref.compare_exchange_strong(
                        expected_phase, full_step, memory_order_acq_rel)) {
                        break;
                    }
                } else if (phase_ref.compare_exchange_strong(
                    expected_phase, half_step, memory_order_acq_rel)) {
                    return false;
                } else if (expected_phase == half_step) {
                    if (phase_ref.compare_exchange_strong(
                        expected_phase, full_step, memory_order_acq_rel)) {
                        break;
                    }
                }
            }
            
            current_expected = last_node + 1;
            current_index >>= 1;
        }
    }

public:
    using arrival_token = byte_t;

    static constexpr ptrdiff_t max() noexcept {
        return numeric_limits<ptrdiff_t>::max();
    }

    tree_barrier(const ptrdiff_t expected, CmplFunc completion)
    : expected_count_(expected),
      expected_adjustment_(0),
      completion_function_(_MSTL move(completion)),
      current_phase_(static_cast<byte_t>(0)) {
        size_t const count = (expected_count_ + 1) >> 1;
        state_array_ = make_unique<state_data[]>(count);
    }

    MSTL_NODISCARD arrival_token arrive(ptrdiff_t update) {
        constexpr hash<thread::id> hasher;
        const size_t current_index = hasher(this_thread::get_id());
        phase_ref_t phase_ref(current_phase_);
        const auto old_phase = phase_ref.load(memory_order_relaxed);
        const auto current_phase_value = static_cast<byte_t>(old_phase);
        
        for(; update; --update) {
            if(do_arrive(old_phase, current_index)) {
                completion_function_();
                expected_count_ += expected_adjustment_.load(memory_order_relaxed);
                expected_adjustment_.store(0, memory_order_relaxed);
                const auto new_phase = static_cast<byte_t>(current_phase_value + 2);
                phase_ref.store(new_phase, memory_order_release);
                phase_ref.notify_all();
            }
        }
        return old_phase;
    }

    void wait(arrival_token&& old_phase) const {
        phase_cref_t phase_ref(current_phase_);
        auto const test_function = [=] {
            return phase_ref.load(memory_order_acquire) != old_phase;
        };
        _MSTL atomic_wait_address(&current_phase_, test_function);
    }

    void arrive_and_drop() {
        expected_adjustment_.fetch_sub(1, memory_order_relaxed);
        static_cast<void>(arrive(1));
    }
};


template <typename CmplFunc = empty_completion>
class barrier {
    using algorithm_type = tree_barrier<CmplFunc>;
    algorithm_type barrier_impl_;

public:
    class arrival_token final {
    public:
        arrival_token(arrival_token&&) = default;
        arrival_token& operator=(arrival_token&&) = default;
        ~arrival_token() = default;

    private:
        friend class barrier;
        using internal_token = typename algorithm_type::arrival_token;
        explicit arrival_token(internal_token token) noexcept : token_(token) { }
        internal_token token_;
    };

    static constexpr ptrdiff_t max() noexcept {
        return algorithm_type::max();
    }

    explicit barrier(ptrdiff_t count, CmplFunc completion = CmplFunc())
        : barrier_impl_(count, _MSTL move(completion)) { }

    barrier(barrier const&) = delete;
    barrier& operator=(barrier const&) = delete;

    MSTL_NODISCARD arrival_token arrive(ptrdiff_t update = 1) {
        return arrival_token{barrier_impl_.arrive(update)};
    }

    void wait(arrival_token&& phase) const {
        barrier_impl_.wait(_MSTL move(phase.token_));
    }

    void arrive_and_wait() {
        this->wait(arrive());
    }

    void arrive_and_drop() {
        barrier_impl_.arrive_and_drop();
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_BARRIER_HPP__
