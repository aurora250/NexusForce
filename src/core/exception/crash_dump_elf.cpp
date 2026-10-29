#include "crash_dump_native.hpp"
#include <NeForce/core/algorithm/compare.hpp>
#include <NeForce/core/memory/memory.hpp>
#ifdef NEFORCE_SUPPORT_NATIVE_DUMP_POSIX
#    ifdef NEFORCE_SUPPORT_NATIVE_DUMP_LINUX
#        include <elf.h>
#        include <fcntl.h>
#        include <sys/procfs.h>
#        include <sys/syscall.h>
#        include <sys/ucontext.h>
#        include <unistd.h>
#        ifdef NEFORCE_ARCH_X86_64
#            include <asm/prctl.h>
#        endif
#    endif
#    ifdef NEFORCE_SUPPORT_NATIVE_DUMP_FREEBSD
#        include <fcntl.h>
#        include <sys/types.h>
#        include <sys/elf_common.h>
#        include <sys/elf64.h>
#        include <sys/procfs.h>
#        include <sys/sysctl.h>
#        include <sys/user.h>
#        include <ucontext.h>
#        include <unistd.h>
#    endif
#    include <cerrno>
#    include <csignal>
#    include <ctime>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

namespace {
    using inner::native_dump_request;
    using inner::native_dump_result;

    constexpr size_t max_segment_capacity = 4096;
    constexpr uint64_t max_segment_bytes = 512UL * 1024UL * 1024UL;
    constexpr size_t max_process_args = 80;

#    if defined(NEFORCE_SUPPORT_NATIVE_DUMP_LINUX)
    constexpr size_t max_maps_bytes = 512UL * 1024UL;
    constexpr size_t max_path_bytes = 128UL * 1024UL;
    constexpr size_t max_auxv_entries = 256;
    constexpr size_t max_file_name = 16;
    constexpr char note_name[] = "CORE";
    constexpr uint32_t note_type_prstatus = 1;
    constexpr uint32_t note_type_fpregset = 2;
    constexpr uint32_t note_type_prpsinfo = 3;
    constexpr uint32_t note_type_auxv = 6;
    constexpr uint32_t note_type_siginfo = 0x53494749U;
    constexpr uint32_t note_type_file = 0x46494c45U;
#    else
    constexpr size_t max_vmmap_bytes = 512UL * 1024UL;
    constexpr char note_name[] = "FreeBSD";
    constexpr uint32_t note_type_prstatus = NT_PRSTATUS;
    constexpr uint32_t note_type_prpsinfo = NT_PRPSINFO;
#    endif

    constexpr size_t note_name_size = sizeof(note_name);

    struct core_segment {
        uint64_t start{0};
        uint64_t end{0};
        uint64_t file_offset{0};
        uint64_t output_offset{0};
        uint32_t flags{0};
#    if defined(NEFORCE_SUPPORT_NATIVE_DUMP_LINUX)
        uint32_t path_offset{0};
        uint32_t path_size{0};
#    endif
    };

    struct writer_state {
        core_segment segments[max_segment_capacity]{};
        ::Elf64_Phdr program_headers[max_segment_capacity + 1]{};
        size_t segment_count{0};
        bool truncated{false};
#    if defined(NEFORCE_SUPPORT_NATIVE_DUMP_LINUX)
        ::Elf64_auxv_t auxv[max_auxv_entries]{};
        char maps[max_maps_bytes + 1]{};
        char paths[max_path_bytes + 1]{};
        size_t path_used{0};
        size_t auxv_count{0};
        uint64_t file_count{0};
        uint64_t file_name_bytes{0};
#    else
        char vmmap[max_vmmap_bytes + 1]{};
#    endif
    };

    writer_state& state() noexcept {
        static writer_state value{};
        return value;
    }

    constexpr uint64_t align_up(const uint64_t value, const uint64_t alignment) noexcept {
        return ((value + alignment - 1U) / alignment) * alignment;
    }

    constexpr uint64_t align4(const uint64_t value) noexcept { return align_up(value, 4U); }

    uint64_t page_size() noexcept { return static_cast<uint64_t>(::getpagesize()); }

