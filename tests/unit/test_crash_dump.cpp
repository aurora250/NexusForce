#include <NeForce/core/exception/crash_dump.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/string/string.hpp>
#include <csignal>
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_POSIX
#    include <sys/resource.h>
#    include <unistd.h>
#endif
#if NEFORCE_SUPPORT_NATIVE_DUMP_LINUX
#    include <elf.h>
#endif
#if NEFORCE_SUPPORT_NATIVE_DUMP_FREEBSD
#    include <sys/types.h>
#    include <sys/elf_common.h>
#    include <sys/elf64.h>
#    include <sys/procfs.h>
#endif
using namespace neforce;

namespace {

    bool native_writer_available() {
#if NEFORCE_SUPPORT_NATIVE_DUMP
        return true;
#else
        return false;
#endif
    }

    string read_all(const path& target) {
        if (!target.exists()) {
            return string{};
        }
        file input(target, false, file_access::read, file_shared::share_read_write);
        if (!input.is_opened()) {
            return string{};
        }
        return input.read_binary();
    }

    struct captured_report {
        bool called{false};
        crash_dump::reason reason{crash_dump::reason::manual};
        bool written{false};
        string message;
        string file;
    };

    captured_report g_capture;

    void capture_report(const crash_dump::report& report, void*) noexcept {
        g_capture.called = true;
        g_capture.reason = report.reason;
        g_capture.written = report.written;
        g_capture.message = report.message;
        g_capture.file = report.file;
    }

    path g_death_directory;
    path g_death_marker;

    void record_crash(const crash_dump::report& report, void*) noexcept {
        string content{report.file};
        content.append("\n");
        content.append(report.message);
        static_cast<void>(filesystem::create_and_write(g_death_marker, content, false));
    }

} // namespace

class CrashDumpTest : public ::testing::Test {
protected:
    void SetUp() override {
        directory_ = path::temp_directory_path() / path("neforce_crash_dump_test");
        static_cast<void>(filesystem::remove_all(directory_));
        static_cast<void>(filesystem::create_directories(directory_));
    }

    void TearDown() override { static_cast<void>(filesystem::remove_all(directory_)); }

    path directory_;
};

TEST_F(CrashDumpTest, FormatAndReasonNames) {
    EXPECT_EQ(to_string(crash_dump::format::automatic), "automatic");
    EXPECT_EQ(to_string(crash_dump::format::minidump), "minidump");
    EXPECT_EQ(to_string(crash_dump::format::elf_core), "elf-core");
    EXPECT_EQ(to_string(crash_dump::reason::manual), "manual");
    EXPECT_EQ(to_string(crash_dump::reason::fatal_signal), "fatal-signal");
    EXPECT_EQ(to_string(crash_dump::reason::unhandled_exception), "unhandled-exception");
    EXPECT_EQ(to_string(crash_dump::reason::structured_exception), "structured-exception");
}

TEST_F(CrashDumpTest, NativeFormatMatchesPlatform) {
#if NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(crash_dump::native_format(), crash_dump::format::minidump);
#else
    EXPECT_EQ(crash_dump::native_format(), crash_dump::format::elf_core);
#endif
    EXPECT_EQ(crash_dump::native_supported(), native_writer_available());
}

TEST_F(CrashDumpTest, ManualDumpWritesNativeFile) {
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "manual";
    options.include_memory = false;

    const auto result = crash_dump::write(options);
    ASSERT_EQ(result.written, native_writer_available());
    EXPECT_EQ(result.format, crash_dump::native_format());
    if (!native_writer_available()) {
        return;
    }
    ASSERT_TRUE(result.file.exists());
    EXPECT_EQ(result.error, 0);
    EXPECT_EQ(result.bytes, static_cast<uint64_t>(filesystem::size(result.file).bytes()));
    EXPECT_TRUE(result.file.filename().contains("manual-"));
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(result.file.extension(), "dmp");
#else
    EXPECT_EQ(result.file.extension(), "core");
#endif
}

