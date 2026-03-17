#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/stacktrace.hpp>
NEFORCE_BEGIN_NAMESPACE__

void throw_with_stack(const exception& err) {
    printcln(color::red(), "\nException : (", err.type(), ") ", err.what());
    printcln(color::red(), stacktrace());
    throw err;
}

NEFORCE_END_NAMESPACE__
