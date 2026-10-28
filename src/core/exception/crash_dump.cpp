#include <NeForce/core/exception/crash_dump.hpp>
#include <NeForce/core/async/atomic.hpp>
#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/path.hpp>
#include <exception>
#include "crash_dump_native.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <windows.h>
#    include <DbgHelp.h>
#    ifdef NEFORCE_COMPILER_MSVC
#        include <intrin.h>
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <csignal>
#    include <sys/resource.h>
#    include <sys/syscall.h>
#    include <ucontext.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr size_t directory_capacity = 2048;
    constexpr size_t prefix_capacity = 96;
    constexpr size_t summary_capacity = 1024;
    constexpr size_t detail_capacity = 256;
    constexpr size_t default_max_segments = 2048;
    constexpr uint64_t default_max_file_bytes = 2ULL * 1024ULL * 1024ULL * 1024ULL;

    using inner::native_dump_path_capacity;
    using inner::native_dump_request;
    using inner::native_dump_result;
#ifdef NEFORCE_SUPPORT_NATIVE_DUMP
    using inner::write_native_dump;
#endif

    struct dump_setup {
        const char* directory{nullptr};
        const char* prefix{nullptr};
        bool include_memory{false};
        uint32_t max_segments{static_cast<uint32_t>(default_max_segments)};
        uint64_t max_file_bytes{default_max_file_bytes};
        bool kernel_core_fallback{true};
        int64_t utc_offset_seconds{0};
        crash_dump::callback callback{nullptr};
        void* user_data{nullptr};
    };

    struct published_state {
        char directory[directory_capacity]{};
        char prefix[prefix_capacity]{};
        bool include_memory{false};
        uint32_t max_segments{static_cast<uint32_t>(default_max_segments)};
        uint64_t max_file_bytes{default_max_file_bytes};
        bool kernel_core_fallback{true};
        int64_t utc_offset_seconds{0};
        crash_dump::callback callback{nullptr};
        void* user_data{nullptr};
    };

    published_state g_published{};
    atomic<bool> g_installed{false};
    atomic<bool> g_in_crash{false};
    char g_summary[summary_capacity]{};

    terminate_handler g_previous_terminate_handler{nullptr};
    terminate_handler g_previous_neforce_terminate_handler{nullptr};
    bool g_terminate_installed{false};

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter{nullptr};
    bool g_filter_installed{false};
#elif defined(NEFORCE_PLATFORM_LINUX)
    constexpr int32_t fatal_signals[] = {SIGABRT, SIGSEGV, SIGILL, SIGFPE, SIGBUS};
    constexpr size_t fatal_signal_count = sizeof(fatal_signals) / sizeof(fatal_signals[0]);
    struct ::sigaction g_previous_actions[fatal_signal_count]{};
    bool g_signal_installed{false};
    alignas(16) char g_alternate_stack[64 * 1024]{};
