#ifndef MSTL_TRACE_MEMORY_HPP__
#define MSTL_TRACE_MEMORY_HPP__
#include "../container/unordered_map.hpp"
#include "../utility/console.hpp"
#include "../utility/stacktrace.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
class trace_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using self = trace_allocator<T>;

    template <typename U>
    struct rebind {
        using other = trace_allocator<U>;
    };

    trace_allocator() = default;

    template <typename U>
    trace_allocator(const trace_allocator<U>& a) : traces_(a.traces_) {}
    self& operator =(const self& a) {
        if (_MSTL addressof(a) == this) return *this;
        traces_ = a.traces_;
        return *this;
    }

    ~trace_allocator() {
#ifdef MSTL_STATE_DEBUG__
        if (!traces_.empty()) {
            _MSTL printcln(color::red(), "Memory leaks detected! \n");
            print_stacktrace();
        }
#endif
    }

    void print_stacktrace() const {
        for(auto& entry : traces_) {
            if (entry.first == 0) continue;
            _MSTL printcln(color::red(), "Leaked pointer: ", static_cast<void*>(entry.first));
            _MSTL printcln(color::red(), "Allocation stack trace:\n", entry.second);
        }
    }

    MSTL_NODISCARD MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        pointer ptr = _MSTL allocator<T>().allocate(n);
        stacktrace st{};
        traces_[ptr] = _MSTL move(st);
        return ptr;
    }

    MSTL_NODISCARD MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return _MSTL allocator<T>().allocate(sizeof(value_type));
    }

    void deallocate(pointer p) noexcept {
        auto it = traces_.find(p);
        if (it != traces_.end()) {
            traces_.erase(it);
        }
        _MSTL allocator<T>().deallocate(p, sizeof(value_type));
    }

    void deallocate(pointer p, const size_type n) noexcept {
        auto it = traces_.find(p);
        if (it != traces_.end()) {
            traces_.erase(it);
        }
        _MSTL allocator<T>().deallocate(p, n);
    }

private:
    _MSTL unordered_map<T*, _MSTL stacktrace> traces_;
};
template <typename T, typename U>
bool operator ==(const trace_allocator<T>&, const trace_allocator<U>&) noexcept {
    return true;
}
template <typename T, typename U>
bool operator !=(const trace_allocator<T>&, const trace_allocator<U>&) noexcept {
    return false;
}

MSTL_END_NAMESPACE__
#endif // MSTL_TRACE_MEMORY_HPP__
