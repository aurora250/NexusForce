#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/string/to_numerics.hpp>
#include <NeForce/core/system/environment.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <NeForce/core/config/windef.hpp>
#    include <NeForce/core/exception/error_code.hpp>
#    include <NeForce/core/string/utf.hpp>
#    include <memoryapi.h>
#    include <windef.h>
#    include <WinBase.h>
#    include <WinUser.h>
#    include <Psapi.h>
#    include <shellapi.h>
#    include <securitybaseapi.h>
#    include <processthreadsapi.h>
#    include <TlHelp32.h>
#    include <winternl.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <NeForce/core/file/file.hpp>
#    include <NeForce/core/system/console.hpp>
#    include <cerrno>
#    include <csignal>
#    include <ctime>
#    include <pwd.h>
#    include <sched.h>
#    include <sys/resource.h>
#    include <sys/select.h>
#    include <sys/syscall.h>
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS

    string escape_arg_runas(const string& arg) {
        if (arg.find_first_of(" \t\"") == string::npos) {
            return arg;
        }
        string escaped = "\"";
        for (const char c: arg) {
            if (c == '\"') {
                escaped += "\\\"";
            } else if (c == '\\') {
                escaped += "\\\\";
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }

    string build_params_string(const vector<string>& args) {
        string params;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) {
                params += ' ';
            }
            params += escape_arg_runas(args[i]);
        }
        return params;
    }

    constexpr uint64_t filetime_epoch_difference = 116444736000000000ULL;

    uint64_t filetime_ticks(const ::FILETIME& value) noexcept {
        ::ULARGE_INTEGER ticks{};
        ticks.LowPart = value.dwLowDateTime;
        ticks.HighPart = value.dwHighDateTime;
        return ticks.QuadPart;
    }

    uint64_t filetime_to_unix_ms(const ::FILETIME& value) noexcept {
        const uint64_t ticks = filetime_ticks(value);
        return ticks < filetime_epoch_difference ? 0 : (ticks - filetime_epoch_difference) / 10000;
    }

    uint64_t filetime_span_ms(const ::FILETIME& value) noexcept { return filetime_ticks(value) / 10000; }

    uint64_t unix_time_ms() noexcept {
        ::FILETIME now{};
        ::GetSystemTimeAsFileTime(&now);
        return filetime_to_unix_ms(now);
    }

    string query_process_command_line(const ::HANDLE process_handle) {
        if (process_handle == nullptr) {
            return {};
        }

        struct nt_unicode_string {
            uint16_t length;
            uint16_t maximum_length;
            wchar_t* buffer;
        };
        using nt_query_information_process_fn =
                LONG(WINAPI*)(void*, unsigned long, void*, unsigned long, unsigned long*);

        const ::HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
        if (ntdll == nullptr) {
            return {};
        }
        const auto query =
                reinterpret_cast<nt_query_information_process_fn>(::GetProcAddress(ntdll, "NtQueryInformationProcess"));
        if (query == nullptr) {
            return {};
        }

        constexpr unsigned long process_command_line_information = 60;
        constexpr LONG status_info_length_mismatch = static_cast<LONG>(0xC0000004);

        // The first query reports the required buffer size in ReturnLength; the string is copied
        // inline after the UNICODE_STRING header, so the returned buffer must be that large.
        nt_unicode_string command_line{};
        unsigned long needed = 0;
        LONG status = query(process_handle, process_command_line_information, &command_line,
                            static_cast<unsigned long>(sizeof(command_line)), &needed);
        if (status >= 0 && command_line.buffer != nullptr && command_line.length != 0) {
            return wcharacter::to_string(
                    basic_string_view<wchar_t>(command_line.buffer, command_line.length / sizeof(wchar_t)));
        }
        if (status != status_info_length_mismatch || needed <= sizeof(command_line)) {
            return {};
        }

        vector<::BYTE> buffer(needed);
        unsigned long returned = 0;
        status = query(process_handle, process_command_line_information, buffer.data(),
                       static_cast<unsigned long>(buffer.size()), &returned);
        if (status < 0) {
            return {};
        }
        const auto* info = reinterpret_cast<const nt_unicode_string*>(buffer.data());
        if (info->buffer == nullptr || info->length == 0) {
            return {};
        }
        return wcharacter::to_string(basic_string_view<wchar_t>(info->buffer, info->length / sizeof(wchar_t)));
    }

    string user_name_of_handle(const ::HANDLE process_handle) {
        if (process_handle == nullptr) {
            return {};
        }
        ::HANDLE token = nullptr;
        if (::OpenProcessToken(process_handle, TOKEN_QUERY, &token) == FALSE) {
            return {};
        }
        ::DWORD size = 0;
        ::GetTokenInformation(token, ::TokenUser, nullptr, 0, &size);
        if (size == 0) {
            ::CloseHandle(token);
            return {};
        }
        vector<::BYTE> buffer(size);
        if (::GetTokenInformation(token, ::TokenUser, buffer.data(), size, &size) == FALSE) {
            ::CloseHandle(token);
            return {};
        }
        ::CloseHandle(token);

        const auto* user = reinterpret_cast<const ::TOKEN_USER*>(buffer.data());
        wchar_t account[256] = {};
        wchar_t domain[256] = {};
        auto account_length = static_cast<::DWORD>(sizeof(account) / sizeof(wchar_t));
        auto domain_length = static_cast<::DWORD>(sizeof(domain) / sizeof(wchar_t));
        ::SID_NAME_USE use = ::SidTypeUnknown;
        if (::LookupAccountSidW(nullptr, user->User.Sid, account, &account_length, domain, &domain_length, &use) ==
            FALSE) {
            return {};
        }
        const string account_text = wcharacter::to_string(account);
        const string domain_text = wcharacter::to_string(domain);
        return domain_text.empty() ? account_text : domain_text + "\\" + account_text;
    }

    struct system_process_basic_info {
        ::ULONG next_entry_offset;
        ::ULONG number_of_threads;
        ::LARGE_INTEGER working_set_private_size;
        ::ULONG hard_fault_count;
        ::ULONG number_of_threads_high_watermark;
        ::ULONGLONG cycle_time;
        ::LARGE_INTEGER create_time;
        ::LARGE_INTEGER user_time;
        ::LARGE_INTEGER kernel_time;
        ::UNICODE_STRING image_name;
        ::LONG base_priority;
        ::HANDLE unique_process_id;
        ::HANDLE inherited_from_unique_process_id;
        ::ULONG handle_count;
        ::ULONG session_id;
        ::ULONG_PTR unique_process_key;
        ::SIZE_T peak_virtual_size;
        ::SIZE_T virtual_size;
    };

    struct system_process_facts {
        uint64_t virtual_size{0};
        uint32_t threads{0};
        uint64_t create_time_ms{0};
    };

    map<uint32_t, system_process_facts> system_process_facts_of_processes() {
        // SystemProcessInformation == 5, resolved dynamically to avoid linking ntdll.
        constexpr ::ULONG system_process_information = 5;
        constexpr auto status_info_length_mismatch = static_cast<::LONG>(0xC0000004);
        map<uint32_t, system_process_facts> facts;
        const ::HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
        if (ntdll == nullptr) {
            return facts;
        }
        using nt_query_system_information_fn = ::LONG(WINAPI*)(::ULONG, void*, ::ULONG, ::PULONG);
        const auto query =
                reinterpret_cast<nt_query_system_information_fn>(::GetProcAddress(ntdll, "NtQuerySystemInformation"));
        if (query == nullptr) {
            return facts;
        }
        ::ULONG size = 0;
        ::LONG status = query(system_process_information, nullptr, 0, &size);
        for (int attempt = 0; attempt < 4 && size > 0; ++attempt) {
            vector<::BYTE> buffer(size + 65536);
            ::ULONG returned = 0;
            status = query(system_process_information, buffer.data(), static_cast<::ULONG>(buffer.size()), &returned);
            if (status == status_info_length_mismatch) {
                size = returned;
                continue;
            }
            if (status < 0) {
                break;
            }
            const ::BYTE* cursor = buffer.data();
            const ::BYTE* end = buffer.data() + (returned != 0 ? returned : buffer.size());
            while (cursor + sizeof(system_process_basic_info) <= end) {
                const auto* entry = reinterpret_cast<const system_process_basic_info*>(cursor);
                const auto process_id = static_cast<uint32_t>(reinterpret_cast<::ULONG_PTR>(entry->unique_process_id));
                if (process_id != 0) {
                    system_process_facts fact;
                    fact.virtual_size = static_cast<uint64_t>(entry->virtual_size);
                    fact.threads = static_cast<uint32_t>(entry->number_of_threads);
                    const auto ticks = static_cast<uint64_t>(entry->create_time.QuadPart);
                    if (ticks != 0) {
                        ::FILETIME create_filetime{};
                        create_filetime.dwLowDateTime = static_cast<::DWORD>(ticks & 0xFFFFFFFFU);
                        create_filetime.dwHighDateTime = static_cast<::DWORD>((ticks >> 32) & 0xFFFFFFFFU);
                        fact.create_time_ms = filetime_to_unix_ms(create_filetime);
                    }
                    facts[process_id] = fact;
                }
                if (entry->next_entry_offset == 0) {
                    break;
                }
                if (cursor + entry->next_entry_offset > end) {
                    break;
                }
                cursor += entry->next_entry_offset;
            }
            break;
        }
        return facts;
    }

    process::io_info io_info_of_handle(const ::HANDLE process_handle) {
        process::io_info info;
        if (process_handle == nullptr) {
            return info;
        }
        ::IO_COUNTERS counters{};
        if (::GetProcessIoCounters(process_handle, &counters) == FALSE) {
            return info;
        }
        info.read_bytes = counters.ReadTransferCount;
        info.write_bytes = counters.WriteTransferCount;
        info.read_operations = counters.ReadOperationCount;
        info.write_operations = counters.WriteOperationCount;
        info.other_bytes = counters.OtherTransferCount;
        return info;
    }

    bool priority_from_class(const ::DWORD priority_class, process::priority& level) noexcept {
        switch (priority_class) {
            case IDLE_PRIORITY_CLASS:
                level = process::priority::idle;
                return true;
            case BELOW_NORMAL_PRIORITY_CLASS:
                level = process::priority::below_normal;
                return true;
            case NORMAL_PRIORITY_CLASS:
                level = process::priority::normal;
                return true;
            case ABOVE_NORMAL_PRIORITY_CLASS:
                level = process::priority::normal_above;
                return true;
            case HIGH_PRIORITY_CLASS:
                level = process::priority::high;
                return true;
            case REALTIME_PRIORITY_CLASS:
                level = process::priority::realtime;
                return true;
            default:
                return false;
        }
    }

    ::DWORD class_for_priority(const process::priority level) noexcept {
        switch (level) {
            case process::priority::idle:
                return IDLE_PRIORITY_CLASS;
            case process::priority::below_normal:
                return BELOW_NORMAL_PRIORITY_CLASS;
            case process::priority::normal:
                return NORMAL_PRIORITY_CLASS;
            case process::priority::normal_above:
                return ABOVE_NORMAL_PRIORITY_CLASS;
            case process::priority::high:
                return HIGH_PRIORITY_CLASS;
            case process::priority::realtime:
                return REALTIME_PRIORITY_CLASS;
            default:
                return 0;
        }
    }

    string thread_name_of_handle(const ::HANDLE thread_handle) {
        using get_thread_description_fn = ::HRESULT(WINAPI*)(::HANDLE, ::PWSTR*);
        const ::HMODULE kernel32 = ::GetModuleHandleW(L"kernel32.dll");
        if (kernel32 == nullptr) {
            return {};
        }
        const auto get_description =
                reinterpret_cast<get_thread_description_fn>(::GetProcAddress(kernel32, "GetThreadDescription"));
        if (get_description == nullptr) {
            return {};
        }
        ::PWSTR description = nullptr;
        if (get_description(thread_handle, &description) < 0 || description == nullptr) {
            return {};
        }
        string result = wcharacter::to_string(description);
        ::LocalFree(description);
        return result;
    }

    size_t virtual_size_of(const ::HANDLE process_handle) noexcept {
        size_t total = 0;
        const unsigned char* address = nullptr;
        ::MEMORY_BASIC_INFORMATION info{};
        while (::VirtualQueryEx(process_handle, address, &info, sizeof(info)) == sizeof(info)) {
            if (info.State != MEM_FREE) {
                total += static_cast<size_t>(info.RegionSize);
            }
            const auto* next = static_cast<const unsigned char*>(info.BaseAddress) + info.RegionSize;
            if (next <= address) {
                break;
            }
            address = next;
        }
        return total;
    }