#endif

    mutex& install_mutex() noexcept {
        static mutex value{};
        return value;
    }

    size_t write_text(char* buffer, const size_t capacity, const size_t offset, const char* text) noexcept {
        size_t position = offset;
        while (text != nullptr && *text != '\0' && position + 1 < capacity) {
            buffer[position++] = *text++;
        }
        buffer[position] = '\0';
        return position;
    }

    size_t write_unsigned(char* buffer, const size_t capacity, const size_t offset, uint64_t value,
                          const int width) noexcept {
        char digits[24]{};
        int count = 0;
        do {
            digits[count++] = static_cast<char>('0' + value % 10U);
            value /= 10U;
        } while (value != 0U && count < sizeof(digits));
        size_t position = offset;
        for (int padding = count; padding < width && position + 1 < capacity; ++padding) {
            buffer[position++] = '0';
        }
        while (count > 0 && position + 1 < capacity) {
            buffer[position++] = digits[--count];
        }
        buffer[position] = '\0';
        return position;
    }

    size_t write_hex(char* buffer, const size_t capacity, const size_t offset, const uint64_t value,
                     const int digits) noexcept {
        static constexpr char alphabet[] = "0123456789abcdef";
        size_t position = offset;
        for (int index = digits - 1; index >= 0 && position + 1 < capacity; --index) {
            buffer[position++] = alphabet[(value >> (index * 4)) & 0xFU];
        }
        buffer[position] = '\0';
        return position;
    }

    struct text_builder {
        char* buffer{nullptr};
        size_t capacity{0};
        size_t used{0};

        text_builder(char* target, const size_t size) noexcept :
        buffer(target),
        capacity(size) {
            if (buffer != nullptr && capacity > 0) {
                buffer[0] = '\0';
            }
        }

        void append(const char* text) noexcept { used = write_text(buffer, capacity, used, text); }

        void append_unsigned(const uint64_t value) noexcept { used = write_unsigned(buffer, capacity, used, value, 1); }

        void append_signed(const int64_t value) noexcept {
            if (value < 0) {
                append("-");
                append_unsigned(static_cast<uint64_t>(-(value + 1)) + 1U);
            } else {
                append_unsigned(static_cast<uint64_t>(value));
            }
        }

        void append_hex(const uint64_t value, const int digits) noexcept {
            used = write_hex(buffer, capacity, used, value, digits);
        }
    };

    const char* signal_name(const int32_t number) noexcept {
#ifdef NEFORCE_PLATFORM_LINUX
        switch (number) {
            case SIGABRT:
                return "SIGABRT";
            case SIGSEGV:
                return "SIGSEGV";
            case SIGILL:
                return "SIGILL";
            case SIGFPE:
                return "SIGFPE";
            case SIGBUS:
                return "SIGBUS";
            default:
                break;
        }
#else
        static_cast<void>(number);
#endif
        return "unknown";
    }

    uint32_t current_thread_id() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return static_cast<uint32_t>(::GetCurrentThreadId());
#elif defined(NEFORCE_PLATFORM_LINUX)
        return static_cast<uint32_t>(::syscall(SYS_gettid));
#else
        return 0;
#endif
    }

#ifdef NEFORCE_PLATFORM_LINUX
    int64_t local_utc_offset_seconds() noexcept {
        const ::time_t now = ::time(nullptr);
        ::tm local{};
        if (::localtime_r(&now, &local) == nullptr) {
            return 0;
        }
        char zone[16]{};
        if (::strftime(zone, sizeof(zone), "%z", &local) == 0) {
            return 0;
        }
        const bool negative = zone[0] == '-';
        const bool positive = zone[0] == '+';
        if (!negative && !positive) {
            return 0;
        }
        int hours = 0;
        int minutes = 0;
        if (string_length(zone) >= 5) {
            hours = (zone[1] - '0') * 10 + (zone[2] - '0');
            minutes = (zone[3] - '0') * 10 + (zone[4] - '0');
        }
        const int64_t offset = static_cast<int64_t>(hours) * 3600 + static_cast<int64_t>(minutes) * 60;
        return negative ? -offset : offset;
    }
#else
    int64_t local_utc_offset_seconds() noexcept { return 0; }
