#include <MSTL/core/typeinfo/check_type.hpp>
#ifdef MSTL_COMPILER_GNUC__
#include <MSTL/core/memory/unique_ptr.hpp>
#include <cxxabi.h>
#include <cstdlib>
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_INNER__

string real_symbol_name(string name) {
    auto deleter = [](char* p) { if (p) std::free(p); };
    _MSTL unique_ptr<char, decltype(deleter)> real_name {
        ::abi::__cxa_demangle(name.data(), nullptr, nullptr, nullptr), deleter
    };
   return {real_name.get()};
}

MSTL_END_INNER__
MSTL_END_NAMESPACE__
#endif // MSTL_COMPILER_GNUC__
