#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/system/stacktrace.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/async/call_once.hpp>
#include <NeForce/core/async/mutex.hpp>
#include <windef.h>
#include <WinBase.h>
#ifdef NEFORCE_COMPILER_MSVC
#include <verrsrc.h>
#endif
#ifdef NEFORCE_COMPILER_MINGW
#include <winver.h>
#endif
#include <DbgHelp.h>
#else
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <cstdlib>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_WINDOWS

static void ensure_initialized() noexcept {
    static once_flag init_flag{};
    call_once(init_flag, [](){
        ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        if (!::SymInitialize(::GetCurrentProcess(), nullptr, 1)) {}
    });
}

static mutex& dbghelp_mutex() {
    static mutex mtx{};
    return mtx;
}

#endif

string stacktrace::frame::name() const {
    if (!address_) return "<empty>";
#ifdef NEFORCE_PLATFORM_WINDOWS
    ensure_initialized();

    lock<mutex> lock(dbghelp_mutex());

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
        result += " in " + _NEFORCE move(symbol);
    }
    return result;
}

stacktrace::stacktrace(const size_t skip, const size_t max_depth) {
#ifdef NEFORCE_PLATFORM_WINDOWS
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

    const size_t to_skip = _NEFORCE min(static_cast<size_t>(captured), skip + 2);
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
        result += _NEFORCE to_string("#", i, " ", frames_[i]);
        if (i + 1 < frames_.size()) result += "\n";
    }
    return result;
}

NEFORCE_END_NAMESPACE__