#endif

    void sanitize_prefix(const string_view source, char* target, const size_t capacity) noexcept {
        size_t used = 0;
        for (const char character: source) {
            const bool allowed = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                                 (character >= '0' && character <= '9') || character == '-' || character == '_';
            if (used + 1 >= capacity) {
                break;
            }
            target[used++] = allowed ? character : '_';
        }
        if (used == 0) {
            used = write_text(target, capacity, 0, "nexusforce");
        }
        target[used] = '\0';
    }

    bool prepare_published_state(const crash_dump::options& options) {
        const path directory = options.directory.empty() ? path::current_path() : options.directory;
        const string_view directory_text = directory.view();
        if (directory_text.size() + 1 >= sizeof(g_published.directory)) {
            return false;
        }
        memory_copy(g_published.directory, directory_text.data(), directory_text.size());
        g_published.directory[directory_text.size()] = '\0';
        filesystem::create_directories(directory);

        sanitize_prefix(options.file_prefix.view(), g_published.prefix, sizeof(g_published.prefix));

        g_published.include_memory = options.include_memory;
        g_published.max_segments = options.max_segments;
        g_published.max_file_bytes = options.max_file_bytes;
        g_published.kernel_core_fallback = options.kernel_core_fallback;
        g_published.utc_offset_seconds = local_utc_offset_seconds();
        g_published.callback = options.callback;
        g_published.user_data = options.user_data;
        return true;
    }

    dump_setup published_setup() noexcept {
        dump_setup setup{};
        setup.directory = g_published.directory[0] != '\0' ? g_published.directory : ".";
        setup.prefix = g_published.prefix[0] != '\0' ? g_published.prefix : "nexusforce";
        setup.include_memory = g_published.include_memory;
        setup.max_segments = g_published.max_segments;
        setup.max_file_bytes = g_published.max_file_bytes;
        setup.kernel_core_fallback = g_published.kernel_core_fallback;
        setup.utc_offset_seconds = g_published.utc_offset_seconds;
        setup.callback = g_published.callback;
        setup.user_data = g_published.user_data;
        return setup;
    }

    int32_t unsupported_error_code() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return static_cast<int32_t>(ERROR_NOT_SUPPORTED);
#else
        return ENOSYS;
