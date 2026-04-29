#include <NeForce/NeForce.hpp>
#include <gtest/gtest.h>

namespace {
    const neforce::path& res_root() {
        static neforce::path res_root
#ifdef NEFORCE_PLATFORM_WINDOWS
                {R"(D:/Workspace/Cpp Workspace/CLine Workspace/NexusForce/tests/resource)"};
#elif defined(NEFORCE_PLATFORM_LINUX)
                {R"(/media/huenqi/Programming/Workspace/Cpp Workspace/CLine Workspace/NexusForce-Linux/tests/resource)"};
#endif
        return res_root;
    }

    const neforce::path g_test_file{"test_temp_file.txt"};
    const neforce::path g_test_dir{"test_temp_dir"};
    const neforce::path g_test_sub_dir{g_test_dir / "sub_dir"};
    const neforce::string g_test_content = "Hello!\nSecond line.\r\nThird line";
} // namespace

class FileTest : public ::testing::Test {
protected:
    void SetUp() override { clean_up(); }

    void TearDown() override { clean_up(); }

    void clean_up() {
        using namespace neforce;

        if (g_test_file.exists()) {
            filesystem::remove(g_test_file);
        }
        path copy_file{g_test_file.str() + ".copy"};
        if (copy_file.exists()) {
            filesystem::remove(copy_file);
        }
        filesystem::remove_all(g_test_dir);
    }
};

TEST_F(FileTest, BasicOperations) {
    using namespace neforce;

    {
        bool create_ok = filesystem::create_and_write(g_test_file, g_test_content);
        ASSERT_TRUE(create_ok);
        ASSERT_TRUE(g_test_file.exists());
        ASSERT_TRUE(g_test_file.is_file());
        ASSERT_FALSE(g_test_file.is_directory());
        ASSERT_EQ(filesystem::size(g_test_file).bytes(), g_test_content.size());
    }
    {
        file read_file(g_test_file);
        string read_content = read_file.read();
        ASSERT_EQ(read_content, g_test_content);
    }
    {
        file f;
        ASSERT_FALSE(f.is_opened());
        ASSERT_TRUE(f.open(g_test_file));
        ASSERT_TRUE(f.is_opened());
        ASSERT_EQ(f.file_path(), g_test_file);

        string line;
        ASSERT_TRUE(f.read_line(line));
        ASSERT_EQ(line, "Hello!");
        ASSERT_TRUE(f.read_line(line));
        ASSERT_EQ(line, "Second line.");
        ASSERT_TRUE(f.read_line(line));
        ASSERT_EQ(line, "Third line");

        ASSERT_TRUE(f.seek(0, file_pointer::BEGIN));
        ASSERT_EQ(f.tell(), 0);
        ASSERT_TRUE(f.seek(5, file_pointer::CURRENT));
        ASSERT_EQ(f.tell(), 5);

        ASSERT_TRUE(f.truncate(10));
        ASSERT_EQ(f.size(), 10);

        ASSERT_TRUE(f.seek(0, file_pointer::BEGIN));
        string new_content = "New content after truncate";
        size_t written = f.write(new_content);
        ASSERT_EQ(written, new_content.size());
        ASSERT_TRUE(f.flush());

        ASSERT_EQ(f.size(), new_content.size());

        f.close();
        ASSERT_FALSE(f.is_opened());
    }
}

TEST_F(FileTest, DirectoryOperations) {
    using namespace neforce;

    ASSERT_FALSE(g_test_sub_dir.exists());
    bool dir_ok = filesystem::create_directories(g_test_sub_dir);
    ASSERT_TRUE(dir_ok);
    ASSERT_TRUE(g_test_sub_dir.exists());
    ASSERT_TRUE(g_test_sub_dir.is_directory());

    path sub_file = {g_test_sub_dir / "sub_file.txt"};
    ASSERT_TRUE(filesystem::create_and_write(sub_file, "sub content"));
    ASSERT_TRUE(sub_file.exists());
}

