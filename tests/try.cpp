#include "try.h"

static const path TEST_FILE{"test_temp_file.txt"};
static const path TEST_DIR{"test_temp_dir"};
static const path TEST_SUB_DIR{TEST_DIR / "sub_dir"};
static const string TEST_CONTENT = "Hello!\nSecond line.\r\nThird line";

static const path res_root
#ifdef MSTL_PLATFORM_WINDOWS__
    {R"(D:/Workspace/Cpp Workspace/CLine Workspace/MSTL/tests/resource)"};
#elif defined(MSTL_PLATFORM_LINUX__)
    {R"(/home/huenqi/Workspace/MSTL/tests/resource)"};
    // {R"(/mnt/d/Workspace/Cpp Workspace/CLine Workspace/MSTL/tests/resource)"};
#endif

void test_file_basic_operations() {
    bool create_ok = file::create_and_write(TEST_FILE, TEST_CONTENT);
    assert(create_ok);
    assert(TEST_FILE.exists());
    assert(TEST_FILE.is_file());
    assert(!TEST_FILE.is_directory());

    assert(file::size(TEST_FILE) == TEST_CONTENT.size());

    string read_content;
    bool read_ok = file::read(TEST_FILE, read_content);
    assert(read_ok);
    assert(read_content == TEST_CONTENT);

    {
        file f;
        assert(!f.opened());
        assert(f.open(TEST_FILE));
        assert(f.opened());
        assert(f.get_path() == TEST_FILE);

        string line;
        assert(f.read_line(line));
        assert(line == "Hello!");
        assert(f.read_line(line));
        assert(line == "Second line.");
        assert(f.read_line(line));
        assert(line == "Third line");

        assert(f.seek(0, FILE_POINTER::BEGIN));
        assert(f.tell() == 0);
        assert(f.seek(5, FILE_POINTER::CURRENT));
        assert(f.tell() == 5);

        assert(f.truncate(10));
        assert(f.size() == 10);

        assert(f.seek(0, FILE_POINTER::BEGIN));
        string new_content = "New content after truncate";
        size_t written = f.write(new_content);
        assert(written == new_content.size());
        assert(f.flush());

        assert(f.size() == new_content.size());

        f.close();
        assert(!f.opened());
    }
    println("test file basic operations passed");
}

void test_directory_operations() {
    assert(!TEST_SUB_DIR.exists());
    bool dir_ok = TEST_SUB_DIR.create_directories();
    assert(dir_ok);
    assert(TEST_SUB_DIR.exists());
    assert(TEST_SUB_DIR.is_directory());

    path sub_file = {TEST_SUB_DIR / "sub_file.txt"};
    assert(file::create_and_write(sub_file, "sub content"));
    assert(sub_file.exists());
    println("test dictionary operations passed");
}

void test_file_attributes_and_times() {
    file f(TEST_FILE);
    assert(f.open(TEST_FILE));

    _MSTL FILE_ATTRI original_attr = f.attributes();
    bool set_attr_ok = f.set_attributes(_MSTL FILE_ATTRI::READONLY);
    assert(set_attr_ok);
    assert(static_cast<bool>(f.attributes() & _MSTL FILE_ATTRI::READONLY));
    assert(f.set_attributes(original_attr));
    assert(f.attributes() == original_attr);

    _MSTL datetime now = _MSTL datetime::now();
    bool set_time_ok = f.set_last_write_time(now);
    assert(set_time_ok);
    assert(f.last_write_time() == now);

    f.close();
    println("test file attributes and times passed");
}

void test_file_lock_and_other_operations() {
    {
        file f(TEST_FILE);
        assert(f.open(TEST_FILE));

        bool locked = f.lock(0, 10, FILE_LOCK::EXCLUSIVE);
        assert(locked);
        bool unlocked = f.unlock(0, 10);
        assert(unlocked);
    }

    path copy_file{TEST_FILE.str() + ".copy"};
    assert(TEST_FILE.copy(copy_file));
    assert(copy_file.exists());
    string copy_content;
    file::read(copy_file, copy_content);

    path move_file{TEST_DIR / "moved_file.txt"};
    assert(copy_file.move(move_file));
    assert(!copy_file.exists());
    assert(move_file.exists());

    path rename_file{TEST_DIR / "renamed_file.txt"};
    assert(move_file.rename(rename_file));
    assert(!move_file.exists());
    assert(rename_file.exists());
    println("test file lock and other operations passed");
}

void test_move_semantics() {
    file f1(TEST_FILE);
    assert(f1.open(TEST_FILE));
    file f2 = _MSTL move(f1);
    assert(!f1.opened());
    assert(f2.opened());
    assert(f2.get_path().str() == TEST_FILE.str());

    file f3;
    f3 = _MSTL move(f2);
    assert(!f2.opened());
    assert(f3.opened());
    assert(f3.get_path() == TEST_FILE);
    println("test move semantics passed");
}

void clean_up() {
    if (TEST_FILE.exists()) {
        if (TEST_FILE.is_directory()) {
            TEST_FILE.remove_directory();
        } else {
            TEST_FILE.remove();
        }
    }
    path sub_file = TEST_SUB_DIR / "sub_file.txt";
    if (sub_file.exists()) {
        sub_file.remove();
    }
    path rename_file = TEST_DIR / "renamed_file.txt";
    if (rename_file.exists()) {
        rename_file.remove();
    }
    if (TEST_SUB_DIR.exists()) {
        TEST_SUB_DIR.remove_directory();
    }
    if (TEST_DIR.exists()) {
        TEST_DIR.remove_directory();
    }
}

void test_file() {
    clean_up();
    try {
        test_file_basic_operations();
        test_directory_operations();
        test_file_lock_and_other_operations();
        test_move_semantics();
        test_file_attributes_and_times();
        clean_up();
    } catch (...) {
        clean_up();
    }
}


void test_date() {
    _MSTL date d1(2024, 2, 29);
    assert(d1.year() == 2024 && d1.month() == 2 && d1.day() == 29);

    _MSTL date d2(2023, 2, 29);
    assert(d2 == _MSTL date::epoch());

    assert(_MSTL date::is_leap_year(2020) == true);
    assert(_MSTL date::is_leap_year(2019) == false);
    assert(_MSTL date::is_leap_year(2100) == false);
    assert(_MSTL date::is_leap_year(2400) == true);

    assert(_MSTL date::days_of_month(2024, 2) == 29);
    assert(_MSTL date::days_of_month(2023, 2) == 28);
    assert(_MSTL date::days_of_month(2023, 4) == 30);

    _MSTL date d3(2024, 1, 1);
    assert(d3.days_of_week() == 1);

    _MSTL date d4(2024, 3, 1);
    assert(d4.days_of_year() == 61);

    _MSTL date d5(2024, 2, 28);
    d5 += 2;
    assert(d5.month() == 3 && d5.day() == 1);

    _MSTL date d6(2024, 3, 1);
    d6 -= 1;
    assert(d6.month() == 2 && d6.day() == 29);

    assert(_MSTL date(2024, 1, 1) < _MSTL date(2024, 1, 2));
    assert(_MSTL date(2024, 1, 1) > _MSTL date(2023, 12, 31));

    auto str = _MSTL date(2024, 5, 10).to_string();
    assert(str == "2024-05-10");
    assert(_MSTL date::parse("2024-05-10") == _MSTL date(2024, 5, 10));
    assert(_MSTL date().try_parse("invalid") == false);

    println("test_date passed");
}