#endif
    }

    native_dump_result run_dump(const dump_setup& setup, const crash_dump::reason reason, const int32_t signal_number,
                                const uint32_t exception_code, const void* address, const void* signal_info,
                                const void* context, const char* inner) noexcept {
        native_dump_request request{};
        request.directory = setup.directory;
        request.prefix = setup.prefix;
        request.include_memory = setup.include_memory;
        request.max_segments = setup.max_segments;
        request.max_file_bytes = setup.max_file_bytes;
        request.signal_number = signal_number;
        request.signal_info = signal_info;
        request.context = context;
        request.thread_id = current_thread_id();
        request.utc_offset_seconds = setup.utc_offset_seconds;

        native_dump_result result{};
#ifdef NEFORCE_SUPPORT_NATIVE_DUMP
        write_native_dump(request, result);
#else
        result.error = unsupported_error_code();
#endif

        crash_dump::report report{};
        report.reason = reason;
        report.written = result.written;
        report.truncated = result.truncated;
        report.bytes = result.bytes;
        report.segments = result.segments;
        report.error = result.error;
        report.signal_number = signal_number;
        report.exception_code = exception_code;
        report.address = address;
        report.thread_id = request.thread_id;

        text_builder builder{g_summary, sizeof(g_summary)};
        switch (reason) {
            case crash_dump::reason::fatal_signal:
                builder.append("fatal signal ");
                builder.append_signed(signal_number);
                builder.append(" (");
                builder.append(signal_name(signal_number));
                builder.append(")");
#ifdef NEFORCE_PLATFORM_LINUX
                if (signal_info != nullptr) {
                    const auto* info = static_cast<const siginfo_t*>(signal_info);
                    builder.append(" code=");
                    builder.append_signed(info->si_code);
                }
#endif
                break;
            case crash_dump::reason::structured_exception:
                builder.append("unhandled exception code=0x");
                builder.append_hex(exception_code, 8);
                break;
            case crash_dump::reason::unhandled_exception:
                builder.append("std::terminate");
                break;
            case crash_dump::reason::manual:
            default:
                builder.append("manual dump");
                break;
        }
        if (inner != nullptr && inner[0] != '\0') {
            builder.append(": ");
            builder.append(inner);
        }
        if (address != nullptr) {
            builder.append(" address=0x");
            builder.append_hex(reinterpret_cast<uintptr_t>(address), static_cast<int>(sizeof(void*) * 2));
        }
        builder.append(" thread=");
        builder.append_unsigned(request.thread_id);
        if (result.written) {
            builder.append(" dump=");
            builder.append(result.file);
            builder.append(" bytes=");
            builder.append_unsigned(result.bytes);
            builder.append(" segments=");
            builder.append_unsigned(result.segments);
            if (result.truncated) {
                builder.append(" truncated");
            }
        } else {
            builder.append(" dump-failed error=");
            builder.append_signed(result.error);
        }

        report.file = result.file;
        report.message = g_summary;
        if (setup.callback != nullptr) {
            setup.callback(report, setup.user_data);
        }
        return result;
    }

    void copy_detail(char* target, const size_t capacity, const char* source) noexcept {
        if (source == nullptr) {
            target[0] = '\0';
            return;
        }
        size_t used = 0;
        while (source[used] != '\0' && used + 1 < capacity) {
            target[used] = source[used];
            ++used;
        }
        target[used] = '\0';
    }

    void crash_terminate_handler() {
        if (g_in_crash.exchange(true)) {
#ifdef NEFORCE_PLATFORM_WINDOWS
            immediate_exit(3);
#else
            immediate_exit(128 + SIGABRT);
#endif
        }

        char inner[detail_capacity]{};
        try {
            const auto current = current_exception();
            if (current) {
                rethrow_exception(current);
            }
        } catch (const exception& error) {
            copy_detail(inner, sizeof(inner), error.what());
        } catch (const std::exception& error) {
            copy_detail(inner, sizeof(inner), error.what());
        } catch (...) {
            copy_detail(inner, sizeof(inner), "unknown exception");
        }

        const dump_setup setup = published_setup();
        int32_t signal_number = 0;
        const void* context = nullptr;

#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CONTEXT win_context{};
        ::RtlCaptureContext(&win_context);
        ::EXCEPTION_RECORD record{};
        record.ExceptionCode = 0xE0004E46U;
        record.ExceptionAddress = reinterpret_cast<void*>(_ReturnAddress());
        ::EXCEPTION_POINTERS pointers{};
        pointers.ExceptionRecord = &record;
        pointers.ContextRecord = &win_context;
        context = &win_context;
        run_dump(setup, crash_dump::reason::unhandled_exception, signal_number, record.ExceptionCode,
                 record.ExceptionAddress, &pointers, context, inner);
        immediate_exit(3);
#else
#    if defined(NEFORCE_PLATFORM_LINUX)
        ::ucontext_t signal_context{};
        if (::getcontext(&signal_context) == 0) {
            context = &signal_context;
        }
        signal_number = SIGABRT;
#    endif
        run_dump(setup, crash_dump::reason::unhandled_exception, signal_number, 0, nullptr, nullptr, context, inner);
        immediate_exit(128 + SIGABRT);
#endif
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LONG WINAPI unhandled_exception_filter(::EXCEPTION_POINTERS* info) {
        if (g_in_crash.exchange(true)) {
            return EXCEPTION_CONTINUE_SEARCH;
        }
        const ::DWORD exception_code =
                info != nullptr && info->ExceptionRecord != nullptr ? info->ExceptionRecord->ExceptionCode : 0;
        const void* address =
                info != nullptr && info->ExceptionRecord != nullptr ? info->ExceptionRecord->ExceptionAddress : nullptr;
        const void* context = info != nullptr ? info->ContextRecord : nullptr;
        const dump_setup setup = published_setup();
        static_cast<void>(run_dump(setup, crash_dump::reason::structured_exception, 0, exception_code, address, info,
                                   context, nullptr));
        if (g_previous_filter != nullptr) {
            return g_previous_filter(info);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
#elif defined(NEFORCE_PLATFORM_LINUX)
    void fatal_signal_handler(const int32_t signal_number, siginfo_t* info, void* context) {
        if (g_in_crash.exchange(true)) {
            ::_exit(128 + signal_number);
        }
        const dump_setup setup = published_setup();
        const void* address = info != nullptr ? info->si_addr : nullptr;
        const native_dump_result result =
                run_dump(setup, crash_dump::reason::fatal_signal, signal_number, 0, address, info, context, nullptr);
        if (!result.written && setup.kernel_core_fallback) {
            struct ::sigaction default_action{};
            default_action.sa_handler = SIG_DFL;
            ::sigemptyset(&default_action.sa_mask);
            ::sigaction(signal_number, &default_action, nullptr);
            ::sigset_t unblock{};
            ::sigemptyset(&unblock);
            ::sigaddset(&unblock, signal_number);
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            ::sigprocmask(SIG_UNBLOCK, &unblock, nullptr);
            ::raise(signal_number);
        }
        ::_exit(128 + signal_number);
    }

    bool install_signal_handlers() noexcept {
        stack_t alternative{};
        alternative.ss_sp = g_alternate_stack;
        alternative.ss_size = sizeof(g_alternate_stack);
        alternative.ss_flags = 0;
        if (::sigaltstack(&alternative, nullptr) != 0) {
            return false;
        }

        struct ::sigaction action{};
        action.sa_sigaction = fatal_signal_handler;
        ::sigemptyset(&action.sa_mask);
        ::sigfillset(&action.sa_mask);
        action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;

        bool installed = true;
        for (size_t index = 0; index < fatal_signal_count; ++index) {
            installed = ::sigaction(fatal_signals[index], &action, &g_previous_actions[index]) == 0 && installed;
        }
        g_signal_installed = true;
        return installed;
    }

    void raise_core_limit() noexcept {
        struct ::rlimit limit{};
        if (::getrlimit(RLIMIT_CORE, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY &&
            limit.rlim_cur < limit.rlim_max) {
            limit.rlim_cur = limit.rlim_max;
            static_cast<void>(::setrlimit(RLIMIT_CORE, &limit));
        }
    }
#endif

} // namespace


#ifdef NEFORCE_PLATFORM_WINDOWS
NEFORCE_BEGIN_INNER__

bool write_native_dump(const native_dump_request& request, native_dump_result& result) noexcept {
    result = native_dump_result{};
    if (request.directory == nullptr || request.directory[0] == '\0' || request.prefix == nullptr ||
        request.prefix[0] == '\0') {
        result.error = static_cast<int32_t>(ERROR_INVALID_PARAMETER);
        return false;
    }

    char path[native_dump_path_capacity]{};
    wchar_t wide_path[native_dump_path_capacity]{};

    ::SYSTEMTIME stamp{};
    ::GetLocalTime(&stamp);

    ::HANDLE file = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 32; ++attempt) {
        size_t offset = 0;
        path[0] = '\0';
        offset = write_text(path, sizeof(path), offset, request.directory);
        if (offset > 0 && path[offset - 1] != '\\' && path[offset - 1] != '/') {
            offset = write_text(path, sizeof(path), offset, "\\");
        }
        offset = write_text(path, sizeof(path), offset, request.prefix);
        offset = write_text(path, sizeof(path), offset, "-");
        offset = write_unsigned(path, sizeof(path), offset, stamp.wYear, 4);
        offset = write_unsigned(path, sizeof(path), offset, stamp.wMonth, 2);
        offset = write_unsigned(path, sizeof(path), offset, stamp.wDay, 2);
        offset = write_text(path, sizeof(path), offset, "-");
        offset = write_unsigned(path, sizeof(path), offset, stamp.wHour, 2);
        offset = write_unsigned(path, sizeof(path), offset, stamp.wMinute, 2);
        offset = write_unsigned(path, sizeof(path), offset, stamp.wSecond, 2);
        offset = write_text(path, sizeof(path), offset, "-");
        offset = write_unsigned(path, sizeof(path), offset, static_cast<uint64_t>(::GetCurrentProcessId()), 1);
        if (attempt > 0) {
            offset = write_text(path, sizeof(path), offset, "-");
            offset = write_unsigned(path, sizeof(path), offset, static_cast<uint64_t>(attempt), 1);
        }
        offset = write_text(path, sizeof(path), offset, ".dmp");
        if (offset + 1 >= sizeof(path)) {
            result.error = static_cast<int32_t>(ERROR_BUFFER_OVERFLOW);
            return false;
        }

        if (::MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path, static_cast<int>(sizeof(wide_path))) == 0) {
            result.error = static_cast<int32_t>(::GetLastError());
            return false;
        }
        file = ::CreateFileW(wide_path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            break;
        }
        if (::GetLastError() != ERROR_FILE_EXISTS) {
            result.error = static_cast<int32_t>(::GetLastError());
            return false;
        }
    }
    if (file == INVALID_HANDLE_VALUE) {
        result.error = static_cast<int32_t>(::GetLastError());
        return false;
    }

    ::MINIDUMP_TYPE type =
            static_cast<::MINIDUMP_TYPE>(::MiniDumpNormal | ::MiniDumpWithThreadInfo | ::MiniDumpWithUnloadedModules |
                                         ::MiniDumpWithProcessThreadData);
    if (request.include_memory) {
        type = static_cast<::MINIDUMP_TYPE>(type | ::MiniDumpWithDataSegs | ::MiniDumpWithIndirectlyReferencedMemory |
                                            ::MiniDumpScanMemory);
    }

    ::MINIDUMP_EXCEPTION_INFORMATION exception_information{};
    ::PMINIDUMP_EXCEPTION_INFORMATION exception_pointer = nullptr;
    if (request.signal_info != nullptr) {
        exception_information.ThreadId = static_cast<::DWORD>(request.thread_id);
        exception_information.ExceptionPointers =
                const_cast<::EXCEPTION_POINTERS*>(static_cast<const ::EXCEPTION_POINTERS*>(request.signal_info));
        exception_information.ClientPointers = FALSE;
        exception_pointer = &exception_information;
    }

    const ::BOOL written = ::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), file, type,
                                               exception_pointer, nullptr, nullptr);
    if (written == FALSE) {
        result.error = static_cast<int32_t>(::GetLastError());
        result.file[0] = '\0';
    } else {
        ::LARGE_INTEGER size{};
        if (::GetFileSizeEx(file, &size) != FALSE) {
            result.bytes = static_cast<uint64_t>(size.QuadPart);
        }
        result.written = true;
        copy_detail(result.file, sizeof(result.file), path);
    }
    ::CloseHandle(file);
    return result.written;
}