#endif

#ifdef NEFORCE_PLATFORM_POSIX
    char** build_argv(const string& executable, const vector<string>& args) {
        const size_t argc = args.size() + 2;
        auto* const argv = new char*[argc];

        argv[0] = new char[executable.length() + 1];
        string_copy(argv[0], executable.data());

        for (size_t i = 0; i < args.size(); ++i) {
            argv[i + 1] = new char[args[i].length() + 1];
            string_copy(argv[i + 1], args[i].data());
        }

        argv[argc - 1] = nullptr;
        return argv;
    }

    void free_argv(char** argv) noexcept {
        if (argv != nullptr) {
            for (int i = 0; argv[i] != nullptr; ++i) {
                delete[] argv[i];
            }
            delete[] argv;
        }
    }

    char** build_envp(const flat_unordered_map<string, string>& env_map) {
        auto** const new_env = new char*[env_map.size() + 1];
        int idx = 0;
        for (const auto& entry: env_map) {
            const string full = entry.first + "=" + entry.second;
            auto* str = new char[full.length() + 1];
            string_copy(str, full.data());
            new_env[idx++] = str;
        }
        new_env[idx] = nullptr;
        return new_env;
    }

    void free_envp(char** envp) noexcept {
        if (envp != nullptr) {
            for (int i = 0; envp[i] != nullptr; ++i) {
                delete[] envp[i];
            }
            delete[] envp;
        }
    }

    uint32_t process_page_size() noexcept {
        static const uint32_t cached = []() -> uint32_t {
            const long ps = ::sysconf(_SC_PAGESIZE);
            return ps > 0 ? static_cast<uint32_t>(ps) : 4096;
        }();
        return cached;
    }

    bool has_pkexec() noexcept { return ::access("/usr/bin/pkexec", X_OK) == 0; }

    struct linux_proc_stat {
        string name;
        char state{'?'};
        uint32_t parent_pid{0};
        uint32_t threads{0};
        int32_t nice{0};
        uint64_t flags{0};
        uint64_t user_ticks{0};
        uint64_t system_ticks{0};
        uint64_t start_ticks{0};
        uint64_t virtual_bytes{0};
        uint64_t resident_pages{0};
    };

    string read_text_file(const path& file_path) {
        file handle(file_path, false, file_access::read);
        if (!handle.is_opened()) {
            return {};
        }
        string content;
        for (;;) {
            const string chunk = handle.read(65536);
            if (chunk.empty()) {
                break;
            }
            content.append(chunk);
            if (chunk.size() < 65536) {
                break;
            }
        }
        return content;
    }

    uint64_t clock_ticks_per_second() noexcept {
        static const uint64_t cached = []() noexcept -> uint64_t {
            const long ticks = ::sysconf(_SC_CLK_TCK);
            return ticks > 0 ? static_cast<uint64_t>(ticks) : 100;
        }();
        return cached;
    }

    uint64_t boot_time_ms() {
        static const uint64_t cached = []() -> uint64_t {
            const string text = read_text_file(path{"/proc/stat"});
            size_t position = 0;
            string line;
            while (getline(text.view(), position, line)) {
                if (line.starts_with("btime ")) {
                    try {
                        return to_uint64(line.view(6)) * 1000;
                    } catch (const exception&) {
                        return 0;
                    }
                }
            }
            return 0;
        }();
        return cached;
    }

    uint64_t unix_time_ms() noexcept {
        ::timespec now{};
        if (::clock_gettime(CLOCK_REALTIME, &now) != 0) {
            return 0;
        }
        return static_cast<uint64_t>(now.tv_sec) * 1000 + static_cast<uint64_t>(now.tv_nsec) / 1000000;
    }

    uint64_t start_time_ms(const uint64_t start_ticks) {
        return boot_time_ms() + start_ticks * 1000 / clock_ticks_per_second();
    }

    process::state state_from_char(const char value) noexcept {
        switch (value) {
            case 'T':
                return process::state::suspended;
            case 't':
                return process::state::stopped;
            case 'Z':
            case 'X':
            case 'x':
                return process::state::exited;
            case 'R':
            case 'S':
            case 'D':
            case 'I':
            case 'W':
            case 'P':
                return process::state::running;
            default:
                return process::state::unknown;
        }
    }

    bool parse_proc_stat_text(const string& text, linux_proc_stat& out) {
        if (text.empty()) {
            return false;
        }
        const size_t open = text.find('(');
        const size_t close = text.rfind(')');
        if (open == string::npos || close == string::npos || close <= open || close + 2 >= text.size()) {
            return false;
        }
        const auto fields = text.substr(close + 2).split(" \t");
        if (fields.size() < 22) {
            return false;
        }
        try {
            out.name = text.substr(open + 1, close - open - 1);
            out.state = fields[0].empty() ? '?' : fields[0][0];
            out.parent_pid = static_cast<uint32_t>(to_uint64(fields[1].view()));
            out.flags = to_uint64(fields[6].view());
            out.user_ticks = to_uint64(fields[11].view());
            out.system_ticks = to_uint64(fields[12].view());
            out.threads = static_cast<uint32_t>(to_uint64(fields[17].view()));
            out.nice = static_cast<int32_t>(to_int64(fields[16].view()));
            out.start_ticks = to_uint64(fields[19].view());
            out.virtual_bytes = to_uint64(fields[20].view());
            out.resident_pages = to_uint64(fields[21].view());
        } catch (const exception&) {
            return false;
        }
        return true;
    }

    bool parse_proc_stat(const uint32_t process_id, linux_proc_stat& out) {
        const bool is_self = compare_equal(process_id, ::getpid());
        if (parse_proc_stat_text(read_text_file(path("/proc") / to_string(process_id).view() / "stat"), out)) {
            if (!is_self || out.threads != 0) {
                return true;
            }
        } else if (!is_self) {
            return false;
        }
        // User-mode emulators such as qemu-user synthesize /proc/<self>/stat with zeroed counters.
        // A real process always owns at least one thread, so fall back to the kernel-provided thread entry.
        return parse_proc_stat_text(read_text_file(path("/proc/thread-self/stat")), out);
    }

    bool owner_uid_of(const uint32_t process_id, uint32_t& uid) {
        const string text = read_text_file(path("/proc") / to_string(process_id).view() / "status");
        size_t position = 0;
        string line;
        while (getline(text.view(), position, line)) {
            if (!line.starts_with("Uid:")) {
                continue;
            }
            const auto parts = line.substr(4).split(" \t");
            if (parts.empty()) {
                return false;
            }
            try {
                uid = static_cast<uint32_t>(to_uint64(parts[0].view()));
            } catch (const exception&) {
                return false;
            }
            return true;
        }
        return false;
    }

    uint64_t kilobytes_value(const string_view line) {
        const size_t colon = line.find(':');
        if (colon == string::npos) {
            return 0;
        }
        string_view value = line.substr(colon + 1);
        value = value.trim_left();
        size_t end = 0;
        while (end < value.size() && value[end] >= '0' && value[end] <= '9') {
            ++end;
        }
        if (end == 0) {
            return 0;
        }
        try {
            return to_uint64(value.substr(0, end));
        } catch (const exception&) {
            return 0;
        }
    }

    int native_signal_of(const process::signal sig) noexcept {
        switch (sig) {
            case process::signal::terminate:
                return SIGTERM;
            case process::signal::kill:
                return SIGKILL;
            case process::signal::interrupt:
                return SIGINT;
            case process::signal::quit:
                return SIGQUIT;
            case process::signal::hangup:
                return SIGHUP;
            case process::signal::stop:
                return SIGSTOP;
            case process::signal::resume:
                return SIGCONT;
            case process::signal::user1:
                return SIGUSR1;
            case process::signal::user2:
                return SIGUSR2;
            default:
                return 0;
        }
    }

    bool send_native_signal(const int process_id, const int native_signal) noexcept {
        if (process_id <= 0 || native_signal <= 0) {
            return false;
        }
#    ifdef SYS_pidfd_open
        const int descriptor = static_cast<int>(::syscall(SYS_pidfd_open, process_id, 0));
        if (descriptor >= 0) {
            const bool sent = ::syscall(SYS_pidfd_send_signal, descriptor, native_signal, nullptr, 0) == 0;
            ::close(descriptor);
            return sent;
        }
#    endif
        return ::kill(process_id, native_signal) == 0;
    }

    string user_name_of_uid(const uint32_t uid) {
        char buffer[16384] = {};
        ::passwd entry{};
        ::passwd* result = nullptr;
        if (::getpwuid_r(static_cast<::uid_t>(uid), &entry, buffer, sizeof(buffer), &result) != 0 ||
            result == nullptr) {
            return {};
        }
        return result->pw_name;
    }

    process::priority priority_from_nice(const int32_t nice) noexcept {
        if (nice >= 15) {
            return process::priority::idle;
        }
        if (nice >= 1) {
            return process::priority::below_normal;
        }
        if (nice == 0) {
            return process::priority::normal;
        }
        if (nice >= -5) {
            return process::priority::normal_above;
        }
        if (nice >= -14) {
            return process::priority::high;
        }
        return process::priority::realtime;
    }

    int nice_for_priority(const process::priority level) noexcept {
        switch (level) {
            case process::priority::idle:
                return 19;
            case process::priority::below_normal:
                return 10;
            case process::priority::normal:
                return 0;
            case process::priority::normal_above:
                return -5;
            case process::priority::high:
                return -10;
            case process::priority::realtime:
                return -19;
            default:
                return 0;
        }
    }

    process::io_info io_info_from_proc(const uint32_t process_id) {
        process::io_info info;
        const string text = read_text_file(path("/proc") / to_string(process_id).view() / "io");
        size_t position = 0;
        string line;
        while (getline(text.view(), position, line)) {
            if (line.starts_with("read_bytes:")) {
                info.read_bytes = kilobytes_value(line.view());
            } else if (line.starts_with("write_bytes:")) {
                info.write_bytes = kilobytes_value(line.view());
            } else if (line.starts_with("syscr:")) {
                info.read_operations = kilobytes_value(line.view());
            } else if (line.starts_with("syscw:")) {
                info.write_operations = kilobytes_value(line.view());
            }
        }
        return info;
    }


