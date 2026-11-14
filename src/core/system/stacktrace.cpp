#include <NeForce/core/system/stacktrace.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/system/dynamic_library.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/string/format.hpp>
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
#    include <NeForce/core/string/to_numerics.hpp>
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
    struct symbol_info {
        string name;
        string file;
        size_t line{0};
        bool source_checked{false};
    };

    flat_unordered_map<void*, symbol_info>& symbol_cache() {
        static flat_unordered_map<void*, symbol_info> cache;
        return cache;
    }

    mutex& symbol_cache_mutex() {
        static mutex mtx{};
        return mtx;
    }

    string demangle_symbol(const char* symbol) {
        if (symbol == nullptr) {
            return {};
        }
        int status = 0;
        char* demangled = ::abi::__cxa_demangle(symbol, nullptr, nullptr, &status);
        if (status == 0 && demangled != nullptr) {
            string result(demangled);
            ::free(demangled);
            return result;
        }
        return symbol;
    }

    string shell_quote(const string& value) {
        string quoted("'");
        for (const char c: value) {
            if (c == '\'') {
                quoted.append("'\\''");
            } else {
                quoted.push_back(c);
            }
        }
        quoted.push_back('\'');
        return quoted;
    }

#    if defined(NEFORCE_PLATFORM_LINUX) || defined(NEFORCE_PLATFORM_FREEBSD)
    bool elf_is_position_independent(const string& module_path) {
        const file input(path{module_path}, false, file_access::read, file_shared::share_read_write);
        if (!input.is_opened()) {
            return true;
        }
        unsigned char header[20] = {};
        const auto count = input.read_binary(header, sizeof(header));
        if (count < 18 || header[0] != 0x7F || header[1] != 'E' || header[2] != 'L' || header[3] != 'F') {
            return true;
        }
        const bool little_endian = header[5] == 1;
        const unsigned char low = little_endian ? header[16] : header[17];
        const unsigned char high = little_endian ? header[17] : header[16];
        const auto type = static_cast<uint16_t>(low | (static_cast<uint16_t>(high) << 8U));
        return type == 3;
    }
#    endif

#    if defined(NEFORCE_PLATFORM_LINUX) || defined(NEFORCE_PLATFORM_FREEBSD)
    void resolve_source_location(void* address, symbol_info& info) {
        info.source_checked = true;
        const string module_path = dynamic_library::symbol_location(address);
        if (module_path.empty()) {
            return;
        }
        ::Dl_info dl{};
        const uintptr_t base =
                ::dladdr(address, &dl) != 0 ? reinterpret_cast<uintptr_t>(dl.dli_fbase) : static_cast<uintptr_t>(0);
        const auto runtime = reinterpret_cast<uintptr_t>(address);
        const uintptr_t relative =
                elf_is_position_independent(module_path) && base != 0 && runtime >= base ? runtime - base : runtime;
        const string command = format("addr2line -e {} -f -C 0x{:x} 2>/dev/null", shell_quote(module_path), relative);
        ::FILE* fp = ::popen(command.data(), "r");
        if (fp == nullptr) {
            return;
        }
        char function_buffer[1024] = {};
        char location_buffer[1024] = {};
        if (::fgets(function_buffer, sizeof(function_buffer), fp) != nullptr &&
            ::fgets(location_buffer, sizeof(location_buffer), fp) != nullptr) {
            string function(function_buffer);
            while (!function.empty() && (function.back() == '\n' || function.back() == '\r')) {
                function.pop_back();
            }
            if (info.name.empty() && !function.empty() && function != "??") {
                info.name = move(function);
            }
            string location(location_buffer);
            while (!location.empty() && (location.back() == '\n' || location.back() == '\r')) {
                location.pop_back();
            }
            const size_t colon = location.rfind(':');
            if (colon != string::npos && colon + 1 < location.size()) {
                try {
                    const auto line = to_uint64(location.view().substr(colon + 1));
                    info.file = location.substr(0, colon);
                    info.line = static_cast<size_t>(line);
                } catch (const exception&) {
                    info.file.clear();
                    info.line = 0;
                }
            }
        }
        ::pclose(fp);
    }
#    elif defined(NEFORCE_PLATFORM_MACOS)
    void resolve_source_location(void* address, symbol_info& info) {
        info.source_checked = true;
        const string module_path = dynamic_library::symbol_location(address);
        if (module_path.empty()) {
            return;
        }
        ::Dl_info dl{};
        const uintptr_t base =
                ::dladdr(address, &dl) != 0 ? reinterpret_cast<uintptr_t>(dl.dli_fbase) : static_cast<uintptr_t>(0);
        const auto runtime = reinterpret_cast<uintptr_t>(address);
        const string command =
                base != 0 ? format("atos -o {} -l 0x{:x} 0x{:x} 2>/dev/null", shell_quote(module_path), base, runtime)
                          : format("atos -o {} 0x{:x} 2>/dev/null", shell_quote(module_path), runtime);
        ::FILE* fp = ::popen(command.data(), "r");
        if (fp == nullptr) {
            return;
        }
        char output[2048] = {};
        if (::fgets(output, sizeof(output), fp) != nullptr) {
            string text(output);
            while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
                text.pop_back();
            }
            const size_t in_position = text.find(" (in ");
            if (in_position != string::npos) {
                if (info.name.empty()) {
                    info.name = text.substr(0, in_position);
                }
                const size_t close = text.rfind(')');
                const size_t open = close == string::npos ? string::npos : text.rfind('(', close);
                if (open != string::npos && close != string::npos && open < close) {
                    const string location = text.substr(open + 1, close - open - 1);
                    const size_t colon = location.rfind(':');
                    if (colon != string::npos && colon + 1 < location.size()) {
                        try {
                            const auto line = to_uint64(location.view().substr(colon + 1));
                            info.file = location.substr(0, colon);
                            info.line = static_cast<size_t>(line);
                        } catch (const exception&) {
                            info.file.clear();
                            info.line = 0;
                        }
                    }
                }
            } else if (info.name.empty()) {
                info.name = move(text);
            }
        }
        ::pclose(fp);
    }
#    else
    void resolve_source_location(void*, symbol_info& info) { info.source_checked = true; }
#    endif

    symbol_info get_symbol(void* address, const bool need_source) {
        lock<mutex> guard(symbol_cache_mutex());
        auto& cache = symbol_cache();
        const auto found = cache.find(address);
        symbol_info info = found != cache.end() ? found->second : symbol_info{};
        if (info.name.empty() && !info.source_checked) {
            ::Dl_info dl{};
            if (::dladdr(address, &dl) != 0 && dl.dli_sname != nullptr) {
                info.name = demangle_symbol(dl.dli_sname);
            }
        }
        if ((need_source || info.name.empty()) && !info.source_checked) {
            resolve_source_location(address, info);
        }
        cache[address] = info;
        return info;
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
    const symbol_info info = get_symbol(address_, false);
    if (info.name.empty()) {
        return {"<unknown>"};
    }
    return info.name;
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
    return get_symbol(address_, true).file;
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
    return get_symbol(address_, true).line;
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

    const size_t to_skip = min(static_cast<size_t>(captured), skip + 1);
    const size_t valid_frames = captured - to_skip;

    frames_.reserve(valid_frames);
    for (size_t i = to_skip; compare_less(i, captured) && frames_.size() < max_depth; ++i) {
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
