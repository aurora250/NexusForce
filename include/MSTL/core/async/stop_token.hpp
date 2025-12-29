#ifndef MSTL_CORE_ASYNC_STOP_TOKEN_HPP__
#define MSTL_CORE_ASYNC_STOP_TOKEN_HPP__
#include "semaphore.hpp"
#include "thread.hpp"
#include "atomic.hpp"
MSTL_BEGIN_NAMESPACE__

struct nostopstate_t {
    constexpr explicit nostopstate_t() = default;
};
MSTL_INLINE17 constexpr nostopstate_t nostopstate{};

class stop_source;


class stop_token {
public:
    stop_token() noexcept = default;

    stop_token(const stop_token&) noexcept = default;
    stop_token(stop_token&&) noexcept = default;

    ~stop_token() = default;

    stop_token& operator =(const stop_token&) noexcept = default;
    stop_token& operator =(stop_token&&) noexcept = default;

    MSTL_NODISCARD bool stop_possible() const noexcept {
        return static_cast<bool>(state_ref_) && state_ref_->stop_possible();
    }
    MSTL_NODISCARD bool stop_requested() const noexcept {
        return static_cast<bool>(state_ref_) && state_ref_->stop_requested();
    }

    void swap(stop_token& other) noexcept {
        state_ref_.swap(other.state_ref_);
    }

    MSTL_NODISCARD bool operator ==(const stop_token& rhs) const {
        return state_ref_ == rhs.state_ref_;
    }

private:
    friend class stop_source;
    template<typename Callback>
    friend class stop_callback;

    static void yield() noexcept {
        _INNER thread_relax();
    }

    struct stop_callback_node {
#ifdef MSTL_STANDARD_14__
        using callback_type = void(stop_callback_node*);
#else
        using callback_type = void(stop_callback_node*) noexcept;
#endif
        callback_type* callback;
        stop_callback_node* prev = nullptr;
        stop_callback_node* next = nullptr;
        bool* destroyed = nullptr;
        binary_semaphore done_semaphore{0};

        explicit stop_callback_node(callback_type* cb) : callback(cb) {}

        void run() noexcept {
            callback(this);
        }
    };

    struct stop_state {
        using value_type = uint32_t;
        static constexpr value_type stop_requested_bit = 1;
        static constexpr value_type locked_bit = 2;
        static constexpr value_type ssrc_counter_inc = 4;

        atomic<value_type> owners{1};
        atomic<value_type> value{ssrc_counter_inc};
        stop_callback_node* head = nullptr;
        thread::id requester_thread_id;

        stop_state() = default;

        bool stop_possible() noexcept {
            return value.load(memory_order::acquire) & ~locked_bit;
        }
        bool stop_requested() noexcept {
            return value.load(memory_order::acquire) & stop_requested_bit;
        }

        void add_owner() noexcept {
            owners.fetch_add(1, memory_order::relaxed);
        }

        void release_ownership() noexcept {
            if (owners.fetch_sub(1, memory_order::acq_rel) == 1)
                delete this;
        }

        void add_stop_source() noexcept {
            value.fetch_add(ssrc_counter_inc, memory_order::relaxed);
        }
        void remove_stop_source() noexcept {
            value.fetch_sub(ssrc_counter_inc, memory_order::release);
        }

        void lock() noexcept {
            auto old_value = value.load(memory_order::relaxed);
            while (!try_lock(old_value, memory_order::relaxed)) { }
        }
        void unlock() noexcept {
            value.fetch_sub(locked_bit, memory_order::release);
        }

        bool request_stop() noexcept {
            auto old_value = value.load(memory_order::acquire);
            do {
                if (old_value & stop_requested_bit)
                    return false;
            } while (!try_lock_and_stop(old_value));

            requester_thread_id = this_thread::get_id();

            while (head) {
                bool last_callback;
                stop_callback_node* callback_node = head;
                head = head->next;
                if (head) {
                    head->prev = nullptr;
                    last_callback = false;
                } else {
                    last_callback = true;
                }
                unlock();

                bool destroyed = false;
                callback_node->destroyed = &destroyed;
                callback_node->run();
                if (!destroyed) {
                    callback_node->destroyed = nullptr;
                    callback_node->done_semaphore.release();
                }

                if (last_callback) return true;
                lock();
            }
            unlock();
            return true;
        }