#endif
} // namespace


process::~process() { close(); }

process::process(process&& other) noexcept :
process_id_(other.process_id_),
exit_code_(other.exit_code_),
started_(other.started_),
finished_(other.finished_),
work_dir_(move(other.work_dir_)),
env_vars_(move(other.env_vars_)),
capture_stdout_(other.capture_stdout_),
capture_stderr_(other.capture_stderr_),
stdin_data_(move(other.stdin_data_)),
stdout_file_(move(other.stdout_file_)),
stderr_file_(move(other.stderr_file_)),
external_stdin_pipe_(other.external_stdin_pipe_),
external_stdout_pipe_(other.external_stdout_pipe_),
external_stderr_pipe_(other.external_stderr_pipe_),
detached_(other.detached_),
stdout_pipe_(move(other.stdout_pipe_)),
stderr_pipe_(move(other.stderr_pipe_)),
stdin_pipe_(move(other.stdin_pipe_)),
stdout_buf_(move(other.stdout_buf_)),
stderr_buf_(move(other.stderr_buf_)),
reader_thread_(move(other.reader_thread_)),
reader_running_(other.reader_running_.load())
#ifdef NEFORCE_PLATFORM_WINDOWS
,
process_handle_(other.process_handle_),
thread_handle_(other.thread_handle_)
#endif
{
    other.process_id_ = 0;
    other.exit_code_ = -1;
    other.started_ = false;
    other.finished_ = false;
    other.capture_stdout_ = false;
    other.capture_stderr_ = false;
    other.detached_ = false;
    other.external_stdin_pipe_ = nullptr;
    other.external_stdout_pipe_ = nullptr;
    other.external_stderr_pipe_ = nullptr;
    other.reader_running_ = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
    other.process_handle_ = nullptr;
    other.thread_handle_ = nullptr;
#endif
}

process& process::operator=(process&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    close();

    process_id_ = other.process_id_;
    exit_code_ = other.exit_code_;
    started_ = other.started_;
    finished_ = other.finished_;
    work_dir_ = move(other.work_dir_);
    env_vars_ = move(other.env_vars_);
    capture_stdout_ = other.capture_stdout_;
    capture_stderr_ = other.capture_stderr_;
    stdin_data_ = move(other.stdin_data_);
    stdout_file_ = move(other.stdout_file_);
    stderr_file_ = move(other.stderr_file_);
    external_stdin_pipe_ = other.external_stdin_pipe_;
    external_stdout_pipe_ = other.external_stdout_pipe_;
    external_stderr_pipe_ = other.external_stderr_pipe_;
    detached_ = other.detached_;
    stdout_pipe_ = move(other.stdout_pipe_);
    stderr_pipe_ = move(other.stderr_pipe_);
    stdin_pipe_ = move(other.stdin_pipe_);
    stdout_buf_ = move(other.stdout_buf_);
    stderr_buf_ = move(other.stderr_buf_);
    reader_thread_ = move(other.reader_thread_);
    reader_running_.store(other.reader_running_.load());

#ifdef NEFORCE_PLATFORM_WINDOWS
    process_handle_ = other.process_handle_;
    thread_handle_ = other.thread_handle_;
    other.process_handle_ = nullptr;
    other.thread_handle_ = nullptr;
#endif

    other.process_id_ = 0;
    other.exit_code_ = -1;
    other.started_ = false;
    other.finished_ = false;
    other.capture_stdout_ = false;
    other.capture_stderr_ = false;
    other.detached_ = false;
    other.external_stdin_pipe_ = nullptr;
    other.external_stdout_pipe_ = nullptr;
    other.external_stderr_pipe_ = nullptr;
    other.reader_running_ = false;

    return *this;
}

void process::close() noexcept {
    reader_running_ = false;

    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }

    if (!detached_) {
        close_handles();
    }

    // Only close pipes this own (not externally provided ones)
    if (external_stdout_pipe_ == nullptr) {
        stdout_pipe_.close();
    }
    if (external_stderr_pipe_ == nullptr) {
        stderr_pipe_.close();
    }
    if (external_stdin_pipe_ == nullptr) {
        stdin_pipe_.close();
    }

    external_stdin_pipe_ = nullptr;
    external_stdout_pipe_ = nullptr;
    external_stderr_pipe_ = nullptr;

    process_id_ = 0;
    exit_code_ = -1;
    started_ = false;
    finished_ = false;
}

void process::detach_handles() noexcept {
    if (!detached_) {
        detached_ = true;
        close_handles();
    }
}

void process::close_handles() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (thread_handle_ != nullptr) {
        ::CloseHandle(thread_handle_);
        thread_handle_ = nullptr;
    }
    if (process_handle_ != nullptr) {
        ::CloseHandle(process_handle_);
        process_handle_ = nullptr;
    }
#endif
}

process& process::set_work_dir(const string& dir) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    work_dir_ = dir;
    return *this;
}

process& process::set_env(const string& key, const string& value) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    env_vars_.emplace_back(key, value);
    return *this;
}

process& process::set_capture_stdout(const bool v) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    capture_stdout_ = v;
    return *this;
}

process& process::set_capture_stderr(const bool v) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    capture_stderr_ = v;
    return *this;
}

process& process::set_stdin_data(const string& data) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    stdin_data_ = data;
    return *this;
}

process& process::set_stdout_file(const string& file_path) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    stdout_file_ = file_path;
    return *this;
}

process& process::set_stderr_file(const string& file_path) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    stderr_file_ = file_path;
    return *this;
}

process& process::set_external_stdin(pipe& external_pipe) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    external_stdin_pipe_ = &external_pipe;
    return *this;
}

process& process::set_external_stdout(pipe& external_pipe) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    external_stdout_pipe_ = &external_pipe;
    return *this;
}

process& process::set_external_stderr(pipe& external_pipe) {
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Cannot configure after process has been started"));
    }
    external_stderr_pipe_ = &external_pipe;
    return *this;
}

void process::reader_loop() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    constexpr ::DWORD buf_size = MEMORY_BIG_ALLOC_THRESHHOLD;
    char buffer[buf_size];

    while (reader_running_) {
        bool any_data = false;

        if (capture_stdout_) {
            ::DWORD available = 0;
            if (::PeekNamedPipe(stdout_pipe_.native_read_handle(), nullptr, 0, nullptr, &available, nullptr) == TRUE &&
                available > 0) {
                ::DWORD bytes_read = 0;
                if (::ReadFile(stdout_pipe_.native_read_handle(), buffer, buf_size - 1, &bytes_read, nullptr) == TRUE &&
                    bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    stdout_buf_.append(buffer, bytes_read);
                    any_data = true;
                }
            }
        }

        if (capture_stderr_) {
            ::DWORD available = 0;
            if (::PeekNamedPipe(stderr_pipe_.native_read_handle(), nullptr, 0, nullptr, &available, nullptr) == TRUE &&
                available > 0) {
                ::DWORD bytes_read = 0;
                if (::ReadFile(stderr_pipe_.native_read_handle(), buffer, buf_size - 1, &bytes_read, nullptr) == TRUE &&
                    bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    stderr_buf_.append(buffer, bytes_read);
                    any_data = true;
                }
            }
        }

        const bool stdout_alive = capture_stdout_ && stdout_pipe_.is_valid();
        const bool stderr_alive = capture_stderr_ && stderr_pipe_.is_valid();

        if (!stdout_alive && !stderr_alive) {
            break;
        }

        if (!any_data) {
            ::Sleep(10);
        }
    }