    bool write_at(const int fd, uint64_t offset, const void* data, size_t size) noexcept {
        const auto* cursor = static_cast<const char*>(data);
        while (size > 0) {
            const ssize_t written = ::pwrite(fd, cursor, size, static_cast<off_t>(offset));
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return false;
            }
            if (written == 0) {
                return false;
            }
            const auto step = static_cast<size_t>(written);
            cursor += step;
            size -= step;
            offset += step;
        }
        return true;
    }

    bool write_zeros_at(const int fd, uint64_t offset, uint64_t size) noexcept {
        static constexpr char zeros[4096]{};
        while (size > 0) {
            const auto chunk = static_cast<size_t>(size < sizeof(zeros) ? size : sizeof(zeros));
            if (!write_at(fd, offset, zeros, chunk)) {
                return false;
            }
            offset += chunk;
            size -= chunk;
        }
        return true;
    }

    struct civil_time {
        int64_t year{1970};
        uint32_t month{1};
        uint32_t day{1};
        uint32_t hour{0};
        uint32_t minute{0};
        uint32_t second{0};
    };

    civil_time make_civil_time(const int64_t seconds) noexcept {
        civil_time result{};
        int64_t days = seconds / 86400;
        int64_t remainder = seconds % 86400;
        if (remainder < 0) {
            remainder += 86400;
            --days;
        }
        result.hour = static_cast<uint32_t>(remainder / 3600);
        result.minute = static_cast<uint32_t>((remainder % 3600) / 60);
        result.second = static_cast<uint32_t>(remainder % 60);

        const int64_t shifted = days + 719468;
        const int64_t era = (shifted >= 0 ? shifted : shifted - 146096) / 146097;
        const auto day_of_era = static_cast<uint32_t>(shifted - era * 146097);
        const uint32_t year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
        const int64_t year = static_cast<int64_t>(year_of_era) + era * 400;
        const uint32_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
        const uint32_t month_prime = (5 * day_of_year + 2) / 153;
        result.day = day_of_year - (153 * month_prime + 2) / 5 + 1;
        result.month = month_prime < 10 ? month_prime + 3 : month_prime - 9;
        result.year = year + (result.month <= 2 ? 1 : 0);
        return result;
    }

    size_t append_text(char* buffer, const size_t capacity, const size_t offset, const char* text) noexcept {
        size_t position = offset;
        while (*text != '\0' && position + 1 < capacity) {
            buffer[position++] = *text++;
        }
        buffer[position] = '\0';
        return position;
    }

    size_t append_number(char* buffer, const size_t capacity, const size_t offset, const unsigned long long value,
                         const int width) noexcept {
        char digits[24]{};
        int count = 0;
        unsigned long long rest = value;
        do {
            digits[count++] = static_cast<char>('0' + rest % 10U);
            rest /= 10U;
        } while (rest != 0U && count < static_cast<int>(sizeof(digits)));
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

    size_t make_timestamp(char* buffer, const size_t capacity, const int64_t utc_offset_seconds) noexcept {
        ::timespec now{};
        if (::clock_gettime(CLOCK_REALTIME, &now) != 0) {
            return append_text(buffer, capacity, 0, "19700101-000000");
        }
        const civil_time value = make_civil_time(static_cast<int64_t>(now.tv_sec) + utc_offset_seconds);
        size_t offset = 0;
        offset = append_number(buffer, capacity, offset, static_cast<unsigned long long>(value.year), 4);
        offset = append_number(buffer, capacity, offset, value.month, 2);
        offset = append_number(buffer, capacity, offset, value.day, 2);
        offset = append_text(buffer, capacity, offset, "-");
        offset = append_number(buffer, capacity, offset, value.hour, 2);
        offset = append_number(buffer, capacity, offset, value.minute, 2);
        offset = append_number(buffer, capacity, offset, value.second, 2);
        return offset;
    }

    int open_core_file(const char* directory, const char* prefix, const int64_t utc_offset_seconds, char* path,
                       const size_t capacity) noexcept {
        char stamp[32]{};
        make_timestamp(stamp, sizeof(stamp), utc_offset_seconds);
        const auto process_id = static_cast<unsigned long long>(::getpid());

        for (int attempt = 0; attempt < 32; ++attempt) {
            size_t offset = 0;
            path[0] = '\0';
            offset = append_text(path, capacity, offset, directory);
            if (offset > 0 && path[offset - 1] != '/') {
                offset = append_text(path, capacity, offset, "/");
            }
            offset = append_text(path, capacity, offset, prefix);
            offset = append_text(path, capacity, offset, "-");
            offset = append_text(path, capacity, offset, stamp);
            offset = append_text(path, capacity, offset, "-");
            offset = append_number(path, capacity, offset, process_id, 1);
            if (attempt > 0) {
                offset = append_text(path, capacity, offset, "-");
                offset = append_number(path, capacity, offset, static_cast<unsigned long long>(attempt), 1);
            }
            offset = append_text(path, capacity, offset, ".core");
            if (offset + 1 >= capacity) {
                path[0] = '\0';
                return -1;
            }
            const int fd = ::open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
            if (fd >= 0) {
                return fd;
            }
            if (errno != EEXIST) {
                return -1;
            }
        }
        path[0] = '\0';
        return -1;
    }

    bool write_note_prefix(const int fd, uint64_t& offset, const uint32_t type,
                           const uint32_t description_size) noexcept {
        ::Elf64_Nhdr header{};
        header.n_namesz = static_cast<uint32_t>(note_name_size);
        header.n_descsz = description_size;
        header.n_type = type;
        if (!write_at(fd, offset, &header, sizeof(header))) {
            return false;
        }
        offset += sizeof(header);
        if (!write_at(fd, offset, note_name, sizeof(note_name))) {
            return false;
        }
        offset += sizeof(note_name);
        constexpr uint64_t padding = align4(note_name_size) - note_name_size;
        if (!write_zeros_at(fd, offset, padding)) {
            return false;
        }
        offset += padding;
        return true;
    }

    bool write_note(const int fd, uint64_t& offset, const uint32_t type, const void* description,
                    const uint32_t description_size) noexcept {
        if (!write_note_prefix(fd, offset, type, description_size)) {
            return false;
        }
        if (description_size > 0 && !write_at(fd, offset, description, description_size)) {
            return false;
        }
        offset += description_size;
        const uint64_t padding = align4(description_size) - description_size;
        if (!write_zeros_at(fd, offset, padding)) {
            return false;
        }
        offset += padding;
        return true;
    }

    constexpr uint64_t note_size(const uint32_t description_size) noexcept {
        return sizeof(::Elf64_Nhdr) + align4(note_name_size) + align4(description_size);
    }

    bool write_segment(const int fd, writer_state& current, const core_segment& item) noexcept {
        uint64_t offset = item.output_offset;
        uint64_t remaining = item.end - item.start;
        const auto* cursor = reinterpret_cast<const char*>(static_cast<uintptr_t>(item.start));
        while (remaining > 0) {
            const auto chunk = static_cast<size_t>(remaining < 1024UL * 1024UL ? remaining : 1024UL * 1024UL);
            if (!write_at(fd, offset, cursor, chunk)) {
                if (errno == EFAULT || errno == EIO) {
                    current.truncated = true;
                    return write_zeros_at(fd, offset, remaining);
                }
                return false;
            }
            offset += chunk;
            cursor += chunk;
            remaining -= chunk;
        }
        return true;
    }

    constexpr unsigned char elf_osabi() noexcept {
#    if defined(NEFORCE_SUPPORT_NATIVE_DUMP_FREEBSD)
        return ELFOSABI_FREEBSD;
#    else
        return ELFOSABI_NONE;
#    endif
    }

#    if defined(NEFORCE_SUPPORT_NATIVE_DUMP_LINUX)

    ssize_t read_file(const char* file, void* data, const size_t capacity) noexcept {
        const int fd = ::open(file, O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            return -1;
        }
        size_t used = 0;
        while (used < capacity) {
            const ssize_t count = ::read(fd, static_cast<char*>(data) + used, capacity - used);
            if (count < 0) {
                if (errno == EINTR) {
                    continue;
                }
                ::close(fd);
                return -1;
            }
            if (count == 0) {
                break;
            }
            used += static_cast<size_t>(count);
        }
        ::close(fd);
        return static_cast<ssize_t>(used);
    }

    bool parse_hex(const char*& cursor, uint64_t& value) noexcept {
        while (*cursor == ' ') {
            ++cursor;
        }
        uint64_t parsed = 0;
        bool found = false;
        for (;;) {
            const char character = *cursor;
            int digit = -1;
            if (character >= '0' && character <= '9') {
                digit = character - '0';
            } else if (character >= 'a' && character <= 'f') {
                digit = character - 'a' + 10;
            } else if (character >= 'A' && character <= 'F') {
                digit = character - 'A' + 10;
            } else {
                break;
            }
            parsed = (parsed << 4U) | static_cast<uint64_t>(digit);
            ++cursor;
            found = true;
        }
        value = parsed;
        return found;
    }

    void skip_token(const char*& cursor) noexcept {
        while (*cursor == ' ') {
            ++cursor;
        }
        while (*cursor != '\0' && *cursor != ' ') {
            ++cursor;
        }
        while (*cursor == ' ') {
            ++cursor;
        }
    }

    bool skipped_path(const char* path, const size_t size) noexcept {
        if (path == nullptr) {
            return true;
        }
        if (size == 0) {
            return false;
        }
        if (path[0] == '[') {
            return (size >= 6 && string_compare(path, "[vvar]", 6) == 0) ||
                   (size >= 10 && string_compare(path, "[vsyscall]", 10) == 0);
        }
        if (path[0] != '/') {
            return false;
        }
        return (size >= 5 && string_compare(path, "/dev/", 5) == 0) ||
               (size >= 5 && string_compare(path, "/sys/", 5) == 0) ||
               (size >= 6 && string_compare(path, "/proc/", 6) == 0);
    }

    void parse_map_line(writer_state& current, char* line, const bool include_memory,
                        const uint32_t max_segments) noexcept {
        const char* cursor = line;
        uint64_t start = 0;
        uint64_t end = 0;
        uint64_t file_offset = 0;
        if (!parse_hex(cursor, start) || *cursor != '-') {
            return;
        }
        ++cursor;
        if (!parse_hex(cursor, end) || *cursor != ' ') {
            return;
        }
        ++cursor;
        if (string_length(cursor) < 4) {
            return;
        }
        const bool readable = cursor[0] == 'r';
        const bool writable = cursor[1] == 'w';
        const bool executable = cursor[2] == 'x';
        cursor += 4;
        if (*cursor == ' ') {
            ++cursor;
        }
        if (!parse_hex(cursor, file_offset)) {
            return;
        }
        skip_token(cursor); // device
        skip_token(cursor); // inode
        const char* path = cursor;
        const size_t path_size = string_length(path);
        if (end <= start || !readable || skipped_path(path, path_size)) {
            return;
        }

        const uint64_t length = end - start;
        if (current.segment_count >= max_segment_capacity ||
            (include_memory && current.segment_count >= max_segments)) {
            current.truncated = true;
            return;
        }
        if (include_memory && length > max_segment_bytes) {
            current.truncated = true;
            return;
        }

        auto& item = current.segments[current.segment_count];
        item.start = start;
        item.end = end;
        item.file_offset = file_offset;
        item.flags = static_cast<uint32_t>((readable ? PF_R : 0U) | (writable ? PF_W : 0U) | (executable ? PF_X : 0U));
        item.path_offset = 0;
        item.path_size = 0;
        if (path_size > 0 && path[0] == '/') {
            if (current.path_used + path_size + 1 < sizeof(current.paths)) {
                memory_copy(current.paths + current.path_used, path, path_size);
                current.paths[current.path_used + path_size] = '\0';
                item.path_offset = static_cast<uint32_t>(current.path_used);
                item.path_size = static_cast<uint32_t>(path_size);
                current.path_used += path_size + 1;
                ++current.file_count;
                current.file_name_bytes += path_size + 1;
            } else {
                current.truncated = true;
            }
        }
        ++current.segment_count;
    }

    void load_segments(writer_state& current, const native_dump_request& request) noexcept {
        memory_zero(current.maps, sizeof(current.maps));
        memory_zero(current.paths, sizeof(current.paths));
        current.segment_count = 0;
        current.path_used = 0;
        current.auxv_count = 0;
        current.file_count = 0;
        current.file_name_bytes = 0;
        current.truncated = false;

        const ssize_t count = read_file("/proc/self/maps", current.maps, max_maps_bytes);
        if (count <= 0) {
            current.truncated = true;
        } else {
            current.maps[static_cast<size_t>(count)] = '\0';
            const uint32_t max_segments =
                    request.max_segments == 0 ? static_cast<uint32_t>(max_segment_capacity) : request.max_segments;
            char* cursor = current.maps;
            while (*cursor != '\0') {
                char* end = cursor;
                while (*end != '\0' && *end != '\n') {
                    ++end;
                }
                const bool last = *end == '\0';
                *end = '\0';
                parse_map_line(current, cursor, request.include_memory, max_segments);
                if (last) {
                    break;
                }
                cursor = end + 1;
            }
        }

        const ssize_t auxv_size = read_file("/proc/self/auxv", current.auxv, sizeof(current.auxv));
        if (auxv_size > 0) {
            current.auxv_count = static_cast<size_t>(auxv_size) / sizeof(::Elf64_auxv_t);
        }
    }

#        ifdef NEFORCE_ARCH_X86_64
    void fill_registers(::elf_gregset_t& registers, const ucontext_t* context) noexcept {
        memory_zero(registers, sizeof(::elf_gregset_t));
        if (context == nullptr) {
            return;
        }
        const auto& gregs = context->uc_mcontext.gregs;
        registers[0] = static_cast<::elf_greg_t>(gregs[REG_R15]);
        registers[1] = static_cast<::elf_greg_t>(gregs[REG_R14]);
        registers[2] = static_cast<::elf_greg_t>(gregs[REG_R13]);
        registers[3] = static_cast<::elf_greg_t>(gregs[REG_R12]);
        registers[4] = static_cast<::elf_greg_t>(gregs[REG_RBP]);
        registers[5] = static_cast<::elf_greg_t>(gregs[REG_RBX]);
        registers[6] = static_cast<::elf_greg_t>(gregs[REG_R11]);
        registers[7] = static_cast<::elf_greg_t>(gregs[REG_R10]);
        registers[8] = static_cast<::elf_greg_t>(gregs[REG_R9]);
        registers[9] = static_cast<::elf_greg_t>(gregs[REG_R8]);
        registers[10] = static_cast<::elf_greg_t>(gregs[REG_RAX]);
        registers[11] = static_cast<::elf_greg_t>(gregs[REG_RCX]);
        registers[12] = static_cast<::elf_greg_t>(gregs[REG_RDX]);
        registers[13] = static_cast<::elf_greg_t>(gregs[REG_RSI]);
        registers[14] = static_cast<::elf_greg_t>(gregs[REG_RDI]);
        registers[15] = 0; // orig_rax has no ucontext slot
        registers[16] = static_cast<::elf_greg_t>(gregs[REG_RIP]);
        registers[17] = static_cast<::elf_greg_t>(gregs[REG_CSGSFS] & 0xffffU);
        registers[18] = static_cast<::elf_greg_t>(gregs[REG_EFL]);
        registers[19] = static_cast<::elf_greg_t>(gregs[REG_RSP]);
        registers[20] = 0; // ss has no ucontext slot
        unsigned long fs_base = 0;
        unsigned long gs_base = 0;
        static_cast<void>(::syscall(SYS_arch_prctl, ARCH_GET_FS, &fs_base));
        static_cast<void>(::syscall(SYS_arch_prctl, ARCH_GET_GS, &gs_base));
        registers[21] = static_cast<::elf_greg_t>(fs_base);
        registers[22] = static_cast<::elf_greg_t>(gs_base);
    }
#        elif defined(NEFORCE_ARCH_AARCH64)
    void fill_registers(::elf_gregset_t& registers, const ::ucontext_t* context) noexcept {
        memory_zero(registers, sizeof(::elf_gregset_t));
        if (context == nullptr) {
            return;
        }
        for (size_t index = 0; index < 31 && index < ELF_NGREG; ++index) {
            registers[index] = static_cast<::elf_greg_t>(context->uc_mcontext.regs[index]);
        }
        NEFORCE_IF_CONSTEXPR(ELF_NGREG > 31) { registers[31] = static_cast<::elf_greg_t>(context->uc_mcontext.sp); }
        NEFORCE_IF_CONSTEXPR(ELF_NGREG > 32) { registers[32] = static_cast<::elf_greg_t>(context->uc_mcontext.pc); }
        NEFORCE_IF_CONSTEXPR(ELF_NGREG > 33) { registers[33] = static_cast<::elf_greg_t>(context->uc_mcontext.pstate); }
    }
#        endif

    void fill_process_info(::elf_prpsinfo& info) noexcept {
        memory_zero(&info);
        info.pr_state = 'R';
        info.pr_sname = 'R';
        info.pr_zomb = 0;
        info.pr_nice = 0;
        info.pr_flag = 0;
        info.pr_uid = static_cast<decltype(info.pr_uid)>(::getuid());
        info.pr_gid = static_cast<decltype(info.pr_gid)>(::getgid());
        info.pr_pid = ::getpid();
        info.pr_ppid = ::getppid();
        info.pr_pgrp = ::getpgrp();
        info.pr_sid = ::getsid(0);

        char name[max_file_name]{};
        const ssize_t name_size = read_file("/proc/self/comm", name, sizeof(name) - 1);
        if (name_size > 0) {
            auto length = static_cast<size_t>(name_size);
            while (length > 0 && (name[length - 1] == '\n' || name[length - 1] == '\r')) {
                --length;
            }
            length = min(length, sizeof(info.pr_fname) - 1);
            memory_copy(info.pr_fname, name, length);
        }

        char args[max_process_args]{};
        const ssize_t args_size = read_file("/proc/self/cmdline", args, sizeof(args) - 1);
        if (args_size > 0) {
            auto length = static_cast<size_t>(args_size);
            length = min(length, sizeof(info.pr_psargs) - 1);
            for (size_t index = 0; index < length; ++index) {
                info.pr_psargs[index] = args[index] == '\0' ? ' ' : args[index];
            }
        }
    }

    void fill_status(::elf_prstatus& status, const int32_t signal_number, const ::siginfo_t* signal_info,
                     const ::ucontext_t* context) noexcept {
        memory_zero(&status);
        if (signal_info != nullptr) {
            status.pr_info.si_signo = signal_info->si_signo;
            status.pr_info.si_code = signal_info->si_code;
            status.pr_info.si_errno = signal_info->si_errno;
        } else {
            status.pr_info.si_signo = signal_number;
        }
        status.pr_cursig = static_cast<short>(signal_number);
        status.pr_pid = static_cast<::pid_t>(::syscall(SYS_gettid));
        status.pr_ppid = ::getppid();
        status.pr_pgrp = ::getpgrp();
        status.pr_sid = ::getsid(0);
        fill_registers(status.pr_reg, context);
        status.pr_fpvalid = 0;
#        ifdef NEFORCE_ARCH_X86_64
        if (context != nullptr && context->uc_mcontext.fpregs != nullptr) {
            status.pr_fpvalid = 1;
        }
#        endif
    }

    bool write_file_note(const int fd, uint64_t& offset, const writer_state& current) noexcept {
        const uint64_t description_size =
                current.file_count * (3U * sizeof(uint64_t)) + 2U * sizeof(uint64_t) + current.file_name_bytes;
        if (current.file_count == 0 || description_size > 0xffffffffULL) {
            return true;
        }
        if (!write_note_prefix(fd, offset, note_type_file, static_cast<uint32_t>(description_size))) {
            return false;
        }
        const uint64_t page = page_size();
        if (!write_at(fd, offset, &current.file_count, sizeof(current.file_count))) {
            return false;
        }
        offset += sizeof(current.file_count);
        if (!write_at(fd, offset, &page, sizeof(page))) {
            return false;
        }
        offset += sizeof(page);
        for (size_t index = 0; index < current.segment_count; ++index) {
            const auto& item = current.segments[index];
            if (item.path_size == 0) {
                continue;
            }
            const uint64_t values[3] = {item.start, item.end, item.file_offset / page};
            if (!write_at(fd, offset, values, sizeof(values))) {
                return false;
            }
            offset += sizeof(values);
        }
        for (size_t index = 0; index < current.segment_count; ++index) {
            const auto& item = current.segments[index];
            if (item.path_size == 0) {
                continue;
            }
            if (!write_at(fd, offset, current.paths + item.path_offset, item.path_size)) {
                return false;
            }
            offset += item.path_size;
            constexpr char terminator = '\0';
            if (!write_at(fd, offset, &terminator, 1)) {
                return false;
            }
            offset += 1;
        }
        const uint64_t padding = align4(description_size) - description_size;
        if (!write_zeros_at(fd, offset, padding)) {
            return false;
        }
        offset += padding;
        return true;
    }

    struct core_notes {
        ::elf_prstatus thread{};
        ::elf_prpsinfo process{};
        ::siginfo_t crash{};
        ::elf_fpregset_t fpregs{};
        bool has_fpregs{false};
    };

    void prepare_notes(core_notes& notes, const native_dump_request& request,
                       const writer_state& /*current*/) noexcept {
        const auto* signal_info = static_cast<const ::siginfo_t*>(request.signal_info);
        const auto* context = static_cast<const ::ucontext_t*>(request.context);
        fill_status(notes.thread, request.signal_number, signal_info, context);
        fill_process_info(notes.process);
        if (signal_info != nullptr) {
            notes.crash = *signal_info;
        } else {
            notes.crash.si_signo = request.signal_number;
        }
#        ifdef NEFORCE_ARCH_X86_64
        if (context != nullptr && context->uc_mcontext.fpregs != nullptr) {
            memory_copy(&notes.fpregs, context->uc_mcontext.fpregs, sizeof(notes.fpregs));
            notes.has_fpregs = true;
        }
#        endif
    }

    uint64_t notes_size(const core_notes& notes, const writer_state& current) noexcept {
        const uint64_t file_description =
                current.file_count * (3U * sizeof(uint64_t)) + 2U * sizeof(uint64_t) + current.file_name_bytes;
        uint64_t size = note_size(static_cast<uint32_t>(sizeof(::elf_prstatus))) +
                        note_size(static_cast<uint32_t>(sizeof(::elf_prpsinfo))) +
                        note_size(static_cast<uint32_t>(sizeof(::siginfo_t)));
        if (notes.has_fpregs) {
            size += note_size(static_cast<uint32_t>(sizeof(::elf_fpregset_t)));
        }
        if (current.auxv_count > 0) {
            size += note_size(static_cast<uint32_t>(current.auxv_count * sizeof(::Elf64_auxv_t)));
        }
        if (current.file_count > 0) {
            size += note_size(static_cast<uint32_t>(file_description));
        }
        return size;
    }

    bool write_notes(const int fd, uint64_t& cursor, const core_notes& notes, const writer_state& current) noexcept {
        bool ok =
                write_note(fd, cursor, note_type_prstatus, &notes.thread, static_cast<uint32_t>(sizeof(notes.thread)));
        if (ok) {
            ok = write_note(fd, cursor, note_type_prpsinfo, &notes.process,
                            static_cast<uint32_t>(sizeof(notes.process)));
        }
        if (ok && notes.has_fpregs) {
            ok = write_note(fd, cursor, note_type_fpregset, &notes.fpregs, static_cast<uint32_t>(sizeof(notes.fpregs)));
        }
        if (ok) {
            ok = write_note(fd, cursor, note_type_siginfo, &notes.crash, static_cast<uint32_t>(sizeof(notes.crash)));
        }
        if (ok && current.auxv_count > 0) {
            ok = write_note(fd, cursor, note_type_auxv, current.auxv,
                            static_cast<uint32_t>(current.auxv_count * sizeof(::Elf64_auxv_t)));
        }
        if (ok) {
            ok = write_file_note(fd, cursor, current);
        }
        return ok;
    }

#    else // NEFORCE_SUPPORT_NATIVE_DUMP_FREEBSD

    int32_t kernel_release_date() noexcept {
        int value = 0;
        size_t size = sizeof(value);
        constexpr int mib[2] = {CTL_KERN, KERN_OSRELDATE};
        if (::sysctl(mib, 2, &value, &size, nullptr, 0) != 0) {
            return 0;
        }
        return static_cast<int32_t>(value);
    }

    void load_segments(writer_state& current, const native_dump_request& request) noexcept {
        current.segment_count = 0;
        current.truncated = false;

        const int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_VMMAP, ::getpid()};
        size_t required = 0;
        if (::sysctl(mib, 4, nullptr, &required, nullptr, 0) != 0) {
            current.truncated = true;
            return;
        }
        size_t size = required;
        if (size > sizeof(current.vmmap)) {
            size = sizeof(current.vmmap);
            current.truncated = true;
        }
        memory_zero(current.vmmap, size);
        if (size == 0 || (::sysctl(mib, 4, current.vmmap, &size, nullptr, 0) != 0 && errno != ENOMEM)) {
            current.truncated = true;
            return;
        }

        const uint32_t max_segments =
                request.max_segments == 0 ? static_cast<uint32_t>(max_segment_capacity) : request.max_segments;
        constexpr size_t minimal_record = offsetof(::kinfo_vmentry, kve_protection) + sizeof(int);
        size_t offset = 0;
        while (offset + minimal_record <= size) {
            const auto* entry = reinterpret_cast<const ::kinfo_vmentry*>(current.vmmap + offset);
            const int record_size = entry->kve_structsize;
            if (record_size < static_cast<int>(minimal_record) || offset + static_cast<size_t>(record_size) > size) {
                current.truncated = true;
                break;
            }
            offset += static_cast<size_t>(record_size);

            const uint64_t start = entry->kve_start;
            const uint64_t end = entry->kve_end;
            if (end <= start || (entry->kve_protection & KVME_PROT_READ) == 0) {
                continue;
            }
            const uint64_t length = end - start;
            if (current.segment_count >= max_segment_capacity ||
                (request.include_memory && (current.segment_count >= max_segments || length > max_segment_bytes))) {
                current.truncated = true;
                continue;
            }
            auto& item = current.segments[current.segment_count];
            item.start = start;
            item.end = end;
            item.file_offset = entry->kve_offset;
            item.flags = static_cast<uint32_t>(((entry->kve_protection & KVME_PROT_READ) != 0 ? PF_R : 0U) |
                                               ((entry->kve_protection & KVME_PROT_WRITE) != 0 ? PF_W : 0U) |
                                               ((entry->kve_protection & KVME_PROT_EXEC) != 0 ? PF_X : 0U));
            ++current.segment_count;
        }
    }

    void fill_registers(::gregset_t& registers, const ::ucontext_t* context) noexcept {
        memory_zero(&registers);
        if (context == nullptr) {
            return;
        }
        const auto& machine = context->uc_mcontext;
#        if defined(NEFORCE_ARCH_X86_64)
        registers.r_r15 = machine.mc_r15;
        registers.r_r14 = machine.mc_r14;
        registers.r_r13 = machine.mc_r13;
        registers.r_r12 = machine.mc_r12;
        registers.r_r11 = machine.mc_r11;
        registers.r_r10 = machine.mc_r10;
        registers.r_r9 = machine.mc_r9;
        registers.r_r8 = machine.mc_r8;
        registers.r_rdi = machine.mc_rdi;
        registers.r_rsi = machine.mc_rsi;
        registers.r_rbp = machine.mc_rbp;
        registers.r_rbx = machine.mc_rbx;
        registers.r_rdx = machine.mc_rdx;
        registers.r_rcx = machine.mc_rcx;
        registers.r_rax = machine.mc_rax;
        registers.r_trapno = machine.mc_trapno;
        registers.r_fs = machine.mc_fs;
        registers.r_gs = machine.mc_gs;
        registers.r_err = machine.mc_err;
        registers.r_es = machine.mc_es;
        registers.r_ds = machine.mc_ds;
        registers.r_rip = machine.mc_rip;
        registers.r_cs = machine.mc_cs;
        registers.r_rflags = machine.mc_rflags;
        registers.r_rsp = machine.mc_rsp;
        registers.r_ss = machine.mc_ss;
#        elif defined(NEFORCE_ARCH_AARCH64)
        const auto& general = machine.mc_gpregs;
        for (size_t index = 0; index < 30; ++index) {
            registers.x[index] = general.gp_x[index];
        }
        registers.lr = general.gp_lr;
        registers.sp = general.gp_sp;
        registers.elr = general.gp_elr;
        registers.spsr = general.gp_spsr;
#        endif
    }

    void fill_process_info(::prpsinfo_t& info) noexcept {
        memory_zero(&info);
        info.pr_version = PRPSINFO_VERSION;
        info.pr_psinfosz = sizeof(::prpsinfo_t);
        info.pr_pid = ::getpid();

        const char* name = ::getprogname();
        if (name != nullptr) {
            size_t length = 0;
            while (name[length] != '\0' && length + 1 < sizeof(info.pr_fname)) {
                info.pr_fname[length] = name[length];
                ++length;
            }
        }

        char args[max_process_args * 2]{};
        const int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ARGS, ::getpid()};
        size_t size = sizeof(args);
        if (::sysctl(mib, 4, args, &size, nullptr, 0) == 0 && size > 0) {
            const size_t length = min(size, sizeof(info.pr_psargs) - 1);
            for (size_t index = 0; index < length; ++index) {
                info.pr_psargs[index] = args[index] == '\0' ? ' ' : args[index];
            }
        }
    }

    void fill_status(::prstatus_t& status, const native_dump_request& request, const ::ucontext_t* context) noexcept {
        memory_zero(&status);
        status.pr_version = PRSTATUS_VERSION;
        status.pr_statussz = sizeof(::prstatus_t);
        status.pr_gregsetsz = sizeof(::gregset_t);
        status.pr_fpregsetsz = sizeof(::fpregset_t);
        status.pr_osreldate = kernel_release_date();
        status.pr_cursig = request.signal_number;
        status.pr_pid = static_cast<::pid_t>(request.thread_id);
        fill_registers(status.pr_reg, context);
    }

    struct core_notes {
        ::prpsinfo_t process{};
        ::prstatus_t thread{};
    };

    void prepare_notes(core_notes& notes, const native_dump_request& request,
                       const writer_state& /*current*/) noexcept {
        fill_process_info(notes.process);
        fill_status(notes.thread, request, static_cast<const ::ucontext_t*>(request.context));
    }

    uint64_t notes_size(const core_notes& /*notes*/, const writer_state& /*current*/) noexcept {
        return note_size(static_cast<uint32_t>(sizeof(::prpsinfo_t))) +
               note_size(static_cast<uint32_t>(sizeof(::prstatus_t)));
    }

    bool write_notes(const int fd, uint64_t& cursor, const core_notes& notes,
                     const writer_state& /*current*/) noexcept {
        return write_note(fd, cursor, note_type_prpsinfo, &notes.process,
                          static_cast<uint32_t>(sizeof(notes.process))) &&
               write_note(fd, cursor, note_type_prstatus, &notes.thread, static_cast<uint32_t>(sizeof(notes.thread)));
    }

