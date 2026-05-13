#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <exception>
NEFORCE_BEGIN_NAMESPACE__

exception_ptr current_exception() noexcept {
    std::exception_ptr std_ep = std::current_exception();
    if (!std_ep) {
        return {};
    }

    try {
        std::rethrow_exception(std_ep);
    } catch (const exception& e) {
        return make_exception_ptr(e);
    } catch (const std::exception& e) {
        return make_exception_ptr(value_exception(e.what()));
    } catch (...) {
        return make_exception_ptr(value_exception("Unknown exception"));
    }
}

void rethrow_exception(const exception_ptr& p) {
    if (!p || p.ecb_ == nullptr || !p.ecb_->wrapper) {
        terminate();
    }
    p.ecb_->wrapper->rethrow();
}

NEFORCE_END_NAMESPACE__