#else
    char buffer[MEMORY_BIG_ALLOC_THRESHHOLD];

    while (reader_running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        int stdout_fd = capture_stdout_ ? stdout_pipe_.native_read_handle() : -1;
        int stderr_fd = capture_stderr_ ? stderr_pipe_.native_read_handle() : -1;

        if (stdout_fd >= 0) {
            FD_SET(stdout_fd, &read_fds);
            max_fd = max(stdout_fd, max_fd);
        }
        if (stderr_fd >= 0) {
            FD_SET(stderr_fd, &read_fds);
            max_fd = max(stderr_fd, max_fd);
        }
        if (max_fd < 0) {
            break;
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        const int ret = ::select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
        if (ret < 0) {
            break;
        }

        if (stdout_fd >= 0 && FD_ISSET(stdout_fd, &read_fds)) {
            const ssize_t n = ::read(stdout_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                stdout_buf_.append(buffer, static_cast<size_t>(n));
            } else {
                stdout_pipe_.close_read();
            }
        }

        if (stderr_fd >= 0 && FD_ISSET(stderr_fd, &read_fds)) {
            const ssize_t n = ::read(stderr_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                stderr_buf_.append(buffer, static_cast<size_t>(n));
            } else {
                stderr_pipe_.close_read();
            }
        }
    }
#endif
}

void process::start(const string& executable, const vector<string>& args) {
    if (executable.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Executable path is empty"));
    }
    if (started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process already started"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const bool use_stdout_ext = external_stdout_pipe_ != nullptr;
    const bool use_stderr_ext = external_stderr_pipe_ != nullptr;
    const bool use_stdin_ext = external_stdin_pipe_ != nullptr;

    auto* stdout_write_handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    auto* stderr_write_handle = ::GetStdHandle(STD_ERROR_HANDLE);
    auto* stdin_read_handle = ::GetStdHandle(STD_INPUT_HANDLE);

    if (use_stdout_ext) {
        stdout_write_handle = external_stdout_pipe_->native_write_handle();
    } else if (capture_stdout_) {
        stdout_pipe_ = pipe(true);
        ::SetHandleInformation(stdout_pipe_.native_read_handle(), HANDLE_FLAG_INHERIT, 0);
        stdout_write_handle = stdout_pipe_.native_write_handle();
    } else if (!stdout_file_.empty()) {
        ::SECURITY_ATTRIBUTES sa{sizeof(::SECURITY_ATTRIBUTES), nullptr, TRUE};
        stdout_write_handle = ::CreateFileA(stdout_file_.data(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (stdout_write_handle == INVALID_HANDLE_VALUE) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(process_exception(error));
        }
    }

    if (use_stderr_ext) {
        stderr_write_handle = external_stderr_pipe_->native_write_handle();
    } else if (capture_stderr_) {
        stderr_pipe_ = pipe(true);
        ::SetHandleInformation(stderr_pipe_.native_read_handle(), HANDLE_FLAG_INHERIT, 0);
        stderr_write_handle = stderr_pipe_.native_write_handle();
    } else if (!stderr_file_.empty()) {
        ::SECURITY_ATTRIBUTES sa{sizeof(::SECURITY_ATTRIBUTES), nullptr, TRUE};
        const wstring stderr_file = character::to_wstring(stderr_file_.view());
        stderr_write_handle = ::CreateFileW(stderr_file.data(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (stderr_write_handle == INVALID_HANDLE_VALUE) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(process_exception(error));
        }
    } else if (capture_stdout_ && !capture_stderr_) {
        stderr_write_handle = stdout_write_handle;
    }

    if (use_stdin_ext) {
        stdin_read_handle = external_stdin_pipe_->native_read_handle();
    } else if (!stdin_data_.empty()) {
        stdin_pipe_ = pipe(true);
        ::SetHandleInformation(stdin_pipe_.native_write_handle(), HANDLE_FLAG_INHERIT, 0);
        stdin_read_handle = stdin_pipe_.native_read_handle();
    }

    ::STARTUPINFOW si{};
    si.cb = sizeof(::STARTUPINFOW);
    const bool has_custom_io = capture_stdout_ || capture_stderr_ || !stdin_data_.empty() || use_stdout_ext ||
                               use_stderr_ext || use_stdin_ext || !stdout_file_.empty() || !stderr_file_.empty();

    if (has_custom_io) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = stdin_read_handle;
        si.hStdOutput = stdout_write_handle;
        si.hStdError = stderr_write_handle;
    }

    const string cmd_line = process::build_command_line(executable, args);

    void* env_block = nullptr;
    string env_block_str;
    if (!env_vars_.empty()) {
        auto env_map = environment::all_envs();
        for (const auto& ov: env_vars_) {
            env_map[ov.first] = ov.second;
        }
        for (const auto& entry: env_map) {
            env_block_str += entry.first + "=" + entry.second;
            env_block_str.push_back('\0');
        }
        env_block_str.push_back('\0');
        env_block = env_block_str.data();
    }

    ::PROCESS_INFORMATION pi;
    wstring wcmd_line = character::to_wstring(cmd_line.view());
    wstring wwork_dir = character::to_wstring(work_dir_.view());
    const ::BOOL success =
            ::CreateProcessW(nullptr, wcmd_line.data(), nullptr, nullptr, has_custom_io ? TRUE : FALSE,
                             CREATE_NO_WINDOW, env_block, wwork_dir.empty() ? nullptr : wwork_dir.data(), &si, &pi);

    if (success == FALSE) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error));
    }

    process_handle_ = pi.hProcess;
    thread_handle_ = pi.hThread;
    process_id_ = pi.dwProcessId;

    if (capture_stdout_ && !use_stdout_ext) {
        stdout_pipe_.close_write();
    }
    if (capture_stderr_ && !use_stderr_ext) {
        stderr_pipe_.close_write();
    }
    if (!stdin_data_.empty() && !use_stdin_ext) {
        stdin_pipe_.close_read();
    }

    if (!stdout_file_.empty() && stdout_write_handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(stdout_write_handle);
    }
    if (!stderr_file_.empty() && stderr_write_handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(stderr_write_handle);
    }