void test_time() {
    using MSTL::time;
    _MSTL time t1(23, 59, 59);
    assert(t1.hours() == 23 && t1.minutes() == 59 && t1.seconds() == 59);

    _MSTL time t2(25, 60, 60);
    assert(t2 == _MSTL time(0, 0, 0));

    assert(_MSTL time(1, 2, 3).to_seconds() == 3600 + 120 + 3);

    _MSTL time t3(23, 59, 59);
    t3 += 2;  // 00:00:01
    assert(t3.hours() == 0 && t3.seconds() == 1);

    _MSTL time t4(0, 0, 1);
    t4 -= 2;  // 23:59:59
    assert(t4.hours() == 23 && t4.seconds() == 59);

    assert(_MSTL time(12, 0, 0) < _MSTL time(13, 0, 0));
    assert(_MSTL time(12, 30, 0) > _MSTL time(12, 29, 59));

    assert(_MSTL time(9, 8, 7).to_string() == "09:08:07");
    assert(_MSTL time::parse("09:08:07") == _MSTL time(9, 8, 7));
    assert(_MSTL time().try_parse("invalid") == false);

    println("test_time passed");
}

void test_datetime() {
    using MSTL::time;
    _MSTL datetime dt1(_MSTL date(2024, 1, 1), _MSTL time(12, 0, 0));
    assert(dt1.year() == 2024 && dt1.hours() == 12);

    _MSTL datetime dt2(2024, 2, 28, 23, 59, 59);
    dt2 += 2;
    assert(dt2.month() == 2 && dt2.day() == 29 && dt2.seconds() == 1);

    _MSTL datetime dt3(2024, 3, 1, 0, 0, 0);
    dt3 -= 1;
    assert(dt3.month() == 2 && dt3.day() == 29);

    _MSTL datetime dt4(2024, 1, 1, 0, 0, 0);
    _MSTL datetime dt5(2023, 12, 31, 23, 59, 59);
    assert(dt4 - dt5 == 1);

    assert(_MSTL datetime(2024, 5, 10, 9, 8, 7).to_string() == "2024-05-10 09:08:07");
    assert(_MSTL datetime::parse("2024-05-10 09:08:07")
        == _MSTL datetime(2024, 5, 10, 9, 8, 7));
    assert(_MSTL datetime().try_parse("invalid") == false);

    println(datetime::now());

    println("test_datetime passed");
}

void test_timestamp() {
    _MSTL datetime epoch = _MSTL datetime::epoch();
    _MSTL timestamp ts1(epoch);
    assert(ts1.seconds() == 0);
    assert(ts1.to_datetime() == epoch);

    _MSTL timestamp ts2(86400);
    _MSTL datetime dt = ts2.to_datetime();
    assert(dt.day() == 2);

    _MSTL timestamp ts3(100);
    _MSTL timestamp ts4(200);
    assert(ts3 < ts4);
    assert(ts4 - ts3 == 100);

    println("test_timestamp passed");
}

void test_utc_conversion() {
    _MSTL datetime dt(2024, 1, 1, 0, 0, 0);
    _MSTL datetime utc = dt.to_UTC();
    _MSTL datetime local = _MSTL datetime::from_UTC(dt);
    assert(local != dt);

    println("test_utc_conversion passed");
}


void test_datetimes() {
    test_date();
    test_time();
    test_datetime();
    test_timestamp();
    test_utc_conversion();
}

void test_print() {
    decimal_t f = _CONSTANTS PI;
    _INNER FUNCTION_OPERATE enu = _INNER FUNCTION_OPERATE::GET_PTR;
    _INNER __nocopy_type uni{};
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
#ifdef MSTL_STANDARD_14__
    void (bit_reference::* mfp)() const = &bit_reference::flip;
#else
    void (bit_reference::* mfp)() const noexcept = &bit_reference::flip;
#endif
    typecast_exception err;
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
#ifdef MSTL_STANDARD_20__
    const char8_t* u8emoji = u8"\n胡Hello, World! 😇👩‍🦳🎗️⚽🥠🍋‍🟩⛴️🪣💖🚯🕕😊🌟🚀✔";
    println(u8emoji);
#endif

    string tup_str = to_string(tup);
    println(tup);
    println(pir, cp);
    println('c', nullptr);
    println(&RB_TREE_RED, &RB_TREE_BLACK);
    println(f, static_cast<size_t>(enu), uni);
    println(escape("\n\\\"\v"), cs, err);
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
    hexadecimal hex("F");
    println(hex);
    console.readln(hex);
    println(hex);
    int rawi = 3;
    println(rawi);
    console.read(rawi);
    println(rawi);
    _MSTL boolean b;
    b.try_parse("true");
    println(b);
    console.readln(b);
    println(b);
    integer32 i32;
    console.read(i32);
    println(i32);
    println();
    float32 fp;
    console.read(fp);
    println(fp);
    println(to_string(fp));
}

void test_dev() {
#ifdef MSTL_PLATFORM_WINDOWS__
    auto devs = diskdrive::enumerate_all();
    for (const auto &dev : devs) {
        println(dev);
    }
    console.pause();
    diskdrive& udisk = devs.back();

    if(udisk.volume_label() != "Standby") {
        println("剩余容量", udisk.free_capacity());
        path path{udisk.device_path()};

        println("正在卸载卷...");
        if (udisk.force_dismount()) {
            println("卷卸载成功");
        } else {
            println("卷卸载失败");
        }

        println("正在禁用设备...");
        if (udisk.base_drive().disable()) {
            println("设备禁用成功");
        } else {
            println("设备禁用失败");
        }

        this_thread::sleep_for_ms(2000);

        bool success = path::copy_directory(
            _MSTL path(R"(D:\Workspace\Cpp Workspace\CLine Workspace\MSTL\cmake-build-release-msvc-x64\bin)"),
            path
        );
        println("第一次拷贝结果:", success);
        console.pause();

        println("正在恢复访问权限...");
        udisk.base_drive().enable();

        success = path::copy_directory(
            _MSTL path(R"(D:\Workspace\Cpp Workspace\CLine Workspace\MSTL\cmake-build-release-msvc-x64\bin)"),
            path
        );
        println("第二次拷贝结果:", success);
        console.pause();

        println("正在删除拷贝文件...");
        path.remove_all_in_directory();
        console.pause();
    } else {
        println("无U盘设备");
    }
#endif
}


void test_rnd() {
    println(_MSTL secret::is_supported(), secret::next_double(), secret::next_int(1, 10));
    println(_MSTL random_lcd::next_int(10, 20), random_lcd::next_int(10, 20), random_lcd::next_int(10, 20));
    println(_MSTL random_mt::next_int(10, 20), random_mt::next_int(10, 20), random_mt::next_int(10, 20));
}

