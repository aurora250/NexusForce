#include "test.h"

void test_lz4() {
#ifdef NEFORCE_SUPPORT_LZ4
    {
        string_view original = "Hello, World! This is a test string for compression.";

        auto compressed = lz4_compressor::compress(original);
        println("Original size:", original.size(), "bytes");
        println("Compressed size:", compressed.size(), "bytes");

        auto decompressed = lz4_compressor::decompress(compressed.view(), original.size());
        string_view result(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());

        println("Decompressed:", result);
        println("Match:", (original == result ? "Yes" : "No"));
    }
    {
        string base = "Hello World! ";
        string repeating;
        for (int i = 0; i < 100; ++i) {
            repeating += base;
        }

        auto compressed = lz4_compressor::compress(repeating.view());
        println("Original size:", repeating.size(), "bytes");
        println("Compressed size:", compressed.size(), "bytes");

        auto decompressed = lz4_compressor::decompress(compressed.view(), repeating.size());
        println("Compression ratio:", static_cast<double>(compressed.size()) / repeating.size());
    }
#endif
}

void test_zlib() {
#ifdef NEFORCE_SUPPORT_ZLIB
    {
        string original = "Hello, World! This is a test string for zlib compression. "
                          "The quick brown fox jumps over the lazy dog. "
                          "Repeat: The quick brown fox jumps over the lazy dog.";

        printfln("Original: {} bytes", original.size());

        auto compressed = zlib_compressor::compress(original.view());
        printfln("Compressed: {} bytes ({:.1f}%)", compressed.size(), 100.0 * compressed.size() / original.size());

        auto decompressed = zlib_compressor::decompress(compressed.view());

        string result(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
        printfln("Match: {}\n", result == original ? "✓" : "✗");
    }
    {
        string data(3000, 'A');
        printfln("Test data: {} bytes", data.size());

        auto test_level = [&](compress_level level, const char* name) {
            auto compressed = zlib_compressor::compress(data.view(), level);
            printfln("  {:20} : {:5} bytes ({:.1f}%)", name, compressed.size(),
                     100.0 * compressed.size() / data.size());
        };

        test_level(compress_level::best_speed, "Best Speed");
        test_level(compress_level::default_level, "Default");
        test_level(compress_level::best_compression, "Best Compression");
        println();
    }
    {
        zlib_compressor::stream_compressor comp;

        vector<string_view> chunks = {"First chunk. ", "Second chunk. ", "Third chunk. ", "Final chunk."};

        byte_vector all_compressed;
        string original;

        for (size_t i = 0; i < chunks.size(); ++i) {
            bool is_last = (i == chunks.size() - 1);
            auto compressed = comp.compress(chunks[i], is_last);
            all_compressed.insert(all_compressed.end(), compressed.begin(), compressed.end());
            original += chunks[i];
        }

        printfln("Input: {} bytes", comp.bytes_input());
        printfln("Output: {} bytes", comp.bytes_output());
        printfln("Ratio: {:.2f}%", comp.compression_ratio() * 100);

        zlib_compressor::stream_decompressor decomp;
        auto decompressed = decomp.decompress(all_compressed.view(), true);

        string result(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
        printfln("Match: {}\n", result == original ? "✓" : "✗");
    }
    {
        constexpr size_t data_size = 1024 * 1024;
        byte_vector large_data(data_size);

        for (size_t i = 0; i < data_size; ++i) {
            large_data[i] = static_cast<byte_t>(i % 256);
        }

        auto compressed = zlib_compressor::compress(large_data);

        printfln("Original: {} KB", data_size / 1024);
        printfln("Compressed: {} KB ({:.2f}%)", compressed.size() / 1024, 100.0 * compressed.size() / data_size);

        auto decompressed = zlib_compressor::decompress(compressed.view(), data_size);

        bool match = (decompressed.size() == large_data.size()) &&
                     equal(decompressed.begin(), decompressed.end(), large_data.begin());
        printfln("Match: {}\n", match ? "✓" : "✗");
    }
    {
        zlib_compressor::stream_compressor comp(compress_level::best_compression);
        zlib_compressor::stream_decompressor decomp;

        string data = "Test data for statistics. "_s.repeat(10);

        auto compressed = comp.compress(data.view(), true);
        auto decompressed = decomp.decompress(compressed.view(), true);

        println("Compression:");
        printfln("  Input:  {} bytes", comp.bytes_input());
        printfln("  Output: {} bytes", comp.bytes_output());
        printfln("  Ratio:  {:.2f}%", comp.compression_ratio() * 100);

        println("Decompression:");
        printfln("  Input:  {} bytes", decomp.bytes_input());
        printfln("  Output: {} bytes", decomp.bytes_output());
        printfln("  Ratio:  {:.2f}x", decomp.expansion_ratio());
        println();
    }
#endif
}

void test_regex() {
    {
        regex re(R"(^\d{4}-\d{2}-\d{2}$)");

        string date1 = "2024-01-15";
        string date2 = "2024/01/15";

        printfln("  '{}' 匹配日期格式: {}", date1, re.match(date1));
        printfln("  '{}' 匹配日期格式: {}", date2, re.match(date2));
    }
    {
        regex re(R"(\d+)");
        string text = "abc123def456ghi";

        auto result = re.search(text);
        if (result.matched()) {
            printfln("  首次匹配: '{}'", result.data());
            printfln("  位置: {}, 长度: {}", result.position(), result.length());
            printfln("  前缀: '{}'", result.prefix());
            printfln("  后缀: '{}'\n", result.suffix());
        }
    }
    {
        regex re(R"((\w+)@(\w+)\.(\w+))");
        string email = "user@example.com";

        auto result = re.search(email);
        if (result.matched()) {
            printfln("  完整匹配: '{}'", result[0]);
            printfln("  用户名: '{}'", result[1]);
            printfln("  域名: '{}'", result[2]);
            printfln("  顶级域: '{}'", result[3]);

            printfln("  共有 {} 个捕获组", result.size());

            println("  所有组:");
            for (const auto& group: result) {
                printfln("    '{}'", group);
            }
        }
        println("");
    }
    {
        regex re(R"(\d+)");
        string text = "abc123def456ghi789";

        println("  使用迭代器遍历:");
        auto begin = re.begin(text);
        auto end = re.end(text);

        int count = 1;
        for (auto it = begin; it != end; ++it) {
            printfln("    匹配 {}: '{}' 在位置 {}", count++, it->data(), it->position());
        }
        println("");
    }
    {
        regex re(R"(\s+)");
        string text = "one two   three\tfour\nfive";

        regex_token_iterator it(&re, text, -1);
        regex_token_iterator end;

        println("  标记结果:");
        int token_num = 1;
        for (; it != end; ++it) {
            printfln("    标记 {}: '{}' (长度: {})", token_num++, *it, (*it).length());
        }
        println("");
    }
}

void test_format() {
    {
        hexadecimal x(255);
        println(format("{:#x}", x)); // "0xff"
        println(format("{:#X}", x)); // "0XFF"
        println(format("{:x}", x));  // "ff"

        println(format("{:#10x}", x));   // "      0xff"
        println(format("{:*>#10x}", x)); // "******0xff"
        println(format("{:0=#10x}", x)); // "0x000000ff"

        println(format("{:<#10x}", x)); // "0xff      "
        println(format("{:=#10x}", x)); // "0x      ff"

        println(format("{:-=#10X}", x)); // "0X      FF"

        hexadecimal neg(-255);
        println(absolute(neg.value()), sign(neg.value()));
        println(format("{:0=#10x}", neg)); // "-0x00000ff"
        println(format("{:#10x}", neg));   // "      -0xff"

        println(format("{:#x}", x));   // "0xff"
        println(format("{:#X}", x));   // "0XFF"
        println(format("{:#08x}", x)); // "0x0000ff"
        println(format("{:x}", x));    // "ff"


        hexadecimal hex(255);
        string result1 = format("{:#010X}", hex); // "0X000000FF"
        println(result1);
        hexadecimal tmp(222);
        _NEFORCE swap(hex, tmp);
        println(format("{:#010X}", hex)); // "0X000000DE"
        println(hexadecimal(222));

        integer64 num(12345);
        string result2 = format("{:+>15d}", num); // "        +12345"
        println(result2);

        string result3 = format("{:#b}", num); // "0b11000000111001"
        println(result3);

        decimal dec(3.14159);
        string result4 = format("{:.2f}", dec); // "3.14"
        println(result4);
        string result5 = format("{:.3e}", dec); // "3.142e+00"
        println(result5);

        string result6 = format("{:^20x}", hex); // "       de       "
        println(result6);
        string result7 = format("{:#o}", num); // "0o30071"
        println(result7);
        string result8 = format("{:+10.4f}", dec); // "   +3.1415"
        println(result8);

        string r1 = format("{:.2f}", dec); // "3.14"
        println(r1);
        string r2 = format("{:.4e}", dec); // "3.1416e+00"
        println(r2);
        string r3 = format("{:.6g}", dec); // "3.141590"
        println(r3);
        string r4 = format("{:+10.3f}", dec); // "    +3.141"
        println(r4);
    }
    {
        uinteger32 x(255u);
        println(format("{:d}", x));    // "255"
        println(format("{:b}", x));    // "11111111"
        println(format("{:#b}", x));   // "0b11111111"
        println(format("{:10d}", x));  // "       255"
        println(format("{:<10d}", x)); // "255       "
        println(format("{:010d}", x)); // "0000000255"

        uint64_t ull = 123456789ULL;
        println(ull);
        println(format("{:#x}", ull));
    }
}

void test_color() {
    color red(255, 0, 0, 255);
    color semiRed(255, 0, 0, 128);
    color transRed(255, 0, 0, 64);
    color background(255, 255, 255);

    println(red);
    println(semiRed);
    println(transRed);
    println(semiRed.opacity());
    println(semiRed.transparent());
    println(semiRed.is_opaque());

    println(semiRed.blend(background));
    println(color::parse("#FF000080"));
    println(red * 0.5);

    color adjustableColor = color::red();
    adjustableColor.set_opacity(0.3);
    println(adjustableColor);
    println(semiRed.to_premultiplied());


    color custom(128, 64, 192);

    printcln(color::red(), "这是红色文本");
    printcln(color::green(), "这是绿色文本");
    printcln(custom, "这是使用基本颜色的文本");

    console.set_color(custom, true);
    println("这是使用256色的文本");
    console.reset_color();
}


void test_enctype() {
    string encrypted = XOR_encrypt("Hello", "key");
    string decrypted = XOR_decrypt(encrypted, "key");
    println(escape(encrypted));
    println(decrypted);

    string encoded = base64_encode("Hello World");
    string decoded = base64_decode(encoded);
    println(encoded);
    println(decoded);

    string md5_hash = md5("Hello World"_sv);
    string sha256_hash = sha256("Hello World"_sv);
    println("MD5: ", md5_hash);
    println("SHA256: ", sha256_hash);

    string key = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    string aes_encrypted = aes256_encrypt("Hello World"_sv, key);
    string aes_decrypted = aes256_decrypt(aes_encrypted, key);
    println(aes_encrypted);
    println(aes_decrypted);
}

void test_ini() {
    ini_builder builder;
    builder.key("global_key")
            .value("global_value")
            .begin_section("database")
            .key("host")
            .value("localhost")
            .key("port")
            .value(5432)
            .key("enabled")
            .value(true)
            .end_section()
            .begin_section("logging")
            .key("level")
            .value("debug")
            .key("file")
            .value("/var/log/app.log")
            .end_section();

    println(builder.build()->to_string());


    file fi(res_root() / "test.ini");

    ini_parser parser(fi.read());
    auto doc = parser.parse();

    string host = doc->get_string("database", "host");
    int port = doc->get_int("database", "port");
    bool enabled = doc->get_bool("database", "enabled");

    println(host, ", ", port, ", ", enabled);
}

void test_env() {
    env_builder builder;
    builder.comment("Application Configuration")
            .blank_line()
            .add("APP_NAME", "MyApp")
            .add("APP_VERSION", "1.0.0")
            .blank_line()
            .comment("Database Configuration")
            .add("DB_HOST", "localhost")
            .add("DB_PORT", 5432)
            .add("DB_NAME", "mydb")
            .key("DB_PASSWORD")
            .double_quoted()
            .value("secret123")
            .blank_line()
            .comment("Feature Flags")
            .add("FEATURE_ENABLED", true)
            .add_export("PATH", "/usr/local/bin:$PATH");

    auto bdoc = builder.build();
    println(bdoc->to_string());


    file fi(res_root() / "test.env");

    env_parser parser(fi.read());
    auto doc = parser.parse();

    string app_name = doc->get_string("APP_NAME");
    int db_port = doc->get_int("DB_PORT");
    bool feature_enabled = doc->get_bool("FEATURE_ENABLED");

    println(app_name, ", ", db_port, ", ", feature_enabled);
}

void test_toml() {
    file fi(res_root() / "test.toml");

    try {
        toml_parser parser(fi.read());
        unique_ptr<toml_table> root = parser.parse();
        printcln(color::blue(), root->to_document());

        toml_builder builder;
        builder.key("title")
                .value("My Config")
                .key("owner")
                .value_inline_table([](toml_builder& b) {
                    b.key("name").value("Tom").key("dob").value_datetime("1979-05-27T07:32:00Z",
                                                                         toml_datetime::OffsetDateTime);
                })
                .begin_table("database")
                .key("server")
                .value("192.168.1.1")
                .key("ports")
                .value_array([](toml_builder& b) { b.value(8001).value(8002).value(8003); })
                .end_table();

        unique_ptr<toml_table> broot = builder.build();
        println(broot->to_document());
    } catch (...) {
    }
}

void test_yaml() {
    try {
        file fi(res_root() / "test.yaml");
        yaml_parser parser(fi.read());
        auto docs = parser.parse_documents();
        for (const auto& doc: docs) {
            println(doc->to_document());
        }
    } catch (...) {
    }
}

void test_json() {
    file fi(res_root() / "test.json");

    try {
        unique_ptr<json_value> root = json_parser(fi.read()).parse();
        println(*root);

        if (root->is_object()) {
            const json_object* obj = root->as_object();
            const json_value* number_val = obj->get_member("numbers");
            if (number_val && number_val->is_object()) {
                const json_object* number_obj = number_val->as_object();
                const json_value* sn_val = number_obj->get_member("scientific_notation");
                println("scientific_notation: ", static_cast<uint64_t>(sn_val->as_number()->get_value()));
            }
        }

        auto json2 = json_builder()
                             .begin_array()
                             .value(1)
                             .value(2)
                             .begin_object()
                             .key("x")
                             .value(10)
                             .end_object()
                             .end_array()
                             .build();
        println(*json2);

        map<string, vector<int>> nested_data = {
                {"group1", {10, 20, 30}}, {"group2", {40, 50}}, {"group3", {60, 70, 80, 90}}};
        auto json3 =
                json_builder().begin_object().key("data").value(nested_data).key("total").value(8).end_object().build();
        println(*json3);

    } catch (...) {
    }
}

void test_timer() {
    _NEFORCE steady_timer timer1;
    timer1.expires_after(seconds(5));
    timer1.async_wait([]() { println("5秒后执行"); });

    _NEFORCE system_timer timer2;
    auto now = system_clock::now();
    auto target = now + hours(1);
    timer2.expires_at(target);
    timer2.async_wait([]() { println("1小时后执行"); });

    _NEFORCE steady_timer timer3;
    timer3.expires_from_now(1000);
    timer3.async_wait([]() { println("1秒后执行"); });

    timer1.cancel();

    timer1.expires_after(seconds(3));
    timer1.async_wait([]() { println("3秒后执行"); });

    this_thread::sleep_for(seconds(7));
}

void test_vthread() {
#ifdef NEFORCE_STANDARD_20
    virtual_thread::initialize(4);

    auto vt1 = virtual_thread::start([] {
        for (int i = 0; i < 5; ++i) {
            println("VT-1: Task", i);
            this_thread::sleep_for(milliseconds(100));
        }
    });

    auto vt2 = virtual_thread::start([] {
        for (int i = 0; i < 5; ++i) {
            println("VT-2: Task", i);
            this_thread::sleep_for(milliseconds(150));
        }
    });

    auto vt3 = virtual_thread::start([] {
        for (int i = 0; i < 3; ++i) {
            println("VT-3: Task", i);
            this_thread::sleep_for(milliseconds(200));
        }
    });

    this_thread::sleep_for(seconds(2));
    virtual_thread::shutdown();
#endif
}

void test_logging() {
    auto& logger = logger::instance();
    logger.set_level(log_level::DEBUG);
    logger.add_context("app", "myapp");
    logger.set_filter([](const log_event& ev) -> bool { return ev.level >= log_level::INFO; });
    const auto sink = make_shared<console_sink>();
    sink->set_formatter(make_unique<log_formatter>("[{time}][{level}][{context.app}] {message}"));
    logger.add_sink(sink);
    logger.enable_async(true);

    NEFORCE_LOG_INFO("This is a info message");
    logger.flush();

    logger.enable_async(false);
    logger.flush();
}

void test_ranges() {
#ifdef NEFORCE_STANDARD_20
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto view1 = vec | rv::filter([](int x) { return x % 2 == 0; }) | rv::transform([](int x) { return x * x; });
    println(view1);

    auto view2 = vec | rv::filter([](int x) { return x > 3; }) | rv::filter([](int x) { return x % 2 == 1; }) |
                 rv::transform([](int x) { return x * 10; });
    println(view2);

    auto view3 = vec | rv::drop(3) | rv::take(4) | rv::transform([](int x) { return x * 2; });
    println(view3);

    auto adaptor = rv::filter([](int x) { return x % 2 == 0; }) | rv::transform([](int x) { return x + 100; });
    auto view4 = vec | adaptor;
    println(view4);

    vector<string> words = {"hello", "world", "cpp", "range", "view", "library"};
    auto view5 = words | rv::filter([](const string& s) { return s.length() > 3; }) |
                 rv::transform([](const string& s) { return s.length(); }) |
                 rv::filter([](size_t len) { return len < 6; });
    println(view5);

    auto view6 = vec | rv::filter([](int x) { return x <= 5; }) | rv::reverse();
    println(view6);


    int filter_count = 0;
    int transform_count = 0;

    auto view7 = vec | rv::filter([&filter_count](int x) {
                     ++filter_count;
                     println("  Filter called for", x);
                     return x % 2 == 0;
                 }) |
                 rv::transform([&transform_count](int x) {
                     ++transform_count;
                     println("  Transform called for", x);
                     return x * x;
                 });
    println("filter_count: ", filter_count, ", transform_count: ", transform_count);

    int result_count = 0;
    for (auto x: view7) {
        println(x);
        if (++result_count >= 3) {
            break;
        }
    }
    println("filter_count: ", filter_count, ", transform_count: ", transform_count);

    auto view8 = vec | rv::take(8) | rv::drop(2) | rv::filter([](int x) { return x % 2 == 1; }) |
                 rv::transform([](int x) { return x * x; }) | rv::filter([](int x) { return x < 50; });
    println(view8);

    auto view9 = rv::transform(rv::filter(vec, [](int x) { return x > 5; }), [](int x) { return x * 10; });
    println(view9);

    const vector<int> const_vec = {1, 2, 3, 4, 5};
    const auto view10 =
            const_vec | rv::filter([](int x) { return x % 2 == 1; }) | rv::transform([](int x) { return x * 5; });
    println(view10);

    auto view11 = vec | rv::take_while([](int x) { return x < 7; }) | rv::transform([](int x) { return x * 3; });
    println(view11);

    auto view12 = vec | rv::drop_while([](int x) { return x < 5; }) | rv::transform([](int x) { return x + 100; });
    println(view12);

    auto view13 = vec | rv::drop_while([](int x) { return x % 2 == 1; }) |
                  rv::take_while([](int x) { return x <= 8; }) | rv::transform([](int x) { return x * 2; });
    println(view13);

    vector<int> a{1, 2, 3};
    vector<int> b{4, 5, 6};

    auto view14 = a | rv::concat(b) | rv::transform([](int x) { return x * 10; });
    println(view14);

    string sentence = "hello world cpp ranges views";
    auto word_str = sentence | rv::split(' ');
    auto upper_words = word_str | rv::transform([](auto subrange) {
                           return subrange | rv::transform([](char c) { return _NEFORCE to_uppercase(c); });
                       });

    for (const auto& word_view: upper_words) {
        for (char c: word_view) {
            print(c);
        }
        println();
    }

    auto view15 = vec | rv::slice(2, 5) | rv::filter([](int x) { return x > 4; }) | rv::take(2);
    println(view15);
#endif
}

void simple_task(const string& name) { println("Executing task: " + name); }

int compute_sum(int a, int b) {
    int result = a + b;
    println("Computing: " + to_string(a) + " + " + to_string(b) + " = " + to_string(result));
    return result;
}

void periodic_work(int counter) { println("Periodic task #" + to_string(counter)); }


static auto& thread_pool_instance() {
    static thread_pool instance;
    return instance;
}


void test_ext_tpool() {
    auto& pool = thread_pool_instance();
    pool.start(5);

    pool.submit_task([] { println("Normal task"); });
    pool.submit_task(static_cast<thread_pool::priority_type>(10), [] { println("High priority task"); });
    pool.submit_task(static_cast<thread_pool::priority_type>(1), [] { println("Low priority task"); });
    pool.submit_after(1000, static_cast<thread_pool::priority_type>(5), [] { println("Delayed high priority"); });
    this_thread::sleep_for(seconds(3));

    println(timestamp::now());
    auto future1 = pool.submit_after(2000, []() { println(timestamp::now()); });
    this_thread::sleep_for(seconds(3));

    auto future2 = pool.submit_after(1000, simple_task, "Task with parameters");
    auto future3 = pool.submit_after(1500, compute_sum, 42, 58);
    println(future3.future.get());

    auto counter = make_shared<atomic<int>>(0);
    thread_pool::periodic_token token1;

    token1 = pool.submit_every(1000, [counter, &pool, &token1]() {
        int count = counter->fetch_add(1) + 1;
        println("Periodic task iteration #" + to_string(count));

        if (count >= 5) {
            println("Cancelling periodic task after 5 iterations");
            pool.cancel_periodic_task(token1);
        }
    });
    this_thread::sleep_for(seconds(6));

    thread_pool::periodic_token token2 = pool.submit_every(500, []() { println("Fast periodic task executing..."); });
    this_thread::sleep_for(seconds(3));
    pool.cancel_periodic_task(token2);
    this_thread::sleep_for(seconds(2));

    auto token3 = pool.submit_every(800, []() { println("  Task A (800ms interval)"); });
    auto token4 = pool.submit_every(1200, []() { println("  Task B (1200ms interval)"); });
    auto token5 = pool.submit_every(1500, []() { println("  Task C (1500ms interval)"); });
    this_thread::sleep_for(seconds(5));

    auto state = pool.statistics();
    println(state);
    pool.cancel_periodic_task(token3);
    pool.cancel_periodic_task(token4);
    pool.cancel_periodic_task(token5);
    this_thread::sleep_for(seconds(2));

    pool.stop();
}

void test_tpool() {
    auto& pool = thread_pool_instance();
    pool.start();
    click clk;
    {
        scoped_click grd(clk);
        pool.submit_task([&pool] {
            pool.submit_task(test_sysinfo);
            pool.submit_task(test_locale);
            pool.submit_task(test_file);
            pool.submit_task(test_datetimes);
            pool.submit_task(test_print);
            pool.submit_task(test_env_var);
            pool.submit_task(test_rnd);
            pool.submit_task(test_atomic);
            pool.submit_task(test_regex);
            pool.submit_task(test_format);
            pool.submit_task(test_color);
            pool.submit_task(test_enctype);
            pool.submit_task(test_ini);
            pool.submit_task(test_toml);
            pool.submit_task(test_json);
            pool.submit_task(test_list);
            pool.submit_task(test_deque);
            pool.submit_task(test_stack);
            pool.submit_task(test_vector);
            pool.submit_task(test_pqueue);
            pool.submit_task(test_rbtree);
            pool.submit_task(test_hashtable);
            pool.submit_task(test_tuple);
            pool.submit_task(test_variant);
            pool.submit_task(test_option);
            pool.submit_task(test_reflect);
            pool.submit_task(test_any);
            pool.submit_task(test_math);
            pool.submit_task(test_sql);
            pool.submit_task(test_lz4);
            pool.submit_task(test_zlib);
        });
        println(pool.stop());
    }
    println(clk.during().count());
}
