#ifndef MSTL_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
#define MSTL_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
#include "../memory/unique_ptr.hpp"
#include "../async/atomic.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
class lock_free_queue {
private:
    struct node_counter {
        uint32_t internal_count : 30;
        uint32_t external_counters : 2;
    };

    struct node;

    struct counted_node_ptr {
        int external_count;
        node* ptr;

        counted_node_ptr() : external_count(0), ptr(nullptr) {}
    };

    struct node {
        _MSTL atomic<T*> data;
        _MSTL atomic<node_counter> count;
        _MSTL atomic<counted_node_ptr> next;

        node(int external_count = 2) {
            node_counter new_count;
            new_count.internal_count = 0;
            new_count.external_counters = external_count;
            count.store(new_count);

            counted_node_ptr node_ptr;
			node_ptr.ptr = nullptr;
			node_ptr.external_count = 0;

            next.store(node_ptr);
        }

        void release_ref() {
            node_counter old_counter = count.load(_MSTL memory_order_relaxed);
            node_counter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            }
            while (!count.compare_exchange_strong(old_counter, new_counter,
                _MSTL memory_order_acquire, _MSTL memory_order_relaxed));
            if (!new_counter.internal_count && !new_counter.external_counters) {
                delete this;
                destruct_count.fetch_add(1);
            }
        }
    };

private:
    static _MSTL atomic<int> destruct_count;
    static _MSTL atomic<int> construct_count;

    _MSTL atomic<counted_node_ptr> head;
    _MSTL atomic<counted_node_ptr> tail;

private:
    void set_new_tail(counted_node_ptr& old_tail, counted_node_ptr const& new_tail) {
        node* const current_tail_ptr = old_tail.ptr;
        while (!tail.compare_exchange_weak(old_tail, new_tail) &&
            old_tail.ptr == current_tail_ptr) {}
        if (old_tail.ptr == current_tail_ptr)
            lock_free_queue::free_external_counter(old_tail);
        else
            current_tail_ptr->release_ref();
    }

    static void free_external_counter(counted_node_ptr& old_node_ptr) {
        node* const ptr = old_node_ptr.ptr;
        int const count_increase = old_node_ptr.external_count - 2;
        node_counter old_counter = ptr->count.load(_MSTL memory_order_relaxed);
        node_counter new_counter;
        do {
            new_counter = old_counter;
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        }
        while (!ptr->count.compare_exchange_strong(old_counter, new_counter,
            _MSTL memory_order_acquire, _MSTL memory_order_relaxed));
        if (!new_counter.internal_count && !new_counter.external_counters) {
            destruct_count.fetch_add(1);
            delete ptr;
        }
    }

    static void increase_external_count(_MSTL atomic<counted_node_ptr>& counter, counted_node_ptr& old_counter) {
        counted_node_ptr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (!counter.compare_exchange_strong(old_counter, new_counter,
            _MSTL memory_order_acquire, _MSTL memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }

public:
    lock_free_queue() {
		counted_node_ptr new_next;
		new_next.ptr = new node();
		new_next.external_count = 1;
		tail.store(new_next);
		head.store(new_next);
    }

    ~lock_free_queue() {
        while (pop()) {}
        auto head_counted_node = head.load();
        delete head_counted_node.ptr;
    }

    void push(T new_value) {
        _MSTL unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr = new node;
        new_next.external_count = 1;
        counted_node_ptr old_tail = tail.load();
        for (;;) {
            lock_free_queue::increase_external_count(tail, old_tail);
            T* old_data = nullptr;
            if (old_tail.ptr->data.compare_exchange_strong(old_data, new_data.get())) {
                counted_node_ptr old_next;
                counted_node_ptr now_next = old_tail.ptr->next.load();
                if (!old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                    delete new_next.ptr;
                    new_next = old_next;
                }
                this->set_new_tail(old_tail, new_next);
                new_data.release();
                break;
            }
            else {
                counted_node_ptr old_next;
                if (old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                    old_next = new_next;
                    new_next.ptr = new node;
                }
                this->set_new_tail(old_tail, old_next);
            }
        }
        ++construct_count;
    }

    unique_ptr<T> pop() {
        counted_node_ptr old_head = head.load(_MSTL memory_order_relaxed);
        for (;;) {
            lock_free_queue::increase_external_count(head, old_head);
            node* const ptr = old_head.ptr;
            if (ptr == tail.load().ptr) {
                ptr->release_ref();
                return _MSTL make_unique<T>();
            }
            counted_node_ptr next = ptr->next.load();
            if (head.compare_exchange_strong(old_head, next)) {
                T* const res = ptr->data.exchange(nullptr);
                lock_free_queue::free_external_counter(old_head);
                return _MSTL make_unique<T>(res);
            }
            ptr->release_ref();
        }
    }
};

template <typename T>
_MSTL atomic<int> lock_free_queue<T>::destruct_count{0};

template <typename T>
_MSTL atomic<int> lock_free_queue<T>::construct_count{0};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