TEST_F(CrashDumpTest, ManualDumpProducesValidElfCore) {
#ifdef NEFORCE_SUPPORT_NATIVE_DUMP_POSIX
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "elf";
    options.include_memory = false;

    const auto result = crash_dump::write(options);
    ASSERT_TRUE(result.written);

    file input(result.file, false, file_access::read, file_shared::share_read_write);
    ASSERT_TRUE(input.is_opened());
    Elf64_Ehdr header{};
    ASSERT_EQ(input.read_binary(&header, sizeof(header)), sizeof(header));
    EXPECT_EQ(memory_compare(header.e_ident, ELFMAG, SELFMAG), 0);
    EXPECT_EQ(static_cast<int>(header.e_ident[EI_CLASS]), ELFCLASS64);
    EXPECT_EQ(static_cast<int>(header.e_ident[EI_DATA]), ELFDATA2LSB);
    EXPECT_EQ(static_cast<int>(header.e_type), ET_CORE);
#    ifdef NEFORCE_SUPPORT_NATIVE_DUMP_FREEBSD
    EXPECT_EQ(static_cast<int>(header.e_ident[EI_OSABI]), ELFOSABI_FREEBSD);
#    else
    EXPECT_EQ(static_cast<int>(header.e_ident[EI_OSABI]), ELFOSABI_NONE);
#    endif
#    if defined(NEFORCE_ARCH_X86_64)
    EXPECT_EQ(static_cast<int>(header.e_machine), EM_X86_64);
#    else
    EXPECT_EQ(static_cast<int>(header.e_machine), EM_AARCH64);
#    endif
    EXPECT_GE(static_cast<int>(header.e_phnum), 1);
    EXPECT_EQ(header.e_phoff, sizeof(Elf64_Ehdr));
    EXPECT_EQ(header.e_phentsize, sizeof(Elf64_Phdr));

    Elf64_Phdr note{};
    ASSERT_EQ(header.e_phnum >= 1, true);
    ASSERT_EQ(input.read_binary(&note, sizeof(note)), sizeof(note));
    EXPECT_EQ(static_cast<int>(note.p_type), PT_NOTE);
    EXPECT_GT(note.p_filesz, 0U);
    EXPECT_LE(note.p_offset + note.p_filesz, result.bytes);
#else
    GTEST_SKIP() << "skip in platform without native dump writer";
#endif
}

TEST_F(CrashDumpTest, ManualDumpUsesFreebsdNotes) {
#if NEFORCE_SUPPORT_NATIVE_DUMP_FREEBSD
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "freebsd";
    options.include_memory = false;

    const auto result = crash_dump::write(options);
    ASSERT_TRUE(result.written);

    file input(result.file, false, file_access::read, file_shared::share_read_write);
    ASSERT_TRUE(input.is_opened());
    Elf64_Ehdr header{};
    ASSERT_EQ(input.read_binary(&header, sizeof(header)), sizeof(header));

    Elf64_Phdr note{};
    ASSERT_EQ(input.read_binary(&note, sizeof(note)), sizeof(note));
    ASSERT_EQ(static_cast<int>(note.p_type), PT_NOTE);
    ASSERT_TRUE(input.seek(static_cast<int64_t>(note.p_offset), file_pointer::begin));

    Elf64_Nhdr note_header{};
    ASSERT_EQ(input.read_binary(&note_header, sizeof(note_header)), sizeof(note_header));
    EXPECT_EQ(static_cast<int>(note_header.n_type), NT_PRPSINFO);
    EXPECT_EQ(note_header.n_namesz, sizeof("FreeBSD"));
    char name[sizeof("FreeBSD")]{};
    ASSERT_EQ(input.read_binary(name, sizeof(name)), sizeof(name));
    EXPECT_EQ(memory_compare(name, "FreeBSD", sizeof(name)), 0);

    prpsinfo_t info{};
    ASSERT_EQ(input.read_binary(&info, sizeof(info)), sizeof(info));
    EXPECT_EQ(info.pr_version, PRPSINFO_VERSION);
    EXPECT_EQ(info.pr_psinfosz, sizeof(prpsinfo_t));
    EXPECT_EQ(info.pr_pid, ::getpid());
    EXPECT_GT(string_length(info.pr_fname), 0U);

    const auto note_prefix = static_cast<int64_t>(sizeof(Elf64_Nhdr) + sizeof("FreeBSD"));
    const auto prpsinfo_end = static_cast<int64_t>(note.p_offset) + note_prefix +
                              static_cast<int64_t>((sizeof(prpsinfo_t) + 3U) & ~size_t(3U));
    ASSERT_TRUE(input.seek(prpsinfo_end, file_pointer::begin));

    Elf64_Nhdr status_header{};
    ASSERT_EQ(input.read_binary(&status_header, sizeof(status_header)), sizeof(status_header));
    EXPECT_EQ(static_cast<int>(status_header.n_type), NT_PRSTATUS);
    EXPECT_EQ(status_header.n_namesz, sizeof("FreeBSD"));
    prstatus_t status{};
    ASSERT_EQ(input.read_binary(&status, sizeof(status)), sizeof(status));
    EXPECT_EQ(status.pr_version, PRSTATUS_VERSION);
    EXPECT_EQ(status.pr_statussz, sizeof(prstatus_t));
    EXPECT_EQ(status.pr_gregsetsz, sizeof(gregset_t));
    EXPECT_GT(status.pr_pid, 0);

    const auto status_end = prpsinfo_end + note_prefix + static_cast<int64_t>((sizeof(prstatus_t) + 3U) & ~size_t(3U));
    if (input.seek(status_end, file_pointer::begin)) {
        Elf64_Nhdr fpregs_header{};
        if (input.read_binary(&fpregs_header, sizeof(fpregs_header)) == sizeof(fpregs_header) &&
            fpregs_header.n_type == NT_FPREGSET) {
            EXPECT_EQ(fpregs_header.n_descsz, sizeof(fpregset_t));
        }
    }
#else
    GTEST_SKIP() << "skip in non-FreeBSD platform";
#endif
}

