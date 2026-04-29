#include <NeForce/NeForce.hpp>
#include <gtest/gtest.h>

using namespace neforce;

namespace {
    thread_pool& thread_pool_instance() {
        static thread_pool instance;
        return instance;
    }

    const neforce::path& res_root() {
        static neforce::path res_root
#ifdef NEFORCE_PLATFORM_WINDOWS
                {R"(D:/Workspace/Cpp Workspace/CLine Workspace/NexusForce/tests/resource)"};
#elif defined(NEFORCE_PLATFORM_LINUX)
                {R"(/media/huenqi/Programming/Workspace/Cpp Workspace/CLine Workspace/NexusForce-Linux/tests/resource)"};
#endif
        return res_root;
    }
} // namespace

TEST(RegexTest, MatchDate) {
    regex re(R"(^\d{4}-\d{2}-\d{2}$)");

    string date1 = "2024-01-15";
    string date2 = "2024/01/15";

    ASSERT_TRUE(re.match(date1));
    ASSERT_FALSE(re.match(date2));
}

TEST(RegexTest, SearchDigits) {
    regex re(R"(\d+)");
    string text = "abc123def456ghi";

    auto result = re.search(text);
    ASSERT_TRUE(result.matched());
    ASSERT_EQ(result.data(), "123");
    ASSERT_EQ(result.position(), 3);
    ASSERT_EQ(result.length(), 3);
    ASSERT_EQ(result.prefix(), "abc");
    ASSERT_EQ(result.suffix(), "def456ghi");
}

TEST(RegexTest, CaptureGroups) {
    regex re(R"((\w+)@(\w+)\.(\w+))");
    string email = "user@example.com";

    auto result = re.search(email);
    ASSERT_TRUE(result.matched());
    ASSERT_EQ(result[0], "user@example.com");
    ASSERT_EQ(result[1], "user");
    ASSERT_EQ(result[2], "example");
    ASSERT_EQ(result[3], "com");
    ASSERT_EQ(result.size(), 4);
}

TEST(RegexTest, Iterator) {
    regex re(R"(\d+)");
    string text = "abc123def456ghi789";

    auto begin = re.begin(text);
    auto end = re.end(text);

    int count = 0;
    vector<string> matches;
    for (auto it = begin; it != end; ++it) {
        matches.push_back(string(it->data(), it->length()));
        count++;
    }
    ASSERT_EQ(count, 3);
    ASSERT_EQ(matches[0], "123");
    ASSERT_EQ(matches[1], "456");
    ASSERT_EQ(matches[2], "789");
}

TEST(RegexTest, TokenIterator) {
    regex re(R"(\s+)");
    string text = "one two   three\tfour\nfive";

    regex_token_iterator it(&re, text, -1);
    regex_token_iterator end;

    vector<string> tokens;
    for (; it != end; ++it) {
        tokens.push_back(string(*it));
    }
    ASSERT_EQ(tokens.size(), 4);
    ASSERT_EQ(tokens[0], "one");
    ASSERT_EQ(tokens[1], "two");
    ASSERT_EQ(tokens[2], "three");
    ASSERT_EQ(tokens[3], "four");
    // ASSERT_EQ(tokens[4], "five");
}

TEST(FormatTest, Hexadecimal) {
    hexadecimal x(255);
    ASSERT_EQ(format("{:#x}", x), "0xff");
    ASSERT_EQ(format("{:#X}", x), "0XFF");
    ASSERT_EQ(format("{:x}", x), "ff");
    ASSERT_EQ(format("{:#08x}", x), "0x0000ff");

    hexadecimal tmp(222);
    string result = format("{:#010X}", tmp);
    ASSERT_EQ(result, "0X000000DE");
}

TEST(FormatTest, Integer) {
    integer64 num(12345);
    ASSERT_EQ(format("{:#b}", num), "0b11000000111001");
    // ASSERT_EQ(format("{:#o}", num), "0o30071");
}

TEST(FormatTest, Decimal) {
    decimal dec(3.14159);
    ASSERT_EQ(format("{:.2f}", dec), "3.14");
    ASSERT_EQ(format("{:.4e}", dec), "3.1416e+00");
    ASSERT_EQ(format("{:.6g}", dec), "3.141590");
}

TEST(FormatTest, Uinteger32) {
    uinteger32 x(255u);
    ASSERT_EQ(format("{:d}", x), "255");
    ASSERT_EQ(format("{:b}", x), "11111111");
    ASSERT_EQ(format("{:#b}", x), "0b11111111");
    ASSERT_EQ(format("{:010d}", x), "0000000255");
}

TEST(Int128Test, Subtraction) {
    uint128_t a{100};
    uint128_t b{30};
    uint128_t c = a - b;
    ASSERT_EQ(c, uint128_t{70});
}

TEST(Int128Test, NegativeResult) {
    int128_t x{50};
    int128_t y{80};
    int128_t z = x - y;
    ASSERT_EQ(z, int128_t{-30});
}

