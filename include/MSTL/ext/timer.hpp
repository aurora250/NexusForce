#ifndef MSTL_TIMER_HPP__
#define MSTL_TIMER_HPP__
#include "MSTL/core/set.hpp"
#include "MSTL/core/functional.hpp"
#include <chrono>
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

struct __timer_node_base {
protected:
	int64_t expire_ = 0;
	size_t id_ = 0;

public:
    __timer_node_base() = default;
    __timer_node_base(const int64_t expire, const size_t id)
    : expire_(expire), id_(id) {}

    __timer_node_base(const __timer_node_base &) = default;
    __timer_node_base &operator=(const __timer_node_base &) = default;
    __timer_node_base(__timer_node_base &&) = default;
    __timer_node_base &operator=(__timer_node_base &&) = default;
    ~__timer_node_base() = default;

    MSTL_NODISCARD int64_t expire() const { return expire_; }
    MSTL_NODISCARD size_t id() const { return id_; }
};

inline bool operator <(const __timer_node_base& lh, const __timer_node_base& rh) {
    if (lh.expire() < rh.expire()) return true;
    if (lh.expire() > rh.expire()) return false;
    return lh.id() < rh.id();
}

MSTL_END_INNER__


struct timer_node : _INNER __timer_node_base {
public:
    using call_back = _MSTL function<void(const timer_node& node)>;

private:
	call_back func_;

public:
    timer_node() = default;
    timer_node(const int64_t expire, const size_t id, call_back&& func)
    : __timer_node_base(expire, id), func_(_MSTL move(func)) {}

    timer_node(const timer_node& node) = default;
    timer_node& operator=(const timer_node& node) = default;
    timer_node(timer_node&& node) = default;
    timer_node& operator=(timer_node&& node) = default;
    ~timer_node() = default;

    MSTL_NODISCARD const call_back& hold() const { return func_; }
};


class timer {
public:
    using node_type = timer_node;
    using func_type = timer_node::call_back;

private:
    _MSTL set<node_type> timer_set_;

    static size_t generate_id() {
        static size_t timer_id_ = 0;
        return ++timer_id_;
    }

public:
    timer() = default;
    ~timer() = default;

    static int64_t get_tick() {
        const auto sc
            = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
        const auto sub
            = std::chrono::duration_cast<std::chrono::microseconds>(sc.time_since_epoch());
        return sub.count();
    }

    node_type add(const int64_t ms, func_type&& func) {
        node_type tnode(get_tick() + ms, generate_id(), _MSTL move(func));
        timer_set_.insert(tnode);
        return tnode;
    }

    bool erase(const node_type& node)  {
        const auto iter = timer_set_.find(node);
        if (iter != timer_set_.end()) {
            timer_set_.erase(iter);
            return true;
        }
        return false;
    }

    bool check() {
        const auto iter = timer_set_.begin();
        if (iter != timer_set_.end() && iter->expire() <= get_tick()) {
            iter->hold()(*iter);
            timer_set_.erase(iter);
            return true;
        }
        return false;
    }

    int64_t sleep() {
        const auto iter = timer_set_.begin();
        if (iter == timer_set_.end()) return -1;
        const int64_t dis = iter->expire() - get_tick();
        return dis > 0 ? dis : 0;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_TIMER_HPP__