        bool register_callback(stop_callback_node* callback_node) noexcept {
            auto old_value = value.load(memory_order::acquire);
            do {
                if (old_value & stop_requested_bit) {
                    callback_node->run();
                    return false;
                }

                if (old_value < ssrc_counter_inc)
                    return false;
            } while (!try_lock(old_value));

            callback_node->next = head;
            if (head) {
                head->prev = callback_node;
            }
            head = callback_node;
            unlock();
            return true;
        }

        void remove_callback(stop_callback_node* callback_node) {
            lock();

            if (callback_node == head) {
                head = head->next;
                if (head)
                    head->prev = nullptr;
                unlock();
                return;
            } else if (callback_node->prev) {
                callback_node->prev->next = callback_node->next;
                if (callback_node->next)
                    callback_node->next->prev = callback_node->prev;
                unlock();
                return;
            }

            unlock();

            if (requester_thread_id != this_thread::get_id()) {
                callback_node->done_semaphore.acquire();
                return;
            }

            if (callback_node->destroyed)
                *callback_node->destroyed = true;
        }

        bool try_lock(value_type& current_value,
            const memory_order failure_order = memory_order::acquire) noexcept {
            return do_try_lock(current_value, 0, memory_order::acquire, failure_order);
        }

        bool try_lock_and_stop(value_type& current_value) noexcept {
            return do_try_lock(current_value, stop_requested_bit,
                memory_order::acq_rel, memory_order::acquire);
        }

        bool do_try_lock(value_type& current_value, value_type new_bits,
            const memory_order success_order, const memory_order failure_order) noexcept {
            if (current_value & locked_bit) {
                yield();
                current_value = value.load(failure_order);
                return false;
            }
            new_bits |= locked_bit;
            return value.compare_exchange_weak(current_value,
                current_value | new_bits, success_order, failure_order);
        }
    };

    struct stop_state_reference {
        stop_state_reference() = default;

        explicit stop_state_reference(const stop_source&)
            : ptr_(new stop_state()) {}

        stop_state_reference(const stop_state_reference& other) noexcept
            : ptr_(other.ptr_) {
            if (ptr_) {
                ptr_->add_owner();
            }
        }

        stop_state_reference(stop_state_reference&& other) noexcept
            : ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }

        stop_state_reference& operator =(const stop_state_reference& other) noexcept {
            auto new_ptr = other.ptr_;
            if (new_ptr != ptr_) {
                if (new_ptr) {
                    new_ptr->add_owner();
                }
                if (ptr_) {
                    ptr_->release_ownership();
                }
                ptr_ = new_ptr;
            }
            return *this;
        }

        stop_state_reference& operator =(stop_state_reference&& other) noexcept {
            stop_state_reference(move(other)).swap(*this);
            return *this;
        }

        ~stop_state_reference() {
            if (ptr_) {
                ptr_->release_ownership();
            }
        }

        void swap(stop_state_reference& other) noexcept {
            _MSTL swap(ptr_, other.ptr_);
        }

        explicit operator bool() const noexcept {
            return ptr_ != nullptr;
        }

        stop_state* operator->() const noexcept {
            return ptr_;
        }

        bool operator ==(const stop_state_reference& rhs) const noexcept {
            return ptr_ == rhs.ptr_;
        }
        bool operator !=(const stop_state_reference& rhs) const noexcept {
            return ptr_ != rhs.ptr_;
        }

    private:
        stop_state* ptr_ = nullptr;
    };

    stop_state_reference state_ref_;

    explicit stop_token(stop_state_reference state_ref) noexcept
        : state_ref_{move(state_ref)} {}
};