TEST_F(FileTest, AttributesAndTimes) {
    using namespace neforce;

    file f(g_test_file, false, file_access::READ_WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
    ASSERT_TRUE(f.is_opened());

    file_attri original_attr = f.info().attributes();
    bool set_attr_ok = f.info().set_attributes(file_attri::READONLY);
    ASSERT_TRUE(set_attr_ok);
    ASSERT_TRUE(static_cast<bool>(f.info().attributes() & file_attri::READONLY));
    ASSERT_TRUE(f.info().set_attributes(original_attr));
    ASSERT_EQ(f.info().attributes(), original_attr);

    datetime now = datetime::now();
    bool set_time_ok = f.info().set_last_write_time(now);
    ASSERT_TRUE(set_time_ok);
    ASSERT_EQ(f.info().last_write_time(), now.to_UTC());

    f.close();
}

TEST_F(FileTest, LockAndOtherOperations) {
    using namespace neforce;

    {
        file f(g_test_file, false, file_access::READ_WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        ASSERT_TRUE(f.is_opened());

        bool locked = f.locker().lock_whole();
        ASSERT_TRUE(locked);
        bool unlocked = f.locker().unlock_whole();
        ASSERT_TRUE(unlocked);
    }
    {
        file copy_file{path{g_test_file.str() + ".copy"}};
        ASSERT_TRUE(filesystem::copy(g_test_file, copy_file.file_path()));
        ASSERT_TRUE(copy_file.file_path().exists());
        string copy_content;
        copy_file.read(copy_content);

        filesystem::create_directories(g_test_dir);
        path move_file{g_test_dir / "moved_file.txt"};
        ASSERT_TRUE(filesystem::move(copy_file.file_path(), move_file));
        ASSERT_FALSE(copy_file.file_path().exists());
        ASSERT_TRUE(move_file.exists());

        path rename_file{g_test_dir / "renamed_file.txt"};
        ASSERT_TRUE(filesystem::rename(move_file, rename_file));
        ASSERT_FALSE(move_file.exists());
        ASSERT_TRUE(rename_file.exists());
    }
}

TEST(PathTreeTest, ScanAndTraverse) {
    using namespace neforce;

    path_tree::scan_options opts;
    opts.extensions = {"cpp", "hpp"};
    opts.include_hidden = false;

    path project(res_root() / "../../include/NeForce");
    path_tree tree = path_tree::scan(project, opts);

    auto headers = tree.find_by_extension("hpp");

    tree.traverse_files([](const path_tree::node& n) -> path_tree::visit_result {
        // Just verify it doesn't crash
        auto p = n.get_path();
        return path_tree::visit_result::proceed;
    });

    path_tree compress_tree = tree.subtree(project / "compress");

    path_tree large_files = tree.prune([](const path_tree::node& n) {
        if (!n.is_file()) {
            return true;
        }
        return filesystem::size(n.get_path()) > 1_KB;
    });
}

TEST(DateTest, BasicDate) {
    using namespace neforce;

    date d1(2024, 2, 29);
    ASSERT_TRUE(d1.year() == 2024 && d1.month() == 2 && d1.day() == 29);

    date d2(2023, 2, 29);
    ASSERT_EQ(d2, date::epoch());
}

TEST(DateTest, LeapYear) {
    using namespace neforce;

    ASSERT_TRUE(date::is_leap_year(2020));
    ASSERT_FALSE(date::is_leap_year(2019));
    ASSERT_FALSE(date::is_leap_year(2100));
    ASSERT_TRUE(date::is_leap_year(2400));
}

TEST(DateTest, DaysOfMonth) {
    using namespace neforce;

    ASSERT_EQ(date::days_of_month(2024, 2), 29);
    ASSERT_EQ(date::days_of_month(2023, 2), 28);
    ASSERT_EQ(date::days_of_month(2023, 4), 30);
}

TEST(DateTest, DayCalculations) {
    using namespace neforce;

    date d3(2024, 1, 1);
    ASSERT_EQ(d3.days_of_week(), 1);

    date d4(2024, 3, 1);
    ASSERT_EQ(d4.days_of_year(), 61);
}

TEST(DateTest, Arithmetic) {
    using namespace neforce;

    date d5(2024, 2, 28);
    d5 += 2;
    ASSERT_TRUE(d5.month() == 3 && d5.day() == 1);

    date d6(2024, 3, 1);
    d6 -= 1;
    ASSERT_TRUE(d6.month() == 2 && d6.day() == 29);
}

TEST(DateTest, Comparison) {
    using namespace neforce;

    ASSERT_LT(date(2024, 1, 1), date(2024, 1, 2));
    ASSERT_GT(date(2024, 1, 1), date(2023, 12, 31));
}

TEST(DateTest, StringConversion) {
    using namespace neforce;

    auto str = date(2024, 5, 10).to_string();
    ASSERT_EQ(str, "2024-05-10");
    ASSERT_EQ(date::parse("2024-05-10"), date(2024, 5, 10));
    ASSERT_FALSE(date().try_parse("invalid"));
}

TEST(TimeTest, BasicTime) {
    using neforce::time;
    time t1(23, 59, 59);
    ASSERT_TRUE(t1.hours() == 23 && t1.minutes() == 59 && t1.seconds() == 59);

    time t2(25, 60, 60);
    ASSERT_EQ(t2, time(0, 0, 0));

    ASSERT_EQ(time(1, 2, 3).to_seconds(), 3600 + 120 + 3);
}

TEST(TimeTest, Arithmetic) {
    using neforce::time;
    time t3(23, 59, 59);
    t3 += 2; // 00:00:01
    ASSERT_TRUE(t3.hours() == 0 && t3.seconds() == 1);

    time t4(0, 0, 1);
    t4 -= 2; // 23:59:59
    ASSERT_TRUE(t4.hours() == 23 && t4.seconds() == 59);
}

TEST(TimeTest, Comparison) {
    using neforce::time;
    ASSERT_LT(time(12, 0, 0), time(13, 0, 0));
    ASSERT_GT(time(12, 30, 0), time(12, 29, 59));
}

TEST(TimeTest, StringConversion) {
    using neforce::time;
    ASSERT_EQ(time(9, 8, 7).to_string(), "09:08:07");
    ASSERT_EQ(time::parse("09:08:07"), time(9, 8, 7));
    ASSERT_FALSE(time().try_parse("invalid"));
}

TEST(DateTimeTest, BasicDateTime) {
    using namespace neforce;
    using neforce::time;

    datetime dt1(date(2024, 1, 1), time(12, 0, 0));
    ASSERT_TRUE(dt1.year() == 2024 && dt1.hours() == 12);
}

TEST(DateTimeTest, Arithmetic) {
    using namespace neforce;

    datetime dt2(2024, 2, 28, 23, 59, 59);
    dt2 += 2;
    ASSERT_TRUE(dt2.month() == 2 && dt2.day() == 29 && dt2.seconds() == 1);

    datetime dt3(2024, 3, 1, 0, 0, 0);
    dt3 -= 1;
    ASSERT_TRUE(dt3.month() == 2 && dt3.day() == 29);
}

TEST(DateTimeTest, Difference) {
    using namespace neforce;

    datetime dt4(2024, 1, 1, 0, 0, 0);
    datetime dt5(2023, 12, 31, 23, 59, 59);
    ASSERT_EQ(dt4 - dt5, 1);
}

TEST(DateTimeTest, StringConversion) {
    using namespace neforce;

    ASSERT_EQ(datetime(2024, 5, 10, 9, 8, 7).to_string(), "2024-05-10 09:08:07");
    ASSERT_EQ(datetime::parse("2024-05-10 09:08:07"), datetime(2024, 5, 10, 9, 8, 7));
    ASSERT_FALSE(datetime().try_parse("invalid"));
}

TEST(TimestampTest, BasicTimestamp) {
    using namespace neforce;

    datetime epoch = datetime::epoch();
    timestamp ts1(epoch);
    ASSERT_EQ(ts1.value(), 0);
    ASSERT_EQ(ts1.to_datetime(), epoch);

    timestamp ts2(86400);
    datetime dt = ts2.to_datetime();
    ASSERT_EQ(dt.day(), 2);
}

TEST(TimestampTest, Comparison) {
    using namespace neforce;

    timestamp ts3(100);
    timestamp ts4(200);
    ASSERT_LT(ts3, ts4);
    ASSERT_EQ(ts4.value() - ts3.value(), 100);
}

TEST(UtcConversionTest, Conversion) {
    using namespace neforce;

    datetime dt(2024, 1, 1, 0, 0, 0);
    datetime utc = dt.to_UTC();
    datetime local = datetime::from_UTC(utc);
    ASSERT_NE(local, dt);
}