#    endif

} // namespace


bool write_native_dump(const native_dump_request& request, native_dump_result& result) noexcept {
    result = native_dump_result{};
    if (request.directory == nullptr || request.directory[0] == '\0' || request.prefix == nullptr) {
        return false;
    }

    const int fd = open_core_file(request.directory, request.prefix, request.utc_offset_seconds, result.file,
                                  sizeof(result.file));
    if (fd < 0) {
        result.error = errno;
        result.file[0] = '\0';
        return false;
    }

    auto& current = state();
    load_segments(current, request);

    core_notes notes{};
    prepare_notes(notes, request, current);

    const uint64_t page = page_size();
    constexpr uint64_t note_alignment = 8U;

    const auto layout_size = [&]() noexcept -> uint64_t {
        const size_t headers = (request.include_memory ? current.segment_count : 0U) + 1U;
        const uint64_t start = align_up(sizeof(::Elf64_Ehdr) + headers * sizeof(::Elf64_Phdr), note_alignment);
        uint64_t end = align_up(start + notes_size(notes, current), page);
        if (request.include_memory) {
            for (size_t index = 0; index < current.segment_count; ++index) {
                end = align_up(end + (current.segments[index].end - current.segments[index].start), page);
            }
        }
        return end;
    };

    // Drop trailing mappings until the dump fits the caller's budget.
    uint64_t total_size = layout_size();
    while (request.include_memory && request.max_file_bytes != 0 && total_size > request.max_file_bytes &&
           current.segment_count > 0) {
#    if defined(NEFORCE_SUPPORT_NATIVE_DUMP_LINUX)
        auto& dropped = current.segments[current.segment_count - 1];
        if (dropped.path_size > 0) {
            --current.file_count;
            current.file_name_bytes -= static_cast<uint64_t>(dropped.path_size) + 1U;
        }
#    endif
        --current.segment_count;
        current.truncated = true;
        total_size = layout_size();
    }

    const size_t load_count = request.include_memory ? current.segment_count : 0U;
    const size_t header_count = load_count + 1U;
    const uint64_t note_start = align_up(sizeof(::Elf64_Ehdr) + header_count * sizeof(::Elf64_Phdr), note_alignment);
    uint64_t output_offset = align_up(note_start + notes_size(notes, current), page);
    for (size_t index = 0; index < load_count; ++index) {
        current.segments[index].output_offset = output_offset;
        output_offset += current.segments[index].end - current.segments[index].start;
        output_offset = align_up(output_offset, page);
    }
    const uint64_t final_size = output_offset;

    ::Elf64_Ehdr header{};
    memory_copy(header.e_ident, ELFMAG, SELFMAG);
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = EV_CURRENT;
    header.e_ident[EI_OSABI] = elf_osabi();
    header.e_type = ET_CORE;
#    if defined(NEFORCE_ARCH_X86_64)
    header.e_machine = EM_X86_64;
#    elif defined(NEFORCE_ARCH_AARCH64)
    header.e_machine = EM_AARCH64;
#    endif
    header.e_version = EV_CURRENT;
    header.e_phoff = sizeof(::Elf64_Ehdr);
    header.e_ehsize = sizeof(::Elf64_Ehdr);
    header.e_phentsize = sizeof(::Elf64_Phdr);
    header.e_phnum = static_cast<::Elf64_Half>(header_count);

    current.program_headers[0] = ::Elf64_Phdr{};
    current.program_headers[0].p_type = PT_NOTE;
    current.program_headers[0].p_offset = note_start;
    current.program_headers[0].p_filesz = notes_size(notes, current);
    current.program_headers[0].p_memsz = current.program_headers[0].p_filesz;
    current.program_headers[0].p_align = 4;

    for (size_t index = 0; index < load_count; ++index) {
        auto& target = current.program_headers[index + 1];
        target = ::Elf64_Phdr{};
        target.p_type = PT_LOAD;
        target.p_flags = current.segments[index].flags;
        target.p_offset = current.segments[index].output_offset;
        target.p_vaddr = current.segments[index].start;
        target.p_paddr = 0;
        target.p_filesz = current.segments[index].end - current.segments[index].start;
        target.p_memsz = target.p_filesz;
        target.p_align = page;
    }

    bool ok = write_at(fd, 0, &header, sizeof(header)) &&
              write_at(fd, sizeof(header), current.program_headers, header_count * sizeof(::Elf64_Phdr));

    uint64_t cursor = note_start;
    if (ok) {
        ok = write_notes(fd, cursor, notes, current);
    }
    if (ok && request.include_memory) {
        for (size_t index = 0; index < load_count; ++index) {
            if (!write_segment(fd, current, current.segments[index])) {
                ok = false;
                break;
            }
        }
    }
    if (ok) {
        ok = ::ftruncate(fd, static_cast<::off_t>(final_size)) == 0;
        if (!ok) {
            result.error = errno;
        }
    } else if (result.error == 0) {
        result.error = errno;
    }

    result.written = ok;
    if (!ok) {
        result.file[0] = '\0';
    }
    result.bytes = final_size;
    result.segments = static_cast<uint32_t>(load_count);
    result.truncated = current.truncated;
    ::close(fd);
    return ok;
}

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
#endif
