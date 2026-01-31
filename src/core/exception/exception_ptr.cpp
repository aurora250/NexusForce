#include <MSTL/core/exception/exception.hpp>
#include <MSTL/core/exception/exception_ptr.hpp>
#include <MSTL/core/exception/terminate.hpp>
#include <exception>
MSTL_BEGIN_NAMESPACE__

exception_ptr current_exception() noexcept {
    if (std::uncaught_exceptions() == 0) {
        return exception_ptr();
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
    if (!p || !p.ecb_ || !p.ecb_->wrapper) {
        _MSTL terminate();
    }
    p.ecb_->wrapper->rethrow();
}

MSTL_END_NAMESPACE__
