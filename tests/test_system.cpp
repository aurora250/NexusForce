#include "test.h"


void test_print() {
    decimal_t f = constants::PI;
    inner::FUNCTION_OPERATE enu = inner::FUNCTION_OPERATE::GET_PTR;
    inner::__nocopy_type uni{};
    int c_arr[2];
    int* pa = ::new int[2];
    string address = to_string(pa);
    println(address);
    delete[] pa;
    pa = nullptr;
    int* p = &c_arr[0];
    const char* cs = "Hello World!";
    int pair<int, char>::* mop = &pair<int, char>::first;
    int pair<int, char>::* null_mop = nullptr;
#ifdef NEFORCE_STANDARD_14
    void (bit_reference::* mfp)() const = &bit_reference::flip;
#else
    void (bit_reference::* mfp)() const noexcept = &bit_reference::flip;
#endif
    compressed_pair<io_base<int>, int> cp;
    tuple<int, char, decimal_t, int*> tup{1, 't', f, nullptr};
    pair<int, char> pir{1, '1'};
    vector<int> v{1, 2, 3};
    vector<int>::iterator iter = v.begin();
    vector<pair<int, char>> pir_vec{{1, '1'}, {2, '2'}, {3, '3'}};
    array<int, 5> arr{1,2,3,4,5};
    variant<int, char> var{v[0]};
    var.emplace<1>('c');
    list<int> lls{arr.begin(), arr.end()};
    map<int, int> mmi{{1,2}};
    unordered_map<int, int> umi{{1,2}};
    set<int> s{1,2,3};
    unordered_set<int> us{1,2,3};
    bitmap bm{10, false};
    string str = "胡";
    string_view sv = cs;
    wstring ws = L"WSTRING胡";
    // ensure you used external utf-8 console
    const char* emoji = "\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    const wchar_t* wemoji = L"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    const char16_t* u16emoji = u"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    const char32_t* u32emoji = U"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
#ifdef NEFORCE_STANDARD_20
    const char8_t* u8emoji = u8"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    println(u8emoji);
#endif

    string tup_str = to_string(tup);
    println(tup);
    println(pir, cp);
    println('c', nullptr);
    println(&RB_TREE_RED, &RB_TREE_BLACK);
    println(f, static_cast<size_t>(enu));
    println(escape("\n\\\"\v"), cs);
    println(p, c_arr, arr);
    println(lls);
    println(bm);
    println(v);
    println(str, sv, ws);
    println(emoji, wemoji);
    println(u16emoji, u32emoji);
    println(mmi, umi);
    println(s, us);
}

void test_console() {
    if (console.confirmation("Save changes? (s/c): ", 's', 'c')) {
        console.progress_bar(0.7);
        println("Saving...");
    } else {
        println("Cancelled.");
    }

    auto size = console.get_console_size();
    println("Terminal size: ", size.width, "x", size.height);
    println("Supports colors: ", console.supports_colors());
    println("Supports truecolor: ", console.supports_truecolor());
    println("Terminal type: ", console.console_type());

    string password = console.password("Enter password (masked): ", '*');
    console.typewriter_println("This appears character by character...");

    console.fade_in("Welcome to the system!", seconds(2));
    console.fade_out("Goodbye!", seconds(2));
    console.fade_in_out("Important Message!", milliseconds(500), seconds(2), milliseconds(500));

    console.beep();
    // console.flash_screen();
    console.notification("Download completed!", seconds(3));
    console.pause();

    hexadecimal hex("F");
    println(hex);
    console.readln(hex);
    println(hex);
    int rawi = 3;
    println(rawi);
    console.readln(rawi);
    println(rawi);
    boolean b;
    b.try_parse("true");
    println(b);
    console.readln(b);
    println(b);
    integer32 i32;
    console.readln(i32);
    println(i32);
    float32 fp;
    console.readln(fp);
    println(fp);
}

void test_device() {

}

void test_sysinfo() {
    auto& sysinfo = sysinfo::instance();

    const auto& os_info = sysinfo.get_os_version_info();
    printfln("OS: {} {}", os_info.product_name, os_info.version());

    const auto& cpu_info = sysinfo.get_CPU_info();
    printfln("CPU: {} ({} cores)", cpu_info.brand, cpu_info.cores);

    const auto &mem_info = sysinfo.get_memory_info();
    printfln("Memory: {:.1f}% used", mem_info.physical_memory_usage());
    printfln("RAM: {} / {}",
        sysinfo::format_bytes(mem_info.total_physical - mem_info.available_physical),
        sysinfo::format_bytes(mem_info.total_physical));

    const auto arch = sysinfo.get_architecture();
    printfln("Architecture: {}",
        arch == sysinfo::architecture::X64 ? "x64" :
        arch == sysinfo::architecture::X86 ? "x86" : "Other");
}