void test_format() {
    {
        hexadecimal x(255);
        println(format("{#x}", x));  // "0xff"
        println(format("{#X}", x));  // "0XFF"
        println(format("{x}", x));   // "ff"

        println(format("{#10x}", x));     // "      0xff"
        println(format("{*>#10x}", x));   // "******0xff"
        println(format("{0=#10x}", x));   // "0x000000ff"

        println(format("{<#10x}", x));  // "0xff      "
        println(format("{=#10x}", x));  // "0x      ff"

        println(format("{-=#10X}", x));  // "0X      FF"

        hexadecimal neg(-255);
        println(absolute(neg), sign(neg));
        println(format("{0=#10x}", neg)); // "-0x00000ff"
        println(format("{#10x}", neg));   // "      -0xff"

        println(format("{#x}", x));    // "0xff"
        println(format("{#X}", x));    // "0XFF"
        println(format("{#08x}", x));  // "0x0000ff"
        println(format("{x}", x));     // "ff"


        hexadecimal hex(255);
        string result1 = format("{#010X}", hex);  // "0X000000FF"
        println(result1);
        hexadecimal tmp(222);
        _MSTL swap(hex, tmp);
        println(format("{#010X}", hex));          // "0X000000DE"
        println(hexadecimal(222));

        integer64 num(12345);
        string result2 = format("{+>15d}", num);  // "        +12345"
        println(result2);

        string result3 = format("{#b}", num);  // "0b11000000111001"
        println(result3);

        decimal dec(3.14159);
        string result4 = format("{.2f}", dec);  // "3.14"
        println(result4);
        string result5 = format("{.3e}", dec);  // "3.141e+00"
        println(result5);

        string result6 = format("{^20x}", hex);      // "       de       "
        println(result6);
        string result7 = format("{#o}", num);        // "0o30071"
        println(result7);
        string result8 = format("{+10.4f}", dec);    // "   +3.1415"
        println(result8);

        string r1 = format("{.2f}", dec);  // "3.14"
        println(r1);
        string r2 = format("{.4e}", dec);  // "3.1415e+00"
        println(r2);
        string r3 = format("{.6g}", dec);  // "3.141589"
        println(r3);
        string r4 = format("{+10.3f}", dec);  // "    +3.141"
        println(r4);
    }
    {
        uinteger32 x(255u);
        println(format("{d}", x));  // "255"
        println(format("{b}", x));  // "11111111"
        println(format("{#b}", x));  // "0b11111111"
        println(format("{10d}", x));  // "       255"
        println(format("{<10d}", x)); // "255       "
        println(format("{010d}", x)); // "0000000255"

        MSTL::uint64_t ull = 123456789ULL;
        println(ull);
        println(format("{#x}", ull));
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
    println(semiRed.is_opacity());
    println(semiRed.transparent());
    println(semiRed.is_opaque());

    println(semiRed.blend(background));
    println(color::parse("#FF000080"));
    println(red * 0.5);

    color adjustableColor = color::red();
    adjustableColor.set_opacity(0.3);
    println(adjustableColor);
    println(semiRed.premultiply_alpha());


    color custom(128, 64, 192);

    printcln(color::red(), "这是红色文本");
    printcln(color::green(), "这是绿色文本");
    printcln(custom, "这是使用基本颜色的文本");

    console.set_color(custom, true);
    println("这是使用256色的文本");
    console.reset_color();
}


void test_enctype() {
    string encrypted = xor_encrypt("Hello", "key");
    string decrypted = xor_decrypt(encrypted, "key");
    println(escape(encrypted));
    println(decrypted);

    string encoded = base64_encode("Hello World");
    string decoded = base64_decode(encoded);
    println(encoded);
    println(decoded);

    string md5_hash = md5("Hello World");
    string sha256_hash = sha256("Hello World");
    println("MD5: ", md5_hash);
    println("SHA256: ", sha256_hash);

    string key = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    string aes_encrypted = aes256_encrypt("Hello World", key);
    string aes_decrypted = aes256_decrypt(aes_encrypted, key);
    println(aes_encrypted);
    println(aes_decrypted);
}

void test_ini() {
    ini_builder builder;
    builder.key("global_key").value("global_value")
           .begin_section("database")
               .key("host").value("localhost")
               .key("port").value(5432)
               .key("enabled").value(true)
           .end_section()
           .begin_section("logging")
               .key("level").value("debug")
               .key("file").value("/var/log/app.log")
           .end_section();

    println(builder.build()->to_string());


    file fi(res_root / "test.ini");

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
           .key("DB_PASSWORD").double_quoted().value("secret123")
           .blank_line()
           .comment("Feature Flags")
           .add("FEATURE_ENABLED", true)
           .add_export("PATH", "/usr/local/bin:$PATH");

    auto bdoc = builder.build();
    println(bdoc->to_string());


    file fi(res_root / "test.env");

    env_parser parser(fi.read());
    auto doc = parser.parse();

    string app_name = doc->get_string("APP_NAME");
    int db_port = doc->get_int("DB_PORT");
    bool feature_enabled = doc->get_bool("FEATURE_ENABLED");

    println(app_name, ", ", db_port, ", ", feature_enabled);
}

void test_toml() {
    file fi(res_root / "test.toml");

    try {
        toml_parser parser(fi.read());
        unique_ptr<toml_table> root = parser.parse();
        printcln(color::blue(), root->to_document());

        toml_builder builder;
        builder.key("title").value("My Config")
               .key("owner").value_inline_table([](toml_builder& b) {
                   b.key("name").value("Tom")
                    .key("dob").value_datetime("1979-05-27T07:32:00Z", toml_datetime::OffsetDateTime);
               })
               .begin_table("database")
                   .key("server").value("192.168.1.1")
                   .key("ports").value_array([](toml_builder& b) {
                       b.value(8001).value(8002).value(8003);
                   })
               .end_table();

        unique_ptr<toml_table> broot = builder.build();
        println(broot->to_document());
    } catch (...) {}
}

void test_yaml() {
    try {
        file fi(res_root / "test.yaml");
        yaml_parser parser(fi.read());
        auto docs = parser.parse_documents();
        for (const auto& doc : docs) {
            println(doc->to_document());
        }
    } catch (...) {}
}

void test_json() {
    file fi(res_root / "test.json");

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
            .key("x").value(10)
            .end_object()
            .end_array()
            .build();
        println(*json2);

        map<string, vector<int>> nested_data = {
            {"group1", {10, 20, 30}},
            {"group2", {40, 50}},
            {"group3", {60, 70, 80, 90}}
        };
        auto json3 = json_builder()
            .begin_object()
            .key("data").value(nested_data)
            .key("total").value(8)
            .end_object()
            .build();
        println(*json3);

    } catch (...) {}
}

void test_https_server() {
    try {
        http_server server(8443, 128, "/home/huenqi/server.crt", "/home/huenqi/server.key");

        http_router& r = server.router();

        r.get("/", [](http_request& req, http_response& res) {
            printcln(color::cyan(), "HTTPS Request from: " + req.header("User-Agent"));

            res.set_ok();
            res.set_status_msg("OK");
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);

            string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>HTTPS Test</title>
</head>
<body>
    <h1>HTTPS Connection Successful!</h1>
    <p>This is served over HTTPS.</p>
    <p>SSL/TLS is working correctly.</p>
    <div id="status"></div>

    <script>
        document.getElementById('status').textContent =
            'Protocol: ' + window.location.protocol;
    </script>
</body>
</html>
            )";
            res.set_body(html);
        });

        r.get("/api/info", [](http_request& req, http_response& res) {
            res.set_ok();
            res.set_content_type(HTTP_CONTENT::JSON_APP);

            json_builder response;
            response.begin_object()
                .key("https").value(req.is_https())
                .key("method").value(req.method().to_string())
                .key("path").value(req.path())
                .key("user_agent").value(req.header("User-Agent"))
                .end_object();

            res.set_body(response.build()->to_string());
        });

        r.post("/api/echo", [](http_request& req, http_response& res) {
            res.set_ok();
            res.set_content_type(HTTP_CONTENT::JSON_APP);

            json_builder response;
            response.begin_object()
                .key("https").value(req.is_https())
                .key("body").value(req.body())
                .key("content_type").value(req.content_type())
                .end_object();

            res.set_body(response.build()->to_string());
        });

        r.set_not_found_handler([](http_request&, http_response& res) {
            res.set_not_found();
            res.set_body("HTTPS 404 - Not Found");
        });

        if (server.start()) {
            printcln(color::green(), "HTTPS Server started on port 8443");
            printcln(color::yellow(), "Note: Using self-signed certificate");
            printcln(color::yellow(), "Press Ctrl+C to stop");

            while (true) {
                _MSTL this_thread::sleep_for(_MSTL_CHRONO seconds(1));
            }
        }
    } catch (const exception& e) {
        printcln(color::red(), "HTTPS Server error: " + string(e.what()));
    }
}

