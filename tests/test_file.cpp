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
    {
        bool create_ok = filesystem::create_and_write(TEST_FILE, TEST_CONTENT);
        NEFORCE_ASSERTION(create_ok);
        NEFORCE_ASSERTION(TEST_FILE.exists());
        NEFORCE_ASSERTION(TEST_FILE.is_file());
        NEFORCE_ASSERTION(!TEST_FILE.is_directory());
        NEFORCE_ASSERTION(filesystem::size(TEST_FILE) == TEST_CONTENT.size());
    }
    {
        file read_file(TEST_FILE);
        string read_content = read_file.read();
        NEFORCE_ASSERTION(read_content == TEST_CONTENT);
    }
    {
        file f;
        NEFORCE_ASSERTION(!f.is_opened());
        NEFORCE_ASSERTION(f.open(TEST_FILE));
        NEFORCE_ASSERTION(f.is_opened());
        NEFORCE_ASSERTION(f.file_path() == TEST_FILE);

        string line;
        NEFORCE_ASSERTION(f.read_line(line));
        NEFORCE_ASSERTION(line == "Hello!");
        NEFORCE_ASSERTION(f.read_line(line));
        NEFORCE_ASSERTION(line == "Second line.");
        NEFORCE_ASSERTION(f.read_line(line));
        NEFORCE_ASSERTION(line == "Third line");

        NEFORCE_ASSERTION(f.seek(0, file_pointer::BEGIN));
        NEFORCE_ASSERTION(f.tell() == 0);
        NEFORCE_ASSERTION(f.seek(5, file_pointer::CURRENT));
        NEFORCE_ASSERTION(f.tell() == 5);

        NEFORCE_ASSERTION(f.truncate(10));
        NEFORCE_ASSERTION(f.size() == 10);

        NEFORCE_ASSERTION(f.seek(0, file_pointer::BEGIN));
        string new_content = "New content after truncate";
        size_t written = f.write(new_content);
        NEFORCE_ASSERTION(written == new_content.size());
        NEFORCE_ASSERTION(f.flush());

        NEFORCE_ASSERTION(f.size() == new_content.size());

        f.close();
        NEFORCE_ASSERTION(!f.is_opened());
    }
    println("test file basic operations passed");
}

void test_directory_operations() {
    NEFORCE_ASSERTION(!TEST_SUB_DIR.exists());
    bool dir_ok = filesystem::create_directories(TEST_SUB_DIR);
    NEFORCE_ASSERTION(dir_ok);
    NEFORCE_ASSERTION(TEST_SUB_DIR.exists());
    NEFORCE_ASSERTION(TEST_SUB_DIR.is_directory());

    path sub_file = {TEST_SUB_DIR / "sub_file.txt"};
    NEFORCE_ASSERTION(filesystem::create_and_write(sub_file, "sub content"));
    NEFORCE_ASSERTION(sub_file.exists());
    println("test dictionary operations passed");
}

void test_file_attributes_and_times() {
    file f(TEST_FILE);
    NEFORCE_ASSERTION(f.open(TEST_FILE));

    file_attri original_attr = f.info().attributes();
    bool set_attr_ok = f.info().set_attributes(file_attri::READONLY);
    NEFORCE_ASSERTION(set_attr_ok);
    NEFORCE_ASSERTION(static_cast<bool>(f.info().attributes() & file_attri::READONLY));
    NEFORCE_ASSERTION(f.info().set_attributes(original_attr));
    NEFORCE_ASSERTION(f.info().attributes() == original_attr);

    datetime now = datetime::now();
    bool set_time_ok = f.info().set_last_write_time(now);
    NEFORCE_ASSERTION(set_time_ok);
    NEFORCE_ASSERTION(f.info().last_write_time() == now);

    f.close();
    println("test file attributes and times passed");
}

void test_file_lock_and_other_operations() {
    {
        file f(TEST_FILE);
        NEFORCE_ASSERTION(f.is_opened());

        bool locked = f.locker().lock_whole();
        NEFORCE_ASSERTION(locked);
        bool unlocked = f.locker().unlock_whole();
        NEFORCE_ASSERTION(unlocked);
    }
    {
        file copy_file{path{TEST_FILE.str() + ".copy"}};
        NEFORCE_ASSERTION(filesystem::copy(TEST_FILE, copy_file.file_path()));
        NEFORCE_ASSERTION(copy_file.file_path().exists());
        string copy_content;
        copy_file.read(copy_content);

        path move_file{TEST_DIR / "moved_file.txt"};
        NEFORCE_ASSERTION(filesystem::move(copy_file.file_path(), move_file));
        NEFORCE_ASSERTION(!copy_file.file_path().exists());
        NEFORCE_ASSERTION(move_file.exists());

        path rename_file{TEST_DIR / "renamed_file.txt"};
        NEFORCE_ASSERTION(filesystem::rename(move_file, rename_file));
        NEFORCE_ASSERTION(!move_file.exists());
        NEFORCE_ASSERTION(rename_file.exists());
    }
    println("test file lock and other operations passed");
}

void clean_up() {
    if (TEST_FILE.exists()) {
        println(filesystem::remove(TEST_FILE));
    }
    path sub_file = TEST_SUB_DIR / "sub_file.txt";
    if (sub_file.exists()) {
        filesystem::remove(sub_file);
    }
    path rename_file = TEST_DIR / "renamed_file.txt";
    if (rename_file.exists()) {
        filesystem::remove(rename_file);
    }
    if (TEST_SUB_DIR.exists()) {
        filesystem::remove_directory(TEST_SUB_DIR);
    }
    if (TEST_DIR.exists()) {
        filesystem::remove_directory(TEST_DIR);
    }
}