#else
    const bool use_stdout_ext = external_stdout_pipe_ != nullptr;
    const bool use_stderr_ext = external_stderr_pipe_ != nullptr;
    const bool use_stdin_ext = external_stdin_pipe_ != nullptr;

    if (!use_stdout_ext && capture_stdout_) {
        stdout_pipe_ = pipe(false);
    }
    if (!use_stderr_ext && capture_stderr_) {
        stderr_pipe_ = pipe(false);
    }
    if (!use_stdin_ext && !stdin_data_.empty()) {
        stdin_pipe_ = pipe(false);
    }

    int notify_fds[2] = {-1, -1};
    if (::pipe2(notify_fds, O_CLOEXEC) == -1) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error));
    }

    const ::pid_t pid = ::fork();
    if (pid < 0) {
        ::close(notify_fds[0]);
        ::close(notify_fds[1]);
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error));
    }

    if (pid == 0) {
        ::close(notify_fds[0]);

        // Setup stdin
        int stdin_fd = -1;
        if (use_stdin_ext && external_stdin_pipe_->native_read_handle() >= 0) {
            stdin_fd = external_stdin_pipe_->native_read_handle();
        } else if (!stdin_data_.empty() && stdin_pipe_.native_read_handle() >= 0) {
            stdin_pipe_.close_write();
            stdin_fd = stdin_pipe_.native_read_handle();
        }

        if (stdin_fd >= 0) {
            if (::dup2(stdin_fd, STDIN_FILENO) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
            if (!use_stdin_ext) {
                stdin_pipe_.close_read();
            }
        }

        // Setup stdout
        int stdout_fd = -1;
        if (!stdout_file_.empty()) {
            stdout_fd = ::open(stdout_file_.data(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (stdout_fd < 0) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
        } else if (use_stdout_ext && external_stdout_pipe_->native_write_handle() >= 0) {
            external_stdout_pipe_->close_read();
            stdout_fd = external_stdout_pipe_->native_write_handle();
        } else if (capture_stdout_ && stdout_pipe_.native_write_handle() >= 0) {
            stdout_pipe_.close_read();
            stdout_fd = stdout_pipe_.native_write_handle();
        }

        if (stdout_fd >= 0) {
            if (::dup2(stdout_fd, STDOUT_FILENO) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
            if (!use_stdout_ext) {
                stdout_pipe_.close_write();
            }
        }

        // Setup stderr
        int stderr_fd = -1;
        if (!stderr_file_.empty()) {
            stderr_fd = ::open(stderr_file_.data(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (stderr_fd < 0) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
        } else if (use_stderr_ext && external_stderr_pipe_->native_write_handle() >= 0) {
            external_stderr_pipe_->close_read();
            stderr_fd = external_stderr_pipe_->native_write_handle();
        } else if (capture_stderr_ && stderr_pipe_.native_write_handle() >= 0) {
            stderr_pipe_.close_read();
            stderr_fd = stderr_pipe_.native_write_handle();
        } else if (capture_stdout_ && stdout_fd >= 0) {
            // Merge stderr into stdout
            if (::dup2(stdout_fd, STDERR_FILENO) == -1) {
                // Non-fatal: stderr merging failed
            }
        }

        if (stderr_fd >= 0) {
            if (::dup2(stderr_fd, STDERR_FILENO) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
            if (!use_stderr_ext) {
                stderr_pipe_.close_write();
            }
        }

        if (!work_dir_.empty()) {
            if (::chdir(work_dir_.data()) == -1) {
                const auto error = last_error();
                const int saved_errno = error.value();
                ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
                ::_exit(1);
            }
        }

        char** argv = build_argv(executable, args);

        if (env_vars_.empty()) {
            ::execvp(executable.data(), argv);
        } else {
            auto env_map = environment::all_envs();
            for (const auto& ov: env_vars_) {
                env_map[ov.first] = ov.second;
            }
            char** envp = build_envp(env_map);
            ::execve(executable.data(), argv, envp);
            free_envp(envp);
        }

        const auto error = last_error();
        const int saved_errno = error.value();
        ::write(notify_fds[1], &saved_errno, sizeof(saved_errno));
        ::close(notify_fds[1]);
        free_argv(argv);
        ::_exit(1);
    }

    ::close(notify_fds[1]);

    if (capture_stdout_ && !use_stdout_ext) {
        stdout_pipe_.close_write();
    }
    if (capture_stderr_ && !use_stderr_ext) {
        stderr_pipe_.close_write();
    }
    if (!stdin_data_.empty() && !use_stdin_ext) {
        stdin_pipe_.close_read();
    }

    int child_errno = 0;
    const ssize_t n = ::read(notify_fds[0], &child_errno, sizeof(child_errno));
    ::close(notify_fds[0]);

    if (n > 0) {
        int status = 0;
        ::waitpid(pid, &status, 0);
        const error_code error{child_errno, system_category()};
        NEFORCE_THROW_EXCEPTION(process_exception(error));
    }

    process_id_ = pid;
#endif

    started_ = true;

    if (capture_stdout_ || capture_stderr_) {
        reader_running_ = true;
        reader_thread_ = thread(&process::reader_loop, this);
    }

    if (!stdin_data_.empty()) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::DWORD written = 0;
        ::WriteFile(stdin_pipe_.native_write_handle(), stdin_data_.data(), static_cast<::DWORD>(stdin_data_.size()),
                    &written, nullptr);
#else
        stdin_pipe_.write(stdin_data_.data(), stdin_data_.size());
#endif
        close_stdin();
        stdin_data_.clear();
    }
}

void process::start_elevated(const string& executable, const vector<string>& args, elevation_tool tool) {
    if (capture_stdout_ || capture_stderr_ || !stdin_data_.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Pipe capture is not supported for elevated processes"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const string params = build_params_string(args);
    wstring wexecutable = character::to_wstring(executable.view());
    wstring wparams = character::to_wstring(params.view());
    wstring wwork_dir = character::to_wstring(work_dir_.view());

    ::SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = L"runas";
    sei.lpFile = wexecutable.data();
    sei.lpParameters = wparams.empty() ? nullptr : wparams.data();
    sei.lpDirectory = wwork_dir.empty() ? nullptr : wwork_dir.data();
    sei.nShow = SW_HIDE;

    if (::ShellExecuteExW(&sei) == FALSE) {
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_CANCELLED) {
            NEFORCE_THROW_EXCEPTION(process_exception("User cancelled elevation prompt"));
        }
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error));
    }

    process_handle_ = sei.hProcess;
    process_id_ = ::GetProcessId(sei.hProcess);
    started_ = true;
#else
    string elevation_tool;
    switch (tool) {
        case elevation_tool::sudo:
            elevation_tool = "/usr/bin/sudo";
            break;
        case elevation_tool::pkexec:
            elevation_tool = "/usr/bin/pkexec";
            break;
        case elevation_tool::auto_:
        default:
            elevation_tool = has_pkexec() ? "/usr/bin/pkexec" : "/usr/bin/sudo";
            break;
    }

    vector<string> elevated_args;
    if (elevation_tool == "/usr/bin/sudo") {
        elevated_args.push_back("--");
    }
    elevated_args.push_back(executable);
    for (const auto& a: args) {
        elevated_args.push_back(a);
    }

    start(elevation_tool, elevated_args);
#endif
}

int process::wait(const int timeout_ms) {
    if (!started_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not started"));
    }
    if (finished_) {
        return exit_code_;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD timeout = (timeout_ms < 0) ? numeric_traits<::DWORD>::max() : static_cast<::DWORD>(timeout_ms);
    const ::DWORD result = ::WaitForSingleObject(process_handle_, timeout);

    if (result == WAIT_TIMEOUT) {
        return -1;
    }
    if (result == WAIT_FAILED) {
        NEFORCE_THROW_EXCEPTION(process_exception("WaitForSingleObject failed"));
    }

    ::DWORD exit = 0;
    if (::GetExitCodeProcess(process_handle_, &exit) == FALSE) {
        NEFORCE_THROW_EXCEPTION(process_exception("GetExitCodeProcess failed"));
    }
    exit_code_ = static_cast<int>(exit);
#else
    int status = 0;

    if (timeout_ms < 0) {
        if (::waitpid(process_id_, &status, 0) == -1) {
            const auto error = last_error();
            NEFORCE_THROW_EXCEPTION(process_exception(error));
        }
    } else {
        int elapsed = 0;

        while (elapsed < timeout_ms) {
            constexpr int interval = 50;
            const ::pid_t result = ::waitpid(process_id_, &status, WNOHANG);
            if (result == -1) {
                const auto error = last_error();
                if (error.error() == errc::no_child_process) {
                    exit_code_ = 0;
                    finished_ = true;
                    reader_running_ = false;
                    if (reader_thread_.joinable()) {
                        reader_thread_.join();
                    }
                    return exit_code_;
                }
                NEFORCE_THROW_EXCEPTION(process_exception(error));
            }
            if (result > 0) {
                break;
            }

            ::usleep(interval * 1000);
            elapsed += interval;
        }

        if (elapsed >= timeout_ms) {
            return -1;
        }
    }

    if (WIFEXITED(status)) {
        exit_code_ = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exit_code_ = -static_cast<int>(WTERMSIG(status));
    } else {
        exit_code_ = -1;
    }
#endif

    finished_ = true;

    reader_running_ = false;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }

    if (capture_stdout_ && stdout_pipe_.is_valid()) {
        stdout_buf_ += stdout_pipe_.read_available();
    }
    if (capture_stderr_ && stderr_pipe_.is_valid()) {
        stderr_buf_ += stderr_pipe_.read_available();
    }

    return exit_code_;
}

void process::terminate() {
    if (!started_ || finished_) {
        return;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::TerminateProcess(process_handle_, 1);
    ::WaitForSingleObject(process_handle_, 5000);
#else
    ::kill(process_id_, SIGTERM);
    ::usleep(100000);

    if (::kill(process_id_, 0) == 0) {
        ::kill(process_id_, SIGKILL);
    }

    int status = 0;
    while (::waitpid(process_id_, &status, 0) == -1 && errno == EINTR) {
        this_thread::yield();
    }
#endif

    finished_ = true;

    reader_running_ = false;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }

    if (capture_stdout_ && stdout_pipe_.is_valid()) {
        stdout_buf_ += stdout_pipe_.read_available();
    }
    if (capture_stderr_ && stderr_pipe_.is_valid()) {
        stderr_buf_ += stderr_pipe_.read_available();
    }
}

void process::suspend() {
    if (!started_ || finished_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not in a suspendable state"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        NEFORCE_THROW_EXCEPTION(process_exception("CreateToolhelp32Snapshot failed"));
    }

    ::THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (::Thread32First(hSnapshot, &te) == TRUE) {
        do {
            if (te.th32OwnerProcessID == process_id_) {
                const ::HANDLE hThread = ::OpenThread(
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                if (hThread != nullptr) {
                    ::SuspendThread(hThread);
                    ::CloseHandle(hThread);
                }
            }
        } while (::Thread32Next(hSnapshot, &te) == TRUE);
    }

    ::CloseHandle(hSnapshot);
#else
    if (::kill(process_id_, SIGSTOP) != 0) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error));
    }
#endif
}

void process::resume() {
    if (!started_ || finished_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not in a resumable state"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        NEFORCE_THROW_EXCEPTION(process_exception("CreateToolhelp32Snapshot failed"));
    }

    ::THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (::Thread32First(hSnapshot, &te) == TRUE) {
        do {
            if (te.th32OwnerProcessID == process_id_) {
                const ::HANDLE hThread = ::OpenThread(
                        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                if (hThread != nullptr) {
                    ::ResumeThread(hThread);
                    ::CloseHandle(hThread);
                }
            }
        } while (::Thread32Next(hSnapshot, &te) == TRUE);
    }

    ::CloseHandle(hSnapshot);
#else
    if (::kill(process_id_, SIGCONT) != 0) {
        const auto error = last_error();
        NEFORCE_THROW_EXCEPTION(process_exception(error));
    }
#endif
}

bool process::is_running() const {
    if (!started_ || finished_) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD exit = 0;
    if (::GetExitCodeProcess(process_handle_, &exit) == TRUE) {
        return exit == STILL_ACTIVE;
    }
    return false;
#else
    const state s = get_state(process_id_);
    if (s == state::exited) {
        return false;
    }
    if (s != state::unknown) {
        return true;
    }
    return ::kill(process_id_, 0) == 0;
#endif
}

process::state process::get_state() const {
    if (!started_) {
        return state::unknown;
    }
    if (finished_) {
        return state::exited;
    }

    return get_state(process_id_);
}

process::memory_info process::get_memory_info() const {
    if (!started_) {
        return {};
    }
    return get_memory_info(process_id_);
}

process::time_info process::get_time_info() const {
    if (!started_) {
        return {};
    }
    return get_time_info(process_id_);
}

void process::write_stdin(const string& data) {
    if (!started_ || finished_) {
        NEFORCE_THROW_EXCEPTION(process_exception("Process not running, cannot write to stdin"));
    }
    if (!stdin_pipe_.is_valid()) {
        NEFORCE_THROW_EXCEPTION(process_exception("stdin pipe not open"));
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD written = 0;
    ::WriteFile(stdin_pipe_.native_write_handle(), data.data(), static_cast<::DWORD>(data.size()), &written, nullptr);
#else
    stdin_pipe_.write(data.data(), data.size());
#endif
}

void process::close_stdin() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    stdin_pipe_.close_write();
#else
    stdin_pipe_.close_write();
#endif
}

process::shell_result process::execute_shell(const string& command, const int timeout_ms) {
    if (command.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Empty shell command"));
    }

    process p;
    p.set_capture_stdout(true);
    p.set_capture_stderr(true);

#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", command});
#else
    p.start("/bin/sh", {"-c", command});
#endif

    const int exit_code = p.wait(timeout_ms);

    if (exit_code == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Command execution timeout"));
    }

    return {exit_code, p.stdout_buf_, p.stderr_buf_};
}

process::shell_result process::execute_elevated_shell(const string& command, const int timeout_ms,
                                                      const elevation_tool tool) {
    if (command.empty()) {
        NEFORCE_THROW_EXCEPTION(process_exception("Empty shell command"));
    }

    process p;

#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start_elevated("cmd.exe", {"/c", command}, tool);
#else
    p.start_elevated("/bin/sh", {"-c", command}, tool);
#endif

    const int exit_code = p.wait(timeout_ms);

    if (exit_code == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Command execution timeout"));
    }

    return {exit_code, "", ""};
}

process::native_id_type process::current_id() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetCurrentProcessId();
#else
    return ::getpid();
#endif
}

process::privilege_level process::current_privilege_level() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::BOOL is_admin = FALSE;
    ::SID_IDENTIFIER_AUTHORITY nt_auth = SECURITY_NT_AUTHORITY;
    ::PSID admin_sid = nullptr;
    if (::AllocateAndInitializeSid(&nt_auth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                   &admin_sid) == TRUE) {
        ::CheckTokenMembership(nullptr, admin_sid, &is_admin);
        ::FreeSid(admin_sid);
        return is_admin == TRUE ? privilege_level::privileged : privilege_level::not_privileged;
    }
    return privilege_level::unknown;
#else
    return (::geteuid() == 0) ? privilege_level::privileged : privilege_level::not_privileged;
#endif
}

