#include <MSTL/core/exception/exception.hpp>
#include <MSTL/core/async/atomic.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/system/stacktrace.hpp>
#include <cstdlib> // std::abort
MSTL_BEGIN_NAMESPACE__

void throw_with_stack(const exception& err) {
    printcln(color::red(), "\nException : (", err.type(), ") ", err.what());
    printcln(color::red(), stacktrace());
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
