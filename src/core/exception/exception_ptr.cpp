#include <NeForce/core/exception/debug.hpp>
#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <exception>
NEFORCE_BEGIN_NAMESPACE__

exception_ptr current_exception() noexcept {
    if (std::current_exception() == nullptr) {
        return {};
    }

    try {
        throw;
    } catch (const exception& error) {
        try {
            unique_ptr<exception> owned(error.clone());
            unique_ptr<exception_wrapper> wrapper(new owned_exception_wrapper(_NEFORCE move(owned)));
            unique_ptr<exception_ptr::ecb> block(new exception_ptr::ecb(_NEFORCE move(wrapper)));
            exception_ptr result;
            result.ecb_ = block.release();
            return result;
        } catch (...) {
            return {};
        }
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
    unreachable(); // rethrow not return, help GCC to identify
}

NEFORCE_END_NAMESPACE__