vector<process::snapshot_entry> process::snapshot() {
    vector<snapshot_entry> out;
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE tool_snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (tool_snapshot == INVALID_HANDLE_VALUE) {
        return out;
    }
    const auto system_facts = system_process_facts_of_processes();
    ::PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (::Process32FirstW(tool_snapshot, &entry) == TRUE) {
        do {
            if (entry.th32ProcessID == 0) {
                continue;
            }
            snapshot_entry item;
            item.pid = static_cast<uint32_t>(entry.th32ProcessID);
            item.parent_pid = static_cast<uint32_t>(entry.th32ParentProcessID);
            item.threads = static_cast<uint32_t>(entry.cntThreads);
            item.name = wcharacter::to_string(entry.szExeFile);
            const auto facts = system_facts.find(item.pid);
            if (facts != system_facts.end()) {
                item.virtual_bytes = facts->second.virtual_size;
                if (facts->second.threads != 0) {
                    item.threads = facts->second.threads;
                }
                if (facts->second.create_time_ms != 0) {
                    item.start_time_ms = facts->second.create_time_ms;
                }
            }
            const ::HANDLE handle =
                    ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
            if (handle != nullptr) {
                ::DWORD state_code = 0;
                if (::GetExitCodeProcess(handle, &state_code) == TRUE) {
                    item.status = state_code == STILL_ACTIVE ? state::running : state::exited;
                }
                ::PROCESS_MEMORY_COUNTERS counters{};
                if (::GetProcessMemoryInfo(handle, &counters, sizeof(counters)) == TRUE) {
                    item.rss_bytes = counters.WorkingSetSize;
                }
                ::FILETIME creation{};
                ::FILETIME exit_time{};
                ::FILETIME kernel_time{};
                ::FILETIME user_time{};
                if (::GetProcessTimes(handle, &creation, &exit_time, &kernel_time, &user_time) == TRUE) {
                    item.start_time_ms = filetime_to_unix_ms(creation);
                    item.user_time_ms = filetime_span_ms(user_time);
                    item.system_time_ms = filetime_span_ms(kernel_time);
                }
                item.command = query_process_command_line(handle);
                item.io = io_info_of_handle(handle);
                item.user = user_name_of_handle(handle);
                ::CloseHandle(handle);
            }
            out.push_back(move(item));
        } while (::Process32NextW(tool_snapshot, &entry) == TRUE);
    }
    ::CloseHandle(tool_snapshot);
#elif defined(NEFORCE_PLATFORM_LINUX)
    const auto page_size = static_cast<uint64_t>(process_page_size());
    const uint64_t ticks = clock_ticks_per_second();
    const path proc("/proc");
    for (const auto& directory: proc.children()) {
        const auto name_view = directory.filename();
        if (name_view.empty() || name_view[0] < '0' || name_view[0] > '9') {
            continue;
        }
        uint32_t process_id = 0;
        try {
            process_id = static_cast<uint32_t>(to_uint64(name_view));
        } catch (const exception&) {
            continue;
        }
        linux_proc_stat stat_data;
        if (!parse_proc_stat(process_id, stat_data)) {
            continue;
        }
        snapshot_entry item;
        item.pid = process_id;
        item.parent_pid = stat_data.parent_pid;
        item.name = move(stat_data.name);
        item.status = state_from_char(stat_data.state);
        item.state_code = stat_data.state;
        item.threads = stat_data.threads;
        item.rss_bytes = stat_data.resident_pages * page_size;
        item.virtual_bytes = stat_data.virtual_bytes;
        item.user_time_ms = stat_data.user_ticks * 1000 / ticks;
        item.system_time_ms = stat_data.system_ticks * 1000 / ticks;
        item.start_time_ms = start_time_ms(stat_data.start_ticks);
        item.nice = stat_data.nice;
        item.kernel_thread = (stat_data.flags & 0x00200000ULL) != 0;
        item.io = io_info_from_proc(process_id);
        uint32_t uid = 0;
        if (owner_uid_of(process_id, uid)) {
            item.uid = uid;
            item.user = user_name_of_uid(uid);
        }
        string command = read_text_file(proc / to_string(process_id).view() / "cmdline");
        if (!command.empty()) {
            for (auto& value: command) {
                if (value == '\0') {
                    value = ' ';
                }
            }
            item.command = command.trim();
        }
        out.push_back(move(item));
    }
#endif
    return out;
}

string process::get_name(const native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (handle == nullptr) {
        return {};
    }
    wchar_t process_name[MAX_PATH] = {};
    ::HMODULE module = nullptr;
    ::DWORD needed = 0;
    string name;
    if (::EnumProcessModules(handle, &module, sizeof(::HMODULE), &needed) == TRUE) {
        if (::GetModuleBaseNameW(handle, module, process_name, MAX_PATH) != 0) {
            name = wcharacter::to_string(process_name);
        }
    }
    ::CloseHandle(handle);
    return name;
#else
    linux_proc_stat stat_data;
    if (parse_proc_stat(static_cast<uint32_t>(process_id), stat_data)) {
        return move(stat_data.name);
    }
    return {};
#endif
}

string process::current_name() { return get_name(current_id()); }

process::state process::current_state() { return get_state(current_id()); }

string process::current_command_line() { return get_command_line(current_id()); }

string process::get_command_line(native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (handle == nullptr) {
        return {};
    }
    string command = query_process_command_line(handle);
    ::CloseHandle(handle);
    return command;
#else
    string command = read_text_file(path("/proc") / to_string(process_id).view() / "cmdline");
    if (command.empty()) {
        return {};
    }
    for (auto& value: command) {
        if (value == '\0') {
            value = ' ';
        }
    }
    return command.trim();
#endif
}

string process::get_executable_path(native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (handle == nullptr) {
        return {};
    }
    wchar_t buffer[MAX_PATH] = {};
    string result;
    if (::GetModuleFileNameExW(handle, nullptr, buffer, MAX_PATH) != 0) {
        result = wcharacter::to_string(buffer);
    }
    ::CloseHandle(handle);
    return result;
#else
    char buffer[4096] = {};
    const string link = string("/proc/") + to_string(process_id) + "/exe";
    const ssize_t length = ::readlink(link.data(), buffer, sizeof(buffer) - 1);
    if (length <= 0) {
        return {};
    }
    return {buffer, static_cast<size_t>(length)};
#endif
}

process::time_info process::get_time_info(const native_id_type process_id) {
    time_info info;
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (handle == nullptr) {
        return info;
    }
    ::FILETIME creation{};
    ::FILETIME exit_time{};
    ::FILETIME kernel_time{};
    ::FILETIME user_time{};
    if (::GetProcessTimes(handle, &creation, &exit_time, &kernel_time, &user_time) == TRUE) {
        const uint64_t started_at = filetime_to_unix_ms(creation);
        const uint64_t now = unix_time_ms();
        info.user_time_ms = filetime_span_ms(user_time);
        info.kernel_time_ms = filetime_span_ms(kernel_time);
        info.wall_time_ms = now > started_at ? now - started_at : 0;
    }
    ::CloseHandle(handle);
#else
    linux_proc_stat stat_data;
    if (!parse_proc_stat(static_cast<uint32_t>(process_id), stat_data)) {
        return info;
    }
    const uint64_t ticks = clock_ticks_per_second();
    info.user_time_ms = stat_data.user_ticks * 1000 / ticks;
    info.kernel_time_ms = stat_data.system_ticks * 1000 / ticks;
    const uint64_t started_at = start_time_ms(stat_data.start_ticks);
    const uint64_t now = unix_time_ms();
    info.wall_time_ms = now > started_at ? now - started_at : 0;
#endif
    return info;
}

process::time_info process::current_time_info() { return get_time_info(current_id()); }

process::memory_info process::get_memory_info(native_id_type process_id) {
    memory_info mem_info;

#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (handle == nullptr) {
        return mem_info;
    }

    ::PROCESS_MEMORY_COUNTERS counters{};
    if (::GetProcessMemoryInfo(handle, &counters, sizeof(counters)) == TRUE) {
        mem_info.working_set_size = counters.WorkingSetSize;
        mem_info.peak_working_set_size = counters.PeakWorkingSetSize;
        mem_info.pagefile_usage = counters.PagefileUsage;
        mem_info.peak_pagefile_usage = counters.PeakPagefileUsage;
    }
    mem_info.virtual_size = virtual_size_of(handle);
    ::CloseHandle(handle);
#else
    linux_proc_stat stat_data;
    if (parse_proc_stat(static_cast<uint32_t>(process_id), stat_data)) {
        mem_info.virtual_size = static_cast<size_t>(stat_data.virtual_bytes);
        mem_info.working_set_size = static_cast<size_t>(stat_data.resident_pages * process_page_size());
    }

    const string status = read_text_file(path("/proc") / to_string(process_id).view() / "status");
    size_t position = 0;
    string line;
    while (getline(status.view(), position, line)) {
        if (line.starts_with("VmHWM:")) {
            mem_info.peak_working_set_size = static_cast<size_t>(kilobytes_value(line.view()) * 1024);
        } else if (line.starts_with("VmSwap:")) {
            mem_info.pagefile_usage = static_cast<size_t>(kilobytes_value(line.view()) * 1024);
        }
    }
    mem_info.peak_pagefile_usage = 0;