void test_http_server() {
    try {
        http_server server(8443, 128, "/home/huenqi/server.crt", "/home/huenqi/server.key");

        http_router& r = server.router();
        r.use(new logging_filter());
        r.use(new cors_filter("http://127.0.0.1:5500"));
        r.use(new static_file_filter(res_root.str()));

        r.post("/old-link", [](http_request&, http_response& response) {
            response.set_redirect("/new-link");
        });
        r.post("/forward-me", [](http_request&, http_response& response) {
            response.set_forward("/forward-target");
        });
        r.post("/forward-target", [](http_request&, http_response& response) {
            response.set_ok();
            response.set_status_msg("OK");
            response.set_body("Forward Successfully");
        });

        r.get_post("/api/session",
            [&server](http_request& req, http_response& res) {
                handle_session_api(req, res, server);
            }
        );
        r.get_post("/api/session-attribute",
            [&server](http_request& req, http_response& res) {
                handle_session_attribute(req, res, server);
            }
        );
        r.post_delete("/api/cookie",
            [](http_request& req, http_response& res) {
                handle_cookie_api(req, res);
            }
        );

        r.get("/api/logger-test", [](http_request&, http_response& res) {
            res.set_ok();
            res.set_status_msg("OK");
            res.set_body("Logging filter test successful");
        });
        r.get("/api/data", [](http_request&, http_response& res) {
            res.set_ok();
            res.set_status_msg("OK");
            res.set_content_type(HTTP_CONTENT::JSON_APP);
            res.set_body(R"({"status":"success"})");
        });

        r.get("/", [](http_request&, http_response& res) {
            res.set_ok();
            res.set_status_msg("OK");
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.set_body(file::read(res_root / "index.html"));
        });

        r.get("/detail", [](http_request&, http_response& res) {
            res.set_ok();
            res.set_status_msg("OK");
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.set_body(file::read(res_root / "detail.html"));
        });

        r.get("/new-link", [](http_request&, http_response& res) {
            res.set_ok();
            res.set_status_msg("OK");
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.set_body(file::read(res_root / "index.html"));
        });

        r.get("/test", [](http_request&, http_response& res) {
            res.set_ok();
            res.set_status_msg("OK");
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            res.set_body(file::read(res_root / "test.html"));
        });

        r.set_not_found_handler([](http_request&, http_response &res) {
            res.set_not_found();
            res.set_status_msg("Not Found");
            res.set_content_type(HTTP_CONTENT::HTML_TEXT);
            try {
                res.set_body(file::read(res_root / "404err.html"));
            } catch (...) {
                res.set_body("<h1>404 - Page Not Found</h1>");
            }
        });

        if (server.start()) {
            printcln(color::green(), "Press Ctrl+C to stop the server.");
            while (true) {
                _MSTL this_thread::sleep_for(_MSTL_CHRONO seconds(1));
            }
        }
        printcln(color::red(), "Failed to start server!");
    } catch (...) {}
}

void test_http_client() {
    try {
        http_client client;

        const auto response = client.request(http_client_request("www.example.com"));
        println("HTTP Version: ", response.version());
        println("Status Code: ", static_cast<int>(response.status()));
        println("Status Message: ", response.status_msg());
        println("Headers:");
        for (const auto& elem : response.all_headers()) {
            const auto& key = elem.first;
            const auto& values = elem.second;
            for (const auto& val : values) {
                println("  ", key, ": ", val);
            }
        }

        println();
        println("Body:");
        println(response.body());

        const auto& cookies = response.cookies();
        if (!cookies.empty()) {
            println();
            println("Cookies received:");
            for (const auto& c : cookies) {
                println("  ", c.name().cookie_name(), "=", c.value());
            }
        }
    } catch (...) {}
}


void test_list() {
    list<int> lls{ 1,2,3,4,5,6,7 };
    println(lls);
    lls.push_back(3);
    lls.push_back(4);
    lls.push_front(10);
    println(lls);
    lls.reverse();
    println(lls);
    lls.sort();
    lls.pop_back();
    lls.pop_front();
    println(lls);
    list<int> lls2 = { 5,3,2,1,1 };
    println(lls2);
    lls2.remove(5);
    lls2.sort();
    println(lls2);
    lls2.unique();
    println(lls2);
    list<MSTL::unique_ptr<int>> nocopy;
    // nocopy.emplace_back(2); also not support in std
    lls.clear();

    list<int> long_list;
    constexpr MSTL::size_t element_count = 100000;
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_list.push_back(i);
        } else {
            long_list.push_front(i);
        }
    }
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_list.pop_back();
        } else {
            long_list.pop_back();
        }
    }
    // println(long_list);
}

void test_check() {
    println(check_type<string>());
    println(check_type<const volatile void* const*&>());
    println(check_type<int(*)[]>());
    println(check_type<const volatile void* (&)[10]>()); // void const volatile * (&) [10]
    println(check_type<int[1][2][3]>());              // int [1] [2] [3]
    println(check_type<char(*(* const)(const int(&)[10]))[10]>());
    println(check_type<int (integer16::* const)[3]>());
    println(check_type<int (integer16::* const)(int, integer16&&, int) volatile>());
    string cstr("const string");
    const string* sr = new string("hai");
    println(check_type<decltype((cstr))>());
    println(check_type<decltype(MSTL::move(cstr))>());
    println(check_type<decltype(sr)>());
    delete sr;
}

void test_deque() {
    deque<int> a{1,2,3,4,5,6,7,8,9,10};
    println(a);
    a.push_back(2);
    a.push_front(10);
    a.push_back(3);
    a.push_back(7);
    a.push_back(6);
    a.insert(a.end(), 100);
    a.emplace(a.begin(), 0);
    println(a);
    a.pop_back();
    a.pop_front();
    println(a);
    a.assign(10, 5);
    println(a);
    deque<int> b{ 1,2,3,4,5 };
    println(b);
    deque<int> c(MSTL::move(b));
    c.resize(10, 6);
    println(c);

    deque<int> long_deque;
    constexpr MSTL::size_t element_count = 100000;
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_deque.push_back(i);
        } else {
            long_deque.push_front(i);
        }
    }
    // println(long_deque);
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_deque.pop_back();
        } else {
            long_deque.pop_front();
        }
    }
    // println(long_deque);
}

void test_stack() {
    stack<int> s;
    s.push(2);
    s.push(3);
    s.push(5);
    s.push(4);
    s.pop();

    stack<int> long_stack;
    constexpr MSTL::size_t element_count = 100000;
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        long_stack.push(i);
    }
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        long_stack.pop();
    }
    // println(long_stack);
}

void test_vector() {
    try{
        vector<int> v{ 1,2,3,4 };
        v.push_back(3);
        v.push_back(4);
        println(v);
        vector<int> v2(v);
        v.insert(v.end(), v2.cbegin(), v2.cend());
        println(v);
        v.pop_back();
        v.clear();
        println(v.empty());
        v.insert(v.end(), v2.cbegin(), v2.cend());
        println(v);
        const auto v3 = MSTL::move(v2);
        println(v3);
        vector<int> v4 = { 3,2,1 };
        v4.shrink_to_fit();
        v4.emplace(v4.begin() + 1, 5);
        v4.erase(--v4.end());
        println(v4);
        vector<int, ctype_allocator<int>> cvec;
        cvec.emplace_back(3);
        cvec.emplace_back(4);
        println(cvec);

        vector<int> vec;
        vec.assign(5, 10);
        println(vec);
        vec.assign({ 1, 2, 3, 4, 5 });
        println(vec);
        vector<int> anotherVec = { 6, 7, 8 };
        vec.assign(anotherVec.begin(), anotherVec.end());
        println(vec);
    }
    catch (exception& error) {
        println(error);
    }

    vector<int> long_vector;
    constexpr MSTL::size_t element_count = 100000;
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        long_vector.push_back(i);
    }
    for (MSTL::size_t i = 0; i < element_count; ++i) {
        long_vector.pop_back();
    }
    // println(long_vector);
}

void test_pqueue() {
    priority_queue<int> q;
    println(typeid(priority_queue<int*>).name());
    q.push(6); q.push(9); q.push(1); q.push(5);
    q.push(8); q.push(4); q.emplace(7); // 9 8 7 5 6 1 4
    q.pop();

    priority_queue<int> long_pque;
    constexpr MSTL::size_t element_count = 100000;
    for (int i = 0; i < element_count; ++i) {
        long_pque.push(random_lcd::next_int(10000));
    }
    for (int i = 0; i < element_count; ++i) {
        long_pque.pop();
    }
}

