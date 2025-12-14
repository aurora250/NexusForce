#include <MSTL/core/system/stacktrace.hpp>
#include <MSTL/core/utility/packages.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
#include <MSTL/core/config/undef_cmacro.hpp>
#include <MSTL/core/async/call_once.hpp>
#else
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <cstdlib>
#endif
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_PLATFORM_WINDOWS__

void stacktrace::frame::ensure_initialized() {
    static once_flag init_flag;
    call_once(init_flag, [](){
        ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        if (!::SymInitialize(::GetCurrentProcess(), nullptr, 1)) {

        }
    });
}

mutex& stacktrace::frame::dbghelp_mutex() {
    static mutex mtx;
    return mtx;
}

#endif

string stacktrace::frame::name() const {
    if (!address_) return "<empty>";
#ifdef MSTL_PLATFORM_WINDOWS__
    ensure_initialized();

    lock_guard<mutex> lock(dbghelp_mutex());

    char buffer[sizeof(::SYMBOL_INFO) + MAX_SYM_NAME * sizeof(::TCHAR)];
    const auto symbol = reinterpret_cast<::PSYMBOL_INFO>(buffer);
    symbol->SizeOfStruct = sizeof(::SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    ::DWORD64 displacement = 0;
    if (::SymFromAddr(::GetCurrentProcess(), reinterpret_cast<::DWORD64>(address_), &displacement, symbol)) {
        return string(symbol->Name);
    }
    return "<unknown>";
#else
    ::Dl_info info;
    if (::dladdr(address_, &info) && info.dli_sname) {
        int status = 0;
        char* demangled = ::abi::__cxa_demangle(info.dli_sname,
            nullptr, nullptr, &status);
        if (status == 0 && demangled) {
            string res(demangled);
            std::free(demangled);
            return res;
        }
        return {info.dli_sname};
    }
    return {"<unknown>"};
#endif
}

string stacktrace::frame::to_string() const {
    if (!address_) return "<empty>";
    string result = address_string(address_);
    const string symbol = name();
    if (!symbol.empty() && symbol != "<unknown>") {
        result += " in " + _MSTL move(symbol);
    }
    return result;
}

stacktrace::stacktrace(const size_t skip, const size_t max_depth) {
#ifdef MSTL_PLATFORM_WINDOWS__
    vector<void *> buffer(max_depth);
    const ::USHORT captured = ::CaptureStackBackTrace(
        static_cast<::DWORD>(skip + 2), static_cast<::DWORD>(max_depth),
        buffer.data(), nullptr);
    frames_.reserve(captured);
    for (::USHORT i = 0; i < captured; ++i) {
        frames_.emplace_back(buffer[i]);
    }
#else
    vector<void*> buffer(max_depth + skip + 2);
    const int captured = ::backtrace(buffer.data(), static_cast<int>(buffer.size()));
    if (captured <= 0) return;

    const size_t to_skip = _MSTL min(static_cast<size_t>(captured), skip + 2);
    const size_t valid_frames = captured - to_skip;

    frames_.reserve(valid_frames);
    for (size_t i = to_skip; i < static_cast<size_t>(captured); ++i) {
        frames_.emplace_back(buffer[i]);
    }
#endif
}

string stacktrace::to_string() const {
    string result;
    for (size_t i = 0; i < frames_.size(); ++i) {
        result += _MSTL to_string("#", i, " ", frames_[i]);
        if (i + 1 < frames_.size()) result += "\n";
    }
    return result;
}

MSTL_END_NAMESPACE__