class stop_source {
public:
    stop_source() : state_ref_(*this) {}

    explicit stop_source(nostopstate_t) noexcept {}

    stop_source(const stop_source& other) noexcept
        : state_ref_(other.state_ref_) {
        if (state_ref_) {
            state_ref_->add_stop_source();
        }
    }

    stop_source(stop_source&&) noexcept = default;

    stop_source& operator =(const stop_source& other) noexcept {
        if (state_ref_ != other.state_ref_) {
            stop_source sink(move(*this));
            state_ref_ = other.state_ref_;
            if (state_ref_) {
                state_ref_->add_stop_source();
            }
        }
        return *this;
    }

    stop_source& operator =(stop_source&&) noexcept = default;

    ~stop_source() {
        if (state_ref_) {
            state_ref_->remove_stop_source();
        }
    }

    MSTL_NODISCARD bool stop_possible() const noexcept {
        return static_cast<bool>(state_ref_);
    }

    MSTL_NODISCARD bool stop_requested() noexcept {
        return stop_possible() && state_ref_->stop_requested();
    }

    bool request_stop() noexcept {
        if (stop_possible()) {
            return state_ref_->request_stop();
        }
        return false;
    }

    MSTL_NODISCARD stop_token get_token() const noexcept {
        return stop_token{state_ref_};
    }

    void swap(stop_source& other) noexcept {
        state_ref_.swap(other.state_ref_);
    }

    MSTL_NODISCARD bool operator ==(const stop_source& rhs) const noexcept {
        return state_ref_ == rhs.state_ref_;
    }
    MSTL_NODISCARD bool operator !=(const stop_source& rhs) const noexcept {
        return state_ref_ != rhs.state_ref_;
    }

private:
    stop_token::stop_state_reference state_ref_;
};


template <typename Callback>
class MSTL_NODISCARD stop_callback {
    static_assert(is_nothrow_destructible_v<Callback>, "Callback should be nothrow destructible.");
    static_assert(is_invocable_v<Callback>, "Callback should be invocable.");

public:
    using callback_type = Callback;

    template <typename Cb, enable_if_t<is_constructible_v<Callback, Cb>, int> = 0>
    explicit stop_callback(const stop_token& token, Cb&& callback)
    noexcept(is_nothrow_constructible_v<Callback, Cb>)
        : callback_impl(forward<Cb>(callback)) {
        if (auto state_ref = token.state_ref_) {
            if (state_ref->register_callback(&callback_impl)) {
                state_ref_.swap(state_ref);
            }
        }
    }

    template <typename Cb, enable_if_t<is_constructible_v<Callback, Cb>, int> = 0>
    explicit stop_callback(stop_token&& token, Cb&& callback)
    noexcept(is_nothrow_constructible_v<Callback, Cb>)
        : callback_impl(forward<Cb>(callback)) {
        if (auto& state_ref = token.state_ref_) {
            if (state_ref->register_callback(&callback_impl)) {
                state_ref_.swap(state_ref);
            }
        }
    }

    ~stop_callback() {
        if (state_ref_) {
            state_ref_->remove_callback(&callback_impl);
        }
    }

    stop_callback(const stop_callback&) = delete;
    stop_callback& operator=(const stop_callback&) = delete;
    stop_callback(stop_callback&&) = delete;
    stop_callback& operator=(stop_callback&&) = delete;

private:
    struct callback_impl : stop_token::stop_callback_node {
        template <typename Cb>
        explicit callback_impl(Cb&& callback)
            : stop_callback_node(&execute_callback),
              callback(forward<Cb>(callback)) {}

        Callback callback;

        static void execute_callback(stop_callback_node* node) noexcept {
            Callback& cb = static_cast<callback_impl*>(node)->callback;
            forward<Callback>(cb)();
        }
    };

    callback_impl callback_impl;
    stop_token::stop_state_reference state_ref_;
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Callback>
stop_callback(stop_token, Callback) -> stop_callback<Callback>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_STOP_TOKEN_HPP__