void test_rbtree() {
    map<int, char> m;
    m.insert(pair<int, char>(1, 'c'));
    m.emplace(3, 'c');
    m.emplace_hint(m.end(), 4, 'd');
    m[1] = 'a';
    m[100] = 'x';
    m[2] = 'b';
    println(m);
    m.erase(m.begin());
    println(m);
    m.clear();

    map<int, float> long_map;
    for (int i = 0; i < 100000; ++i) {
        int key = i;
        long_map.insert({key, random_lcd::next_double(0, 10000)});
    }
    for (int i = 0; i < 100000; ++i) {
        long_map.erase(i);
    }
    // println(long_map);


    multimap<int, const char*> mm;
    mm.emplace(1, "c");
    mm.emplace(2, "b");
    mm.emplace(1, "a");
    println(mm);
    mm.erase(mm.begin());
    mm.insert(mm.begin(), pair<int, const char*>(1, "a"));
    println(mm);
    mm.clear();

    multimap<int, float> long_multimap;
    for (int i = 0; i < 100000; ++i) {
        int key = i;
        long_multimap.insert({key, random_lcd::next_double(0, 10000)});
    }
    for (int i = 0; i < 100000; ++i) {
        long_multimap.erase(i);
    }
    // println(long_multimap);


    set<int> s{ 1,2,3,4,5 };
    s.insert(s.begin(), 1);
    s.emplace(2);
    println(s);
    s.erase(s.begin());
    println(s);
    s.clear();
    println(s);

    set<int> long_set;
    for (int i = 0; i < 100000; ++i) {
        long_set.emplace(i);
    }
    for (int i = 0; i < 100000; ++i) {
        long_set.erase(i);
    }
    // println(long_set);


    multiset<int> ms{ 4,5,6,7,8,8 };
    ms.insert(ms.begin(), 9);
    ms.emplace(10);
    println(ms);

    multiset<int> long_multiset;
    for (int i = 0; i < 100000; ++i) {
        long_multiset.emplace(i);
    }
    for (int i = 0; i < 100000; ++i) {
        long_multiset.erase(i);
    }
    // println(long_multiset);
}

void test_tuple() {
    tuple<int, char, const char*> t(1, 't', "MSTL");
    auto a = get<0>(t);
    println(MSTL::get<1>(t));
    auto forw = MSTL::make_tuple(9, 0);

    tuple<int, double> tuple1(1, 3.14);
    tuple<MSTL::string> tuple2("hello");
    tuple<char> tuple3('A');

    auto combinedTuple = MSTL::tuple_cat(tuple1, tuple2, tuple3);
    println(check_type<decltype(combinedTuple)>());

    println("Combined tuple elements:");
    println(MSTL::get<0>(combinedTuple));
    println(MSTL::get<1>(combinedTuple));
    println(MSTL::get<2>(combinedTuple));
    println(MSTL::get<3>(combinedTuple));

    tuple<int, int, int> args(1, 2, 3);
    int sum = MSTL::apply([](int a, int b, int c) {
        return a + b + c;
    }, args);
    println("Sum:", sum);

    tuple<int, int> mulArgs(4, 5);
    int product = MSTL::apply(multiplies<int>(), mulArgs);
    println("Product:", product);
}

void test_hashtable() {
    unordered_map<int, char> m;
    m[1] = 'a';
    m[2] = 'b';
    m.insert(pair<int, char>(3, 'c'));
    m.emplace(2, 'c');
    m.insert(pair<int, char>(1, 'b'));
    println(m);
    unordered_map<int, char> m2;
    m2.insert(m.begin(), m.end());
    println(m2);
    unordered_multimap<string, int> mm;
    mm.emplace("a", 1);
    mm.emplace("a", 2);
    mm.insert(pair<string, int>(string("a"), 1));
    println(mm);
    mm.clear();
    println(mm);
    unordered_map<int, unique_ptr<int>> uncopy;
    uncopy.emplace(1, MSTL::make_unique<int>(1));
    uncopy.erase(uncopy.begin());

    unordered_set<pair<int, char>> us;
    us.emplace(1, 'c');
    us.insert(pair<int, char>(4, 'r'));
    println(us);
    us.erase(pair<int, char>(4, 'r'));
    us.erase(us.begin());
    println(us);

    unordered_multiset<pair<int, const char*>> ms;
    ms.emplace(1, "234");
    ms.insert(MSTL::make_pair(2, "345"));
    ms.emplace(1, "234");
    println(ms);
    ms.erase(ms.begin());
    println(ms);

    unordered_set<float> fus;
    fus.insert(1.5);
    fus.insert(2.5);
    fus.insert(3.5);
    fus.insert(1.5);
    println(fus);

    unordered_multiset<float> fus2;
    fus2.insert(1.5);
    fus2.insert(2.5);
    fus2.insert(3.5);
    fus2.insert(1.5);
    string fus2_str = to_string(fus2);
    println(fus2_str);
}

void test_math() {
    println(power(2, 10));
    println(power(3, 10));
    println(factorial(10));
    println(sine(1));
    println(cosine(angular2radian(270)));
    println(remainder(73.263, 0.9973));
    println(float_part(_CONSTANTS PI));
    println(exponential(3));
    println(logarithm_e(165));
    println(logarithm_10(147));
    println(logarithm_2(500));
    println(arctangent(100));
    println(radian2angular(arctangent(100)));
    println(arcsine(1), arcsine(0), arcsine(-1));
    println(arccosine(1), arccosine(0), arccosine(-1));
    println(arctangent(numeric_limits<decimal_t>::max()), arctangent(numeric_limits<decimal_t>::min_nega()));
    // println(tangent(_CONSTANTS PI / 2));  // MathError
    println(tangent(0));
    println(around_pi(_CONSTANTS PI), " : ", around_pi(6.28));
}

void test_sort() {
    MSTL::vector<int> vec{ 6,9,1,5,8,4,7 };
    //insertion_sort(vec.begin(), vec.end());
    //bubble_sort(vec.begin(), vec.end());
    //select_sort(vec.begin(), vec.end());
    //shell_sort(vec.begin(), vec.end());
    //partial_sort(vec.begin(), vec.end(), vec.end());
    //counting_sort(vec.begin(), vec.end());
    //sort(vec.begin(), vec.end());
    //introspective_sort(vec.begin(), vec.end(), (size_t)logarithm_2(vec.end() - vec.begin()) * 2);
    //quick_sort(vec.begin(), vec.end());
    //merge_sort(vec.begin(), vec.end());
    //bucket_sort(vec.begin(), vec.end());
    radix_sort(vec.begin(), vec.end());
    //tim_sort(vec.begin(), vec.end());
    //monkey_sort(vec.begin(), vec.end());
    //smooth_sort(vec.begin(), vec.end());
    //cocktail_sort(vec.begin(), vec.end());
    println(vec);
    //vector<Person> people = {
    //{"Alice", 25},
    //{"Bob", 20},
    //{"Charlie", 30},
    //{"David", 20}
    //};
    //counting_sort(people.begin(), people.end(),
    //    [](const Person& a, const Person& b) -> bool { return a.age < b.age; },
    //    [](const Person& p) -> int { return p.age; });
    //radix_sort_greater(people.begin(), people.end(), [](const Person& x) -> int { return x.age; });
    //println(people);
}

struct var_visitor {
    int operator()(int arg) const { return arg * 2; }
    int operator()(const string& arg) const { return arg.length(); }
};

void test_variant() {
    variant<int, string> v1;
    println(v1.index());
    variant<int, string> v2;
    v2.emplace<1>("hello");
    println(v2.index());

    auto& str = v2.get<string>();
    println(str);
    auto ptr = v2.get_if<string>();
    if (!ptr || *ptr != "hello") {
        println("get_if method test failed.");
        return;
    }
    v2.emplace<int>(42);
    println(v2.index(), ":", v2.get<int>());
    int result = v2.visit(var_visitor());
    println(result);

    hash<variant<int, string>> hasher{};
    println(hasher(v1));
}