NEFORCE_END_INNER__
#endif


string to_string(const crash_dump::format format) {
    switch (format) {
        case crash_dump::format::minidump:
            return {"minidump"};
        case crash_dump::format::elf_core:
            return {"elf-core"};
        case crash_dump::format::automatic:
        default:
            return {"automatic"};
    }
}

string to_string(const crash_dump::reason reason) {
    switch (reason) {
        case crash_dump::reason::fatal_signal:
            return {"fatal-signal"};
        case crash_dump::reason::unhandled_exception:
            return {"unhandled-exception"};
        case crash_dump::reason::structured_exception:
            return {"structured-exception"};
        case crash_dump::reason::manual:
        default:
            return {"manual"};
    }
}

crash_dump::format crash_dump::native_format() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return crash_dump::format::minidump;
#else
    return crash_dump::format::elf_core;
#endif
}

bool crash_dump::native_supported() noexcept {
#ifdef NEFORCE_SUPPORT_NATIVE_DUMP
    return true;
#else
    return false;
#endif
}

bool crash_dump::install_handlers(const crash_dump::options& options) noexcept {
    try {
        lock<mutex> guard(install_mutex());
        if (!prepare_published_state(options)) {
            return false;
        }
        g_installed.store(true, memory_order_release);

        bool installed = false;
        if (options.install_terminate_handler) {
            if (!g_terminate_installed) {
                g_previous_terminate_handler = std::set_terminate(crash_terminate_handler);
                g_previous_neforce_terminate_handler = get_terminate();
                g_terminate_installed = true;
            }
            std::set_terminate(crash_terminate_handler);
            set_terminate(crash_terminate_handler);
            installed = true;
        }

#ifdef NEFORCE_PLATFORM_WINDOWS
        if (options.install_exception_filter) {
            if (!g_filter_installed) {
                g_previous_filter = ::SetUnhandledExceptionFilter(unhandled_exception_filter);
                g_filter_installed = true;
            } else {
                ::SetUnhandledExceptionFilter(unhandled_exception_filter);
            }
            installed = true;
        }
#elif defined(NEFORCE_PLATFORM_LINUX)
        if (options.install_signal_handlers) {
            if (options.kernel_core_fallback) {
                raise_core_limit();
            }
            installed = install_signal_handlers() || installed;
        }
#else
        static_cast<void>(options.install_signal_handlers);
        static_cast<void>(options.install_exception_filter);
        static_cast<void>(options.kernel_core_fallback);
#endif
        return installed;
    } catch (...) {
        return false;
    }
}

