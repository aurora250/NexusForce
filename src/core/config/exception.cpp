#include <MSTL/core/async/atomic.hpp>
#include <MSTL/core/config/exception.hpp>
#include <MSTL/core/memory/exception_ptr.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/utility/stacktrace.hpp>
#include <cstdlib> // std::abort
MSTL_BEGIN_NAMESPACE__

void throw_exception(const exception& err) {
#ifdef MSTL_STATE_DEBUG__
    printcln(color::red(), "\nException : (", err.type, ") ", err.info);
    printcln(color::red(), stacktrace());
#endif
    _INNER capture_exception(err);
    throw err;
}


static atomic<terminate_handler>& get_terminate_handler() noexcept {
    static atomic<terminate_handler> handler{nullptr};
    return handler;
}

void set_terminate(terminate_handler handler) noexcept {
    get_terminate_handler().store(handler, memory_order_release);
}

void terminate() noexcept {
    const auto handler = get_terminate_handler().load(memory_order_acquire);
    if (handler) handler();
    std::abort();
}

MSTL_END_NAMESPACE__
