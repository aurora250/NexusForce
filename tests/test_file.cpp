#include "test.h"

const path& res_root() {
    static path res_root
#ifdef NEFORCE_PLATFORM_WINDOWS
        {R"(D:/Workspace/Cpp Workspace/CLine Workspace/NexusForce/tests/resource)"};
#elif defined(NEFORCE_PLATFORM_LINUX)
        {R"(/media/huenqi/Programming/Workspace/Cpp Workspace/CLine Workspace/NexusForce-Linux/tests/resource)"};
#endif
    return res_root;
}

static const path TEST_FILE{"test_temp_file.txt"};
static const path TEST_DIR{"test_temp_dir"};
static const path TEST_SUB_DIR{TEST_DIR / "sub_dir"};
static const string TEST_CONTENT = "Hello!\nSecond line.\r\nThird line";

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
        assert(!f.is_opened());
        assert(f.open(TEST_FILE));
        assert(f.is_opened());
        assert(f.path() == TEST_FILE);

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
        assert(!f.is_opened());
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

    _NEFORCE FILE_ATTRI original_attr = f.attributes();
    bool set_attr_ok = f.set_attributes(_NEFORCE FILE_ATTRI::READONLY);
    assert(set_attr_ok);
    assert(static_cast<bool>(f.attributes() & _NEFORCE FILE_ATTRI::READONLY));
    assert(f.set_attributes(original_attr));
    assert(f.attributes() == original_attr);

    _NEFORCE datetime now = _NEFORCE datetime::now();
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
    file f2 = _NEFORCE move(f1);
    assert(!f1.is_opened());
    assert(f2.is_opened());
    assert(f2.path().str() == TEST_FILE.str());

    file f3;
    f3 = _NEFORCE move(f2);
    assert(!f2.is_opened());
    assert(f3.is_opened());
    assert(f3.path() == TEST_FILE);
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
    _NEFORCE date d1(2024, 2, 29);
    assert(d1.year() == 2024 && d1.month() == 2 && d1.day() == 29);

    _NEFORCE date d2(2023, 2, 29);
    assert(d2 == _NEFORCE date::epoch());

    assert(_NEFORCE date::is_leap_year(2020) == true);
    assert(_NEFORCE date::is_leap_year(2019) == false);
    assert(_NEFORCE date::is_leap_year(2100) == false);
    assert(_NEFORCE date::is_leap_year(2400) == true);

    assert(_NEFORCE date::days_of_month(2024, 2) == 29);
    assert(_NEFORCE date::days_of_month(2023, 2) == 28);
    assert(_NEFORCE date::days_of_month(2023, 4) == 30);

    _NEFORCE date d3(2024, 1, 1);
    assert(d3.days_of_week() == 1);

    _NEFORCE date d4(2024, 3, 1);
    assert(d4.days_of_year() == 61);

    _NEFORCE date d5(2024, 2, 28);
    d5 += 2;
    assert(d5.month() == 3 && d5.day() == 1);

    _NEFORCE date d6(2024, 3, 1);
    d6 -= 1;
    assert(d6.month() == 2 && d6.day() == 29);

    assert(_NEFORCE date(2024, 1, 1) < _NEFORCE date(2024, 1, 2));
    assert(_NEFORCE date(2024, 1, 1) > _NEFORCE date(2023, 12, 31));

    auto str = _NEFORCE date(2024, 5, 10).to_string();
    assert(str == "2024-05-10");
    assert(_NEFORCE date::parse("2024-05-10") == _NEFORCE date(2024, 5, 10));
    assert(_NEFORCE date().try_parse("invalid") == false);

    println("test_date passed");
}

void test_time() {
    using neforce::time;
    _NEFORCE time t1(23, 59, 59);
    assert(t1.hours() == 23 && t1.minutes() == 59 && t1.seconds() == 59);

    _NEFORCE time t2(25, 60, 60);
    assert(t2 == _NEFORCE time(0, 0, 0));

    assert(_NEFORCE time(1, 2, 3).to_seconds() == 3600 + 120 + 3);

    _NEFORCE time t3(23, 59, 59);
    t3 += 2;  // 00:00:01
    assert(t3.hours() == 0 && t3.seconds() == 1);

    _NEFORCE time t4(0, 0, 1);
    t4 -= 2;  // 23:59:59
    assert(t4.hours() == 23 && t4.seconds() == 59);

    assert(_NEFORCE time(12, 0, 0) < _NEFORCE time(13, 0, 0));
    assert(_NEFORCE time(12, 30, 0) > _NEFORCE time(12, 29, 59));

    assert(_NEFORCE time(9, 8, 7).to_string() == "09:08:07");
    assert(_NEFORCE time::parse("09:08:07") == _NEFORCE time(9, 8, 7));
    assert(_NEFORCE time().try_parse("invalid") == false);

    println("test_time passed");
}

void test_datetime() {
    using neforce::time;
    _NEFORCE datetime dt1(_NEFORCE date(2024, 1, 1), _NEFORCE time(12, 0, 0));
    assert(dt1.year() == 2024 && dt1.hours() == 12);

    _NEFORCE datetime dt2(2024, 2, 28, 23, 59, 59);
    dt2 += 2;
    assert(dt2.month() == 2 && dt2.day() == 29 && dt2.seconds() == 1);

    _NEFORCE datetime dt3(2024, 3, 1, 0, 0, 0);
    dt3 -= 1;
    assert(dt3.month() == 2 && dt3.day() == 29);

    _NEFORCE datetime dt4(2024, 1, 1, 0, 0, 0);
    _NEFORCE datetime dt5(2023, 12, 31, 23, 59, 59);
    assert(dt4 - dt5 == 1);

    assert(_NEFORCE datetime(2024, 5, 10, 9, 8, 7).to_string() == "2024-05-10 09:08:07");
    assert(_NEFORCE datetime::parse("2024-05-10 09:08:07")
        == _NEFORCE datetime(2024, 5, 10, 9, 8, 7));
    assert(_NEFORCE datetime().try_parse("invalid") == false);

    println(datetime::now());

    println("test_datetime passed");
}

void test_timestamp() {
    _NEFORCE datetime epoch = _NEFORCE datetime::epoch();
    _NEFORCE timestamp ts1(epoch);
    assert(ts1.value() == 0);
    assert(ts1.to_datetime() == epoch);

    _NEFORCE timestamp ts2(86400);
    _NEFORCE datetime dt = ts2.to_datetime();
    assert(dt.day() == 2);

    _NEFORCE timestamp ts3(100);
    _NEFORCE timestamp ts4(200);
    assert(ts3 < ts4);
    assert(ts4.value() - ts3.value() == 100);

    println("test_timestamp passed");
}

void test_utc_conversion() {
    _NEFORCE datetime dt(2024, 1, 1, 0, 0, 0);
    _NEFORCE datetime utc = dt.to_UTC();
    _NEFORCE datetime local = _NEFORCE datetime::from_UTC(dt);
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
