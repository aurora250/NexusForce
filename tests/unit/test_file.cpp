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