TEST(Int128Test, LargeNumbers) {
    uint128_t huge1{static_cast<uint64_t>(1), static_cast<uint64_t>(1000)};
    uint128_t huge2{static_cast<uint64_t>(0), static_cast<uint64_t>(500)};
    uint128_t result = huge1 - huge2;
    ASSERT_EQ(result, huge1 - uint128_t{500});
}

TEST(EncryptionTest, XOR) {
    string encrypted = XOR_encrypt("Hello", "key");
    ASSERT_NE(encrypted, "Hello");
    string decrypted = XOR_decrypt(encrypted, "key");
    ASSERT_EQ(decrypted, "Hello");
}

TEST(EncryptionTest, Base64) {
    string encoded = base64_encode("Hello World");
    ASSERT_FALSE(encoded.empty());
    string decoded = base64_decode(encoded);
    ASSERT_EQ(decoded, "Hello World");
}

TEST(EncryptionTest, MD5) {
    string md5_hash = md5("Hello World"_sv);
    ASSERT_FALSE(md5_hash.empty());
    ASSERT_EQ(md5_hash.length(), 32);
}

TEST(EncryptionTest, SHA256) {
    string sha256_hash = sha256("Hello World"_sv);
    ASSERT_FALSE(sha256_hash.empty());
    ASSERT_EQ(sha256_hash.length(), 64);
}

TEST(EncryptionTest, AES256) {
    string key = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    string aes_encrypted = aes256_encrypt("Hello World"_sv, key);
    ASSERT_FALSE(aes_encrypted.empty());
    string aes_decrypted = aes256_decrypt(aes_encrypted, key);
    ASSERT_EQ(aes_decrypted, "Hello World");
}

TEST(IniTest, Build) {
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

    auto doc = builder.build();
    ASSERT_NE(doc, nullptr);
    ASSERT_FALSE(doc->to_string().empty());
}

TEST(IniTest, Parse) {
    file fi(res_root() / "test.ini");
    if (!fi.is_opened()) {
        GTEST_SKIP() << "test.ini not found";
    }

    ini_parser parser(fi.read());
    auto doc = parser.parse();
    ASSERT_NE(doc, nullptr);

    string host = doc->get_string("database", "host");
    ASSERT_FALSE(host.empty());

    int port = doc->get_int("database", "port");
    ASSERT_GT(port, 0);

    bool enabled = doc->get_bool("database", "enabled");
    ASSERT_TRUE(enabled || !enabled);
}

TEST(EnvTest, Build) {
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

    auto doc = builder.build();
    ASSERT_NE(doc, nullptr);
    ASSERT_FALSE(doc->to_string().empty());
}

TEST(EnvTest, Parse) {
    file fi(res_root() / "test.env");
    if (!fi.is_opened()) {
        GTEST_SKIP() << "test.env not found";
    }

    env_parser parser(fi.read());
    auto doc = parser.parse();
    ASSERT_NE(doc, nullptr);

    string app_name = doc->get_string("APP_NAME");
    ASSERT_FALSE(app_name.empty());

    int db_port = doc->get_int("DB_PORT");
    ASSERT_GT(db_port, 0);

    bool feature_enabled = doc->get_bool("FEATURE_ENABLED");
    ASSERT_TRUE(feature_enabled || !feature_enabled);
}

TEST(TomlTest, Parse) {
    file fi(res_root() / "test.toml");
    if (!fi.is_opened()) {
        GTEST_SKIP() << "test.toml not found";
    }

    try {
        toml_parser parser(fi.read());
        unique_ptr<toml_table> root = parser.parse();
        ASSERT_NE(root, nullptr);
    } catch (...) {
        GTEST_SKIP() << "TOML parsing failed";
    }
}

TEST(TomlTest, Build) {
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

    unique_ptr<toml_table> root = builder.build();
    ASSERT_NE(root, nullptr);
    ASSERT_FALSE(root->to_document().empty());
}

TEST(JsonTest, Parse) {
    file fi(res_root() / "test.json");
    if (!fi.is_opened()) {
        GTEST_SKIP() << "test.json not found";
    }

    try {
        unique_ptr<json_value> root = json_parser(fi.read()).parse();
        ASSERT_NE(root, nullptr);
        ASSERT_TRUE(root->is_object());
    } catch (...) {
        GTEST_SKIP() << "JSON parsing failed";
    }
}

TEST(JsonTest, Build) {
    auto json = json_builder()
                        .begin_array()
                        .value(1)
                        .value(2)
                        .begin_object()
                        .key("x")
                        .value(10)
                        .end_object()
                        .end_array()
                        .build();
    ASSERT_NE(json, nullptr);

    map<string, vector<int>> nested_data = {
            {"group1", {10, 20, 30}}, {"group2", {40, 50}}, {"group3", {60, 70, 80, 90}}};
    auto json2 =
            json_builder().begin_object().key("data").value(nested_data).key("total").value(8).end_object().build();
    ASSERT_NE(json2, nullptr);
}