void test_env_var() {
    println(environment::all_envs());
}

bool signal_handler(SIGNAL_EVENT event, void* context) {
    printcln(color::green(), "处理信号: ", static_cast<int>(event));

    switch (event) {
        case SIGNAL_EVENT::INTERRUPT: {
            println("收到中断信号 (Ctrl+C)");
            return false;
        }
        case SIGNAL_EVENT::TERMINATE: {
            println("收到终止信号");
            return false;
        }
        case SIGNAL_EVENT::USER1: {
            println("收到用户自定义信号1");
            if (context) {
                const int* value = static_cast<int*>(context);
                println("上下文数据: ", *value);
            }
            return true;
        }
        case SIGNAL_EVENT::TIMEOUT: {
            println("超时信号");
            return false;
        }
        default: {
            println("其他信号");
            return true;
        }
    }
}

void test_signal() {
    {
        signal_guard guard;

        signal_manager::instance().register_handler(
            SIGNAL_EVENT::INTERRUPT,
            signal_handler
        );

        vector<SIGNAL_EVENT> signals = {
            SIGNAL_EVENT::TERMINATE,
            SIGNAL_EVENT::USER1,
            SIGNAL_EVENT::USER2
        };
        signal_manager::instance().register_handlers(
            signals,
            signal_handler
        );

        signal_manager::instance().set_force_exit_timeout(10000);

        printcln(color::cyan(), "信号管理器已启动");
        println("按 Ctrl+C 测试中断信号");
        println("在另一个终端执行: kill -TERM ", process::current_id(), " 测试终止信号");
        println("或执行: kill -USR1 ", process::current_id(), " 测试用户信号");

        for (int i = 0; i < 30; ++i) {
            if (!signal_manager::instance().is_running()) {
                println("收到退出信号，正在退出...");
                break;
            }

            println("工作循环 ", i + 1);
            this_thread::sleep_for(seconds(1));

            if (i == 5) {
                int context_data = 42;
                signal_manager::instance().send_signal(
                    SIGNAL_EVENT::USER1,
                    &context_data
                );
            }
        }
    }
}

void test_cmd(int argc, char* argv[]) {
    cmdline parser;

    parser.add_option("help", 'h', "Show help message", false);
    parser.add_option("verbose", 'v', "Enable verbose output", false, true);
    parser.add_option("input", 'i', "Input file path", true);
    parser.add_option("output", 'o', "Output file path", true, false, "output.txt");
    parser.add_option("count", 'c', "Repeat count", true);

    try {
        parser.parse(argc, argv);

        if (parser.has("help")) {
            parser.print_help();
        }

        if (parser.has("verbose")) {
            println("Verbosity: ", parser.get("verbose"));
        }

        if (parser.has("input")) {
            println("Input file: ", parser.get("input"));
        }

        if (parser.has("output")) {
            println("Output file: ", parser.get("output"));
        }

        if (parser.has("count")) {
            println("Count option was specified ", parser.get("count"));
        }

        auto positional = parser.positional_args();
        if (!positional.empty()) {
            println("Positional arguments:");
            for (const auto& arg : positional) {
                println("  ", arg);
            }
        }
    } catch (const exception& e) {
        println("Error: ", e.what());
    }
}

void test_process() {
    auto pi = process::create(
#ifdef NEFORCE_PLATFORM_WINDOWS
        "python"
#else
        "python3"
#endif
        , {(res_root() / "test.py").str()}, true);
    int res = process::wait_for(pi);
    println(res);
    if (res == 0) {
        println(pi.stdout_output);
    }
}

void test_rnd() {
    println(secret::system_supported(), secret::next_float<double>(), secret::next_int(1, 10));
    println(random_lcd().next_int(10, 20), random_lcd().next_int(10, 20), random_lcd().next_int(10, 20));
    println(random_mt().next_int(10, 20), random_mt().next_int(10, 20), random_mt().next_int(10, 20));

    println("UUID V4:", uuid::v4());
    println("UUID V7:", uuid::v7());
}

void test_atomic() {
    atomic<shared_ptr<int>> aptr{make_shared<int>(2)};
    println(*aptr.load().get());
    aptr.store(make_shared<int>(3));
    println(*aptr.load().get());

    shared_ptr<int> p = make_shared<int>(4);
    atomic<weak_ptr<int>> wptr{p};
    println(wptr.load().expired());
}
