#include <NeForce/core/system/stacktrace.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/async/mutex.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/async/call_once.hpp>
#    include <windef.h>
#    include <WinBase.h>
#    ifdef NEFORCE_COMPILER_MSVC
#        include <verrsrc.h>
#    endif
#    ifdef NEFORCE_COMPILER_MINGW
#        include <winver.h>
#    endif
#    include <DbgHelp.h>
#else
#    include <cstdio>
#    include <cstdlib>
#    include <cxxabi.h>
#    include <dlfcn.h>
#    include <execinfo.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    void ensure_initialized() noexcept {
        static once_flag init_flag{};
        call_once(init_flag, []() {
            ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
            if (!::SymInitialize(::GetCurrentProcess(), nullptr, 1)) {
            }
        });
    }
    mutex& dbghelp_mutex() {
        static mutex mtx{};
        return mtx;
    }
#else
    mutex& resolve_mutex() {
        static mutex mtx{};
        return mtx;
    }

    struct source_location {
        string file;
        size_t line{0};
    };

    flat_unordered_map<void*, source_location>& source_cache() {
        static flat_unordered_map<void*, source_location> cache;
        return cache;
    }

    mutex& source_cache_mutex() {
        static mutex mtx{};
        return mtx;
    }

    source_location resolve_source_location(void* addr) {
        ::Dl_info info;
        if (::dladdr(addr, &info) == 0 || info.dli_fname == nullptr) {
            return {};
        }

        char cmd[1024];
        const int written = ::snprintf(cmd, sizeof(cmd), "addr2line -e '%s' -f -C 0x%zx 2>/dev/null", info.dli_fname,
                                       reinterpret_cast<size_t>(addr));
        if (written < 0 || static_cast<size_t>(written) >= sizeof(cmd)) {
            return {};
        }

        FILE* fp = ::popen(cmd, "r");
        if (fp == nullptr) {
            return {};
        }

        source_location loc;
        char func_buf[1024];
        char file_buf[1024];
        // addr2line -f -C outputs two lines: function name, then source file:line
        if (::fgets(func_buf, sizeof(func_buf), fp) != nullptr && ::fgets(file_buf, sizeof(file_buf), fp) != nullptr) {
            string file_line(file_buf);
            while (!file_line.empty() && (file_line.back() == '\n' || file_line.back() == '\r')) {
                file_line.pop_back();
            }
            if (!file_line.empty() && file_line != "??:0") {
                const size_t colon = file_line.rfind(':');
                if (colon != string::npos && colon + 1 < file_line.size()) {
                    loc.file = file_line.substr(0, colon);
                    loc.line = static_cast<size_t>(::strtoul(file_line.data() + colon + 1, nullptr, 10));
                }
            }
        }
        ::pclose(fp);
        return loc;
    }

    source_location get_cached_source_location(void* addr) {
        {
            lock<mutex> lk(source_cache_mutex());
            auto& cache = source_cache();
            const auto it = cache.find(addr);
            if (it != cache.end()) {
                return it->second;
            }
        }

        source_location loc = resolve_source_location(addr);

        {
            lock<mutex> lk(source_cache_mutex());
            auto& cache = source_cache();
            cache[addr] = loc;
        }
        return loc;
    }
#endif
} // namespace


