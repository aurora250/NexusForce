#ifndef MSTL_STACKTRACE_HPP__
#define MSTL_STACKTRACE_HPP__
#include "../container/vector.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include "mutex.hpp"
#endif
MSTL_BEGIN_NAMESPACE__

class MSTL_API stacktrace : public istringify<stacktrace> {
public:
    class frame : public istringify<frame> {
    private:
        void* address_;

#ifdef MSTL_PLATFORM_WINDOWS__
        static void ensure_initialized();
        static mutex& dbghelp_mutex();
#endif

    public:
        frame() : address_(nullptr) {}
        explicit frame(void* addr) : address_(addr) {}

        MSTL_NODISCARD void* address() const noexcept { return address_; }
        MSTL_NODISCARD string name() const;

        MSTL_NODISCARD bool operator ==(const frame& other) const noexcept {
            return address_ == other.address_;
        }
        MSTL_NODISCARD bool operator !=(const frame& other) const noexcept {
            return !(*this == other);
        }

        MSTL_NODISCARD string to_string() const;
    };

private:
    vector<frame> frames_;

public:
    explicit stacktrace(size_t skip = 0, size_t max_depth = 64);

    MSTL_NODISCARD size_t size() const noexcept { return frames_.size(); }
    MSTL_NODISCARD bool empty() const noexcept { return frames_.empty(); }

    MSTL_NODISCARD const frame& operator [](const size_t idx) const noexcept {
        return frames_[idx];
    }
    MSTL_NODISCARD frame& operator [](const size_t idx) noexcept {
        return frames_[idx];
    }

    MSTL_NODISCARD auto begin() const noexcept { return frames_.begin(); }
    MSTL_NODISCARD auto end() const noexcept { return frames_.end(); }
    MSTL_NODISCARD auto cbegin() const noexcept { return frames_.cbegin(); }
    MSTL_NODISCARD auto cend() const noexcept { return frames_.cend(); }

    MSTL_NODISCARD string to_string() const;
};

MSTL_END_NAMESPACE__
#endif // MSTL_STACKTRACE_HPP__