TEST_F(CrashDumpTest, ManualDumpProducesMinidumpHeader) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "mdmp";
    options.include_memory = false;

    const auto result = crash_dump::write(options);
    ASSERT_TRUE(result.written);

    file input(result.file, false, file_access::read, file_shared::share_read_write);
    ASSERT_TRUE(input.is_opened());
    char signature[4]{};
    ASSERT_EQ(input.read_binary(signature, sizeof(signature)), sizeof(signature));
    EXPECT_EQ(memory_compare(signature, "MDMP", 4), 0);
#else
    GTEST_SKIP() << "skip in non-Windows platform";
#endif
}

TEST_F(CrashDumpTest, ManualDumpIncludesMemoryWithinLimits) {
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "memory";
    options.include_memory = true;
    options.max_segments = 2;
    options.max_file_bytes = 4ULL * 1024ULL * 1024ULL;

    const auto result = crash_dump::write(options);
    if (!native_writer_available()) {
        EXPECT_FALSE(result.written);
        return;
    }
    ASSERT_TRUE(result.written);
    EXPECT_LE(result.segments, 2U);
    EXPECT_LE(static_cast<uint64_t>(filesystem::size(result.file).bytes()), 8ULL * 1024ULL * 1024ULL);
    EXPECT_TRUE(result.truncated);
}

TEST_F(CrashDumpTest, ManualDumpFailsForMissingDirectory) {
    crash_dump::options options;
    options.directory = directory_ / path("missing") / path("nested");
    options.file_prefix = "missing";

    const auto result = crash_dump::write(options);
    if (!native_writer_available()) {
        return;
    }
    EXPECT_FALSE(result.written);
    EXPECT_NE(result.error, 0);
    EXPECT_TRUE(result.file.empty());
}

TEST_F(CrashDumpTest, ManualDumpSanitizesPrefix) {
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "bad/prefix name";
    options.include_memory = false;

    const auto result = crash_dump::write(options);
    ASSERT_EQ(result.written, native_writer_available());
    if (!native_writer_available()) {
        return;
    }
    EXPECT_TRUE(result.file.filename().contains("bad_prefix_name-"));
}

TEST_F(CrashDumpTest, ManualDumpUsesUniqueNames) {
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "unique";
    options.include_memory = false;

    const auto first = crash_dump::write(options);
    const auto second = crash_dump::write(options);
    ASSERT_EQ(first.written, native_writer_available());
    ASSERT_EQ(second.written, native_writer_available());
    if (!native_writer_available()) {
        return;
    }
    EXPECT_NE(first.file.str(), second.file.str());
    EXPECT_TRUE(first.file.exists());
    EXPECT_TRUE(second.file.exists());
}

TEST_F(CrashDumpTest, ManualDumpInvokesCallback) {
    g_capture = captured_report{};

    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "callback";
    options.include_memory = false;
    options.callback = capture_report;

    const auto result = crash_dump::write(options);
    ASSERT_TRUE(g_capture.called);
    EXPECT_EQ(g_capture.reason, crash_dump::reason::manual);
    EXPECT_EQ(g_capture.written, result.written);
    EXPECT_TRUE(g_capture.message.contains("manual dump"));
    if (result.written) {
        EXPECT_EQ(g_capture.file, result.file.str());
        EXPECT_TRUE(g_capture.message.contains(result.file.filename()));
    }
}

TEST_F(CrashDumpTest, InstallAndUninstallHandlers) {
    ASSERT_FALSE(crash_dump::handlers_installed());

    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "installed";
    options.include_memory = false;

    EXPECT_TRUE(crash_dump::install_handlers(options));
    EXPECT_TRUE(crash_dump::handlers_installed());
    EXPECT_TRUE(crash_dump::install_handlers(options));
    EXPECT_TRUE(crash_dump::handlers_installed());

    crash_dump::uninstall_handlers();
    EXPECT_FALSE(crash_dump::handlers_installed());
    crash_dump::uninstall_handlers();
    EXPECT_FALSE(crash_dump::handlers_installed());
}

