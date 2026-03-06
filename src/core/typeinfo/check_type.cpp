#include <NeForce/core/typeinfo/check_type.hpp>
#ifdef NEFORCE_COMPILER_GNUC
#include <NeForce/core/memory/unique_ptr.hpp>
#include <cxxabi.h>
#include <cstdlib>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

string real_symbol_name(string name) {
    auto deleter = [](char* p) { if (p) std::free(p); };
    _NEFORCE unique_ptr<char, decltype(deleter)> real_name {
        ::abi::__cxa_demangle(name.data(), nullptr, nullptr, nullptr), deleter
    };
   return {real_name.get()};
}

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_COMPILER_GNUC