#endif
    return mem_info;
}

process::memory_info process::current_memory_info() { return get_memory_info(current_id()); }

process::state process::get_state(native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (handle == nullptr) {
        return state::unknown;
    }

    ::DWORD exit_code = 0;
    auto result = state::unknown;
    if (::GetExitCodeProcess(handle, &exit_code) == TRUE) {
        if (exit_code != STILL_ACTIVE) {
            result = state::exited;
        } else {
            result = state::running;
            const ::HANDLE tool_snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (tool_snapshot != INVALID_HANDLE_VALUE) {
                ::THREADENTRY32 entry{};
                entry.dwSize = sizeof(entry);
                if (::Thread32First(tool_snapshot, &entry) == TRUE) {
                    do {
                        if (entry.th32OwnerProcessID != process_id) {
                            continue;
                        }
                        const ::HANDLE thread_handle =
                                ::OpenThread(THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID);
                        if (thread_handle != nullptr) {
                            const ::DWORD suspend_count = ::SuspendThread(thread_handle);
                            ::ResumeThread(thread_handle);
                            ::CloseHandle(thread_handle);
                            if (suspend_count != static_cast<::DWORD>(-1) && suspend_count > 0) {
                                result = state::suspended;
                            }
                        }
                        break;
                    } while (::Thread32Next(tool_snapshot, &entry) == TRUE);
                }
                ::CloseHandle(tool_snapshot);
            }
        }
    }
    ::CloseHandle(handle);
    return result;
#else
    linux_proc_stat stat_data;
    if (parse_proc_stat(static_cast<uint32_t>(process_id), stat_data)) {
        return state_from_char(stat_data.state);
    }
    if (::kill(process_id, 0) != 0) {
        return state::exited;
    }
    return state::unknown;
#endif
}

process::privilege_level process::get_privilege_level(native_id_type process_id) {
    try {
#ifdef NEFORCE_PLATFORM_WINDOWS
        if (process_id == ::GetCurrentProcessId()) {
            ::BOOL is_admin = FALSE;
            ::SID_IDENTIFIER_AUTHORITY nt_auth = SECURITY_NT_AUTHORITY;
            ::PSID admin_sid = nullptr;
            if (::AllocateAndInitializeSid(&nt_auth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0,
                                           0, 0, 0, &admin_sid) == TRUE) {
                if (::CheckTokenMembership(nullptr, admin_sid, &is_admin) == FALSE) {
                    is_admin = FALSE;
                }
                ::FreeSid(admin_sid);
                return is_admin == TRUE ? privilege_level::privileged : privilege_level::not_privileged;
            }
            return privilege_level::unknown;
        }

        const ::HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
        if (hProcess == nullptr) {
            return privilege_level::unknown;
        }

        ::HANDLE hToken = nullptr;
        if (::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken) == FALSE) {
            ::CloseHandle(hProcess);
            return privilege_level::unknown;
        }

        ::SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        ::PSID pAdminSid = nullptr;
        if (::AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0,
                                       0, 0, 0, &pAdminSid) == FALSE) {
            ::CloseHandle(hToken);
            ::CloseHandle(hProcess);
            return privilege_level::unknown;
        }

        ::BOOL bIsAdmin = FALSE;
        const ::BOOL checkResult = ::CheckTokenMembership(hToken, pAdminSid, &bIsAdmin);
        ::FreeSid(pAdminSid);
        ::CloseHandle(hToken);
        ::CloseHandle(hProcess);

        if (checkResult == FALSE) {
            return privilege_level::unknown;
        }
        return (bIsAdmin == TRUE) ? privilege_level::privileged : privilege_level::not_privileged;
#else
        const string status = read_text_file(path("/proc") / to_string(process_id).view() / "status");
        size_t position = 0;
        string line;
        while (getline(status.view(), position, line)) {
            if (!line.starts_with("Uid:")) {
                continue;
            }
            const auto parts = line.substr(4).split(" \t");
            if (parts.size() < 2) {
                return privilege_level::unknown;
            }
            try {
                return to_uint64(parts[1].view()) == 0 ? privilege_level::privileged : privilege_level::not_privileged;
            } catch (const exception&) {
                return privilege_level::unknown;
            }
        }
        return privilege_level::unknown;
#endif
    } catch (...) {
        return privilege_level::unknown;
    }
}

bool process::check_permission(native_id_type process_id, permission perm) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD desired_access = 0;
    const int value = static_cast<int>(perm);
    if ((value & static_cast<int>(permission::read)) != 0) {
        desired_access |= PROCESS_VM_READ;
    }
    if ((value & static_cast<int>(permission::write)) != 0) {
        desired_access |= PROCESS_VM_WRITE;
    }
    if ((value & static_cast<int>(permission::terminate)) != 0) {
        desired_access |= PROCESS_TERMINATE;
    }
    if ((value & static_cast<int>(permission::query_info)) != 0) {
        desired_access |= PROCESS_QUERY_INFORMATION;
    }

    const ::HANDLE handle = ::OpenProcess(desired_access, FALSE, process_id);
    if (handle == nullptr) {
        return false;
    }
    ::CloseHandle(handle);
    return true;
#else
    const string proc_path = string("/proc/") + to_string(process_id);
    if (::access(proc_path.data(), F_OK) != 0) {
        return false;
    }

    const int value = static_cast<int>(perm);
    if (value == 0) {
        return true;
    }
    if ((value & static_cast<int>(permission::execute)) != 0) {
        return false;
    }

    constexpr int owner_only = static_cast<int>(permission::read) | static_cast<int>(permission::write) |
                               static_cast<int>(permission::terminate);
    if ((value & owner_only) == 0) {
        return true;
    }
    if (::geteuid() == 0) {
        return true;
    }
    uint32_t uid = 0;
    if (!owner_uid_of(static_cast<uint32_t>(process_id), uid)) {
        return false;
    }
    return uid == static_cast<uint32_t>(::geteuid());
#endif
}

process::native_id_type process::spawn(const string& executable, const vector<string>& args) {
    process p;
    p.start(executable, args);
    const native_id_type id = p.id();
    p.detach_handles();
    return id;
}

int process::run(const string& executable, const vector<string>& args, const int timeout_ms) {
    process p;
    p.start(executable, args);
    const int rc = p.wait(timeout_ms);
    if (rc == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Process execution timeout"));
    }
    return rc;
}

process::shell_result process::run_capture(const string& executable, const vector<string>& args, const int timeout_ms) {
    process p;
    p.set_capture_stdout(true);
    p.set_capture_stderr(true);
    p.start(executable, args);
    const int rc = p.wait(timeout_ms);
    if (rc == -1 && timeout_ms >= 0) {
        p.terminate();
        NEFORCE_THROW_EXCEPTION(process_exception("Process execution timeout"));
    }
    return {rc, p.stdout_output(), p.stderr_output()};
}

void process::chain(process& first, process& second) {
    second.stdin_pipe_ = pipe(false);
    first.set_external_stdout(second.stdin_pipe_);
    second.set_external_stdin(second.stdin_pipe_);
}

void process::chain(const vector<process*>& processes) {
    if (processes.size() < 2) {
        return;
    }
    for (size_t i = 0; i < processes.size() - 1; ++i) {
        chain(*processes[i], *processes[i + 1]);
    }
}

string process::build_command_line(const string& executable, const vector<string>& args) {
    const auto needs_quoting = [](const string& value) -> bool {
        if (value.empty()) {
            return true;
        }

        constexpr uint64_t lo = (1ULL << 0x09) | (1ULL << 0x20) | (1ULL << 0x22) | (1ULL << 0x25) | (1ULL << 0x26) |
                                (1ULL << 0x3C) | (1ULL << 0x3E);
        constexpr uint64_t hi = (1ULL << (0x5E - 64)) | (1ULL << (0x7C - 64));

        for (const byte_t u: value) {
            if (u < 128) {
                const uint64_t word = (u < 64) ? lo : hi;
                if ((word >> (u & 63)) & 1) {
                    return true;
                }
            }
        }
        return false;
    };

    string command_line = needs_quoting(executable) ? ('"' + executable + '"') : executable;
    for (const auto& arg: args) {
        command_line += ' ';
        command_line += needs_quoting(arg) ? ('"' + arg + '"') : arg;
    }
    return command_line;
}

string process::search_path(const string& executable) {
    if (executable.empty()) {
        return "";
    }

    if (executable.contains('/') || executable.contains('\\')) {
        return executable;
    }

    const auto paths = environment::path_list();

#ifdef NEFORCE_PLATFORM_WINDOWS
    const vector<const char*> extensions = {".exe", ".bat", ".cmd", ""};
    for (const auto& dir: paths) {
        for (const char* ext: extensions) {
            const string full_path = dir + "\\" + executable + ext;
            wstring wfull_path = character::to_wstring(full_path.view());
            const ::DWORD attrs = ::GetFileAttributesW(wfull_path.data());
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                return full_path;
            }
        }
    }
#else
    for (const auto& dir: paths) {
        const string full_path = dir + "/" + executable;
        if (::access(full_path.data(), X_OK) == 0) {
            return full_path;
        }
    }
#endif

    return "";
}

bool process::relaunch_current() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    wchar_t executable[MAX_PATH];
    const ::DWORD length = ::GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return false;
    }
    wchar_t* mutable_command = ::GetCommandLineW();
    ::STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    ::PROCESS_INFORMATION child{};
    const ::BOOL created = ::CreateProcessW(executable, mutable_command, nullptr, nullptr, FALSE,
                                            CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &startup, &child);
    if (created == FALSE) {
        return false;
    }
    ::CloseHandle(child.hThread);
    ::CloseHandle(child.hProcess);
    return true;