string generate_random_string(MSTL::size_t length) {
    const string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    string s;
    s.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        s += chars[random_lcd::next_int() % chars.size()];
    }
    return s;
}

void test_short_strings(size_t count, size_t length) {
    vector<string> strings;
    strings.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        if (i % 10000 == 0)
            print(i, " ");
        strings.emplace_back(generate_random_string(length));
    }

    strings.clear();
    strings.shrink_to_fit();

    println("Test 1:", count, "short strings (", length, " chars)");
}

void test_long_string_concat(size_t iterations, size_t chunk_size) {
    string long_str;
    long_str.reserve(iterations * chunk_size);

    for (size_t i = 0; i < iterations; ++i) {
        long_str += generate_random_string(chunk_size);
    }

    println("Test 2: Long string concat (", iterations, "chunks,"
              , chunk_size, "chars each, total: ", long_str.size(), " chars)");
}

void test_string_modification(size_t initial_length, size_t operations) {
    string str = generate_random_string(initial_length);

    for (size_t i = 0; i < operations; ++i) {
        if (i % 2 == 0) {
            size_t pos = random_lcd::next_int() % (str.size() + 1);
            str.insert(pos, 1, 'X');
        } else {
            if (str.empty()) break;
            size_t pos = random_lcd::next_int() % str.size();
            str.erase(pos, 1);
        }
    }

    println("Test 3: ", operations, " modify operations (initial: "
              , initial_length, " chars, final: ", str.size(), " chars)");
}

void test_string_search_replace(size_t str_length, size_t pattern_count) {
    string str = generate_random_string(str_length);
    const string pattern = "ABC";
    const string replacement = "XYZ";

    for (size_t i = 0; i < pattern_count; ++i) {
        size_t pos = random_lcd::next_int() % (str.size() - pattern.size() + 1);
        str.replace(pos, pattern.size(), pattern);
    }

    size_t replace_count = 0;
    size_t pos = 0;
    while ((pos = str.find(pattern, pos)) != string::npos) {
        str.replace(pos, pattern.size(), replacement);
        pos += replacement.size();
        replace_count++;
    }

    println("Test 4: Search & replace (str length: ", str_length
              , ", patterns found: ", replace_count, ")");
}

void test_max_memory_string() {
    try {
        size_t available_memory = get_available_memory();
        size_t max_test_size = available_memory / 2;

        const size_t upper_limit =
#ifdef MSTL_DATA_BUS_WIDTH_64__
            4ULL * 1024 * 1024 * 1024;  // 4GB
#else
            1ULL * 1024 * 1024 * 1024;  // 1GB
#endif
        max_test_size = _MSTL min(max_test_size, upper_limit);

        if (max_test_size == 0) {
            throw_exception(memory_exception("Insufficient system memory for test."));
        }

        string huge_str;
        huge_str.reserve(max_test_size);

        size_t chunk = 1024 * 1024;
        size_t total_written = 0;

        println("Testing max memory string (target size: "
                  , max_test_size / (1024 * 1024), " MB)");

        while (total_written < max_test_size) {
            size_t write = _MSTL min(chunk, max_test_size - total_written);
            huge_str.append(write, 'A');
            total_written += write;

            if (total_written % (100 * 1024 * 1024) == 0) {
                println("Allocated ", total_written / (1024 * 1024), " MB...");
            }
        }
        println("Test 5: Success. Allocated "
                  , total_written / (1024 * 1024), " MB string.");
    }
    catch (const exception& e) {
        println("Test 5: ", e.what());
    }
}

void test_string() {
    test_short_strings(1000000, 32);
    test_long_string_concat(100000, 1024);
    test_string_modification(100000, 500000);
    test_string_search_replace(1000000, 10000);
    test_max_memory_string();

    const string result = to_string("a", 'b', 333, 9.333, "hello", false);
    println(result);
}


void test_option() {
    optional<int> opt1;
    println(opt1.value());

    optional<int> opt2(nullopt);
    println(opt2.value());

    optional<int> opt3(42);
    println(opt3.value());

    opt1 = 100;
    println(opt1.value());

    optional<int> opt4(opt3);
    println(opt4.value());

    opt2 = opt3;
    println(opt2.value());

    optional<string> opt5(inplace_construct_tag{}, "Hello, World!");
    println(opt5.value());

    opt1.emplace(200);
    println(opt1.value());

    opt1.reset();
    println(opt1.value());

    if (opt3.has_value()) {
        println("opt3 has a value.");
    } else {
        println("opt3 has no value.");
    }

    int default_val = opt1.value_or(300);
    println("Value of opt1 or default: ", default_val);

    auto result1 = opt1.or_else([]() { return MSTL::optional<int>(400); });
    println(result1.value());

    auto result2 = opt3.and_then([](int x) { return MSTL::optional<int>(x * 2); });
    println(result2.value());

    auto result3 = opt3.transform([](int x) { return x + 1; });
    println(result3.value());
}

void test_st(){
    trace_allocator<int> alloc;
    auto* ptr = alloc.allocate(1);
}

void test_any() {
    any a1;
    println("Testing default constructor:");
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));

    any a2(42);
    println("\nTesting constructor with value:");
    println("Has value: ", (a2.has_value() ? "Yes" : "No"));
    const int* ptr = MSTL::any_cast<int>(&a2);
    if (ptr) {
        println("Value: ", *ptr);
    }

    any a3(a2);
    println("\nTesting copy constructor:");
    println("Has value: ", (a3.has_value() ? "Yes" : "No"));
    ptr = MSTL::any_cast<int>(&a3);
    if (ptr) {
        println("Value: ", *ptr);
    }

    any a4(any(123));
    println("\nTesting move constructor:");
    println("Has value: ", (a4.has_value() ? "Yes" : "No"));
    ptr = any_cast<int>(&a4);
    if (ptr) {
        println("Value: ", *ptr);
    }

    a1 = a4;
    println("\nTesting assignment operator:");
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));
    ptr = any_cast<int>(&a1);
    if (ptr) {
        println("Value: ", *ptr);
    }

    string str = "Hello, World!";
    string result = a1.emplace<string>(str);
    println("\nTesting emplace method:");
    println(result);
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));
    const string* strPtr = MSTL::any_cast<string>(&a1);
    if (strPtr) {
        println("Value: ", *strPtr);
    }

    a1.reset();
    println("\nTesting reset method:");
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));

    a1 = 10;
    a2 = 20;
    a1.swap(a2);
    println("a1: ", *any_cast<int>(&a1));
    println("a2: ", *any_cast<int>(&a2));
}

void test_timer(){
    _MSTL steady_timer timer1;
    timer1.expires_after(_MSTL_CHRONO seconds(5));
    timer1.async_wait([]() {
        println("5秒后执行");
    });

    _MSTL system_timer timer2;
    auto now = chrono::system_clock::now();
    auto target = now + chrono::hours(1);
    timer2.expires_at(target);
    timer2.async_wait([]() { println("1小时后执行"); });

    _MSTL steady_timer timer3;
    timer3.expires_from_now(1000);
    timer3.async_wait([]() {
        println("1秒后执行");
    });

    timer1.cancel();

    timer1.expires_after(chrono::seconds(3));
    timer1.async_wait([]() { println("3秒后执行");
    });

    this_thread::sleep_for(chrono::seconds(7));
}

void test_log() {
    auto& logger = logger::instance();
    logger.set_level(LOG_LEVEL::DEBUG);
    logger.add_context("app", "myapp");
    logger.set_filter([](const log_event& ev) -> bool {
        return ev.level >= LOG_LEVEL::INFO;
    });
    const auto sink = make_shared<console_sink>();
    sink->set_formatter(make_unique<log_formatter>("[{time}][{level}][{context.app}] {message}"));
    logger.add_sink(sink);
    logger.enable_async(true);

    MSTL_LOG_INFO("This is a info message");
    logger.flush();

    logger.enable_async(false);
    logger.flush();
}