string stacktrace::frame::name() const {
    if (address_ == nullptr) {
        return "<empty>";
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ensure_initialized();

    lock<mutex> lock(dbghelp_mutex());

    char buffer[sizeof(::SYMBOL_INFO) + MAX_SYM_NAME * sizeof(::TCHAR)];
    auto* const symbol = reinterpret_cast<::PSYMBOL_INFO>(buffer);
    symbol->SizeOfStruct = sizeof(::SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    ::DWORD64 displacement = 0;
    if (::SymFromAddr(::GetCurrentProcess(), reinterpret_cast<::DWORD64>(address_), &displacement, symbol) == TRUE) {
        return {symbol->Name};
    }
    return "<unknown>";
#else
    lock<mutex> lock(resolve_mutex());
    ::Dl_info info;
    if (::dladdr(address_, &info) != 0 && info.dli_sname != nullptr) {
        int status = 0;
        char* demangled = ::abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
        if (status == 0 && demangled != nullptr) {
            string res(demangled);
            std::free(demangled);
            return res;
        }
        return {info.dli_sname};
    }
    return {"<unknown>"};
#endif
}

string stacktrace::frame::source_file() const {
    if (address_ == nullptr) {
        return "";
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ensure_initialized();
    lock<mutex> lock(dbghelp_mutex());

    ::IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    ::DWORD displacement = 0;
    if (::SymGetLineFromAddr64(::GetCurrentProcess(), reinterpret_cast<::DWORD64>(address_), &displacement, &line) ==
        TRUE) {
        return {line.FileName};
    }
#else
    const auto loc = get_cached_source_location(address_);
    return loc.file;
#endif
    return "";
}

size_t stacktrace::frame::source_line() const {
    if (address_ == nullptr) {
        return 0;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ensure_initialized();
    lock<mutex> lock(dbghelp_mutex());

    ::IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    ::DWORD displacement = 0;
    if (::SymGetLineFromAddr64(::GetCurrentProcess(), reinterpret_cast<::DWORD64>(address_), &displacement, &line) ==
        TRUE) {
        return static_cast<size_t>(line.LineNumber);
    }
#else
    const auto loc = get_cached_source_location(address_);
    return loc.line;
#endif
    return 0;
}

string stacktrace::frame::to_string() const {
    if (address_ == nullptr) {
        return "<empty>";
    }
    string result = address_string(address_);
    string symbol = name();
    if (!symbol.empty() && symbol != "<unknown>") {
        result += " in " + move(symbol);
    }
    return result;
}

stacktrace stacktrace::current(const size_t skip, const size_t max_depth) { return stacktrace(skip + 1, max_depth); }

stacktrace::stacktrace(const size_t skip, const size_t max_depth) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    vector<void*> buffer(max_depth);
    const ::USHORT captured = ::CaptureStackBackTrace(static_cast<::DWORD>(skip + 2), static_cast<::DWORD>(max_depth),
                                                      buffer.data(), nullptr);
    frames_.reserve(captured);
    for (::USHORT i = 0; i < captured; ++i) {
        frames_.emplace_back(buffer[i]);
    }
#else
    vector<void*> buffer(max_depth + skip + 2);
    const int captured = ::backtrace(buffer.data(), static_cast<int>(buffer.size()));
    if (captured <= 0) {
        return;
    }

    const size_t to_skip = min(static_cast<size_t>(captured), skip + 2);
    const size_t valid_frames = captured - to_skip;

    frames_.reserve(valid_frames);
    for (size_t i = to_skip; i < static_cast<size_t>(captured); ++i) {
        frames_.emplace_back(buffer[i]);
    }
#endif
}

string stacktrace::to_string() const { return to_string(FMT_DEFAULT); }

string stacktrace::to_string(const format_flags flags) const {
    string result;
    for (size_t i = 0; i < frames_.size(); ++i) {
        const auto& f = frames_[i];
        result += "#" + _NEFORCE to_string(i) + " ";
        if ((flags & FMT_NO_ADDRESS) == 0) {
            result += address_string(f.address());
            result += " ";
        }
        const string sym_name = f.name();
        if (!sym_name.empty() && sym_name != "<unknown>") {
            result += "in " + sym_name;
        }
        if ((flags & FMT_SHOW_SOURCE) != 0) {
            const string src = f.source_file();
            if (!src.empty()) {
                result += " at " + src;
                const size_t line = f.source_line();
                if (line > 0) {
                    result += ":" + _NEFORCE to_string(line);
                }
            }
        }
        if (i + 1 < frames_.size()) {
            result += "\n";
        }
    }
    return result;
}

stacktrace stacktrace::from_exception(const exception_ptr& ep, const size_t max_depth) {
    ignore = ep;
    return current(1, max_depth);
}

NEFORCE_END_NAMESPACE__