#else
    vector<string> arguments;
    file cmdline{path{"/proc/self/cmdline"}, false, file_access::read};
    if (cmdline.is_opened()) {
        const string bytes = cmdline.read();
        size_t start = 0;
        while (start < bytes.size()) {
            size_t end = start;
            while (end < bytes.size() && bytes[end] != '\0') {
                ++end;
            }
            if (end > start) {
                arguments.push_back(bytes.substr(start, end - start));
            }
            start = end + 1;
        }
    }
    if (arguments.empty()) {
        return false;
    }
    const ::pid_t child = ::fork();
    if (child < 0) {
        return false;
    }
    if (child == 0) {
        ::setsid();
        vector<char*> argv;
        argv.reserve(arguments.size() + 1);
        for (auto& argument: arguments) {
            argv.push_back(argument.data());
        }
        argv.push_back(nullptr);
        ::execv(arguments.front().data(), argv.data());
        ::_exit(127);
    }
    return true;
#endif
}

process::io_info process::get_io_info(const native_id_type process_id) {
    io_info info;
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (handle == nullptr) {
        return info;
    }
    info = io_info_of_handle(handle);
    ::CloseHandle(handle);
#else
    info = io_info_from_proc(static_cast<uint32_t>(process_id));
#endif
    return info;
}

process::io_info process::current_io_info() { return get_io_info(current_id()); }

process::io_info process::get_io_info() const {
    if (!started_) {
        return {};
    }
    return get_io_info(process_id_);
}

string process::get_user(const native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (handle == nullptr) {
        return {};
    }
    string name = user_name_of_handle(handle);
    ::CloseHandle(handle);
    return name;
#else
    uint32_t uid = 0;
    if (!owner_uid_of(static_cast<uint32_t>(process_id), uid)) {
        return {};
    }
    return user_name_of_uid(uid);
#endif
}

vector<process::thread_info> process::threads(const native_id_type process_id) {
    vector<thread_info> out;
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return out;
    }
    ::THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (::Thread32First(snapshot, &entry) == TRUE) {
        do {
            if (entry.th32OwnerProcessID != process_id) {
                continue;
            }
            thread_info item;
            item.tid = entry.th32ThreadID;
            const ::HANDLE handle = ::OpenThread(THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID);
            if (handle != nullptr) {
                ::FILETIME creation{};
                ::FILETIME exit_time{};
                ::FILETIME kernel_time{};
                ::FILETIME user_time{};
                if (::GetThreadTimes(handle, &creation, &exit_time, &kernel_time, &user_time) == TRUE) {
                    item.start_time_ms = filetime_to_unix_ms(creation);
                    item.user_time_ms = filetime_span_ms(user_time);
                    item.system_time_ms = filetime_span_ms(kernel_time);
                }
                item.name = thread_name_of_handle(handle);
                ::CloseHandle(handle);
            }
            out.push_back(move(item));
        } while (::Thread32Next(snapshot, &entry) == TRUE);
    }
    ::CloseHandle(snapshot);
#else
    const path task_directory = path("/proc") / to_string(static_cast<uint32_t>(process_id)).view() / "task";
    for (const auto& directory: task_directory.children()) {
        const auto name_view = directory.filename();
        if (name_view.empty()) {
            continue;
        }
        uint32_t tid = 0;
        try {
            tid = static_cast<uint32_t>(to_uint64(name_view));
        } catch (const exception&) {
            continue;
        }
        linux_proc_stat stat_data;
        if (!parse_proc_stat_text(read_text_file(path(directory.str() + "/stat")), stat_data)) {
            continue;
        }
        thread_info item;
        item.tid = tid;
        item.name = move(stat_data.name);
        item.status = state_from_char(stat_data.state);
        const uint64_t ticks = clock_ticks_per_second();
        item.user_time_ms = stat_data.user_ticks * 1000 / ticks;
        item.system_time_ms = stat_data.system_ticks * 1000 / ticks;
        item.start_time_ms = start_time_ms(stat_data.start_ticks);
        out.push_back(move(item));
    }
#endif
    return out;
}

map<uint32_t, vector<process::native_id_type>> process::socket_owners() {
    map<uint32_t, vector<native_id_type>> owners;
#ifdef NEFORCE_PLATFORM_LINUX
    for (const auto& directory: path("/proc").children()) {
        const auto name_view = directory.filename();
        if (name_view.empty() || name_view[0] < '0' || name_view[0] > '9') {
            continue;
        }
        native_id_type process_id = 0;
        try {
            process_id = static_cast<native_id_type>(to_uint64(name_view));
        } catch (const exception&) {
            continue;
        }
        for (const auto& descriptor: path(directory.str() + "/fd").children()) {
            char target[256] = {};
            const ssize_t length = ::readlink(descriptor.str().data(), target, sizeof(target) - 1);
            if (length <= 0) {
                continue;
            }
            const string_view text(target, static_cast<size_t>(length));
            if (!text.starts_with("socket:[")) {
                continue;
            }
            const auto close = text.find(']');
            if (close == string_view::npos) {
                continue;
            }
            uint32_t inode = 0;
            try {
                inode = static_cast<uint32_t>(to_uint64(text.substr(8, close - 8)));
            } catch (const exception&) {
                continue;
            }
            auto& pids = owners[inode];
            if (pids.empty() || pids.back() != process_id) {
                pids.push_back(process_id);
            }
        }
    }
#endif
    return owners;
}

bool process::send_signal(const native_id_type process_id, const signal sig) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (sig != signal::terminate && sig != signal::kill) {
        return false;
    }
    const ::HANDLE handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<::DWORD>(process_id));
    if (handle == nullptr) {
        return false;
    }
    const bool sent = ::TerminateProcess(handle, 1) != 0;
    ::CloseHandle(handle);
    return sent;
#else
    return send_native_signal(static_cast<int>(process_id), native_signal_of(sig));
#endif
}

bool process::send_signal(const native_id_type process_id, const int native_signal) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (native_signal == 9 || native_signal == 15) {
        return send_signal(process_id, signal::kill);
    }
    return false;
#else
    return send_native_signal(static_cast<int>(process_id), native_signal);
#endif
}

string_view process::signal_name(const signal sig) noexcept {
    switch (sig) {
        case signal::terminate:
            return "TERM";
        case signal::kill:
            return "KILL";
        case signal::interrupt:
            return "INT";
        case signal::quit:
            return "QUIT";
        case signal::hangup:
            return "HUP";
        case signal::stop:
            return "STOP";
        case signal::resume:
            return "CONT";
        case signal::user1:
            return "USR1";
        case signal::user2:
            return "USR2";
        default:
            return "UNKNOWN";
    }
}

optional<process::priority> process::get_priority(const native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (handle == nullptr) {
        return none;
    }
    const ::DWORD priority_class = ::GetPriorityClass(handle);
    ::CloseHandle(handle);
    if (priority_class == 0) {
        return none;
    }
    auto level = priority::normal;
    if (!priority_from_class(priority_class, level)) {
        return none;
    }
    return level;
#else
    linux_proc_stat stat_data;
    if (!parse_proc_stat(static_cast<uint32_t>(process_id), stat_data)) {
        return none;
    }
    return priority_from_nice(stat_data.nice);
#endif
}

bool process::set_priority(const native_id_type process_id, const priority level) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD priority_class = class_for_priority(level);
    if (priority_class == 0) {
        return false;
    }
    const ::HANDLE handle = ::OpenProcess(PROCESS_SET_INFORMATION, FALSE, process_id);
    if (handle == nullptr) {
        return false;
    }
    const bool changed = ::SetPriorityClass(handle, priority_class) != 0;
    ::CloseHandle(handle);
    return changed;
#else
    return ::setpriority(PRIO_PROCESS, static_cast<::id_t>(process_id), nice_for_priority(level)) == 0;
#endif
}

bool process::set_nice(const native_id_type process_id, const int32_t nice) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const int32_t value = nice < -20 ? -20 : (nice > 19 ? 19 : nice);
    auto level = priority::normal;
    if (value >= 15) {
        level = priority::idle;
    } else if (value >= 1) {
        level = priority::below_normal;
    } else if (value == 0) {
        level = priority::normal;
    } else if (value >= -5) {
        level = priority::normal_above;
    } else if (value >= -14) {
        level = priority::high;
    } else {
        level = priority::realtime;
    }
    return set_priority(process_id, level);
#else
    const int32_t value = nice < -20 ? -20 : (nice > 19 ? 19 : nice);
    return ::setpriority(PRIO_PROCESS, static_cast<::id_t>(process_id), static_cast<int>(value)) == 0;
#endif
}

optional<uint64_t> process::get_affinity(const native_id_type process_id) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (handle == nullptr) {
        return none;
    }
    ::DWORD_PTR process_mask = 0;
    ::DWORD_PTR system_mask = 0;
    const bool ok = ::GetProcessAffinityMask(handle, &process_mask, &system_mask) != 0;
    ::CloseHandle(handle);
    if (!ok) {
        return none;
    }
    return static_cast<uint64_t>(process_mask);
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    if (::sched_getaffinity(static_cast<::pid_t>(process_id), sizeof(set), &set) != 0) {
        return none;
    }
    uint64_t mask = 0;
    for (int index = 0; index < 64 && index < CPU_SETSIZE; ++index) {
        if (CPU_ISSET(index, &set)) {
            mask |= (static_cast<uint64_t>(1) << index);
        }
    }
    return mask;
#endif
}

bool process::set_affinity(const native_id_type process_id, const uint64_t mask) noexcept {
    if (mask == 0) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::HANDLE handle = ::OpenProcess(PROCESS_SET_INFORMATION, FALSE, process_id);
    if (handle == nullptr) {
        return false;
    }
    const bool changed = ::SetProcessAffinityMask(handle, static_cast<::DWORD_PTR>(mask)) != 0;
    ::CloseHandle(handle);
    return changed;
#else
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int index = 0; index < 64 && index < CPU_SETSIZE; ++index) {
        if ((mask & (static_cast<uint64_t>(1) << index)) != 0) {
            CPU_SET(index, &set);
        }
    }
    return ::sched_setaffinity(static_cast<::pid_t>(process_id), sizeof(set), &set) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
