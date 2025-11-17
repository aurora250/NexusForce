#include <MSTL/core/utilities/console.hpp>
#include <MSTL/core/utilities/exception.hpp>
#include <atomic>
#include <cstdlib> // std::abort
MSTL_BEGIN_NAMESPACE__

void Exception(const Error& err) {
    printcln(color::red(), "\nException : (", err.type_, ") ", err.info_);
    throw err;
}


static std::atomic<terminate_handler>& get_terminate_handler() noexcept {
    static std::atomic<terminate_handler> handler{nullptr};
    return handler;
}

void set_terminate(terminate_handler handler) noexcept {
    get_terminate_handler().store(handler, std::memory_order_release);
}

void terminate() noexcept {
    const auto handler = get_terminate_handler().load(std::memory_order_acquire);
    if (handler) handler();
    std::abort();
}

MSTL_END_NAMESPACE__
