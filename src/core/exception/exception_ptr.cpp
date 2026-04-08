#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/terminate.hpp>
NEFORCE_BEGIN_NAMESPACE__

exception_ptr current_exception() noexcept {
    if (uncaught_exceptions() == 0) {
        return {};
    }

    try {
        throw;
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