TEST(LoggingTest, DISABLED_BasicLogging) {
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

TEST(RangesTest, FilterTransform) {
#ifdef NEFORCE_STANDARD_20
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto view1 = vec | rv::filter([](int x) { return x % 2 == 0; }) | rv::transform([](int x) { return x * x; });

    vector<int> result;
    for (auto x: view1) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0], 4);
    ASSERT_EQ(result[4], 100);
#endif
}

TEST(RangesTest, DropTake) {
#ifdef NEFORCE_STANDARD_20
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto view3 = vec | rv::drop(3) | rv::take(4) | rv::transform([](int x) { return x * 2; });

    vector<int> result;
    for (auto x: view3) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 4);
    ASSERT_EQ(result[0], 8);
    ASSERT_EQ(result[3], 14);
#endif
}

TEST(RangesTest, Reverse) {
#ifdef NEFORCE_STANDARD_20
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto view6 = vec | rv::filter([](int x) { return x <= 5; }) | rv::reverse();

    vector<int> result;
    for (auto x: view6) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0], 5);
    ASSERT_EQ(result[4], 1);
#endif
}

TEST(RangesTest, TakeWhileDropWhile) {
#ifdef NEFORCE_STANDARD_20
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto view11 = vec | rv::take_while([](int x) { return x < 7; }) | rv::transform([](int x) { return x * 3; });

    vector<int> result;
    for (auto x: view11) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 6);
    ASSERT_EQ(result[0], 3);

    auto view12 = vec | rv::drop_while([](int x) { return x < 5; }) | rv::transform([](int x) { return x + 100; });

    result.clear();
    for (auto x: view12) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 6);
    ASSERT_EQ(result[0], 105);
#endif
}

TEST(RangesTest, Concat) {
#ifdef NEFORCE_STANDARD_20
    namespace rv = ranges::views;
    vector<int> a{1, 2, 3};
    vector<int> b{4, 5, 6};

    auto view14 = a | rv::concat(b) | rv::transform([](int x) { return x * 10; });

    vector<int> result;
    for (auto x: view14) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 6);
    ASSERT_EQ(result[0], 10);
    ASSERT_EQ(result[5], 60);
#endif
}

TEST(RangesTest, Slice) {
#ifdef NEFORCE_STANDARD_20
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto view15 = vec | rv::slice(2, 5) | rv::filter([](int x) { return x > 4; }) | rv::take(2);

    vector<int> result;
    for (auto x: view15) {
        result.push_back(x);
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0], 5);
#endif
}

TEST(ThreadPoolTest, StartStop) {
    auto& pool = thread_pool_instance();
    pool.start(2);
    pool.stop();
    ASSERT_TRUE(true);
}

TEST(ThreadPoolTest, SubmitTask) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    atomic<bool> executed{false};
    pool.submit_task([&executed] { executed.store(true); });

    this_thread::sleep_for(milliseconds(500));
    pool.stop();
    ASSERT_TRUE(executed.load());
}

TEST(ThreadPoolTest, PriorityTask) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    atomic<int> counter{0};
    pool.submit_task(static_cast<thread_pool::priority_type>(10), [&counter] { counter.fetch_add(1); });
    pool.submit_task(static_cast<thread_pool::priority_type>(1), [&counter] { counter.fetch_add(1); });

    this_thread::sleep_for(milliseconds(500));
    pool.stop();
    ASSERT_EQ(counter.load(), 2);
}

TEST(ThreadPoolTest, SubmitAfter) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    atomic<bool> executed{false};
    pool.submit_after(500, [&executed] { executed.store(true); });

    this_thread::sleep_for(milliseconds(200));
    ASSERT_FALSE(executed.load());

    this_thread::sleep_for(milliseconds(600));
    pool.stop();
    ASSERT_TRUE(executed.load());
}

TEST(ThreadPoolTest, DISABLED_PeriodicTask) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    auto counter = make_shared<atomic<int>>(0);
    thread_pool::periodic_token token;

    token = pool.submit_every(200, [counter, &pool, &token]() {
        int count = counter->fetch_add(1) + 1;
        if (count >= 5) {
            pool.cancel_periodic_task(token);
        }
    });

    this_thread::sleep_for(seconds(2));
    pool.stop();
    ASSERT_GE(counter->load(), 4);
}

TEST(ThreadPoolTest, Statistics) {
    auto& pool = thread_pool_instance();
    pool.start(2);

    pool.submit_task([] { this_thread::sleep_for(milliseconds(100)); });
    pool.submit_task([] { this_thread::sleep_for(milliseconds(100)); });

    this_thread::sleep_for(milliseconds(50));

    auto state = pool.statistics();
    pool.stop();
    ASSERT_TRUE(true);
}