void test_file() {
    clean_up();
    try {
        test_file_basic_operations();
        test_directory_operations();
        test_file_lock_and_other_operations();
        test_file_attributes_and_times();
        clean_up();
    } catch (...) {
        clean_up();
    }
}


void test_date() {
    date d1(2024, 2, 29);
    NEFORCE_ASSERTION(d1.year() == 2024 && d1.month() == 2 && d1.day() == 29);

    date d2(2023, 2, 29);
    NEFORCE_ASSERTION(d2 == date::epoch());

    NEFORCE_ASSERTION(date::is_leap_year(2020) == true);
    NEFORCE_ASSERTION(date::is_leap_year(2019) == false);
    NEFORCE_ASSERTION(date::is_leap_year(2100) == false);
    NEFORCE_ASSERTION(date::is_leap_year(2400) == true);

    NEFORCE_ASSERTION(date::days_of_month(2024, 2) == 29);
    NEFORCE_ASSERTION(date::days_of_month(2023, 2) == 28);
    NEFORCE_ASSERTION(date::days_of_month(2023, 4) == 30);

    date d3(2024, 1, 1);
    NEFORCE_ASSERTION(d3.days_of_week() == 1);

    date d4(2024, 3, 1);
    NEFORCE_ASSERTION(d4.days_of_year() == 61);

    date d5(2024, 2, 28);
    d5 += 2;
    NEFORCE_ASSERTION(d5.month() == 3 && d5.day() == 1);

    date d6(2024, 3, 1);
    d6 -= 1;
    NEFORCE_ASSERTION(d6.month() == 2 && d6.day() == 29);

    NEFORCE_ASSERTION(date(2024, 1, 1) < date(2024, 1, 2));
    NEFORCE_ASSERTION(date(2024, 1, 1) > date(2023, 12, 31));

    auto str = date(2024, 5, 10).to_string();
    NEFORCE_ASSERTION(str == "2024-05-10");
    NEFORCE_ASSERTION(date::parse("2024-05-10") == date(2024, 5, 10));
    NEFORCE_ASSERTION(date().try_parse("invalid") == false);

    println("test_date passed");
}

void test_time() {
    using neforce::time;
    time t1(23, 59, 59);
    NEFORCE_ASSERTION(t1.hours() == 23 && t1.minutes() == 59 && t1.seconds() == 59);

    time t2(25, 60, 60);
    NEFORCE_ASSERTION(t2 == time(0, 0, 0));

    NEFORCE_ASSERTION(time(1, 2, 3).to_seconds() == 3600 + 120 + 3);

    time t3(23, 59, 59);
    t3 += 2;  // 00:00:01
    NEFORCE_ASSERTION(t3.hours() == 0 && t3.seconds() == 1);

    time t4(0, 0, 1);
    t4 -= 2;  // 23:59:59
    NEFORCE_ASSERTION(t4.hours() == 23 && t4.seconds() == 59);

    NEFORCE_ASSERTION(time(12, 0, 0) < time(13, 0, 0));
    NEFORCE_ASSERTION(time(12, 30, 0) > time(12, 29, 59));

    NEFORCE_ASSERTION(time(9, 8, 7).to_string() == "09:08:07");
    NEFORCE_ASSERTION(time::parse("09:08:07") == time(9, 8, 7));
    NEFORCE_ASSERTION(time().try_parse("invalid") == false);

    println("test_time passed");
}

void test_datetime() {
    using neforce::time;
    datetime dt1(date(2024, 1, 1), time(12, 0, 0));
    NEFORCE_ASSERTION(dt1.year() == 2024 && dt1.hours() == 12);

    datetime dt2(2024, 2, 28, 23, 59, 59);
    dt2 += 2;
    NEFORCE_ASSERTION(dt2.month() == 2 && dt2.day() == 29 && dt2.seconds() == 1);

    datetime dt3(2024, 3, 1, 0, 0, 0);
    dt3 -= 1;
    NEFORCE_ASSERTION(dt3.month() == 2 && dt3.day() == 29);

    datetime dt4(2024, 1, 1, 0, 0, 0);
    datetime dt5(2023, 12, 31, 23, 59, 59);
    NEFORCE_ASSERTION(dt4 - dt5 == 1);

    NEFORCE_ASSERTION(datetime(2024, 5, 10, 9, 8, 7).to_string() == "2024-05-10 09:08:07");
    NEFORCE_ASSERTION(datetime::parse("2024-05-10 09:08:07") == datetime(2024, 5, 10, 9, 8, 7));
    NEFORCE_ASSERTION(datetime().try_parse("invalid") == false);

    println(datetime::now());

    println("test_datetime passed");
}

void test_timestamp() {
    datetime epoch = datetime::epoch();
    timestamp ts1(epoch);
    NEFORCE_ASSERTION(ts1.value() == 0);
    NEFORCE_ASSERTION(ts1.to_datetime() == epoch);

    timestamp ts2(86400);
    datetime dt = ts2.to_datetime();
    NEFORCE_ASSERTION(dt.day() == 2);

    timestamp ts3(100);
    timestamp ts4(200);
    NEFORCE_ASSERTION(ts3 < ts4);
    NEFORCE_ASSERTION(ts4.value() - ts3.value() == 100);

    println("test_timestamp passed");
}

void test_utc_conversion() {
    datetime dt(2024, 1, 1, 0, 0, 0);
    datetime utc = dt.to_UTC();
    datetime local = datetime::from_UTC(utc);
    NEFORCE_ASSERTION(local != dt);

    println("test_utc_conversion passed");
}


void test_datetimes() {
    test_date();
    test_time();
    test_datetime();
    test_timestamp();
    test_utc_conversion();
}
