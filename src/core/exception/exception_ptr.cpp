#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/exception/system_exception.hpp>
#include <exception>
NEFORCE_BEGIN_NAMESPACE__

#define __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(EX) \
    catch (const EX& e) {                       \
        return make_exception_ptr(e);           \
    }

exception_ptr current_exception() noexcept {
    std::exception_ptr std_ep = std::current_exception();
    if (!std_ep) {
        return {};
    }

    try {
        std::rethrow_exception(std_ep);
    }
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(iterator_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(typecast_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(memory_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(device_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(file_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(network_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(system_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(math_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(value_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(database_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(thirdparty_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(exception)
    catch (const std::exception& e) {
        return make_exception_ptr(value_exception(e.what()));
    }
    catch (...) {
        return make_exception_ptr(value_exception("Unknown exception"));
    }
}

#undef __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR


void rethrow_exception(const exception_ptr& p) {
    if (!p || p.ecb_ == nullptr || !p.ecb_->wrapper) {
        terminate();
    }
    p.ecb_->wrapper->rethrow();
    unreachable(); // rethrow not return, help GCC to identify
}

NEFORCE_END_NAMESPACE__
