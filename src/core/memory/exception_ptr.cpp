#include <MSTL/core/exception/exception_ptr.hpp>
#include <MSTL/core/exception/exception.hpp>
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 thread_local exception_ptr current_exception_ptr;


exception_ptr current_exception() noexcept {
    return current_exception_ptr;
}

void rethrow_exception(const exception_ptr& p) {
    if (!p) {
        throw_exception(memory_exception());
    }

    if (p.ecb_ && p.ecb_->wrapper) {
        p.ecb_->wrapper->rethrow();
    } else {
        throw_exception(memory_exception());
    }
}

MSTL_BEGIN_INNER__
void set_current_exception(exception_ptr ptr) noexcept {
    current_exception_ptr = _MSTL move(ptr);
}
MSTL_END_INNER__

MSTL_END_NAMESPACE__