void test_ranges(){
#ifdef MSTL_STANDARD_20__
    namespace rv = ranges::views;
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto view1 = vec
        | rv::filter([](int x) { return x % 2 == 0; })
        | rv::transform([](int x) { return x * x; });
    println(view1);

    auto view2 = vec
        | rv::filter([](int x) { return x > 3; })
        | rv::filter([](int x) { return x % 2 == 1; })
        | rv::transform([](int x) { return x * 10; });
    println(view2);

    auto view3 = vec
        | rv::drop(3)
        | rv::take(4)
        | rv::transform([](int x) { return x * 2; });
    println(view3);

    auto adaptor = rv::filter([](int x) { return x % 2 == 0; })
                 | rv::transform([](int x) { return x + 100; });
    auto view4 = vec | adaptor;
    println(view4);

    vector<string> words = {"hello", "world", "cpp", "range", "view", "library"};
    auto view5 = words
        | rv::filter([](const string& s) { return s.length() > 3; })
        | rv::transform([](const string& s) { return s.length(); })
        | rv::filter([](size_t len) { return len < 6; });
    println(view5);

    auto view6 = vec
        | rv::filter([](int x) { return x <= 5; })
        | rv::reverse();
    println(view6);


    int filter_count = 0;
    int transform_count = 0;

    auto view7 = vec
        | rv::filter([&filter_count](int x) {
            ++filter_count;
            println("  Filter called for", x);
            return x % 2 == 0;
        })
        | rv::transform([&transform_count](int x) {
            ++transform_count;
            println("  Transform called for", x);
            return x * x;
        });
    println("filter_count: ", filter_count, ", transform_count: ", transform_count);

    int result_count = 0;
    for (auto x : view7) {
        println(x);
        if (++result_count >= 3) break;
    }
    println("filter_count: ", filter_count, ", transform_count: ", transform_count);

    auto view8 = vec
            | rv::take(8)
            | rv::drop(2)
            | rv::filter([](int x) { return x % 2 == 1; })
            | rv::transform([](int x) { return x * x; })
            | rv::filter([](int x) { return x < 50; });
    println(view8);

    auto view9 = rv::transform(
        rv::filter(vec, [](int x) { return x > 5; }),
        [](int x) { return x * 10; }
    );
    println(view9);

    const vector<int> const_vec = {1, 2, 3, 4, 5};
    const auto view10 = const_vec
        | rv::filter([](int x) { return x % 2 == 1; })
        | rv::transform([](int x) { return x * 5; });
    println(view10);

    auto view11 = vec
        | rv::take_while([](int x) { return x < 7; })
        | rv::transform([](int x) { return x * 3; });
    println(view11);

    auto view12 = vec
        | rv::drop_while([](int x) { return x < 5; })
        | rv::transform([](int x) { return x + 100; });
    println(view12);

    auto view13 = vec
        | rv::drop_while([](int x){ return x % 2 == 1; })
        | rv::take_while([](int x) { return x <= 8; }) | rv::transform([](int x) { return x * 2; });
    println(view13);

    vector<int> a{1, 2, 3};
    vector<int> b{4, 5, 6};

    auto view14 = a
        | rv::concat(b)
        | rv::transform([](int x) { return x * 10; });
    println(view14);

    string sentence = "hello world cpp ranges views";
    auto word_str = sentence | rv::split(' ');
    auto upper_words = word_str
        | rv::transform([](auto subrange) {
            return subrange | rv::transform([](char c){ return _MSTL to_uppercase(c); });
        });

    for (const auto& word_view : upper_words) {
        for (char c : word_view)
            print(c);
        println();
    }

    auto view15 = vec
        | rv::slice(2, 5)
        | rv::filter([](int x) { return x > 4; })
        | rv::take(2);
    println(view15);
#endif
}

void test_sql(){
    auto sql1 = sql_builder()
        .select({"id", "name", "email"})
        .from("users")
        .where_eq("status", "'active'")
        .where_gt("age", "18")
        .order_by_desc("created_at")
        .limit(10)
        .build();
    println(sql1);

    auto sql2 = sql_builder()
        .select({"u.name", "u.email", "o.order_no", "o.amount"})
        .from("users", "u")
        .left_join("orders o", "u.id = o.user_id")
        .where_ge("o.amount", "100")
        .order_by_desc("o.created_at")
        .build();
    println(sql2);

    auto sql3 = sql_builder()
        .select_count("*", "total")
        .select_sum("amount", "total_amount")
        .select_avg("amount", "avg_amount")
        .from("orders")
        .group_by("user_id")
        .having("SUM(amount) > 1000")
        .build();
    println(sql3);

    auto sql4 = sql_builder()
        .select_all()
        .from("products")
        .where_like("name", "'%phone%'")
        .where_between("price", "100", "500")
        .page(2, 20)
        .build();
    println(sql4);

    auto sql5 = sql_builder()
        .insert_into("users", {"name", "email", "age"})
        .values({"'John'", "'john@example.com'", "25"})
        .build();
    println(sql5);

    auto sql6 = sql_builder()
        .update("users")
        .set("status", "'inactive'")
        .set_increment("login_count")
        .where_eq("id", "123")
        .build();
    println(sql6);

    auto sql7 = sql_builder()
        .delete_from("users")
        .where_eq("status", "'deleted'")
        .where_lt("last_login", "'2020-01-01'")
        .build();
    println(sql7);

    auto sql8 = sql_builder()
        .select({"category", "COUNT(*) as cnt", "AVG(price) as avg_price"})
        .from("products")
        .where_in("status", {"'active'", "'pending'"})
        .where_is_not_null("description")
        .group_by("category")
        .having("COUNT(*) > 10")
        .order_by_asc("category")
        .build();
    println(sql8);

    auto sql9 = sql_builder()
        .select({"u.name", "total_orders"})
        .from_subquery(
            "SELECT user_id, COUNT(*) as total_orders FROM orders GROUP BY user_id",
            "o"
        )
        .inner_join("users u", "u.id = o.user_id")
        .where_gt("o.total_orders", "5")
        .build();
    println(sql9);

    auto sql10 = sql_builder()
        .distinct()
        .select({"city", "country"})
        .from("users")
        .order_by_asc("country")
        .build();
    println(sql10);

    sql_builder builder;
    builder.select({"id", "name"})
           .from("users");

    auto active_users = builder.where_eq("status", "'active'").build();
    builder.reset();
}

void test_mysql() {
#ifdef MSTL_SUPPORT_MYSQL__
    db_config mysql_config = db_config::for_mysql("book");
    mysql_config.password = "147258hu";
    database_pool pool(DB_TYPE::MYSQL, mysql_config, 10, 20, 2);

    const auto sql = sql_builder()
        .select({"ISBN", "BookName"})
        .from("book")
        .where("CollectNumber = ?")
        .build();
    auto pstmt = dynamic_pointer_cast<mysql_connect>(
        pool.get_tb_connect())->prepare_statement(sql);
    pstmt->bind_param(0, 10);
    auto res = pstmt->execute_query();
    println(res->column_names());
    while (res->next()) {
        for (int i = 0; i < res->column_count(); ++i) {
            print(res->get(i), " ");
        }
        println();
    }
#endif
}

void test_redis() {
#ifdef MSTL_SUPPORT_REDIS__
    db_config redis_config = db_config::for_redis("0");
    database_pool pool(DB_TYPE::REDIS, redis_config, 10, 20, 2);
    auto conn = dynamic_pointer_cast<redis_connect>(pool.get_kv_connect());
    println(conn->is_valid());
    println(conn->update("SET age 20"));
    auto res = dynamic_pointer_cast<redis_result>(conn->get("age"));
    println(res->empty());
    while (res->next()) {
        println(res->value());
    }
#endif
}