TEST_F(CrashDumpTest, UninstallRestoresDefaultSignalAction) {
#ifdef NEFORCE_PLATFORM_POSIX
    crash_dump::options options;
    options.directory = directory_;
    options.file_prefix = "restored";
    options.include_memory = false;
    ASSERT_TRUE(crash_dump::install_handlers(options));
    ASSERT_TRUE(crash_dump::install_handlers(options));
    crash_dump::uninstall_handlers();
    ASSERT_FALSE(crash_dump::handlers_installed());

    const string target = directory_.str();
    EXPECT_EXIT(
            {
                static_cast<void>(::chdir(target.data()));
                ::raise(SIGSEGV);
            },
            ::testing::KilledBySignal(SIGSEGV), "");
#else
    GTEST_SKIP() << "skip in non-POSIX platform";
#endif
}

TEST_F(CrashDumpTest, FatalSignalWritesCore) {
#if defined(NEFORCE_PLATFORM_POSIX) && defined(NEFORCE_SUPPORT_NATIVE_DUMP)
    static_cast<void>(filesystem::remove(g_death_marker));
    g_death_directory = directory_;
    g_death_marker = directory_ / path("signal-report.txt");

    EXPECT_EXIT(
            {
                crash_dump::options options;
                options.directory = g_death_directory;
                options.file_prefix = "fatal";
                options.include_memory = false;
                options.callback = record_crash;
                crash_dump::install_handlers(options);
                ::raise(SIGSEGV);
            },
            ::testing::ExitedWithCode(128 + SIGSEGV), "");

    ASSERT_TRUE(g_death_marker.exists());
    const string recorded = read_all(g_death_marker);
    EXPECT_TRUE(recorded.contains("fatal-"));
    EXPECT_TRUE(recorded.contains("fatal signal"));
    const path core(recorded.substr(0, recorded.find('\n')));
    EXPECT_TRUE(core.exists());
    EXPECT_GT(static_cast<uint64_t>(filesystem::size(core).bytes()), 0U);
#else
    GTEST_SKIP() << "skip in platform without POSIX native dump writer";
#endif
}

TEST_F(CrashDumpTest, FatalSignalFallsBackWithoutWriter) {
#if defined(NEFORCE_PLATFORM_POSIX) && !defined(NEFORCE_SUPPORT_NATIVE_DUMP)
    static_cast<void>(filesystem::remove(g_death_marker));
    g_death_directory = directory_;
    g_death_marker = directory_ / path("fallback-report.txt");

    EXPECT_EXIT(
            {
                crash_dump::options options;
                options.directory = g_death_directory;
                options.file_prefix = "fallback";
                options.include_memory = false;
                options.callback = record_crash;
                crash_dump::install_handlers(options);
                ::rlimit limit{};
                limit.rlim_cur = 0;
                limit.rlim_max = 0;
                static_cast<void>(::setrlimit(RLIMIT_CORE, &limit));
                ::raise(SIGSEGV);
            },
            ::testing::KilledBySignal(SIGSEGV), "");

    ASSERT_TRUE(g_death_marker.exists());
    const string recorded = read_all(g_death_marker);
    EXPECT_TRUE(recorded.contains("fatal signal"));
    EXPECT_TRUE(recorded.contains("dump-failed"));
#else
    GTEST_SKIP() << "skip in platform with a native dump writer";
#endif
}

TEST_F(CrashDumpTest, TerminateWritesCore) {
#ifdef NEFORCE_PLATFORM_POSIX
    static_cast<void>(filesystem::remove(g_death_marker));
    g_death_directory = directory_;
    g_death_marker = directory_ / path("terminate-report.txt");

    EXPECT_EXIT(
            {
                crash_dump::options options;
                options.directory = g_death_directory;
                options.file_prefix = "terminate";
                options.include_memory = false;
                options.callback = record_crash;
                crash_dump::install_handlers(options);
                try {
                    throw std::runtime_error("crash dump test");
                } catch (...) {
                    std::terminate();
                }
            },
            ::testing::ExitedWithCode(128 + SIGABRT), "");

    ASSERT_TRUE(g_death_marker.exists());
    const string recorded = read_all(g_death_marker);
    EXPECT_TRUE(recorded.contains("terminate: crash dump test"));
#    if defined(NEFORCE_SUPPORT_NATIVE_DUMP)
    EXPECT_TRUE(recorded.contains("terminate-"));
    const path core(recorded.substr(0, recorded.find('\n')));
    EXPECT_TRUE(core.exists());
#    endif
#else
    GTEST_SKIP() << "skip in non-POSIX platform";
#endif
}