void crash_dump::uninstall_handlers() noexcept {
    try {
        lock<mutex> guard(install_mutex());
        if (g_terminate_installed) {
            std::set_terminate(g_previous_terminate_handler);
            set_terminate(g_previous_neforce_terminate_handler);
            g_terminate_installed = false;
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (g_filter_installed) {
            ::SetUnhandledExceptionFilter(g_previous_filter);
            g_filter_installed = false;
        }
#elif defined(NEFORCE_PLATFORM_LINUX)
        if (g_signal_installed) {
            for (size_t index = 0; index < fatal_signal_count; ++index) {
                static_cast<void>(::sigaction(fatal_signals[index], &g_previous_actions[index], nullptr));
            }
            ::stack_t disable{};
            disable.ss_flags = SS_DISABLE;
            static_cast<void>(::sigaltstack(&disable, nullptr));
            g_signal_installed = false;
        }
#endif
        g_installed.store(false, memory_order_release);
        g_published = published_state{};
    } catch (...) {
        g_installed.store(false, memory_order_release);
    }
}

bool crash_dump::handlers_installed() noexcept { return g_installed.load(memory_order_acquire); }

crash_dump::result crash_dump::write(const crash_dump::options& options) noexcept {
    thread_local char prefix_buffer[prefix_capacity]{};

    crash_dump::result busy{};
    busy.format = crash_dump::native_format();
    if (g_in_crash.exchange(true)) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        busy.error = static_cast<int32_t>(ERROR_BUSY);
#else
        busy.error = EBUSY;
#endif
        return busy;
    }

    try {
        dump_setup setup{};
        setup.directory = options.directory.empty() ? "." : options.directory.str().view().data();
        sanitize_prefix(options.file_prefix.view(), prefix_buffer, sizeof(prefix_buffer));
        setup.prefix = prefix_buffer;
        setup.include_memory = options.include_memory;
        setup.max_segments = options.max_segments;
        setup.max_file_bytes = options.max_file_bytes;
        setup.kernel_core_fallback = options.kernel_core_fallback;
        setup.utc_offset_seconds = local_utc_offset_seconds();
        setup.callback = options.callback;
        setup.user_data = options.user_data;

        const void* context = nullptr;
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CONTEXT win_context{};
        ::RtlCaptureContext(&win_context);
        context = &win_context;
#elif defined(NEFORCE_PLATFORM_LINUX)
        ::ucontext_t signal_context{};
        if (::getcontext(&signal_context) == 0) {
            context = &signal_context;
        }
#endif

        const native_dump_result native =
                run_dump(setup, crash_dump::reason::manual, 0, 0, nullptr, nullptr, context, nullptr);

        crash_dump::result result{};
        result.written = native.written;
        result.truncated = native.truncated;
        result.format = crash_dump::native_format();
        result.bytes = native.bytes;
        result.segments = native.segments;
        result.error = native.error;
        if (native.file[0] != '\0') {
            result.file = path(string(native.file));
        }
        g_in_crash.store(false, memory_order_release);
        return result;
    } catch (...) {
        g_in_crash.store(false, memory_order_release);
#ifdef NEFORCE_PLATFORM_WINDOWS
        busy.error = static_cast<int32_t>(ERROR_NOT_ENOUGH_MEMORY);
#else
        busy.error = ENOMEM;
#endif
        return busy;
    }
}

NEFORCE_END_NAMESPACE__