void test_postgre() {
#ifdef MSTL_SUPPORT_POSTGRESQL__
    db_config postgre_config = db_config::for_postgresql("user");
    postgre_config.password = "483674";
    database_pool pool(DB_TYPE::POSTGRESQL, postgre_config);

    const auto sql = sql_builder()
        .select({"username", "email"})
        .from("user")
        .where_le("age", to_string(30))
        .build();
    auto pstmt = dynamic_pointer_cast<postgresql_connect>(
        pool.get_tb_connect())->prepare_statement(sql);
    pstmt->bind_param(0, 10);
    auto res = pstmt->execute_query();
    println(res->column_names());
    while (res->next()) {
        for (int i = 0; i < res->column_count(); ++i) {
            print(res->get(i), " ");
        }
        println();
    }
#endif
}

void test_dbpool() {
#ifdef MSTL_SUPPORT_MYSQL__
    auto begin = chrono::high_resolution_clock::now();
    db_config mysql_config = db_config::for_mysql("book");
    mysql_config.password = "147258hu";

    {
        database_pool pool(DB_TYPE::MYSQL, mysql_config);
        for (int i = 0; i < 5000; i++) {
            bool fin = pool.get_connect()->update("SELECT 1");
        }
        println((begin - chrono::high_resolution_clock::now()).count());

        auto result = pool.get_tb_connect()->query("SELECT * FROM book");
        while (result->next()) {
            for (int i = 0; i < result->column_count(); i++) {
                if (i == 2) {
                    int count = result->get_int16(i);
                    print("collected :", count, ", ");
                } else if (i == 3) {
                    float count = result->get_float32(i);
                    print("usable :", count, ", ");
                } else if (i == 5) {
                    _MSTL datetime dt = result->get_datetime(i);
                    print("date: ", dt, ", ");
                } else {
                    print(result->get(i), ", ");
                }
            }
            println();
        }
        println(result->row_count(), ", ", result->column_count());
    }

    begin = chrono::high_resolution_clock::now();
    for (int i = 0; i < 5000; i++) {
        char sql[power(2, 10)] = {};
        _MSTL sprintf(sql, "SELECT 1");
        auto* conn = new mysql_connect();
        if(conn->connect_to(mysql_config)) {
            bool fin = conn->update(sql);
        }
        delete conn;
    }
    println((begin - chrono::high_resolution_clock::now()).count());
#endif
}

void simple_task(const string& name) {
    println("Executing task: " + name);
}

int compute_sum(int a, int b) {
    int result = a + b;
    println("Computing: " + to_string(a) + " + " +
        to_string(b) + " = " + to_string(result));
    return result;
}

void periodic_work(int counter) {
    println("Periodic task #" + to_string(counter));
}

void test_ext_tpool() {
    thread_pool& pool = thread_pool::instance();
    pool.start(5);

    pool.submit_task([]{ println("Normal task"); });
    pool.submit_task(10, []{ println("High priority task"); });
    pool.submit_task(1, []{ println("Low priority task"); });
    pool.submit_after(1000, 5, []{ println("Delayed high priority"); });
    this_thread::sleep_for(chrono::seconds(3));

    println(timestamp::now());
    auto future1 = pool.submit_after(2000, []() {
        println(timestamp::now());
    });
    this_thread::sleep_for(chrono::seconds(3));

    auto future2 = pool.submit_after(1000, simple_task, "Task with parameters");
    auto future3 = pool.submit_after(1500, compute_sum, 42, 58);
    println(future3.get());

    auto counter = make_shared<atomic_int>(0);
    thread_pool::periodic_token token1;

    token1 = pool.submit_every(1000, [counter, &pool, &token1]() {
        int count = counter->fetch_add(1) + 1;
        println("Periodic task iteration #" + to_string(count));

        if (count >= 5) {
            println("Cancelling periodic task after 5 iterations");
            pool.cancel_periodic_task(token1);
        }
    });
    this_thread::sleep_for(chrono::seconds(6));

    thread_pool::periodic_token token2 = pool.submit_every(500, []() {
        println("Fast periodic task executing...");
    });
    this_thread::sleep_for(chrono::seconds(3));
    pool.cancel_periodic_task(token2);
    this_thread::sleep_for(chrono::seconds(2));

    auto token3 = pool.submit_every(800, []() {
        println("  Task A (800ms interval)");
    });
    auto token4 = pool.submit_every(1200, []() {
        println("  Task B (1200ms interval)");
    });
    auto token5 = pool.submit_every(1500, []() {
        println("  Task C (1500ms interval)");
    });
    this_thread::sleep_for(chrono::seconds(5));

    auto state = pool.statistics();
    println(state);
    pool.cancel_periodic_task(token3);
    pool.cancel_periodic_task(token4);
    pool.cancel_periodic_task(token5);
    this_thread::sleep_for(chrono::seconds(2));

    pool.stop();
}

void test_dns() {
    try {
        dns_client cloudflare_client("1.1.1.1");
        dns_client opendns_client("208.67.222.222");
        dns_client custom_client("192.168.1.1", 5353);

        auto ips = cloudflare_client.resolve_a("example.com");
        println("IPv4 addresses:");
        println(ips);

        dns_client client;
        auto start = chrono::steady_clock::now();
        auto ipv6_addrs = client.resolve_aaaa("www.google.com");
        auto end = chrono::steady_clock::now();
        auto duration1 = chrono::duration_cast<chrono::milliseconds>(end - start);
        println("IPv6 addresses:");
        println(ipv6_addrs);

        start = chrono::steady_clock::now();
        ipv6_addrs = client.resolve_aaaa("www.google.com");
        end = chrono::steady_clock::now();
        auto duration2 = chrono::duration_cast<chrono::milliseconds>(end - start);
        println("First:", duration1.count(), "Second:", duration2.count());

        auto mx_records = client.resolve_mx("gmail.com");
        println("MX records for gmail.com:");
        println(mx_records);

        string hostname = client.reverse_query("8.8.8.8");
        println("8.8.8.8 resolves to:", hostname);
        hostname = client.reverse_query("1.1.1.1");
        println("1.1.1.1 resolves to:", hostname);

        vector<string> domains = {
            "google.com",
            "facebook.com",
            "twitter.com",
            "github.com",
            "stackoverflow.com"
        };
        auto results = client.batch_query(domains, DNS_RECORD::A);

        for (size_t i = 0; i < domains.size(); ++i) {
            println(domains[i], ":");

            if (results[i].is_success()) {
                for (const auto& answer : results[i].answers) {
                    println("  ", answer.data);
                }
                println("  Query time:", results[i].query_time.count(), "ms");
            } else {
                println("  Query failed\n");
            }
        }
    } catch (...) {}
}

void test_tpool() {
    thread_pool& pool = thread_pool::instance();
    pool.start(5);
    pool.submit_task(test_vector);
    pool.submit_task(test_list);
    pool.submit_task(test_deque);
    pool.submit_task(test_hashtable);
    pool.submit_task(test_rbtree);
    pool.stop();
    pool.set_mode(THREAD_POOL_MODE::MODE_CACHED);
    pool.start();
    // pool.submit_task(test_string);
    pool.submit_task(test_math);
    // pool.submit_task(test_timer);
    pool.submit_task(test_tuple);
    pool.submit_task(test_variant);
    pool.submit_task(test_option);
    pool.submit_task(test_check);
    pool.submit_task(test_any);
    pool.submit_task(test_datetimes);
    pool.submit_task(test_json);
    pool.submit_task(test_ini);
    pool.submit_task(test_env);
    pool.submit_task(test_toml);
    pool.submit_task(test_rnd);
    pool.submit_task(test_print);
    pool.submit_task(test_file);
    pool.submit_task(test_format);
    pool.submit_task(test_enctype);
    pool.submit_task(test_color);
    pool.submit_task(test_sql);
    pool.submit_task(test_ranges);
    pool.stop();
}
