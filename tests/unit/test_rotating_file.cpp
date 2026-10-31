#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/file/rotating_file.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <gtest/gtest.h>

using namespace neforce;

class RotatingFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        directory_ = path::temp_directory_path() / path("neforce_rotating_file_test");
        static_cast<void>(filesystem::remove_all(directory_));
        ASSERT_TRUE(filesystem::create_directories(directory_));
    }

    void TearDown() override { static_cast<void>(filesystem::remove_all(directory_)); }

    NEFORCE_NODISCARD path test_path(const string& name) const { return directory_ / path(name); }

    NEFORCE_NODISCARD string read(const path& target) const {
        file input(target);
        return input.is_opened() ? input.read() : string();
    }

    path directory_;
};

TEST_F(RotatingFileTest, RolloverRotation) {
    const path target = test_path("data.ndjson");
    rotating_file writer(target,
                         {.max_bytes = 10, .max_files = 3, .naming = rotating_file::naming::rollover, .append = false});

    EXPECT_TRUE(writer.write("12345"));
    EXPECT_TRUE(writer.write("67890"));
    EXPECT_EQ(writer.size(), 10ULL);
    EXPECT_TRUE(writer.write("abcde"));
    EXPECT_EQ(writer.size(), 5ULL);
    EXPECT_EQ(writer.current_path().str(), target.str());
    ASSERT_TRUE(writer.flush());

    EXPECT_EQ(read(target), "abcde");
    EXPECT_EQ(read(path(target.str() + ".1")), "1234567890");
}

TEST_F(RotatingFileTest, RolloverRetention) {
    const path target = test_path("retained.ndjson");
    rotating_file writer(target,
                         {.max_bytes = 4, .max_files = 2, .naming = rotating_file::naming::rollover, .append = false});

    for (int index = 1; index <= 5; ++index) {
        const string payload = to_string(index) + "111";
        EXPECT_TRUE(writer.write(payload.view()));
    }
    ASSERT_TRUE(writer.flush());

    EXPECT_EQ(read(target), "5111");
    EXPECT_EQ(read(path(target.str() + ".1")), "4111");
    EXPECT_EQ(read(path(target.str() + ".2")), "3111");
    EXPECT_FALSE(path(target.str() + ".3").exists());
}

TEST_F(RotatingFileTest, AppendExistingFile) {
    const path target = test_path("append.ndjson");
    {
        rotating_file writer(
                target, {.max_bytes = 1024, .max_files = 1, .naming = rotating_file::naming::rollover, .append = true});
        EXPECT_TRUE(writer.write("one"));
        ASSERT_TRUE(writer.flush());
    }
    {
        rotating_file writer(
                target, {.max_bytes = 1024, .max_files = 1, .naming = rotating_file::naming::rollover, .append = true});
        EXPECT_TRUE(writer.write("two"));
        EXPECT_EQ(writer.size(), 6ULL);
        ASSERT_TRUE(writer.flush());
    }
    EXPECT_EQ(read(target), "onetwo");
}

TEST_F(RotatingFileTest, IndexedRotation) {
    const path target = test_path("indexed.ndjson");
    rotating_file writer(target,
                         {.max_bytes = 4, .max_files = 2, .naming = rotating_file::naming::indexed, .append = false});

    EXPECT_TRUE(writer.write("aaaa"));
    EXPECT_TRUE(writer.write("bbbb"));
    EXPECT_EQ(writer.current_path().str(), target.str() + ".1");
    EXPECT_TRUE(writer.write("cccc"));
    EXPECT_EQ(writer.current_path().str(), target.str() + ".2");
    EXPECT_TRUE(writer.write("dddd"));
    EXPECT_EQ(writer.current_path().str(), target.str() + ".3");
    ASSERT_TRUE(writer.flush());

    EXPECT_FALSE(target.exists());
    EXPECT_EQ(read(path(target.str() + ".1")), "bbbb");
    EXPECT_EQ(read(path(target.str() + ".2")), "cccc");
    EXPECT_EQ(read(path(target.str() + ".3")), "dddd");
}

TEST_F(RotatingFileTest, ExplicitRotate) {
    const path target = test_path("explicit.ndjson");
    rotating_file writer(
            target, {.max_bytes = 1024, .max_files = 2, .naming = rotating_file::naming::indexed, .append = false});

    EXPECT_TRUE(writer.write("a"));
    EXPECT_TRUE(writer.rotate());
    EXPECT_EQ(writer.current_path().str(), target.str() + ".1");
    EXPECT_EQ(writer.size(), 0ULL);
    EXPECT_TRUE(writer.write("b"));
    ASSERT_TRUE(writer.flush());

    EXPECT_EQ(read(target), "a");
    EXPECT_EQ(read(path(target.str() + ".1")), "b");
}
