#include <NeForce/NeForce.hpp>
#include <gtest/gtest.h>

using namespace neforce;

namespace {
    bool open_file(const char* path, error_code& ec) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::HANDLE h = ::CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            ec = last_error();
            return false;
        }
        ::CloseHandle(h);
#else
        if (::open(path, O_RDONLY) < 0) {
            ec = last_error();
            return false;
        }
#endif
        ec.clear();
        return true;
    }
} // namespace

bool signal_handler(signal_event event, void* context) {
    switch (event) {
        case signal_event::INTERRUPT:
            return false;
        case signal_event::TERMINATE:
            return false;
        case signal_event::USER1:
            return true;
        case signal_event::TIMEOUT:
            return false;
        default:
            return true;
    }
}


TEST(PrintTest, BasicTypes) {
    decimal_t f = constants::PI;
    inner::FUNCTION_OPERATE enu = inner::FUNCTION_OPERATE::GET_PTR;
    inner::__nocopy_type uni{};
    int c_arr[2];
    int* pa = ::new int[2];
    string address = to_string(pa);
    ASSERT_FALSE(address.empty());
    delete[] pa;
    pa = nullptr;
    int* p = &c_arr[0];
    const char* cs = "Hello World!";
    int pair<int, char>::* mop = &pair<int, char>::first;
    int pair<int, char>::* null_mop = nullptr;
#ifdef NEFORCE_STANDARD_14
    void (bit_reference::*mfp)() const = &bit_reference::flip;
#else
    void (bit_reference::*mfp)() const noexcept = &bit_reference::flip;
#endif
    compressed_pair<plus<int>, int> cp;
    tuple<int, char, decimal_t, int*> tup{1, 't', f, nullptr};
    pair<int, char> pir{1, '1'};
    vector<int> v{1, 2, 3};
    vector<int>::iterator iter = v.begin();
    vector<pair<int, char>> pir_vec{{1, '1'}, {2, '2'}, {3, '3'}};
    array<int, 5> arr{1, 2, 3, 4, 5};
    variant<int, char> var{v[0]};
    var.emplace<1>('c');
    list<int> lls{arr.begin(), arr.end()};
    map<int, int> mmi{{1, 2}};
    unordered_map<int, int> umi{{1, 2}};
    set<int> s{1, 2, 3};
    unordered_set<int> us{1, 2, 3};
    bitmap bm{10, false};
    string str = "胡";
    string_view sv = cs;
    wstring ws = L"WSTRING胡";
    const char* emoji =
            "\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    const wchar_t* wemoji =
            L"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    const char16_t* u16emoji =
            u"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    const char32_t* u32emoji =
            U"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
#ifdef NEFORCE_STANDARD_20
    const char8_t* u8emoji =
            u8"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
#endif

    string tup_str = to_string(tup);
    ASSERT_FALSE(tup_str.empty());
}

TEST(SysInfoTest, OSInfo) {
    auto& sysinfo = sysinfo::instance();
    const auto& os_info = sysinfo.get_os_version_info();
    ASSERT_FALSE(os_info.product_name.empty());
}

TEST(SysInfoTest, CPUInfo) {
    auto& sysinfo = sysinfo::instance();
    const auto& cpu_info = sysinfo.get_CPU_info();
    ASSERT_FALSE(cpu_info.brand.empty());
    ASSERT_GT(cpu_info.cores, 0);
}

TEST(SysInfoTest, MemoryInfo) {
    auto& sysinfo = sysinfo::instance();
    const auto& mem_info = sysinfo.get_memory_info();
    ASSERT_GT(mem_info.total_physical, 0);
    ASSERT_GT(mem_info.available_physical, 0);
}

TEST(SysInfoTest, Architecture) {
    auto& sysinfo = sysinfo::instance();
    const auto arch = sysinfo.get_architecture();
    ASSERT_TRUE(arch == sysinfo::architecture::X64 || arch == sysinfo::architecture::X86);
}

TEST(EnvVarTest, AllEnvs) {
    auto envs = environment::all_envs();
    ASSERT_FALSE(envs.empty());
}

TEST(SignalTest, RegisterHandler) {
    signal_guard guard;
    signal_manager::instance().register_handler(signal_event::INTERRUPT, signal_handler);
    vector<signal_event> signals = {signal_event::TERMINATE, signal_event::USER1, signal_event::USER2};
    signal_manager::instance().register_handlers(signals, signal_handler);
    signal_manager::instance().set_force_exit_timeout(10000);
    ASSERT_TRUE(signal_manager::instance().is_running());
}

TEST(CmdLineTest, BasicParsing) {
    cmdline parser;
    parser.add_option("help", 'h', "Show help message", false);
    parser.add_option("verbose", 'v', "Enable verbose output", false, true);
    parser.add_option("input", 'i', "Input file path", true);
    parser.add_option("output", 'o', "Output file path", true, false, "output.txt");
    parser.add_option("count", 'c', "Repeat count", true);
}

TEST(CmdLineTest, HelpFlag) {
    cmdline parser;
    parser.add_option("help", 'h', "Show help message", false);

    const char* argv[] = {"test", "-h"};
    constexpr int argc = size(argv);
    parser.parse(argc, argv);

    ASSERT_TRUE(parser.has("help"));
}

TEST(LocaleTest, Classic) {
    locale c = locale::classic();
    ASSERT_FALSE(c.name().empty());
}

TEST(LocaleTest, System) {
    locale sys_loc = locale::system();
    ASSERT_FALSE(sys_loc.name().empty());
}

TEST(LocaleTest, FrenchLocale) {
    try {
        locale fr("fr_FR.UTF-8");
        auto ni = fr.numeric();
        ASSERT_FALSE(ni.decimal_point.empty());
        ASSERT_FALSE(ni.thousands_sep.empty());

        int r = fr.compare("café", "cafe");
        ASSERT_NE(r, 0);

        ASSERT_TRUE(fr.is_alpha(U'\u00e9'));
        ASSERT_NE(fr.to_upper(U'\u00e9'), U'\u00e9');
    } catch (const locale_exception& e) {
        GTEST_SKIP() << "French locale not available";
    }
}

TEST(LocaleTest, AvailableLocales) {
    auto av = locale::available_locales();
    ASSERT_FALSE(av.empty());
}

TEST(ErrorCodeTest, FileNotFound) {
    error_code ec;
    ASSERT_FALSE(open_file("/nonexistent/file.txt", ec));
    ASSERT_NE(ec.value(), 0);
    ASSERT_FALSE(ec.message().empty());

    if (ec == errc::no_such_file_or_directory) {
        SUCCEED();
    }
}

TEST(ErrorCodeTest, GenericError) {
    error_code enoent(ENOENT, generic_category());
    ASSERT_FALSE(enoent.message().empty());
    ASSERT_EQ(enoent, errc::no_such_file_or_directory);
}
