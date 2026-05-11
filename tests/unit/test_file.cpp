#include <NeForce/core/async/condition_variable.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/file_watcher.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/path_tree.hpp>
#include <NeForce/core/file/temp_file.hpp>
#include <NeForce/core/time/duration.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    const file::native_handle_type invalid_handle =
#ifdef NEFORCE_PLATFORM_WINDOWS
            INVALID_HANDLE_VALUE;
#else
            -1;
#endif
} // namespace

class FileTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_file_test");
        if (!test_dir_.exists()) {
            filesystem::create_directories(test_dir_);
        }
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_test_path(const string& name) const { return test_dir_ / path(name); }

    path test_dir_;
};

TEST_F(FileTest, DefaultConstructor) {
    file f;
    EXPECT_FALSE(f.is_opened());
    EXPECT_TRUE(f.file_path().empty());
}

TEST_F(FileTest, ConstructorOpenFile) {
    auto p = get_test_path("constructor_test.txt");
    filesystem::create_and_write(p, "hello");

    file f(p);
    EXPECT_TRUE(f.is_opened());
    EXPECT_FALSE(f.is_append());
    EXPECT_EQ(f.read(5), "hello");
}

TEST_F(FileTest, ConstructorOpenFileAppend) {
    auto p = get_test_path("append_test.txt");
    filesystem::create_and_write(p, "hello");

    file f(p, true);
    EXPECT_TRUE(f.is_opened());
    EXPECT_TRUE(f.is_append());
}

TEST_F(FileTest, ConstructorOpenNonExistentFile) {
    auto p = get_test_path("nonexistent.txt");
    file f(p);
    EXPECT_FALSE(f.is_opened());
}

TEST_F(FileTest, MoveConstructor) {
    auto p = get_test_path("move_ctor.txt");
    filesystem::create_and_write(p, "test data");

    file f1(p);
    EXPECT_TRUE(f1.is_opened());

    file f2(move(f1));
    EXPECT_TRUE(f2.is_opened());
    EXPECT_FALSE(f1.is_opened());
    EXPECT_EQ(f2.read(9), "test data");
}

TEST_F(FileTest, MoveAssignment) {
    auto p1 = get_test_path("move_assign1.txt");
    auto p2 = get_test_path("move_assign2.txt");
    filesystem::create_and_write(p1, "first");
    filesystem::create_and_write(p2, "second");

    file f1(p1);
    file f2(p2);

    f1 = move(f2);
    EXPECT_TRUE(f1.is_opened());
    EXPECT_FALSE(f2.is_opened());
    EXPECT_EQ(f1.read(6), "second");
}

TEST_F(FileTest, MoveAssignmentSelf) {
    auto p = get_test_path("move_self.txt");
    filesystem::create_and_write(p, "data");

    file f(p);
    f = move(f);
    EXPECT_TRUE(f.is_opened());
    EXPECT_EQ(f.read(4), "data");
}

TEST_F(FileTest, DestructorClosesFile) {
    auto p = get_test_path("destructor_test.txt");
    {
        file f;
        f.open(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
               file_attri::NORMAL);
        f.write("content", 7);
    }
    file f2(p);
    EXPECT_TRUE(f2.is_opened());
    EXPECT_EQ(f2.read(7), "content");
}

TEST_F(FileTest, OpenCreatesNewFile) {
    auto p = get_test_path("open_create.txt");
    file f;
    bool result = f.open(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
                         file_attri::NORMAL);
    EXPECT_TRUE(result);
    EXPECT_TRUE(f.is_opened());
    EXPECT_TRUE(p.exists());
}

TEST_F(FileTest, OpenExistingFile) {
    auto p = get_test_path("open_existing.txt");
    filesystem::create_and_write(p, "existing");

    file f;
    bool result = f.open(p);
    EXPECT_TRUE(result);
    EXPECT_TRUE(f.is_opened());
    EXPECT_EQ(f.read(8), "existing");
}

TEST_F(FileTest, OpenAppendMode) {
    auto p = get_test_path("open_append.txt");
    filesystem::create_and_write(p, "initial\n");

    file f;
    f.open(p, true);
    f.write("appended\n", 9);
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(), "initial\nappended\n");
}

TEST_F(FileTest, OpenDefaultParameters) {
    auto p = get_test_path("open_default.txt");
    filesystem::create_and_write(p, "default");

    file f;
    bool result = f.open();
    EXPECT_FALSE(result);
}

TEST_F(FileTest, ReopenWithPath) {
    auto p = get_test_path("reopen_path.txt");
    filesystem::create_and_write(p, "first");

    file f(p);
    f.close();

    bool result = f.open(p);
    EXPECT_TRUE(result);
    EXPECT_TRUE(f.is_opened());
}

TEST_F(FileTest, Close) {
    auto p = get_test_path("close_test.txt");
    filesystem::create_and_write(p, "test");

    file f(p);
    EXPECT_TRUE(f.is_opened());

    f.close();
    EXPECT_FALSE(f.is_opened());
}

TEST_F(FileTest, CloseNotOpened) {
    file f;
    f.close();
    EXPECT_FALSE(f.is_opened());
}

TEST_F(FileTest, CloseFlushesBuffer) {
    auto p = get_test_path("close_flush.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);
    f.write("buffered", 8);
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(8), "buffered");
}

TEST_F(FileTest, Flush) {
    auto p = get_test_path("flush_test.txt");
    {
        file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
               file_attri::NORMAL);
        EXPECT_EQ(f.write("before_flush"), 12);
        EXPECT_TRUE(f.flush());
    }
    {
        file f2(p);
        EXPECT_EQ(f2.read(12), "before_flush");
    }
}

TEST_F(FileTest, FlushNotOpened) {
    file f;
    EXPECT_FALSE(f.flush());
}

TEST_F(FileTest, WriteStringWithSize) {
    auto p = get_test_path("write_str_size.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    file::size_type written = f.write(string("hello world"), 5);
    EXPECT_EQ(written, 5);
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(5), "hello");
}

TEST_F(FileTest, WriteFullString) {
    auto p = get_test_path("write_full_str.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    file::size_type written = f.write(string("complete"));
    EXPECT_EQ(written, 8);
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(8), "complete");
}

TEST_F(FileTest, WriteVoidPointer) {
    auto p = get_test_path("write_void.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    const char data[] = "raw data";
    file::size_type written = f.write(static_cast<const void*>(data), 8);
    EXPECT_EQ(written, 8);
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(8), "raw data");
}

TEST_F(FileTest, WriteNullData) {
    auto p = get_test_path("write_null.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    file::size_type written = f.write(static_cast<const void*>(nullptr), 10);
    EXPECT_EQ(written, 0);
}

TEST_F(FileTest, WriteZeroSize) {
    auto p = get_test_path("write_zero.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    file::size_type written = f.write(string("data"), 0);
    EXPECT_EQ(written, 0);
}

TEST_F(FileTest, WriteNotOpened) {
    file f;
    file::size_type written = f.write(string("test"), 4);
    EXPECT_EQ(written, 0);
}

TEST_F(FileTest, WriteLargeData) {
    auto p = get_test_path("write_large.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    string large_data(10000, 'A');
    file::size_type written = f.write(large_data);
    EXPECT_EQ(written, 10000);
    f.close();

    file f2(p);
    EXPECT_EQ(f2.size(), 10000);
}

TEST_F(FileTest, WriteAppendMode) {
    auto p = get_test_path("write_append.txt");
    filesystem::create_and_write(p, "first");

    file f(p, true);
    f.write("second", 6);
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(), "firstsecond");
}

TEST_F(FileTest, ReadToBuffer) {
    auto p = get_test_path("read_buffer.txt");
    filesystem::create_and_write(p, "buffer test data");

    file f(p);
    char buf[12] = {};
    file::size_type read = f.read(static_cast<void*>(buf), 11);
    EXPECT_EQ(read, 11);
    EXPECT_STREQ(buf, "buffer test");
}

TEST_F(FileTest, ReadToStringWithSize) {
    auto p = get_test_path("read_str_size.txt");
    filesystem::create_and_write(p, "string data here");

    file f(p);
    string out;
    file::size_type read = f.read(out, 6);
    EXPECT_EQ(read, 6);
    EXPECT_EQ(out, "string");
}

TEST_F(FileTest, ReadWholeFile) {
    auto p = get_test_path("read_all.txt");
    filesystem::create_and_write(p, "entire content");

    file f(p);
    string content = f.read();
    EXPECT_EQ(content, "entire content");
}

TEST_F(FileTest, ReadAutoSizedString) {
    auto p = get_test_path("read_auto.txt");
    filesystem::create_and_write(p, "hello");

    file f(p);
    string out;
    file::size_type read = f.read(out);
    string error = f.last_error_code().message();
    EXPECT_EQ(read, 5);
    EXPECT_EQ(out, "hello");
}

TEST_F(FileTest, ReadEmptyFile) {
    auto p = get_test_path("read_empty.txt");
    filesystem::create_and_write(p, "");

    file f(p);
    string content = f.read();
    EXPECT_TRUE(content.empty());
}

TEST_F(FileTest, ReadNotOpened) {
    file f;
    char buf[10];
    file::size_type read = f.read(static_cast<void*>(buf), 10);
    EXPECT_EQ(read, 0);
}

TEST_F(FileTest, ReadBinaryToVoid) {
    auto p = get_test_path("read_bin_void.bin");
    {
        file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
               file_attri::NORMAL);
        byte_t data[] = {0x00, 0x01, 0x02, 0xFF};
        f.write(data, 4);
    }

    file f(p);
    byte_t buf[4] = {};
    file::size_type read = f.read_binary(buf, 4);
    EXPECT_EQ(read, 4);
    EXPECT_EQ(buf[0], 0x00);
    EXPECT_EQ(buf[1], 0x01);
    EXPECT_EQ(buf[2], 0x02);
    EXPECT_EQ(buf[3], 0xFF);
}

TEST_F(FileTest, ReadBinaryToString) {
    auto p = get_test_path("read_bin_str.bin");
    {
        file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
               file_attri::NORMAL);
        const char data[] = {0x41, 0x00, 0x42, 0x00};
        f.write(data, 4);
    }

    file f(p);
    string out;
    file::size_type read = f.read_binary(out, 4);
    EXPECT_EQ(read, 4);
    EXPECT_EQ(out.size(), 4);
    EXPECT_EQ(out[0], 'A');
    EXPECT_EQ(out[1], '\0');
    EXPECT_EQ(out[2], 'B');
}

TEST_F(FileTest, ReadBinaryAutoSized) {
    auto p = get_test_path("read_bin_auto.bin");
    {
        file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
               file_attri::NORMAL);
        f.write("binary", 6);
    }

    file f(p);
    string out;
    file::size_type read = f.read_binary(out);
    EXPECT_EQ(read, 6);
    EXPECT_EQ(out, "binary");
}

TEST_F(FileTest, ReadBinaryWholeFile) {
    auto p = get_test_path("read_bin_all.bin");
    {
        file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
               file_attri::NORMAL);
        byte_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
        f.write(data, 4);
    }

    file f(p);
    string content = f.read_binary();
    EXPECT_EQ(content.size(), 4);
    EXPECT_EQ(static_cast<byte_t>(content[0]), 0xDE);
    EXPECT_EQ(static_cast<byte_t>(content[1]), 0xAD);
    EXPECT_EQ(static_cast<byte_t>(content[2]), 0xBE);
    EXPECT_EQ(static_cast<byte_t>(content[3]), 0xEF);
}

TEST_F(FileTest, ReadBinaryNotOpened) {
    file f;
    byte_t buf[4];
    file::size_type read = f.read_binary(buf, 4);
    EXPECT_EQ(read, 0);
}

TEST_F(FileTest, ReadLine) {
    auto p = get_test_path("readline.txt");
    filesystem::create_and_write(p, "line1\nline2\nline3");

    file f(p);
    string line;
    EXPECT_TRUE(f.read_line(line));
    EXPECT_EQ(line, "line1");
    EXPECT_TRUE(f.read_line(line));
    EXPECT_EQ(line, "line2");
    EXPECT_TRUE(f.read_line(line));
    EXPECT_EQ(line, "line3");
    EXPECT_FALSE(f.read_line(line));
}

TEST_F(FileTest, ReadLineCRLF) {
    auto p = get_test_path("readline_crlf.txt");
    {
        file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
               file_attri::NORMAL);
        f.write("first\r\nsecond\r\n", 15);
    }

    file f(p);
    string line;
    EXPECT_TRUE(f.read_line(line));
    EXPECT_EQ(line, "first");
    EXPECT_TRUE(f.read_line(line));
    EXPECT_EQ(line, "second");
}

TEST_F(FileTest, ReadLineReturnsString) {
    auto p = get_test_path("readline_return.txt");
    filesystem::create_and_write(p, "single.line");

    file f(p);
    string line = f.read_line();
    EXPECT_EQ(line, "single.line");
}

TEST_F(FileTest, ReadLines) {
    auto p = get_test_path("readlines.txt");
    filesystem::create_and_write(p, "a\nb\nc\n");

    file f(p);
    auto lines = f.read_lines();
    ASSERT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "a");
    EXPECT_EQ(lines[1], "b");
    EXPECT_EQ(lines[2], "c");
}

TEST_F(FileTest, ReadLinesEmptyFile) {
    auto p = get_test_path("readlines_empty.txt");
    filesystem::create_and_write(p, "");

    file f(p);
    auto lines = f.read_lines();
    EXPECT_TRUE(lines.empty());
}

TEST_F(FileTest, ReadChunks) {
    auto p = get_test_path("read_chunks.txt");
    string content(500, 'X');
    content += string(500, 'Y');
    filesystem::create_and_write(p, content);

    file f(p);
    auto chunks = f.read_chunks(100);
    EXPECT_EQ(chunks.size(), 10);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(chunks[i], string(100, 'X'));
    }
    for (size_t i = 5; i < 10; ++i) {
        EXPECT_EQ(chunks[i], string(100, 'Y'));
    }
}

TEST_F(FileTest, ReadChunksLargeChunkSize) {
    auto p = get_test_path("read_chunks_large.txt");
    filesystem::create_and_write(p, "small");

    file f(p);
    auto chunks = f.read_chunks(1000);
    ASSERT_EQ(chunks.size(), 1);
    EXPECT_EQ(chunks[0], "small");
}

TEST_F(FileTest, ReadChunksNotOpened) {
    file f;
    auto chunks = f.read_chunks(100);
    EXPECT_TRUE(chunks.empty());
}

TEST_F(FileTest, WriteChunks) {
    auto p = get_test_path("write_chunks.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    vector<string> chunks = {"chunk1", "chunk2", "chunk3"};
    EXPECT_TRUE(f.write_chunks(chunks));
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(), "chunk1chunk2chunk3");
}

TEST_F(FileTest, WriteChunksWithEmpty) {
    auto p = get_test_path("write_chunks_empty.txt");
    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ_WRITE, file_creation::OPEN_FORCE,
           file_attri::NORMAL);

    vector<string> chunks = {"", "data", ""};
    EXPECT_TRUE(f.write_chunks(chunks));
    f.close();

    file f2(p);
    EXPECT_EQ(f2.read(), "data");
}

TEST_F(FileTest, WriteChunksNotOpened) {
    file f;
    vector<string> chunks = {"test"};
    EXPECT_FALSE(f.write_chunks(chunks));
}

TEST_F(FileTest, ChunksInfo) {
    auto p = get_test_path("chunks_info.txt");
    filesystem::create_and_write(p, string(100, 'A'));

    file f(p);
    auto info = f.chunks_info(30);
    ASSERT_EQ(info.size(), 4);
    EXPECT_EQ(info[0].index, 0);
    EXPECT_EQ(info[0].offset, 0);
    EXPECT_EQ(info[0].size, 30);
    EXPECT_EQ(info[1].index, 1);
    EXPECT_EQ(info[1].offset, 30);
    EXPECT_EQ(info[1].size, 30);
    EXPECT_EQ(info[2].index, 2);
    EXPECT_EQ(info[2].offset, 60);
    EXPECT_EQ(info[2].size, 30);
    EXPECT_EQ(info[3].index, 3);
    EXPECT_EQ(info[3].offset, 90);
    EXPECT_EQ(info[3].size, 10);
}

TEST_F(FileTest, ChunksInfoDefaultChunkSize) {
    auto p = get_test_path("chunks_info_default.txt");
    filesystem::create_and_write(p, string(100, 'B'));

    file f(p);
    auto info = f.chunks_info(0);
    EXPECT_FALSE(info.empty());
}

TEST_F(FileTest, ChunksInfoEmptyFile) {
    auto p = get_test_path("chunks_info_empty.txt");
    filesystem::create_and_write(p, "");

    file f(p);
    auto info = f.chunks_info(100);
    EXPECT_TRUE(info.empty());
}

TEST_F(FileTest, ChunksInfoNotOpened) {
    file f;
    auto info = f.chunks_info(100);
    EXPECT_TRUE(info.empty());
}

TEST_F(FileTest, SeekBegin) {
    auto p = get_test_path("seek_begin.txt");
    filesystem::create_and_write(p, "0123456789");

    file f(p);
    EXPECT_TRUE(f.seek(5, file_pointer::BEGIN));
    string out;
    f.read(out, 5);
    EXPECT_EQ(out, "56789");
}

TEST_F(FileTest, SeekEnd) {
    auto p = get_test_path("seek_end.txt");
    filesystem::create_and_write(p, "0123456789");

    file f(p);
    EXPECT_TRUE(f.seek(-5, file_pointer::END));
    string out;
    f.read(out, 5);
    EXPECT_EQ(out, "56789");
}

TEST_F(FileTest, SeekCurrent) {
    auto p = get_test_path("seek_current.txt");
    filesystem::create_and_write(p, "0123456789");

    file f(p);
    string out;
    f.read(out, 5);
    EXPECT_TRUE(f.seek(2, file_pointer::CURRENT));
    f.read(out, 3);
    EXPECT_EQ(out, "789");
}

TEST_F(FileTest, SeekAppendModeRestriction) {
    auto p = get_test_path("seek_append.txt");
    filesystem::create_and_write(p, "data");

    file f(p, true);
    EXPECT_FALSE(f.seek(0, file_pointer::BEGIN));
    EXPECT_TRUE(f.seek(0, file_pointer::END));
}

TEST_F(FileTest, SeekNotOpened) {
    file f;
    EXPECT_FALSE(f.seek(10, file_pointer::BEGIN));
}

TEST_F(FileTest, Tell) {
    auto p = get_test_path("tell_test.txt");
    filesystem::create_and_write(p, "0123456789");

    file f(p);
    EXPECT_EQ(f.tell(), 0);
    string out;
    f.read(out, 5);
    EXPECT_EQ(f.tell(), 5);
}

TEST_F(FileTest, TellNotOpened) {
    file f;
    EXPECT_EQ(f.tell(), -1);
}

TEST_F(FileTest, SystemTell) {
    auto p = get_test_path("system_tell.txt");
    filesystem::create_and_write(p, "0123456789");

    file f(p);
    EXPECT_EQ(f.system_tell(), 0);
    f.seek(5, file_pointer::BEGIN);
    EXPECT_EQ(f.system_tell(), 5);
}

TEST_F(FileTest, Prefetch) {
    auto p = get_test_path("prefetch.txt");
    filesystem::create_and_write(p, string(10000, 'P'));

    file f(p);
    EXPECT_TRUE(f.prefetch(5000));
}

TEST_F(FileTest, PrefetchZeroHint) {
    auto p = get_test_path("prefetch_zero.txt");
    filesystem::create_and_write(p, "some data");

    file f(p);
    EXPECT_TRUE(f.prefetch(0));
}

TEST_F(FileTest, PrefetchNotOpened) {
    file f;
    EXPECT_FALSE(f.prefetch(100));
}

TEST_F(FileTest, Truncate) {
    auto p = get_test_path("truncate.txt");
    filesystem::create_and_write(p, "0123456789");

    file f(p);
    EXPECT_TRUE(f.truncate(5));
    f.close();

    file f2(p);
    EXPECT_EQ(f2.size(), 5);
}

TEST_F(FileTest, TruncateExtend) {
    auto p = get_test_path("truncate_extend.txt");
    filesystem::create_and_write(p, "01234");

    file f(p);
    EXPECT_TRUE(f.truncate(10));
    f.close();

    file f2(p);
    EXPECT_EQ(f2.size(), 10);
}

TEST_F(FileTest, TruncateNegative) {
    auto p = get_test_path("truncate_neg.txt");
    filesystem::create_and_write(p, "data");

    file f(p);
    EXPECT_FALSE(f.truncate(-1));
}

TEST_F(FileTest, TruncateAppendMode) {
    auto p = get_test_path("truncate_append.txt");
    filesystem::create_and_write(p, "data");

    file f(p, true);
    EXPECT_FALSE(f.truncate(2));
}

TEST_F(FileTest, TruncateNotOpened) {
    file f;
    EXPECT_FALSE(f.truncate(10));
}

TEST_F(FileTest, Size) {
    auto p = get_test_path("size_test.txt");
    filesystem::create_and_write(p, "12345");

    file f(p);
    EXPECT_EQ(f.size(), 5);
}

TEST_F(FileTest, SizeEmptyFile) {
    auto p = get_test_path("size_empty.txt");
    filesystem::create_and_write(p, "");

    file f(p);
    EXPECT_EQ(f.size(), 0);
}

TEST_F(FileTest, SizeNotOpened) {
    file f;
    EXPECT_EQ(f.size(), 0);
}

TEST_F(FileTest, SizeByRef) {
    auto p = get_test_path("size_ref.txt");
    filesystem::create_and_write(p, "1234567890");

    file f(p);
    file::size_type s = 0;
    EXPECT_TRUE(f.size(s));
    EXPECT_EQ(s, 10);
}

TEST_F(FileTest, SizeByRefNotOpened) {
    file f;
    file::size_type s = 42;
    EXPECT_FALSE(f.size(s));
    EXPECT_EQ(s, 0);
}

TEST_F(FileTest, Size64) {
    auto p = get_test_path("size64.txt");
    filesystem::create_and_write(p, "hello world");

    file f(p);
    EXPECT_EQ(f.size64(), 11);
}

TEST_F(FileTest, Size64NotOpened) {
    file f;
    EXPECT_EQ(f.size64(), 0);
}

TEST_F(FileTest, NativeHandle) {
    auto p = get_test_path("native_handle.txt");
    filesystem::create_and_write(p, "test");

    file f(p);
    EXPECT_NE(f.native_handle(), invalid_handle);
}

TEST_F(FileTest, FilePath) {
    auto p = get_test_path("filepath.txt");
    filesystem::create_and_write(p, "data");

    file f(p);
    EXPECT_EQ(f.file_path().str(), p.str());
}

TEST_F(FileTest, IsOpened) {
    auto p = get_test_path("is_opened.txt");
    filesystem::create_and_write(p, "test");

    file f;
    EXPECT_FALSE(f.is_opened());
    f.open(p);
    EXPECT_TRUE(f.is_opened());
    f.close();
    EXPECT_FALSE(f.is_opened());
}

TEST_F(FileTest, IsAppend) {
    auto p = get_test_path("is_append.txt");
    filesystem::create_and_write(p, "data");
    {
        file f1(p);
        EXPECT_TRUE(f1.is_opened());
        EXPECT_FALSE(f1.is_append());
    }
    {
        file f2(p, true);
        EXPECT_TRUE(f2.is_opened());
        EXPECT_TRUE(f2.is_append());
    }
}

TEST_F(FileTest, LastErrorCode) {
    auto p = get_test_path("error_code.txt");

    file f;
    f.open("nonexistent/path/to/file.txt");
    EXPECT_NE(f.last_error_code().value(), 0);
}

TEST_F(FileTest, ClearError) {
    auto p = get_test_path("clear_error.txt");

    file f;
    f.open("nonexistent/path/file.txt");
    f.clear_error();
    EXPECT_EQ(f.last_error_code().value(), 0);
}

TEST_F(FileTest, LineIteratorRangeFor) {
    auto p = get_test_path("line_iterator.txt");
    filesystem::create_and_write(p, "line1\nline2\nline3\n");

    file f(p);
    vector<string> lines;
    for (auto it = f.begin_lines(); it != f.end_lines(); ++it) {
        lines.push_back(*it);
    }
    ASSERT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
    EXPECT_EQ(lines[2], "line3");
}

TEST_F(FileTest, LineIteratorEquality) {
    auto p = get_test_path("line_iter_eq.txt");
    filesystem::create_and_write(p, "a\nb\n");

    file f(p);
    auto begin = f.begin_lines();
    auto end = f.end_lines();
    EXPECT_NE(begin, end);
    ++begin;
    ++begin;
    EXPECT_EQ(begin, end);
}

TEST_F(FileTest, LineIteratorPostIncrement) {
    auto p = get_test_path("line_iter_post.txt");
    filesystem::create_and_write(p, "first\nsecond\n");

    file f(p);
    auto it = f.begin_lines();
    auto old = it++;
    EXPECT_EQ(*old, "first");
    EXPECT_EQ(*it, "second");
}

TEST_F(FileTest, LineIteratorEmptyFile) {
    auto p = get_test_path("line_iter_empty.txt");
    filesystem::create_and_write(p, "");

    file f(p);
    auto it = f.begin_lines();
    EXPECT_EQ(it, f.end_lines());
}

TEST_F(FileTest, LineIteratorDefaultConstructor) {
    file::line_iterator it;
    EXPECT_EQ(it, file::line_iterator{});
}

class FilesystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_fs_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_test_path(const string& name) const { return test_dir_ / path(name); }

    path test_dir_;
};

TEST_F(FilesystemTest, CreateDirectories) {
    auto p = get_test_path("a/b/c");
    EXPECT_TRUE(filesystem::create_directories(p));
    EXPECT_TRUE(p.exists());
    EXPECT_TRUE(p.is_directory());
}

TEST_F(FilesystemTest, CreateDirectoriesAlreadyExists) {
    auto p = get_test_path("already");
    filesystem::create_directories(p);
    EXPECT_TRUE(filesystem::create_directories(p));
}

TEST_F(FilesystemTest, CreateDirectoriesEmptyPath) { EXPECT_FALSE(filesystem::create_directories(path{})); }

TEST_F(FilesystemTest, RemoveFile) {
    auto p = get_test_path("remove_me.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_TRUE(p.exists());

    EXPECT_TRUE(filesystem::remove(p));
    EXPECT_FALSE(p.exists());
}

TEST_F(FilesystemTest, RemoveNonExistent) {
    auto p = get_test_path("no_such_file.txt");
    EXPECT_FALSE(filesystem::remove(p));
}

TEST_F(FilesystemTest, RemoveDirectoryFails) {
    auto p = get_test_path("dir_not_file");
    filesystem::create_directories(p);
    EXPECT_FALSE(filesystem::remove(p));
}

TEST_F(FilesystemTest, RemoveDirectory) {
    auto p = get_test_path("empty_dir");
    filesystem::create_directories(p);
    EXPECT_TRUE(filesystem::remove_directory(p));
    EXPECT_FALSE(p.exists());
}

TEST_F(FilesystemTest, RemoveDirectoryNonExistent) {
    auto p = get_test_path("no_such_dir");
    EXPECT_FALSE(filesystem::remove_directory(p));
}

TEST_F(FilesystemTest, RemoveDirectoryFileInstead) {
    auto p = get_test_path("file_not_dir.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_FALSE(filesystem::remove_directory(p));
}

TEST_F(FilesystemTest, RemoveAllInDirectoryRecursive) {
    auto dir = get_test_path("rm_all_rec");
    filesystem::create_directories(dir / path("sub"));
    filesystem::create_and_write(dir / path("file1.txt"), "1");
    filesystem::create_and_write(dir / path("sub") / path("file2.txt"), "2");

    EXPECT_TRUE(filesystem::remove_all_in_directory(dir, true));
    EXPECT_TRUE(dir.exists());
    EXPECT_FALSE((dir / path("file1.txt")).exists());
    EXPECT_FALSE((dir / path("sub")).exists());
}

TEST_F(FilesystemTest, RemoveAllInDirectoryNonRecursive) {
    auto dir = get_test_path("rm_all_nonrec");
    filesystem::create_directories(dir / path("sub"));
    filesystem::create_and_write(dir / path("file1.txt"), "1");
    filesystem::create_and_write(dir / path("sub") / path("file2.txt"), "2");

    EXPECT_TRUE(filesystem::remove_all_in_directory(dir, false));
    EXPECT_TRUE(dir.exists());
    EXPECT_FALSE((dir / path("file1.txt")).exists());
    EXPECT_TRUE((dir / path("sub")).exists());
    EXPECT_TRUE((dir / path("sub") / path("file2.txt")).exists());
}

TEST_F(FilesystemTest, RemoveAllInDirectoryNotDirectory) {
    auto p = get_test_path("not_a_dir.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_FALSE(filesystem::remove_all_in_directory(p));
}

TEST_F(FilesystemTest, RemoveAllFile) {
    auto p = get_test_path("remove_all_file.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_TRUE(filesystem::remove_all(p));
    EXPECT_FALSE(p.exists());
}

TEST_F(FilesystemTest, RemoveAllDirectory) {
    auto p = get_test_path("remove_all_dir");
    filesystem::create_directories(p / path("sub"));
    filesystem::create_and_write(p / path("file.txt"), "data");

    EXPECT_TRUE(filesystem::remove_all(p));
    EXPECT_FALSE(p.exists());
}

TEST_F(FilesystemTest, RemoveAllNonExistent) {
    auto p = get_test_path("no_such");
    EXPECT_FALSE(filesystem::remove_all(p));
}

TEST_F(FilesystemTest, CopyFile) {
    auto src = get_test_path("copy_src.txt");
    auto dst = get_test_path("copy_dst.txt");
    filesystem::create_and_write(src, "copy data");

    EXPECT_TRUE(filesystem::copy(src, dst));
    EXPECT_TRUE(dst.exists());
    EXPECT_EQ(filesystem::size(dst), filesystem::size(src));
}

TEST_F(FilesystemTest, CopyFileOverwrite) {
    auto src = get_test_path("copy_over_src.txt");
    auto dst = get_test_path("copy_over_dst.txt");
    filesystem::create_and_write(src, "new data");
    filesystem::create_and_write(dst, "old data");

    EXPECT_TRUE(filesystem::copy(src, dst, true));
    file f(dst);
    EXPECT_EQ(f.read(), "new data");
}

TEST_F(FilesystemTest, CopyFileNoOverwrite) {
    auto src = get_test_path("copy_noovr_src.txt");
    auto dst = get_test_path("copy_noovr_dst.txt");
    filesystem::create_and_write(src, "new data");
    filesystem::create_and_write(dst, "old data");

    EXPECT_FALSE(filesystem::copy(src, dst, false));
}

TEST_F(FilesystemTest, CopyFileToDirectory) {
    auto src = get_test_path("copy_to_dir_src.txt");
    auto dst_dir = get_test_path("target_dir");
    filesystem::create_and_write(src, "data");
    filesystem::create_directories(dst_dir);

    EXPECT_TRUE(filesystem::copy(src, dst_dir));
    EXPECT_TRUE((dst_dir / path(src.filename())).exists());
}

TEST_F(FilesystemTest, CopyNonExistentSource) {
    auto src = get_test_path("no_copy_src.txt");
    auto dst = get_test_path("no_copy_dst.txt");
    EXPECT_FALSE(filesystem::copy(src, dst));
}

TEST_F(FilesystemTest, CopyDirectory) {
    auto src = get_test_path("copydir_src");
    auto dst = get_test_path("copydir_dst");
    filesystem::create_directories(src / path("sub"));
    filesystem::create_and_write(src / path("f1.txt"), "one");
    filesystem::create_and_write(src / path("sub") / path("f2.txt"), "two");

    EXPECT_TRUE(filesystem::copy_directory(src, dst));
    EXPECT_TRUE(dst.exists());
    EXPECT_TRUE((dst / path("f1.txt")).exists());
    EXPECT_TRUE((dst / path("sub") / path("f2.txt")).exists());
}

TEST_F(FilesystemTest, CopyDirectoryNonExistentSource) {
    auto src = get_test_path("no_copydir_src");
    auto dst = get_test_path("no_copydir_dst");
    EXPECT_FALSE(filesystem::copy_directory(src, dst));
}

TEST_F(FilesystemTest, MoveFile) {
    auto src = get_test_path("move_src.txt");
    auto dst = get_test_path("move_dst.txt");
    filesystem::create_and_write(src, "move me");

    EXPECT_TRUE(filesystem::move(src, dst));
    EXPECT_FALSE(src.exists());
    EXPECT_TRUE(dst.exists());
    file f(dst);
    EXPECT_EQ(f.read(), "move me");
}

TEST_F(FilesystemTest, MoveFileOverwrite) {
    auto src = get_test_path("move_ovr_src.txt");
    auto dst = get_test_path("move_ovr_dst.txt");
    filesystem::create_and_write(src, "new");
    filesystem::create_and_write(dst, "old");

    EXPECT_TRUE(filesystem::move(src, dst, true));
    EXPECT_FALSE(src.exists());
    file f(dst);
    EXPECT_EQ(f.read(), "new");
}

TEST_F(FilesystemTest, MoveNonExistent) {
    auto src = get_test_path("no_move_src.txt");
    auto dst = get_test_path("no_move_dst.txt");
    EXPECT_FALSE(filesystem::move(src, dst));
}

TEST_F(FilesystemTest, Rename) {
    auto src = get_test_path("rename_src.txt");
    auto dst = get_test_path("rename_dst.txt");
    filesystem::create_and_write(src, "rename data");

    EXPECT_TRUE(filesystem::rename(src, dst));
    EXPECT_FALSE(src.exists());
    EXPECT_TRUE(dst.exists());
}

TEST_F(FilesystemTest, RenameOverwriteByDefault) {
    auto src = get_test_path("rename_ovr_src.txt");
    auto dst = get_test_path("rename_ovr_dst.txt");
    filesystem::create_and_write(src, "new name");
    filesystem::create_and_write(dst, "old name");

    EXPECT_TRUE(filesystem::rename(src, dst));
    EXPECT_FALSE(src.exists());
    file f(dst);
    EXPECT_EQ(f.read(), "new name");
}

TEST_F(FilesystemTest, CreateAndWrite) {
    auto p = get_test_path("create_write.txt");
    EXPECT_TRUE(filesystem::create_and_write(p, "hello world"));
    EXPECT_TRUE(p.exists());
    file f(p);
    EXPECT_EQ(f.read(), "hello world");
}

TEST_F(FilesystemTest, CreateAndWriteCreatesDirectories) {
    auto p = get_test_path("subdir") / path("nested.txt");
    EXPECT_TRUE(filesystem::create_and_write(p, "nested content"));
    EXPECT_TRUE(p.exists());
}

TEST_F(FilesystemTest, CreateAndWriteAppend) {
    auto p = get_test_path("create_append.txt");
    filesystem::create_and_write(p, "first\n");
    EXPECT_TRUE(filesystem::create_and_write(p, "second\n", true));
    file f(p);
    EXPECT_EQ(f.read(), "first\nsecond\n");
}

TEST_F(FilesystemTest, Size) {
    auto p = get_test_path("fs_size.txt");
    filesystem::create_and_write(p, "12345");
    auto sz = filesystem::size(p);
    EXPECT_EQ(sz, byte_size(5));
}

TEST_F(FilesystemTest, SizeNonExistent) {
    auto p = get_test_path("no_size.txt");
    auto sz = filesystem::size(p);
    EXPECT_EQ(sz, byte_size(0));
}

TEST_F(FilesystemTest, SizeDirectory) {
    auto p = get_test_path("size_dir");
    filesystem::create_directories(p);
    auto sz = filesystem::size(p);
    EXPECT_EQ(sz, byte_size(0));
}

class PathTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_path_test");
        if (!test_dir_.exists()) {
            filesystem::create_directories(test_dir_);
        }
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_test_path(const string& name) const { return test_dir_ / path(name); }

    path test_dir_;
};

TEST_F(PathTest, DefaultConstructor) {
    path p;
    EXPECT_TRUE(p.empty());
    EXPECT_EQ(p.str(), "");
}

TEST_F(PathTest, StringConstructor) {
    path p(string("some/path"));
    EXPECT_EQ(p.str(), "some/path");
}

TEST_F(PathTest, StringViewConstructor) {
    string_view sv = "view/path";
    path p(sv);
    EXPECT_EQ(p.str(), "view/path");
}

TEST_F(PathTest, CStrConstructor) {
    path p("cstr/path");
    EXPECT_EQ(p.str(), "cstr/path");
}

TEST_F(PathTest, CopyConstructor) {
    path p1("original");
    path p2(p1);
    EXPECT_EQ(p2.str(), "original");
}

TEST_F(PathTest, MoveConstructor) {
    path p1("moved");
    path p2(move(p1));
    EXPECT_EQ(p2.str(), "moved");
}

TEST_F(PathTest, CopyAssignment) {
    path p1("first");
    path p2("second");
    p2 = p1;
    EXPECT_EQ(p2.str(), "first");
}

TEST_F(PathTest, MoveAssignment) {
    path p1("source");
    path p2("target");
    p2 = move(p1);
    EXPECT_EQ(p2.str(), "source");
}

TEST_F(PathTest, Str) {
    path p("hello/world");
    EXPECT_EQ(p.str(), "hello/world");
}

TEST_F(PathTest, View) {
    path p("view/me");
    EXPECT_EQ(p.view(), "view/me");
}

TEST_F(PathTest, Data) {
    path p("cstring");
    EXPECT_STREQ(p.data(), "cstring");
}

TEST_F(PathTest, Empty) {
    EXPECT_TRUE(path().empty());
    EXPECT_FALSE(path("x").empty());
}

TEST_F(PathTest, BeginEnd) {
    path p("a/b/c");
    vector<string> parts;
    for (auto it = p.begin(); it != p.end(); ++it) {
        parts.push_back(*it);
    }
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST_F(PathTest, BeginEndAbsolutePath) {
    path p("/usr/local/bin");
    vector<string> parts;
    for (auto it = p.begin(); it != p.end(); ++it) {
        parts.push_back(*it);
    }
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "usr");
}

TEST_F(PathTest, ParentPath) {
    path p("/home/user/file.txt");
    EXPECT_EQ(p.parent_path().str(), "/home/user");

    path p2("file.txt");
    EXPECT_TRUE(p2.parent_path().empty());

    path p3("/");
    EXPECT_EQ(p3.parent_path().str(), "/");
}

TEST_F(PathTest, Filename) {
    path p("/home/user/file.txt");
    EXPECT_EQ(p.filename(), "file.txt");

    path p2("/");
    EXPECT_EQ(p2.filename(), "");
}

TEST_F(PathTest, Stem) {
    path p("/home/user/file.txt");
    EXPECT_EQ(p.stem(), "file");

    path p2("noext");
    EXPECT_EQ(p2.stem(), "noext");

    path p3(".hidden");
    EXPECT_EQ(p3.stem(), ".hidden");
}

TEST_F(PathTest, Extension) {
    path p("/home/user/file.txt");
    EXPECT_EQ(p.extension(), "txt");
}

TEST_F(PathTest, ExtensionNoExtension) {
    path p("noext");
    EXPECT_TRUE(p.extension().empty());
}

TEST_F(PathTest, ExtensionStatic) {
    EXPECT_EQ(path::extension("file.tar.gz"), "gz");
    EXPECT_EQ(path::extension("noext"), "");
    EXPECT_EQ(path::extension(".hidden"), "");
}

TEST_F(PathTest, LexicallyNormal) {
    path p("/home/user/../user/./file.txt");
    auto norm = p.lexically_normal();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(norm.str(), "\\home\\user\\file.txt");
#else
    EXPECT_EQ(norm.str(), "/home/user/file.txt");
#endif
}

TEST_F(PathTest, LexicallyNormalRelative) {
    path p("a/b/../c/./d");
    auto norm = p.lexically_normal();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(norm.str(), "a\\c\\d");
#else
    EXPECT_EQ(norm.str(), "a/c/d");
#endif
}

TEST_F(PathTest, LexicallyNormalEmpty) {
    path p;
    EXPECT_EQ(p.lexically_normal().str(), ".");
}

TEST_F(PathTest, Absolute) {
    path p("relative/path");
    auto abs = p.absolute();
    EXPECT_FALSE(abs.str().empty());
}

TEST_F(PathTest, AbsoluteAlreadyAbsolute) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    path p("C:\\absolute\\path");
#else
    path p("/absolute/path");
#endif
    auto abs = p.absolute();
    EXPECT_FALSE(abs.str().empty());
}

TEST_F(PathTest, Relative) {
    path base("/home/user");
    path target("/home/user/projects/test");
    auto rel = target.relative(base);
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(rel.str(), "projects\\test");
#else
    EXPECT_EQ(rel.str(), "projects/test");
#endif
}

TEST_F(PathTest, RelativeSamePath) {
    path base("/same");
    path target("/same");
    auto rel = target.relative(base);
    EXPECT_EQ(rel.str(), ".");
}

TEST_F(PathTest, CurrentPath) {
    auto cwd = path::current_path();
    EXPECT_FALSE(cwd.empty());
}

TEST_F(PathTest, TempDirectoryPath) {
    auto tmp = path::temp_directory_path();
    EXPECT_FALSE(tmp.empty());
}

TEST_F(PathTest, OperatorSlashEqualsPath) {
    path p("/home");
    p /= path("user");
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(p.str(), "/home\\user");
#else
    EXPECT_EQ(p.str(), "/home/user");
#endif
}

TEST_F(PathTest, OperatorSlashEqualsPathEmpty) {
    path p("/home");
    p /= path();
    EXPECT_EQ(p.str(), "/home");
}

TEST_F(PathTest, OperatorSlashEqualsStringView) {
    path p("/home");
    p /= string_view("user/docs");
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(p.str(), "/home\\user/docs");
#else
    EXPECT_EQ(p.str(), "/home/user/docs");
#endif
}

TEST_F(PathTest, OperatorSlashEqualsStringViewEmpty) {
    path p("/home");
    p /= string_view();
    EXPECT_EQ(p.str(), "/home");
}

TEST_F(PathTest, OperatorSlashPath) {
    path p1("/home");
    path p2("user");
    path result = p1 / p2;
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(result.str(), "/home\\user");
#else
    EXPECT_EQ(result.str(), "/home/user");
#endif
}

TEST_F(PathTest, OperatorSlashStringView) {
    path p1("/home");
    path result = p1 / string_view("user");
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(result.str(), "/home\\user");
#else
    EXPECT_EQ(result.str(), "/home/user");
#endif
}

TEST_F(PathTest, OperatorSlashHandlesSeparators) {
    path p1("/home/");
    path p2("/user");
    path result = p1 / p2;
    EXPECT_EQ(result.str(), "/home/user");
}

TEST_F(PathTest, ToTree) {
    auto dir = get_test_path("totree_test");
    filesystem::create_directories(dir / path("sub1"));
    filesystem::create_and_write(dir / path("f1.txt"), "data");
    filesystem::create_and_write(dir / path("sub1") / path("f2.txt"), "data2");

    auto tree = dir.to_tree();
    EXPECT_FALSE(tree.empty());
}

TEST_F(PathTest, Children) {
    auto dir = get_test_path("children_test");
    filesystem::create_directories(dir);
    filesystem::create_and_write(dir / path("a.txt"), "a");
    filesystem::create_and_write(dir / path("b.txt"), "b");
    filesystem::create_directories(dir / path("sub"));

    auto children = dir.children();
    EXPECT_EQ(children.size(), 3);
}

TEST_F(PathTest, ChildFiles) {
    auto dir = get_test_path("childfiles_test");
    filesystem::create_directories(dir);
    filesystem::create_and_write(dir / path("f1.txt"), "1");
    filesystem::create_and_write(dir / path("f2.txt"), "2");
    filesystem::create_directories(dir / path("sub"));

    auto files = dir.child_files();
    EXPECT_EQ(files.size(), 2);
}

TEST_F(PathTest, ChildDirs) {
    auto dir = get_test_path("childdirs_test");
    filesystem::create_directories(dir);
    filesystem::create_and_write(dir / path("f.txt"), "f");
    filesystem::create_directories(dir / path("d1"));
    filesystem::create_directories(dir / path("d2"));

    auto dirs = dir.child_dirs();
    EXPECT_EQ(dirs.size(), 2);
}

TEST_F(PathTest, Exists) {
    auto p = get_test_path("exists_test.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_TRUE(p.exists());
}

TEST_F(PathTest, ExistsNonExistent) {
    auto p = get_test_path("no_exists.txt");
    EXPECT_FALSE(p.exists());
}

TEST_F(PathTest, ExistsStatic) {
    auto p = get_test_path("exists_static.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_TRUE(path::exists(p.str()));
    EXPECT_FALSE(path::exists("/no/such/path/at/all"));
}

TEST_F(PathTest, IsDirectory) {
    auto p = get_test_path("is_dir");
    filesystem::create_directories(p);
    EXPECT_TRUE(p.is_directory());
}

TEST_F(PathTest, IsDirectoryFalse) {
    auto p = get_test_path("not_dir.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_FALSE(p.is_directory());
}

TEST_F(PathTest, IsDirectoryStatic) {
    auto p = get_test_path("is_dir_static");
    filesystem::create_directories(p);
    EXPECT_TRUE(path::is_directory(p.str()));
}

TEST_F(PathTest, IsFile) {
    auto p = get_test_path("is_file.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_TRUE(p.is_file());
}

TEST_F(PathTest, IsFileFalse) {
    auto p = get_test_path("is_file_dir");
    filesystem::create_directories(p);
    EXPECT_FALSE(p.is_file());
}

TEST_F(PathTest, IsFileStatic) {
    auto p = get_test_path("is_file_static.txt");
    filesystem::create_and_write(p, "data");
    EXPECT_TRUE(path::is_file(p.str()));
}

TEST_F(PathTest, EqualTo) {
    path p1("/home/user");
    path p2("/home/user");
    EXPECT_TRUE(p1.equal_to(p2));
}

TEST_F(PathTest, EqualToDifferent) {
    path p1("/home/user");
    path p2("/home/admin");
    EXPECT_FALSE(p1.equal_to(p2));
}

TEST_F(PathTest, EqualToNormalized) {
    path p1("/home/../home/user");
    path p2("/home/user");
    EXPECT_TRUE(p1.equal_to(p2));
}

TEST_F(PathTest, LessThan) {
    path p1("a");
    path p2("b");
    EXPECT_TRUE(p1.less_than(p2));
    EXPECT_FALSE(p2.less_than(p1));
}

TEST_F(PathTest, ToHash) {
    path p1("/home/user");
    path p2("/home/user");
    EXPECT_EQ(p1.to_hash(), p2.to_hash());
}

TEST_F(PathTest, ToHashDifferent) {
    path p1("/home/user");
    path p2("/home/admin");
    EXPECT_NE(p1.to_hash(), p2.to_hash());
}

TEST_F(PathTest, ToString) {
    path p("/home/../home/user");
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(p.to_string(), "\\home\\user");
#else
    EXPECT_EQ(p.to_string(), "/home/user");
#endif
}

TEST_F(PathTest, OperatorStringView) {
    path p("convert/me");
    string_view sv = static_cast<string_view>(p);
    EXPECT_EQ(sv, "convert/me");
}

TEST_F(PathTest, Swap) {
    path p1("first");
    path p2("second");
    p1.swap(p2);
    EXPECT_EQ(p1.str(), "second");
    EXPECT_EQ(p2.str(), "first");
}

TEST_F(PathTest, SplitIteratorWindowsDriveLetter) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    path p("C:\\Windows\\System32");
    vector<string> parts;
    for (auto it = p.begin(); it != p.end(); ++it) {
        parts.push_back(*it);
    }
    ASSERT_GE(parts.size(), 3);
    EXPECT_EQ(parts[0], "C:");
    EXPECT_EQ(parts[1], "Windows");
    EXPECT_EQ(parts[2], "System32");
#endif
}

TEST_F(PathTest, ChildrenHiddenFilesNotIncluded) {
    auto dir = get_test_path("hidden_test");
    filesystem::create_directories(dir);
    filesystem::create_and_write(dir / path("visible.txt"), "v");
    filesystem::create_and_write(dir / path(".hidden.txt"), "h");

    auto children = dir.children(false);
    EXPECT_EQ(children.size(), 1);
}

TEST_F(PathTest, ChildrenHiddenFilesIncluded) {
    auto dir = get_test_path("hidden_include_test");
    filesystem::create_directories(dir);
    filesystem::create_and_write(dir / path("visible.txt"), "v");
    filesystem::create_and_write(dir / path(".hidden.txt"), "h");

    auto children = dir.children(true);
    EXPECT_EQ(children.size(), 2);
}

TEST_F(PathTest, SplitIteratorBeginEndEmptyPath) {
    path p;
    auto it = p.begin();
    auto end = p.end();
    EXPECT_EQ(it, end);
}

TEST_F(PathTest, SplitIteratorPostfixIncrement) {
    path p("a/b");
    auto it = p.begin();
    auto old = it++;
    EXPECT_EQ(*old, "a");
    EXPECT_EQ(*it, "b");
}

TEST_F(PathTest, SplitIteratorInequality) {
    path p("a/b");
    auto it1 = p.begin();
    auto it2 = p.begin();
    EXPECT_FALSE(it1 != it2);
    ++it1;
    EXPECT_TRUE(it1 != it2);
}

TEST_F(PathTest, LexicallyNormalDotDotBeyondRoot) {
    path p("/../../etc");
    auto norm = p.lexically_normal();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(norm.str(), "\\etc");
#else
    EXPECT_EQ(norm.str(), "/etc");
#endif
}

TEST_F(PathTest, LexicallyNormalRelativeDotDot) {
    path p("a/../../b");
    auto norm = p.lexically_normal();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(norm.str(), "..\\b");
#else
    EXPECT_EQ(norm.str(), "../b");
#endif
}

class PathTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_pathtree_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);

        filesystem::create_directories(test_dir_ / path("empty_dir"));
        filesystem::create_directories(test_dir_ / path("sub1"));
        filesystem::create_directories(test_dir_ / path("sub1") / path("nested"));
        filesystem::create_and_write(test_dir_ / path("file1.txt"), "content1");
        filesystem::create_and_write(test_dir_ / path("file2.txt"), "content2");
        filesystem::create_and_write(test_dir_ / path("sub1") / path("file3.txt"), "content3");
        filesystem::create_and_write(test_dir_ / path("sub1") / path("nested") / path("file4.txt"), "content4");
        filesystem::create_and_write(test_dir_ / path("data.bin"), "binary");
        filesystem::create_and_write(test_dir_ / path("sub1") / path("data.bin"), "binary2");
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path_tree scan_default() const { return path_tree::scan(test_dir_, path_tree::scan_options{}); }

    path test_dir_;
};

TEST_F(PathTreeTest, DefaultConstructor) {
    path_tree tree;
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.root(), nullptr);
}

TEST_F(PathTreeTest, ConstructorWithPath) {
    path_tree tree(test_dir_);
    EXPECT_FALSE(tree.empty());
    EXPECT_NE(tree.root(), nullptr);
    EXPECT_TRUE(tree.root()->is_directory());
    EXPECT_EQ(tree.root()->get_path(), test_dir_);
}

TEST_F(PathTreeTest, ScanDefaultOptions) {
    auto tree = path_tree::scan(test_dir_, path_tree::scan_options{});
    EXPECT_FALSE(tree.empty());
    EXPECT_NE(tree.root(), nullptr);
    EXPECT_TRUE(tree.root()->is_directory());
}

TEST_F(PathTreeTest, ScanNonExistentPath) {
    path non_existent = test_dir_ / path("does_not_exist");
    auto tree = path_tree::scan(non_existent, path_tree::scan_options{});
    EXPECT_TRUE(tree.empty());
}

TEST_F(PathTreeTest, ScanMaxDepth) {
    path_tree::scan_options opts;
    opts.max_depth = 1;

    auto tree = path_tree::scan(test_dir_, opts);
    EXPECT_FALSE(tree.empty());

    auto root = tree.root();
    EXPECT_EQ(root->depth(), 0);

    for (const auto& child: root->children()) {
        EXPECT_EQ(child->depth(), 1);
        EXPECT_TRUE(child->children().empty());
    }
}

TEST_F(PathTreeTest, ScanMaxDepthZero) {
    path_tree::scan_options opts;
    opts.max_depth = 0;

    auto tree = path_tree::scan(test_dir_, opts);
    EXPECT_FALSE(tree.empty());
    EXPECT_GE(tree.size(), 1);
}

TEST_F(PathTreeTest, ScanFilesOnly) {
    path_tree::scan_options opts;
    opts.files_only = true;

    auto tree = path_tree::scan(test_dir_, opts);
    EXPECT_FALSE(tree.empty());

    auto file_paths = tree.all_file_paths();
    auto dir_paths = tree.all_dir_paths();
    EXPECT_GT(file_paths.size(), 0);

    if (tree.root()->is_directory()) {
        EXPECT_EQ(dir_paths.size(), 1);
    }
}

TEST_F(PathTreeTest, ScanDirsOnly) {
    path_tree::scan_options opts;
    opts.dirs_only = true;

    auto tree = path_tree::scan(test_dir_, opts);
    EXPECT_FALSE(tree.empty());

    tree.traverse_dfs([](const path_tree::node& n) -> path_tree::visit_result {
        if (!n.is_root()) {
            EXPECT_TRUE(n.is_directory());
        }
        return path_tree::visit_result::proceed;
    });
}

TEST_F(PathTreeTest, ScanExtensionFilter) {
    path_tree::scan_options opts;
    opts.extensions = {string("txt")};

    auto tree = path_tree::scan(test_dir_, opts);
    EXPECT_FALSE(tree.empty());

    auto txt_files = tree.find_by_extension("txt");
    auto bin_files = tree.find_by_extension("bin");

    EXPECT_GT(txt_files.size(), 0);
    for (const auto& f: txt_files) {
        EXPECT_EQ(f->get_path().extension(), "txt");
    }
}

TEST_F(PathTreeTest, ScanMultipleExtensions) {
    path_tree::scan_options opts;
    opts.extensions = {string("txt"), string("bin")};

    auto tree = path_tree::scan(test_dir_, opts);

    auto txt_files = tree.find_by_extension("txt");
    auto bin_files = tree.find_by_extension("bin");

    EXPECT_GT(txt_files.size(), 0);
    EXPECT_GT(bin_files.size(), 0);
}

TEST_F(PathTreeTest, ScanCustomFilter) {
    path_tree::scan_options opts;
    opts.custom_filter = [](const path_tree::node& n) {
        return n.get_path().filename().find("file") != string_view::npos;
    };

    auto tree = path_tree::scan(test_dir_, opts);
    EXPECT_FALSE(tree.empty());

    auto all_paths = tree.all_paths();
    for (const auto& p: all_paths) {
        if (p == test_dir_ || p == test_dir_.absolute()) {
            continue;
        }
        auto fname = p.filename();
        if (!fname.empty()) {
            EXPECT_NE(fname.find("file"), string_view::npos);
        }
    }
}

TEST_F(PathTreeTest, Empty) {
    path_tree tree;
    EXPECT_TRUE(tree.empty());

    path_tree tree2(test_dir_);
    EXPECT_FALSE(tree2.empty());
}

TEST_F(PathTreeTest, Root) {
    auto tree = scan_default();
    auto root = tree.root();
    EXPECT_NE(root, nullptr);
    EXPECT_TRUE(root->is_root());
    EXPECT_EQ(root->get_path(), test_dir_);
}

TEST_F(PathTreeTest, Size) {
    auto tree = scan_default();
    size_t s = tree.size();
    EXPECT_GT(s, 0);

    path_tree empty_tree;
    EXPECT_EQ(empty_tree.size(), 0);
}

TEST_F(PathTreeTest, MaxDepth) {
    auto tree = scan_default();
    size_t md = tree.max_depth();
    EXPECT_GE(md, 2);
}

TEST_F(PathTreeTest, MaxDepthEmptyTree) {
    path_tree tree;
    EXPECT_EQ(tree.max_depth(), 0);
}

TEST_F(PathTreeTest, Find) {
    auto tree = scan_default();
    auto target = test_dir_ / path("sub1") / path("nested") / path("file4.txt");

    auto found = tree.find(target);
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->get_path(), target);
    EXPECT_TRUE(found->is_file());
}

TEST_F(PathTreeTest, FindDirectory) {
    auto tree = scan_default();
    auto target = test_dir_ / path("sub1") / path("nested");

    auto found = tree.find(target);
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->get_path(), target);
    EXPECT_TRUE(found->is_directory());
}

TEST_F(PathTreeTest, FindNonExistent) {
    auto tree = scan_default();
    auto target = test_dir_ / path("no_such_file.txt");

    auto found = tree.find(target);
    EXPECT_EQ(found, nullptr);
}

TEST_F(PathTreeTest, FindEmptyTree) {
    path_tree tree;
    auto found = tree.find(test_dir_);
    EXPECT_EQ(found, nullptr);
}

TEST_F(PathTreeTest, FindAll) {
    auto tree = scan_default();
    auto results = tree.find_all("data.bin");
    EXPECT_EQ(results.size(), 2);
}

TEST_F(PathTreeTest, FindAllNoMatch) {
    auto tree = scan_default();
    auto results = tree.find_all("no_match.txt");
    EXPECT_TRUE(results.empty());
}

TEST_F(PathTreeTest, FindIf) {
    auto tree = scan_default();
    auto results = tree.find_if([](const path_tree::node& n) { return n.depth() == 2; });
    EXPECT_GT(results.size(), 0);

    for (const auto& r: results) {
        EXPECT_EQ(r->depth(), 2);
    }
}

TEST_F(PathTreeTest, FindByExtension) {
    auto tree = scan_default();
    auto results = tree.find_by_extension("txt");
    EXPECT_GT(results.size(), 0);

    for (const auto& r: results) {
        EXPECT_EQ(r->get_path().extension(), "txt");
        EXPECT_TRUE(r->is_file());
    }
}

TEST_F(PathTreeTest, FindByExtensionNoMatch) {
    auto tree = scan_default();
    auto results = tree.find_by_extension("xyz");
    EXPECT_TRUE(results.empty());
}

TEST_F(PathTreeTest, TraverseDfs) {
    auto tree = scan_default();
    size_t count = 0;
    bool found_root = false;

    tree.traverse_dfs([&](const path_tree::node& n) -> path_tree::visit_result {
        ++count;
        if (n.is_root()) {
            found_root = true;
        }
        return path_tree::visit_result::proceed;
    });

    EXPECT_GT(count, 0);
    EXPECT_TRUE(found_root);
}

TEST_F(PathTreeTest, TraverseDfsStop) {
    auto tree = scan_default();
    size_t count = 0;

    tree.traverse_dfs([&](const path_tree::node& n) -> path_tree::visit_result {
        ++count;
        if (count >= 3) {
            return path_tree::visit_result::stop;
        }
        return path_tree::visit_result::proceed;
    });

    EXPECT_EQ(count, 3);
}

TEST_F(PathTreeTest, TraverseDfsSkip) {
    auto tree = scan_default();
    size_t dir_skip_count = 0;
    size_t file_count = 0;

    tree.traverse_dfs([&](const path_tree::node& n) -> path_tree::visit_result {
        if (n.get_path().filename() == "sub1") {
            ++dir_skip_count;
            return path_tree::visit_result::skip;
        }
        if (n.is_file()) {
            ++file_count;
        }
        return path_tree::visit_result::proceed;
    });

    EXPECT_GT(dir_skip_count, 0);
    EXPECT_EQ(file_count, 3);
}

TEST_F(PathTreeTest, TraverseDfsEmptyTree) {
    path_tree tree;
    size_t count = 0;
    tree.traverse_dfs([&](const path_tree::node&) -> path_tree::visit_result {
        ++count;
        return path_tree::visit_result::proceed;
    });
    EXPECT_EQ(count, 0);
}

TEST_F(PathTreeTest, TraverseBfs) {
    auto tree = scan_default();
    size_t count = 0;
    vector<size_t> depths;

    tree.traverse_bfs([&](const path_tree::node& n) -> path_tree::visit_result {
        ++count;
        depths.push_back(n.depth());
        return path_tree::visit_result::proceed;
    });

    EXPECT_GT(count, 0);
    for (size_t i = 1; i < depths.size(); ++i) {
        EXPECT_GE(depths[i], depths[i - 1]);
    }
}

TEST_F(PathTreeTest, TraverseBfsStop) {
    auto tree = scan_default();
    size_t count = 0;

    tree.traverse_bfs([&](const path_tree::node&) -> path_tree::visit_result {
        ++count;
        if (count >= 3) {
            return path_tree::visit_result::stop;
        }
        return path_tree::visit_result::proceed;
    });

    EXPECT_EQ(count, 3);
}

TEST_F(PathTreeTest, TraverseBfsSkip) {
    auto tree = scan_default();
    size_t dir_skip_count = 0;
    size_t file_count = 0;

    tree.traverse_bfs([&](const path_tree::node& n) -> path_tree::visit_result {
        if (n.get_path().filename() == "sub1") {
            ++dir_skip_count;
            return path_tree::visit_result::skip;
        }
        if (n.is_file()) {
            ++file_count;
        }
        return path_tree::visit_result::proceed;
    });

    EXPECT_GT(dir_skip_count, 0);
    EXPECT_EQ(file_count, 3);
}

TEST_F(PathTreeTest, TraverseBfsEmptyTree) {
    path_tree tree;
    size_t count = 0;
    tree.traverse_bfs([&](const path_tree::node&) -> path_tree::visit_result {
        ++count;
        return path_tree::visit_result::proceed;
    });
    EXPECT_EQ(count, 0);
}

TEST_F(PathTreeTest, TraverseFiles) {
    auto tree = scan_default();
    size_t file_count = 0;

    tree.traverse_files([&](const path_tree::node& n) -> path_tree::visit_result {
        EXPECT_TRUE(n.is_file());
        ++file_count;
        return path_tree::visit_result::proceed;
    });

    EXPECT_GT(file_count, 0);
}

TEST_F(PathTreeTest, TraverseFilesStop) {
    auto tree = scan_default();
    size_t count = 0;

    tree.traverse_files([&](const path_tree::node&) -> path_tree::visit_result {
        ++count;
        if (count >= 2) {
            return path_tree::visit_result::stop;
        }
        return path_tree::visit_result::proceed;
    });

    EXPECT_EQ(count, 2);
}

TEST_F(PathTreeTest, TraverseDirs) {
    auto tree = scan_default();
    size_t dir_count = 0;

    tree.traverse_dirs([&](const path_tree::node& n) -> path_tree::visit_result {
        EXPECT_TRUE(n.is_directory());
        ++dir_count;
        return path_tree::visit_result::proceed;
    });

    EXPECT_GT(dir_count, 0);
}

TEST_F(PathTreeTest, TraverseDirsStop) {
    auto tree = scan_default();
    size_t count = 0;

    tree.traverse_dirs([&](const path_tree::node&) -> path_tree::visit_result {
        ++count;
        if (count >= 2) {
            return path_tree::visit_result::stop;
        }
        return path_tree::visit_result::proceed;
    });

    EXPECT_EQ(count, 2);
}

TEST_F(PathTreeTest, InsertFile) {
    path_tree tree(test_dir_);
    auto inserted = tree.insert(path("inserted_file.txt"), path_tree::node_type::file);
    EXPECT_NE(inserted, nullptr);
    EXPECT_EQ(inserted->get_path(), test_dir_ / path("inserted_file.txt"));
    EXPECT_TRUE(inserted->is_file());

    auto found = tree.find(test_dir_ / path("inserted_file.txt"));
    EXPECT_NE(found, nullptr);
}

TEST_F(PathTreeTest, InsertDirectory) {
    path_tree tree(test_dir_);
    auto inserted = tree.insert(path("inserted_dir"), path_tree::node_type::directory);
    EXPECT_NE(inserted, nullptr);
    EXPECT_EQ(inserted->get_path(), test_dir_ / path("inserted_dir"));
    EXPECT_TRUE(inserted->is_directory());

    auto found = tree.find(test_dir_ / path("inserted_dir"));
    EXPECT_NE(found, nullptr);
}

TEST_F(PathTreeTest, InsertCreatesIntermediateDirectories) {
    path_tree tree(test_dir_);
    auto new_path = path("a/b/c/file.txt");

    auto inserted = tree.insert(new_path, path_tree::node_type::file);
    EXPECT_NE(inserted, nullptr);
    EXPECT_EQ(inserted->get_path(), test_dir_ / new_path);

    auto dir_a = tree.find(test_dir_ / path("a"));
    auto dir_b = tree.find(test_dir_ / path("a/b"));
    auto dir_c = tree.find(test_dir_ / path("a/b/c"));

    EXPECT_NE(dir_a, nullptr);
    EXPECT_NE(dir_b, nullptr);
    EXPECT_NE(dir_c, nullptr);
    EXPECT_TRUE(dir_a->is_directory());
    EXPECT_TRUE(dir_b->is_directory());
    EXPECT_TRUE(dir_c->is_directory());
}

TEST_F(PathTreeTest, InsertExistingPath) {
    auto tree = scan_default();
    auto existing = path("file1.txt");

    size_t size_before = tree.size();
    auto inserted = tree.insert(existing, path_tree::node_type::file);
    size_t size_after = tree.size();

    EXPECT_NE(inserted, nullptr);
    EXPECT_EQ(size_before, size_after);
}

TEST_F(PathTreeTest, InsertEmptyTree) {
    path_tree tree;
    auto result = tree.insert(test_dir_ / path("test.txt"));
    EXPECT_EQ(result, nullptr);
}

TEST_F(PathTreeTest, Remove) {
    auto tree = scan_default();
    auto target = test_dir_ / path("file1.txt");

    EXPECT_NE(tree.find(target), nullptr);
    EXPECT_TRUE(tree.remove(target));
    EXPECT_EQ(tree.find(target), nullptr);
}

TEST_F(PathTreeTest, RemoveDirectory) {
    auto tree = scan_default();
    auto target = test_dir_ / path("sub1") / path("nested");

    EXPECT_NE(tree.find(target), nullptr);
    EXPECT_TRUE(tree.remove(target));
    EXPECT_EQ(tree.find(target), nullptr);
}

TEST_F(PathTreeTest, RemoveRoot) {
    auto tree = scan_default();
    EXPECT_TRUE(tree.remove(tree.root()->get_path()));
    EXPECT_TRUE(tree.empty());
}

TEST_F(PathTreeTest, RemoveNonExistent) {
    auto tree = scan_default();
    auto target = test_dir_ / path("no_such.txt");

    EXPECT_FALSE(tree.remove(target));
}

TEST_F(PathTreeTest, RemoveEmptyTree) {
    path_tree tree;
    EXPECT_FALSE(tree.remove(test_dir_));
}

TEST_F(PathTreeTest, Merge) {
    auto tree1 = scan_default();

    auto other_dir = test_dir_ / path("merge_other");
    filesystem::create_directories(other_dir);
    filesystem::create_and_write(other_dir / path("other_file.txt"), "other");

    auto tree2 = path_tree::scan(other_dir, path_tree::scan_options{});
    tree1.merge(tree2);

    auto found = tree1.find(other_dir / path("other_file.txt"));
    EXPECT_NE(found, nullptr);
}

TEST_F(PathTreeTest, MergeEmptyTree) {
    auto tree1 = scan_default();
    path_tree tree2;

    size_t size_before = tree1.size();
    tree1.merge(tree2);
    size_t size_after = tree1.size();

    EXPECT_EQ(size_before, size_after);
}

TEST_F(PathTreeTest, MergeIntoEmptyTree) {
    path_tree tree1;
    auto tree2 = scan_default();

    tree1.merge(tree2);
    EXPECT_FALSE(tree1.empty());
}

TEST_F(PathTreeTest, Subtree) {
    auto tree = scan_default();
    auto target = test_dir_ / path("sub1");

    auto sub = tree.subtree(target);
    EXPECT_FALSE(sub.empty());
    EXPECT_NE(sub.root(), nullptr);
    EXPECT_EQ(sub.root()->get_path(), target);

    auto nested = sub.find(target / path("nested"));
    EXPECT_NE(nested, nullptr);
}

TEST_F(PathTreeTest, SubtreeNonExistent) {
    auto tree = scan_default();
    auto target = test_dir_ / path("no_such_subtree");

    auto sub = tree.subtree(target);
    EXPECT_TRUE(sub.empty());
}

TEST_F(PathTreeTest, SubtreeEmptyTree) {
    path_tree tree;
    auto sub = tree.subtree(test_dir_);
    EXPECT_TRUE(sub.empty());
}

TEST_F(PathTreeTest, Prune) {
    auto tree = scan_default();

    auto pruned = tree.prune([](const path_tree::node& n) { return n.get_path().extension() == "txt"; });

    EXPECT_FALSE(pruned.empty());

    auto paths = pruned.all_file_paths();
    for (const auto& p: paths) {
        EXPECT_EQ(p.extension(), "txt");
    }
}

TEST_F(PathTreeTest, PruneKeepDirectoriesWithMatchingChildren) {
    auto tree = scan_default();
    auto target = test_dir_ / path("sub1") / path("nested");
    EXPECT_NE(tree.find(target), nullptr);

    auto pruned = tree.prune([](const path_tree::node& n) { return n.is_file() && n.get_path().extension() == "txt"; });

    auto nested_in_pruned = pruned.find(target);
    EXPECT_NE(nested_in_pruned, nullptr);
}

TEST_F(PathTreeTest, PruneEmptyResult) {
    auto tree = scan_default();

    auto pruned = tree.prune([](const path_tree::node&) { return false; });

    EXPECT_TRUE(pruned.empty());
}

TEST_F(PathTreeTest, PruneEmptyTree) {
    path_tree tree;
    auto pruned = tree.prune([](const path_tree::node&) { return true; });
    EXPECT_TRUE(pruned.empty());
}

TEST_F(PathTreeTest, AllPaths) {
    auto tree = scan_default();
    auto paths = tree.all_paths();

    EXPECT_GT(paths.size(), 0);
    EXPECT_EQ(paths[0], tree.root()->get_path());
}

TEST_F(PathTreeTest, AllPathsEmptyTree) {
    path_tree tree;
    auto paths = tree.all_paths();
    EXPECT_TRUE(paths.empty());
}

TEST_F(PathTreeTest, AllFilePaths) {
    auto tree = scan_default();
    auto paths = tree.all_file_paths();

    EXPECT_GT(paths.size(), 0);
    for (const auto& p: paths) {
        EXPECT_TRUE(p.is_file());
    }
}

TEST_F(PathTreeTest, AllDirPaths) {
    auto tree = scan_default();
    auto paths = tree.all_dir_paths();

    EXPECT_GT(paths.size(), 0);
    for (const auto& p: paths) {
        EXPECT_TRUE(p.is_directory());
    }
}

TEST_F(PathTreeTest, ToString) {
    auto tree = scan_default();
    auto str = tree.to_string();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("file1.txt"), string::npos);
}

TEST_F(PathTreeTest, ToStringCustomIndent) {
    auto tree = scan_default();
    auto str = tree.to_string("    ");
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("    file1.txt"), string::npos);
}

TEST_F(PathTreeTest, ToStringEmptyTree) {
    path_tree tree;
    EXPECT_TRUE(tree.to_string().empty());
}

TEST_F(PathTreeTest, ToStringEmptyTreeCustomIndent) {
    path_tree tree;
    EXPECT_TRUE(tree.to_string("  ").empty());
}

class PathTreeNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_node_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path test_dir_;
};

TEST_F(PathTreeNodeTest, DefaultConstructor) {
    path_tree::node n;
    EXPECT_EQ(n.type(), path_tree::node_type::unknown);
    EXPECT_EQ(n.depth(), 0);
    EXPECT_TRUE(n.get_path().empty());
}

TEST_F(PathTreeNodeTest, Constructor) {
    path p("/test/path");
    path_tree::node n(p, path_tree::node_type::file, 3);

    EXPECT_EQ(n.get_path(), p);
    EXPECT_EQ(n.type(), path_tree::node_type::file);
    EXPECT_EQ(n.depth(), 3);
}

TEST_F(PathTreeNodeTest, GetPath) {
    path p("/some/file.txt");
    path_tree::node n(p, path_tree::node_type::file, 0);
    EXPECT_EQ(n.get_path(), p);
}

TEST_F(PathTreeNodeTest, Type) {
    path_tree::node dir_node(path("/dir"), path_tree::node_type::directory, 0);
    EXPECT_EQ(dir_node.type(), path_tree::node_type::directory);

    path_tree::node file_node(path("/file"), path_tree::node_type::file, 0);
    EXPECT_EQ(file_node.type(), path_tree::node_type::file);

    path_tree::node symlink_node(path("/link"), path_tree::node_type::symlink, 0);
    EXPECT_EQ(symlink_node.type(), path_tree::node_type::symlink);
}

TEST_F(PathTreeNodeTest, Depth) {
    path_tree::node n(path("/test"), path_tree::node_type::file, 5);
    EXPECT_EQ(n.depth(), 5);
}

TEST_F(PathTreeNodeTest, IsDirectory) {
    path_tree::node dir(path("/dir"), path_tree::node_type::directory, 0);
    EXPECT_TRUE(dir.is_directory());

    path_tree::node file(path("/file"), path_tree::node_type::file, 0);
    EXPECT_FALSE(file.is_directory());

    path_tree::node unknown(path("/unknown"), path_tree::node_type::unknown, 0);
    EXPECT_FALSE(unknown.is_directory());
}

TEST_F(PathTreeNodeTest, IsFile) {
    path_tree::node file(path("/file"), path_tree::node_type::file, 0);
    EXPECT_TRUE(file.is_file());

    path_tree::node symlink(path("/link"), path_tree::node_type::symlink, 0);
    EXPECT_TRUE(symlink.is_file());

    path_tree::node dir(path("/dir"), path_tree::node_type::directory, 0);
    EXPECT_FALSE(dir.is_file());

    path_tree::node unknown(path("/unknown"), path_tree::node_type::unknown, 0);
    EXPECT_FALSE(unknown.is_file());
}

TEST_F(PathTreeNodeTest, IsRoot) {
    path_tree::node n(path("/root"), path_tree::node_type::directory, 0);
    EXPECT_TRUE(n.is_root());
}

TEST_F(PathTreeNodeTest, IsRootWithParent) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto child = make_shared<path_tree::node>(path("/parent/child"), path_tree::node_type::file, 1);

    parent->add_child(child);

    EXPECT_TRUE(parent->is_root());
    EXPECT_FALSE(child->is_root());
}

TEST_F(PathTreeNodeTest, IsLeaf) {
    path_tree::node n(path("/leaf"), path_tree::node_type::file, 0);
    EXPECT_TRUE(n.is_leaf());
}

TEST_F(PathTreeNodeTest, IsLeafWithChildren) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto child = make_shared<path_tree::node>(path("/parent/child"), path_tree::node_type::file, 1);

    parent->add_child(child);

    EXPECT_FALSE(parent->is_leaf());
    EXPECT_TRUE(child->is_leaf());
}

TEST_F(PathTreeNodeTest, Parent) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto child = make_shared<path_tree::node>(path("/parent/child"), path_tree::node_type::file, 1);

    parent->add_child(child);

    auto child_parent = child->parent();
    EXPECT_EQ(child_parent, parent);

    auto root_parent = parent->parent();
    EXPECT_EQ(root_parent, nullptr);
}

TEST_F(PathTreeNodeTest, Children) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto child1 = make_shared<path_tree::node>(path("/parent/child1"), path_tree::node_type::file, 1);
    auto child2 = make_shared<path_tree::node>(path("/parent/child2"), path_tree::node_type::file, 1);

    parent->add_child(child1);
    parent->add_child(child2);

    const auto& children = parent->children();
    EXPECT_EQ(children.size(), 2);
    EXPECT_EQ(children[0], child1);
    EXPECT_EQ(children[1], child2);
}

TEST_F(PathTreeNodeTest, ChildCount) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);

    EXPECT_EQ(parent->child_count(), 0);

    auto child = make_shared<path_tree::node>(path("/parent/child"), path_tree::node_type::file, 1);
    parent->add_child(child);

    EXPECT_EQ(parent->child_count(), 1);
}

TEST_F(PathTreeNodeTest, FindChild) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto child1 = make_shared<path_tree::node>(path("/parent/file1.txt"), path_tree::node_type::file, 1);
    auto child2 = make_shared<path_tree::node>(path("/parent/file2.txt"), path_tree::node_type::file, 1);

    parent->add_child(child1);
    parent->add_child(child2);

    auto found = parent->find_child("file1.txt");
    EXPECT_EQ(found, child1);

    auto found2 = parent->find_child("file2.txt");
    EXPECT_EQ(found2, child2);

    auto not_found = parent->find_child("file3.txt");
    EXPECT_EQ(not_found, nullptr);
}

TEST_F(PathTreeNodeTest, FindChildEmpty) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto found = parent->find_child("anything");
    EXPECT_EQ(found, nullptr);
}

TEST_F(PathTreeNodeTest, AddChild) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto child = make_shared<path_tree::node>(path("/parent/child"), path_tree::node_type::file, 1);

    parent->add_child(child);

    EXPECT_EQ(parent->child_count(), 1);
    EXPECT_FALSE(child->is_root());
    EXPECT_EQ(child->parent(), parent);
}

TEST_F(PathTreeNodeTest, RemoveChild) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    auto child1 = make_shared<path_tree::node>(path("/parent/child1"), path_tree::node_type::file, 1);
    auto child2 = make_shared<path_tree::node>(path("/parent/child2"), path_tree::node_type::file, 1);

    parent->add_child(child1);
    parent->add_child(child2);

    EXPECT_TRUE(parent->remove_child("child1"));
    EXPECT_EQ(parent->child_count(), 1);
    EXPECT_EQ(parent->find_child("child1"), nullptr);

    EXPECT_FALSE(parent->remove_child("child3"));
}

TEST_F(PathTreeNodeTest, RemoveChildEmpty) {
    auto parent = make_shared<path_tree::node>(path("/parent"), path_tree::node_type::directory, 0);
    EXPECT_FALSE(parent->remove_child("anything"));
}

TEST_F(PathTreeNodeTest, CopyConstructorDeleted) { EXPECT_FALSE(is_copy_constructible_v<path_tree::node>); }

TEST_F(PathTreeNodeTest, CopyAssignmentDeleted) { EXPECT_FALSE(is_copy_assignable_v<path_tree::node>); }

TEST_F(PathTreeNodeTest, MoveConstructor) {
    auto n1 = make_shared<path_tree::node>(path("/test"), path_tree::node_type::file, 2);
    auto n2 = make_shared<path_tree::node>(move(*n1));

    EXPECT_EQ(n2->get_path(), path("/test"));
    EXPECT_EQ(n2->type(), path_tree::node_type::file);
    EXPECT_EQ(n2->depth(), 2);
}

TEST_F(PathTreeNodeTest, MoveAssignment) {
    auto n1 = make_shared<path_tree::node>(path("/first"), path_tree::node_type::file, 1);
    auto n2 = make_shared<path_tree::node>(path("/second"), path_tree::node_type::directory, 3);

    *n2 = move(*n1);

    EXPECT_EQ(n2->get_path(), path("/first"));
    EXPECT_EQ(n2->type(), path_tree::node_type::file);
    EXPECT_EQ(n2->depth(), 1);
}

class TempFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_tempfile_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path test_dir_;
};

TEST_F(TempFileTest, DefaultConstructor) {
    path created_path;
    {
        temp_file tf;
        EXPECT_TRUE(tf.file().is_opened());
        EXPECT_EQ(tf.policy(), temp_file::delete_policy::AUTO_DELETE);
        created_path = tf.file().file_path();
        EXPECT_TRUE(created_path.exists());
        EXPECT_TRUE(created_path.is_file());
    }
    EXPECT_FALSE(created_path.exists());
}

TEST_F(TempFileTest, ConstructorWithPrefixAndSuffix) {
    path created_path;
    {
        temp_file tf("myprefix", ".dat");
        EXPECT_TRUE(tf.file().is_opened());
        created_path = tf.file().file_path();
        auto fname = created_path.filename();
        EXPECT_NE(fname.find("myprefix"), string_view::npos);
        EXPECT_NE(fname.rfind(".dat"), string_view::npos);
    }
    EXPECT_FALSE(created_path.exists());
}

TEST_F(TempFileTest, ConstructorWithCreationMode) {
    path created_path;
    {
        temp_file tf("test", ".txt", file_creation::CREATE_FORCE);
        EXPECT_TRUE(tf.file().is_opened());
        created_path = tf.file().file_path();
        EXPECT_TRUE(created_path.exists());
    }
    EXPECT_FALSE(created_path.exists());
}

TEST_F(TempFileTest, ConstructorWithExistingPath) {
    auto existing = test_dir_ / path("existing_temp.txt");
    filesystem::create_and_write(existing, "hello world");

    {
        temp_file tf(existing);
        EXPECT_TRUE(tf.file().is_opened());
        EXPECT_EQ(tf.file().file_path(), existing);
        EXPECT_EQ(tf.file().read(), "hello world");
    }
    EXPECT_FALSE(existing.exists());
}

TEST_F(TempFileTest, ConstructorWithExistingPathManualDelete) {
    auto existing = test_dir_ / path("manual_existing.txt");
    filesystem::create_and_write(existing, "keep me");

    {
        temp_file tf(existing, temp_file::delete_policy::MANUAL_DELETE);
        EXPECT_TRUE(tf.file().is_opened());
    }
    EXPECT_TRUE(existing.exists());
    filesystem::remove(existing);
}

TEST_F(TempFileTest, ConstructorWithExistingPathKeepOnExit) {
    auto existing = test_dir_ / path("keep_existing.txt");
    filesystem::create_and_write(existing, "keep forever");

    {
        temp_file tf(existing, temp_file::delete_policy::KEEP_ON_EXIT);
        EXPECT_TRUE(tf.file().is_opened());
    }
    EXPECT_TRUE(existing.exists());
    filesystem::remove(existing);
}

TEST_F(TempFileTest, AutoDeletePolicy) {
    path tf_path;
    {
        temp_file tf("auto_del", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::AUTO_DELETE);
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());
    }
    EXPECT_FALSE(tf_path.exists());
}

TEST_F(TempFileTest, ManualDeletePolicy) {
    path tf_path;
    {
        temp_file tf("manual_del", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::MANUAL_DELETE);
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());
    }
    EXPECT_TRUE(tf_path.exists());
    filesystem::remove(tf_path);
}

TEST_F(TempFileTest, KeepOnExitPolicy) {
    path tf_path;
    {
        temp_file tf("keep_exit", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());
    }
    EXPECT_TRUE(tf_path.exists());
    filesystem::remove(tf_path);
}

TEST_F(TempFileTest, MoveConstructor) {
    path original_path;
    temp_file tf1;
    original_path = tf1.file().file_path();
    EXPECT_TRUE(original_path.exists());
    EXPECT_TRUE(tf1.file().is_opened());

    temp_file tf2(move(tf1));
    EXPECT_TRUE(tf2.file().is_opened());
    EXPECT_EQ(tf2.file().file_path(), original_path);
    EXPECT_EQ(tf2.policy(), temp_file::delete_policy::AUTO_DELETE);
    EXPECT_EQ(tf1.policy(), temp_file::delete_policy::KEEP_ON_EXIT);
    EXPECT_FALSE(tf1.file().is_opened());

    tf2.cleanup();
    EXPECT_FALSE(original_path.exists());
}

TEST_F(TempFileTest, MoveAssignment) {
    path path1;
    path path2;
    {
        temp_file tf1("first", ".tmp");
        path1 = tf1.file().file_path();

        temp_file tf2("second", ".tmp");
        path2 = tf2.file().file_path();

        EXPECT_TRUE(path1.exists());
        EXPECT_TRUE(path2.exists());

        tf1 = move(tf2);

        EXPECT_FALSE(path1.exists());
        EXPECT_TRUE(path2.exists());
        EXPECT_EQ(tf1.file().file_path(), path2);
        EXPECT_EQ(tf1.policy(), temp_file::delete_policy::AUTO_DELETE);
        EXPECT_EQ(tf2.policy(), temp_file::delete_policy::KEEP_ON_EXIT);
    }
}

TEST_F(TempFileTest, MoveAssignmentSelf) {
    path tf_path;
    {
        temp_file tf("self_assign", ".tmp");
        tf_path = tf.file().file_path();

        tf = move(tf);

        EXPECT_TRUE(tf.file().is_opened());
        EXPECT_EQ(tf.file().file_path(), tf_path);
        EXPECT_EQ(tf.policy(), temp_file::delete_policy::AUTO_DELETE);
    }
}

TEST_F(TempFileTest, FileAccess) {
    path p;
    {
        temp_file tf("access", ".txt", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
        auto& f = tf.file();
        f.write("test content");
        f.flush();
        p = tf.file().file_path();
    }
    {
        file reader(p);
        EXPECT_EQ(reader.read(), "test content");
    }
    filesystem::remove(p);
    EXPECT_FALSE(p.exists());
}

TEST_F(TempFileTest, ConstFileAccess) {
    const temp_file tf("const_access", ".txt");
    const auto& f = tf.file();
    EXPECT_TRUE(f.is_opened());
}

TEST_F(TempFileTest, FileAccessWritable) {
    path p;
    {
        temp_file tf("writable", ".txt", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
        tf.file().write("data");
        tf.file().flush();
        p = tf.file().file_path();
    }
    {
        file reader(p);
        EXPECT_EQ(reader.read(), "data");
    }
}

TEST_F(TempFileTest, FileAccessKeep) {
    path p;
    {
        temp_file tf("keep", ".txt", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
        auto& f = tf.file();
        f.write("keep content");
        f.flush();
        p = tf.file().file_path();
    }
    EXPECT_TRUE(p.exists());
    {
        file reader(p);
        EXPECT_EQ(reader.read(), "keep content");
    }
    filesystem::remove(p);
}

TEST_F(TempFileTest, SetDeletePolicy) {
    temp_file tf("change_policy", ".tmp");
    EXPECT_EQ(tf.policy(), temp_file::delete_policy::AUTO_DELETE);

    tf.set_delete_policy(temp_file::delete_policy::MANUAL_DELETE);
    EXPECT_EQ(tf.policy(), temp_file::delete_policy::MANUAL_DELETE);

    tf.set_delete_policy(temp_file::delete_policy::KEEP_ON_EXIT);
    EXPECT_EQ(tf.policy(), temp_file::delete_policy::KEEP_ON_EXIT);

    tf.set_delete_policy(temp_file::delete_policy::AUTO_DELETE);
    EXPECT_EQ(tf.policy(), temp_file::delete_policy::AUTO_DELETE);
}

TEST_F(TempFileTest, PolicyGetter) {
    temp_file tf1("policy_auto", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::AUTO_DELETE);
    EXPECT_EQ(tf1.policy(), temp_file::delete_policy::AUTO_DELETE);

    temp_file tf2("policy_manual", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::MANUAL_DELETE);
    EXPECT_EQ(tf2.policy(), temp_file::delete_policy::MANUAL_DELETE);

    temp_file tf3("policy_keep", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
    EXPECT_EQ(tf3.policy(), temp_file::delete_policy::KEEP_ON_EXIT);
}

TEST_F(TempFileTest, CleanupAutoDelete) {
    path tf_path;
    {
        temp_file tf("cleanup_auto", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::AUTO_DELETE);
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());

        tf.cleanup();
        EXPECT_FALSE(tf_path.exists());
        EXPECT_FALSE(tf.file().is_opened());
    }
}

TEST_F(TempFileTest, CleanupManualDelete) {
    path tf_path;
    {
        temp_file tf("cleanup_manual", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::MANUAL_DELETE);
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());

        tf.cleanup();
        EXPECT_TRUE(tf_path.exists());
        EXPECT_FALSE(tf.file().is_opened());
    }
    filesystem::remove(tf_path);
}

TEST_F(TempFileTest, CleanupKeepOnExit) {
    path tf_path;
    {
        temp_file tf("cleanup_keep", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());

        tf.cleanup();
        EXPECT_TRUE(tf_path.exists());
        EXPECT_FALSE(tf.file().is_opened());
    }
    filesystem::remove(tf_path);
}

TEST_F(TempFileTest, CleanupAfterKeep) {
    path tf_path;
    {
        temp_file tf("keep_then_cleanup", ".tmp");
        tf_path = tf.file().file_path();
        tf.keep();

        tf.cleanup();
        EXPECT_TRUE(tf_path.exists());
    }
    filesystem::remove(tf_path);
}

TEST_F(TempFileTest, Release) {
    path tf_path;
    {
        temp_file tf("release_me", ".tmp");
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());

        tf.release();
        EXPECT_EQ(tf.policy(), temp_file::delete_policy::MANUAL_DELETE);
        EXPECT_TRUE(tf.file().is_opened());
    }
    EXPECT_TRUE(tf_path.exists());
    filesystem::remove(tf_path);
}

TEST_F(TempFileTest, ReleaseThenModifyPolicy) {
    path tf_path;
    {
        temp_file tf("release_policy", ".tmp");
        tf_path = tf.file().file_path();

        tf.release();
        EXPECT_EQ(tf.policy(), temp_file::delete_policy::MANUAL_DELETE);

        tf.set_delete_policy(temp_file::delete_policy::AUTO_DELETE);
        EXPECT_EQ(tf.policy(), temp_file::delete_policy::AUTO_DELETE);
    }
    EXPECT_FALSE(tf_path.exists());
}

TEST_F(TempFileTest, CreateTempFileFactory) {
    path created_path;
    {
        auto tf = temp_file::create_temp_file("factory", ".dat");
        EXPECT_TRUE(tf.file().is_opened());
        EXPECT_EQ(tf.policy(), temp_file::delete_policy::AUTO_DELETE);
        created_path = tf.file().file_path();
        auto fname = created_path.filename();
        EXPECT_NE(fname.find("factory"), string_view::npos);
        EXPECT_NE(fname.rfind(".dat"), string_view::npos);
    }
    EXPECT_FALSE(created_path.exists());
}

TEST_F(TempFileTest, CreateTempFileFactoryDefaultArgs) {
    path created_path;
    {
        auto tf = temp_file::create_temp_file();
        EXPECT_TRUE(tf.file().is_opened());
        EXPECT_EQ(tf.policy(), temp_file::delete_policy::AUTO_DELETE);
        created_path = tf.file().file_path();
    }
    EXPECT_FALSE(created_path.exists());
}

TEST_F(TempFileTest, CleanupAllTempFiles) {
    path path1;
    path path2;
    path path3;
    {
        temp_file tf1("all_cleanup_1", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::AUTO_DELETE);
        temp_file tf2("all_cleanup_2", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::AUTO_DELETE);
        temp_file tf3("all_cleanup_3", ".tmp", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);

        path1 = tf1.file().file_path();
        path2 = tf2.file().file_path();
        path3 = tf3.file().file_path();

        EXPECT_TRUE(path1.exists());
        EXPECT_TRUE(path2.exists());
        EXPECT_TRUE(path3.exists());

        tf1.file().close();
        tf2.file().close();
        temp_file::cleanup_all_temp_files();

        EXPECT_FALSE(path1.exists());
        EXPECT_FALSE(path2.exists());
        EXPECT_TRUE(path3.exists());
    }
    filesystem::remove(path3);
}

TEST_F(TempFileTest, CleanupAllTempFilesRegistryCleared) {
    path tf_path;
    {
        temp_file tf("registry_clear", ".tmp");
        tf_path = tf.file().file_path();
        EXPECT_TRUE(tf_path.exists());
    }
    EXPECT_FALSE(tf_path.exists());

    path second_path;
    {
        temp_file tf("registry_clear_2", ".tmp");
        second_path = tf.file().file_path();
        EXPECT_TRUE(second_path.exists());
    }
    EXPECT_FALSE(second_path.exists());
}

TEST_F(TempFileTest, RegisterForCleanup) {
    auto custom_path = test_dir_ / path("custom_cleanup.txt");
    filesystem::create_and_write(custom_path, "custom");

    temp_file::register_for_cleanup(custom_path);
    EXPECT_TRUE(custom_path.exists());

    temp_file::cleanup_all_temp_files();
    EXPECT_FALSE(custom_path.exists());
}

TEST_F(TempFileTest, RegisterForCleanupDuplicate) {
    auto custom_path = test_dir_ / path("dup_cleanup.txt");
    filesystem::create_and_write(custom_path, "duplicate");

    temp_file::register_for_cleanup(custom_path);
    temp_file::register_for_cleanup(custom_path);

    temp_file::cleanup_all_temp_files();
    EXPECT_FALSE(custom_path.exists());
}

TEST_F(TempFileTest, UniqueFileNames) {
    path path1;
    path path2;
    path path3;
    {
        temp_file tf1("unique", ".tmp");
        temp_file tf2("unique", ".tmp");
        temp_file tf3("unique", ".tmp");

        path1 = tf1.file().file_path();
        path2 = tf2.file().file_path();
        path3 = tf3.file().file_path();

        EXPECT_NE(path1, path2);
        EXPECT_NE(path2, path3);
        EXPECT_NE(path1, path3);
    }
}

TEST_F(TempFileTest, FileOperationsOnTempFile) {
    path p;
    {
        temp_file tf("fileops", ".txt", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
        tf.file().write("line1\n");
        tf.file().write("line2\n");
        tf.file().write("line3");
        tf.file().flush();
        p = tf.file().file_path();
    }
    {
        file reader(p, false, file_access::READ, file_shared::SHARE_READ);
        auto lines = reader.read_lines();
        ASSERT_EQ(lines.size(), 3);
        EXPECT_EQ(lines[0], "line1");
        EXPECT_EQ(lines[1], "line2");
        EXPECT_EQ(lines[2], "line3");
    }
}

TEST_F(TempFileTest, TempFileAppend) {
    path tf_path;
    {
        temp_file tf("append_test", ".txt");
        tf_path = tf.file().file_path();
        tf.file().write("first part", 10);
    }
    EXPECT_FALSE(tf_path.exists());
}

TEST_F(TempFileTest, TempFileSeekAndWrite) {
    path p;
    {
        temp_file tf("seek_write", ".txt", file_creation::CREATE_FORCE, temp_file::delete_policy::KEEP_ON_EXIT);
        tf.file().write("0123456789");
        ignore = tf.file().seek(5, file_pointer::BEGIN);
        tf.file().write("ABCDE");
        tf.file().flush();
        p = tf.file().file_path();
    }
    {
        file reader(p);
        EXPECT_EQ(reader.read(), "01234ABCDE");
    }
}

TEST_F(TempFileTest, MoveConstructorPreservesContent) {
    path tf_path;
    {
        temp_file tf1("move_content", ".txt");
        tf1.file().write("preserved content", 17);
        tf1.file().flush();
        tf_path = tf1.file().file_path();

        temp_file tf2(move(tf1));
        tf2.file().seek(0, file_pointer::BEGIN);
        EXPECT_EQ(tf2.file().read(), "preserved content");
    }
    EXPECT_FALSE(tf_path.exists());
}

TEST_F(TempFileTest, CleanupOnAlreadyDeletedFile) {
    path tf_path;
    {
        temp_file tf("early_delete", ".tmp");
        tf_path = tf.file().file_path();
    }
    {
        filesystem::remove(tf_path);
        EXPECT_FALSE(tf_path.exists());
    }
}

TEST_F(TempFileTest, CleanupOnAlreadyDeletedFileManual) {
    path tf_path;
    {
        temp_file tf("early_delete_manual", ".tmp", file_creation::CREATE_FORCE,
                     temp_file::delete_policy::MANUAL_DELETE);
        tf_path = tf.file().file_path();
        filesystem::remove(tf_path);
    }
}

TEST_F(TempFileTest, CleanupAllTempFilesAfterManualDelete) {
    path tf_path;
    {
        temp_file tf("pre_deleted", ".tmp");
        tf_path = tf.file().file_path();
        filesystem::remove(tf_path);
    }

    temp_file::cleanup_all_temp_files();
}

TEST_F(TempFileTest, TempFileInSubdirectory) {
    auto subdir = test_dir_ / path("subdir_for_temp");
    filesystem::create_directories(subdir);

    auto existing = subdir / path("nested_temp.txt");
    filesystem::create_and_write(existing, "nested");

    {
        temp_file tf(existing);
        EXPECT_TRUE(tf.file().is_opened());
        EXPECT_TRUE(existing.exists());
    }
    EXPECT_FALSE(existing.exists());
}

class FileDiffTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_filediff_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_path(const string& name) const { return test_dir_ / path(name); }

    void create_file(const path& p, const string& content) { filesystem::create_and_write(p, content); }

    path test_dir_;
};

TEST_F(FileDiffTest, CompareBinaryIdentical) {
    auto f1 = get_path("binary_a.bin");
    auto f2 = get_path("binary_b.bin");

    byte_t data[] = {0x00, 0x01, 0x02, 0xFF, 0xAA, 0x55};
    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write(data, sizeof(data));
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write(data, sizeof(data));
    }

    EXPECT_TRUE(file_diff::compare_binary(f1, f2));
    EXPECT_TRUE(file_diff::compare(f1, f2, true));
}

TEST_F(FileDiffTest, CompareBinaryDifferentContent) {
    auto f1 = get_path("diff_a.bin");
    auto f2 = get_path("diff_b.bin");

    byte_t data1[] = {0x00, 0x01, 0x02};
    byte_t data2[] = {0x00, 0x01, 0xFF};

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write(data1, sizeof(data1));
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write(data2, sizeof(data2));
    }

    EXPECT_FALSE(file_diff::compare_binary(f1, f2));
}

TEST_F(FileDiffTest, CompareBinaryDifferentSize) {
    auto f1 = get_path("size_a.bin");
    auto f2 = get_path("size_b.bin");

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write("short", 5);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write("longer", 6);
    }

    EXPECT_FALSE(file_diff::compare_binary(f1, f2));
}

TEST_F(FileDiffTest, CompareBinaryEmptyFiles) {
    auto f1 = get_path("empty_a.bin");
    auto f2 = get_path("empty_b.bin");

    create_file(f1, "");
    create_file(f2, "");

    EXPECT_TRUE(file_diff::compare_binary(f1, f2));
}

TEST_F(FileDiffTest, CompareBinaryNonExistentFile) {
    auto f1 = get_path("exists.bin");
    auto f2 = get_path("no_such.bin");

    create_file(f1, "data");

    EXPECT_FALSE(file_diff::compare_binary(f1, f2));
    EXPECT_FALSE(file_diff::compare_binary(f2, f1));
}

TEST_F(FileDiffTest, CompareBinaryLargeFile) {
    auto f1 = get_path("large_a.bin");
    auto f2 = get_path("large_b.bin");

    string large_content(20000, 'X');
    large_content[19999] = 'Y';

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write(large_content);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write(large_content);
    }

    EXPECT_TRUE(file_diff::compare_binary(f1, f2));

    string different_content(20000, 'X');
    different_content[10000] = 'Z';
    {
        file w3(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w3.write(different_content);
    }

    EXPECT_FALSE(file_diff::compare_binary(f1, f2));
}

TEST_F(FileDiffTest, CompareTextIdentical) {
    auto f1 = get_path("text_a.txt");
    auto f2 = get_path("text_b.txt");

    create_file(f1, "hello\nworld\n");
    create_file(f2, "hello\nworld\n");

    EXPECT_TRUE(file_diff::compare_text(f1, f2));
    EXPECT_TRUE(file_diff::compare(f1, f2, false));
}

TEST_F(FileDiffTest, CompareTextDifferent) {
    auto f1 = get_path("text_diff_a.txt");
    auto f2 = get_path("text_diff_b.txt");

    create_file(f1, "hello\nworld\n");
    create_file(f2, "hello\nthere\n");

    EXPECT_FALSE(file_diff::compare_text(f1, f2));
}

TEST_F(FileDiffTest, CompareTextDifferentLineCount) {
    auto f1 = get_path("text_lines_a.txt");
    auto f2 = get_path("text_lines_b.txt");

    create_file(f1, "line1\nline2\n");
    create_file(f2, "line1\nline2\nline3\n");

    EXPECT_FALSE(file_diff::compare_text(f1, f2));
}

TEST_F(FileDiffTest, CompareTextIgnoreCase) {
    auto f1 = get_path("case_a.txt");
    auto f2 = get_path("case_b.txt");

    create_file(f1, "Hello\nWorld\n");
    create_file(f2, "hello\nworld\n");

    EXPECT_FALSE(file_diff::compare_text(f1, f2, false, false));
    EXPECT_TRUE(file_diff::compare_text(f1, f2, true, false));
}

TEST_F(FileDiffTest, CompareTextIgnoreWhitespace) {
    auto f1 = get_path("ws_a.txt");
    auto f2 = get_path("ws_b.txt");

    create_file(f1, "hello  world\n  test  \n");
    create_file(f2, "hello world\ntest\n");

    EXPECT_FALSE(file_diff::compare_text(f1, f2, false, false));
    EXPECT_TRUE(file_diff::compare_text(f1, f2, false, true));
}

TEST_F(FileDiffTest, CompareTextIgnoreWhitespaceTabsAndSpaces) {
    auto f1 = get_path("ws_tabs_a.txt");
    auto f2 = get_path("ws_tabs_b.txt");

    create_file(f1, "hello\t\tworld\n");
    create_file(f2, "hello world\n");

    EXPECT_TRUE(file_diff::compare_text(f1, f2, false, true));
}

TEST_F(FileDiffTest, CompareTextIgnoreCaseAndWhitespace) {
    auto f1 = get_path("case_ws_a.txt");
    auto f2 = get_path("case_ws_b.txt");

    create_file(f1, "  HELLO  WORLD  \n");
    create_file(f2, "hello world\n");

    EXPECT_TRUE(file_diff::compare_text(f1, f2, true, true));
}

TEST_F(FileDiffTest, CompareTextIgnoreWhitespaceMultipleSpaces) {
    auto f1 = get_path("multi_ws_a.txt");
    auto f2 = get_path("multi_ws_b.txt");

    create_file(f1, "a   b    c\n");
    create_file(f2, "a b c\n");

    EXPECT_TRUE(file_diff::compare_text(f1, f2, false, true));
}

TEST_F(FileDiffTest, CompareTextCRLFvsLF) {
    auto f1 = get_path("crlf.txt");
    auto f2 = get_path("lf.txt");

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write("line1\r\nline2\r\n", 14);
    }
    create_file(f2, "line1\nline2\n");

    EXPECT_TRUE(file_diff::compare_text(f1, f2, false, false));
}

TEST_F(FileDiffTest, CompareTextEmptyFiles) {
    auto f1 = get_path("empty_text_a.txt");
    auto f2 = get_path("empty_text_b.txt");

    create_file(f1, "");
    create_file(f2, "");

    EXPECT_TRUE(file_diff::compare_text(f1, f2));
}

TEST_F(FileDiffTest, CompareTextOneEmpty) {
    auto f1 = get_path("empty_one_a.txt");
    auto f2 = get_path("empty_one_b.txt");

    create_file(f1, "");
    create_file(f2, "not empty\n");

    EXPECT_FALSE(file_diff::compare_text(f1, f2));
}

TEST_F(FileDiffTest, CompareTextNonExistentFile) {
    auto f1 = get_path("text_exists.txt");
    auto f2 = get_path("text_no_such.txt");

    create_file(f1, "data\n");

    EXPECT_FALSE(file_diff::compare_text(f1, f2));
    EXPECT_FALSE(file_diff::compare_text(f2, f1));
}

TEST_F(FileDiffTest, CompareDefaultBinary) {
    auto f1 = get_path("default_a.bin");
    auto f2 = get_path("default_b.bin");

    create_file(f1, "same");
    create_file(f2, "same");

    EXPECT_TRUE(file_diff::compare(f1, f2));
}

TEST_F(FileDiffTest, CompareDefaultText) {
    auto f1 = get_path("default_text_a.txt");
    auto f2 = get_path("default_text_b.txt");

    create_file(f1, "different");
    create_file(f2, "DIFFERENT");

    EXPECT_FALSE(file_diff::compare(f1, f2, false));
}

TEST_F(FileDiffTest, BinaryDiffIdentical) {
    auto f1 = get_path("bdiff_a.bin");
    auto f2 = get_path("bdiff_b.bin");

    byte_t data[] = {0x41, 0x42, 0x43};
    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write(data, sizeof(data));
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write(data, sizeof(data));
    }

    auto diffs = file_diff::binary_diff(f1, f2);
    EXPECT_TRUE(diffs.empty());
}

TEST_F(FileDiffTest, BinaryDiffDifferentContent) {
    auto f1 = get_path("bdiff_content_a.bin");
    auto f2 = get_path("bdiff_content_b.bin");

    byte_t data1[] = {0x00, 0x01, 0x02, 0x03};
    byte_t data2[] = {0x00, 0xFF, 0x02, 0xFE};

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write(data1, sizeof(data1));
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write(data2, sizeof(data2));
    }

    auto diffs = file_diff::binary_diff(f1, f2);
    ASSERT_EQ(diffs.size(), 2);

    EXPECT_EQ(diffs[0].offset, 1);
    EXPECT_EQ(diffs[0].byte1, 0x01);
    EXPECT_EQ(diffs[0].byte2, 0xFF);
    EXPECT_FALSE(diffs[0].is_size_diff);

    EXPECT_EQ(diffs[1].offset, 3);
    EXPECT_EQ(diffs[1].byte1, 0x03);
    EXPECT_EQ(diffs[1].byte2, 0xFE);
    EXPECT_FALSE(diffs[1].is_size_diff);
}

TEST_F(FileDiffTest, BinaryDiffSizeDifference) {
    auto f1 = get_path("bdiff_size_a.bin");
    auto f2 = get_path("bdiff_size_b.bin");

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write("abcdef", 6);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write("abcd", 4);
    }

    auto diffs = file_diff::binary_diff(f1, f2);
    ASSERT_GE(diffs.size(), 1);

    bool found_size_diff = false;
    for (const auto& d: diffs) {
        if (d.is_size_diff) {
            found_size_diff = true;
            EXPECT_EQ(d.offset, 4);
            EXPECT_EQ(d.size_diff, 2);
        }
    }
    EXPECT_TRUE(found_size_diff);
}

TEST_F(FileDiffTest, BinaryDiffSizeDifferenceLargerFirst) {
    auto f1 = get_path("bdiff_larger_a.bin");
    auto f2 = get_path("bdiff_larger_b.bin");

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write("xyz", 3);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write("xyz123", 6);
    }

    auto diffs = file_diff::binary_diff(f1, f2);
    ASSERT_GE(diffs.size(), 1);

    bool found_size_diff = false;
    for (const auto& d: diffs) {
        if (d.is_size_diff) {
            found_size_diff = true;
            EXPECT_EQ(d.size_diff, -3);
        }
    }
    EXPECT_TRUE(found_size_diff);
}

TEST_F(FileDiffTest, BinaryDiffMaxDiffs) {
    auto f1 = get_path("bdiff_max_a.bin");
    auto f2 = get_path("bdiff_max_b.bin");

    string data1(1000, 'A');
    string data2(1000, 'B');

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write(data1);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write(data2);
    }

    auto diffs = file_diff::binary_diff(f1, f2, 10);
    EXPECT_EQ(diffs.size(), 10);
}

TEST_F(FileDiffTest, BinaryDiffMaxDiffsZero) {
    auto f1 = get_path("bdiff_zero_a.bin");
    auto f2 = get_path("bdiff_zero_b.bin");

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write("AAA", 3);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write("BBB", 3);
    }

    auto diffs = file_diff::binary_diff(f1, f2, 0);
    EXPECT_TRUE(diffs.empty());
}

TEST_F(FileDiffTest, BinaryDiffEmptyFiles) {
    auto f1 = get_path("bdiff_empty_a.bin");
    auto f2 = get_path("bdiff_empty_b.bin");

    create_file(f1, "");
    create_file(f2, "");

    auto diffs = file_diff::binary_diff(f1, f2);
    EXPECT_TRUE(diffs.empty());
}

TEST_F(FileDiffTest, BinaryDiffOneEmpty) {
    auto f1 = get_path("bdiff_one_empty_a.bin");
    auto f2 = get_path("bdiff_one_empty_b.bin");

    create_file(f1, "");
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write("data", 4);
    }

    auto diffs = file_diff::binary_diff(f1, f2);
    ASSERT_GE(diffs.size(), 1);

    bool found_size_diff = false;
    for (const auto& d: diffs) {
        if (d.is_size_diff) {
            found_size_diff = true;
            EXPECT_EQ(d.offset, 0);
        }
    }
    EXPECT_TRUE(found_size_diff);
}

TEST_F(FileDiffTest, BinaryDiffNonExistentFile) {
    auto f1 = get_path("bdiff_exists.bin");
    auto f2 = get_path("bdiff_nosuch.bin");

    create_file(f1, "data");

    auto diffs = file_diff::binary_diff(f1, f2);
    EXPECT_TRUE(diffs.empty());

    diffs = file_diff::binary_diff(f2, f1);
    EXPECT_TRUE(diffs.empty());
}

TEST_F(FileDiffTest, BinaryDiffSizeTypeValue) {
    auto f1 = get_path("bdiff_sizetype_a.bin");
    auto f2 = get_path("bdiff_sizetype_b.bin");

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write("0123456789", 10);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write("01234ABCDE", 10);
    }

    auto diffs = file_diff::binary_diff(f1, f2);
    EXPECT_EQ(diffs.size(), 5);

    for (const auto& entry: diffs) {
        EXPECT_FALSE(entry.is_size_diff);
        EXPECT_GE(entry.offset, 5);
        EXPECT_LT(entry.offset, 10);
    }
}

TEST_F(FileDiffTest, BinaryDiffLargeFileLimitDiffs) {
    auto f1 = get_path("bdiff_large_a.bin");
    auto f2 = get_path("bdiff_large_b.bin");

    string data1(50000, 'A');
    string data2(50000, 'B');

    {
        file w1(f1, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w1.write(data1);
    }
    {
        file w2(f2, false, file_access::WRITE, file_shared::SHARE_READ, file_creation::OPEN_FORCE);
        w2.write(data2);
    }

    auto diffs = file_diff::binary_diff(f1, f2, 50);
    EXPECT_EQ(diffs.size(), 50);
}

class FileMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_mapper_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_path(const string& name) const { return test_dir_ / path(name); }

    void create_file(const path& p, const string& content) { filesystem::create_and_write(p, content); }

    path test_dir_;
};

TEST_F(FileMapperTest, Constructor) {
    auto p = get_path("ctor_test.bin");
    create_file(p, "constructor test data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_FALSE(mapper.is_mapped());
    EXPECT_EQ(mapper.data(), nullptr);
    EXPECT_EQ(mapper.size(), 0);
    EXPECT_EQ(mapper.offset(), 0);
}

TEST_F(FileMapperTest, MapReadOnly) {
    auto p = get_path("map_read.bin");
    create_file(p, "hello memory mapped file!");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 0, file_access::READ));
    EXPECT_TRUE(mapper.is_mapped());
    EXPECT_NE(mapper.data(), nullptr);
    EXPECT_GT(mapper.size(), 0);

    auto info = mapper.info();
    EXPECT_TRUE(info.is_mapped);
    EXPECT_EQ(info.access, file_access::READ);
    EXPECT_NE(info.address, nullptr);
}

TEST_F(FileMapperTest, MapReadWrite) {
    auto p = get_path("map_readwrite.bin");
    create_file(p, string(100, 'A'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 100, file_access::READ_WRITE));
    EXPECT_TRUE(mapper.is_mapped());
    EXPECT_EQ(mapper.size(), 100);

    auto* data = static_cast<char*>(mapper.data());
    EXPECT_EQ(data[0], 'A');
    EXPECT_EQ(data[50], 'A');
    EXPECT_EQ(data[99], 'A');
}

TEST_F(FileMapperTest, MapWithOffset) {
    auto p = get_path("map_offset.bin");
    create_file(p, "0123456789ABCDEF");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(5, 5, file_access::READ));
    EXPECT_TRUE(mapper.is_mapped());
    EXPECT_EQ(mapper.size(), 5);
    EXPECT_EQ(mapper.offset(), 5);

    auto* data = static_cast<char*>(mapper.data());
    EXPECT_EQ(data[0], '5');
    EXPECT_EQ(data[4], '9');
}

TEST_F(FileMapperTest, MapWithSize) {
    auto p = get_path("map_size.bin");
    create_file(p, string(200, 'X'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 50, file_access::READ));
    EXPECT_TRUE(mapper.is_mapped());
    EXPECT_EQ(mapper.size(), 50);

    auto* data = static_cast<char*>(mapper.data());
    EXPECT_EQ(data[0], 'X');
    EXPECT_EQ(data[49], 'X');
}

TEST_F(FileMapperTest, MapEntireFile) {
    auto p = get_path("map_entire.bin");
    string content(500, 'Y');
    create_file(p, content);

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 0, file_access::READ));
    EXPECT_TRUE(mapper.is_mapped());
    EXPECT_EQ(mapper.size(), 500);

    auto* data = static_cast<char*>(mapper.data());
    for (size_t i = 0; i < 500; ++i) {
        EXPECT_EQ(data[i], 'Y');
    }
}

TEST_F(FileMapperTest, MapSequentialHint) {
    auto p = get_path("map_seq.bin");
    create_file(p, string(100, 'S'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 0, file_access::READ, file_map_hint::SEQUENTIAL));
    EXPECT_TRUE(mapper.is_mapped());
}

TEST_F(FileMapperTest, MapRandomHint) {
    auto p = get_path("map_rand.bin");
    create_file(p, string(100, 'R'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 0, file_access::READ, file_map_hint::RANDOM));
    EXPECT_TRUE(mapper.is_mapped());
}

TEST_F(FileMapperTest, MapNormalHint) {
    auto p = get_path("map_norm.bin");
    create_file(p, string(100, 'N'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 0, file_access::READ, file_map_hint::NORMAL));
    EXPECT_TRUE(mapper.is_mapped());
}

TEST_F(FileMapperTest, Unmap) {
    auto p = get_path("unmap_test.bin");
    create_file(p, "unmap test data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map());
    EXPECT_TRUE(mapper.is_mapped());

    mapper.unmap();
    EXPECT_FALSE(mapper.is_mapped());
    EXPECT_EQ(mapper.data(), nullptr);
    EXPECT_EQ(mapper.size(), 0);
    EXPECT_EQ(mapper.offset(), 0);
}

TEST_F(FileMapperTest, UnmapNotMapped) {
    auto p = get_path("unmap_not.bin");
    create_file(p, "data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_NO_THROW(mapper.unmap());
    EXPECT_FALSE(mapper.is_mapped());
}

TEST_F(FileMapperTest, Remap) {
    auto p = get_path("remap_test.bin");
    create_file(p, "0123456789ABCDEFGHIJ");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 5, file_access::READ));
    EXPECT_EQ(mapper.size(), 5);
    auto* data1 = static_cast<char*>(mapper.data());
    EXPECT_EQ(data1[0], '0');
    EXPECT_EQ(data1[4], '4');

    EXPECT_TRUE(mapper.remap(10, 5));
    EXPECT_EQ(mapper.size(), 5);
    auto* data2 = static_cast<char*>(mapper.data());
    EXPECT_EQ(data2[0], 'A');
    EXPECT_EQ(data2[4], 'E');
}

TEST_F(FileMapperTest, RemapKeepsAccess) {
    auto p = get_path("remap_access.bin");
    create_file(p, string(50, 'Z'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 10, file_access::READ));
    EXPECT_EQ(mapper.access(), file_access::READ);

    EXPECT_TRUE(mapper.remap(20, 10));
    EXPECT_EQ(mapper.access(), file_access::READ);
}

TEST_F(FileMapperTest, FlushReadWrite) {
    auto p = get_path("flush_test.bin");
    create_file(p, string(100, 0));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 100, file_access::READ_WRITE));

    auto* data = static_cast<char*>(mapper.data());
    for (size_t i = 0; i < 100; ++i) {
        data[i] = static_cast<char>('A' + (i % 26));
    }

    EXPECT_TRUE(mapper.flush(false));
}

TEST_F(FileMapperTest, FlushAsync) {
    auto p = get_path("flush_async.bin");
    create_file(p, string(100, 0));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 100, file_access::READ_WRITE));

    auto* data = static_cast<char*>(mapper.data());
    data[0] = 'X';
    data[99] = 'Z';

    EXPECT_TRUE(mapper.flush(true));
}

TEST_F(FileMapperTest, FlushNotMapped) {
    auto p = get_path("flush_not_mapped.bin");
    create_file(p, "data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_FALSE(mapper.flush(false));
    EXPECT_FALSE(mapper.flush(true));
}

TEST_F(FileMapperTest, FlushReadOnly) {
    auto p = get_path("flush_readonly.bin");
    create_file(p, "readonly data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 0, file_access::READ));
    EXPECT_TRUE(mapper.flush(false));
}

TEST_F(FileMapperTest, LockPages) {
    auto p = get_path("lock_pages.bin");
    create_file(p, string(100, 'L'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 100, file_access::READ));
    EXPECT_TRUE(mapper.lock_pages(true));
}

TEST_F(FileMapperTest, UnlockPages) {
    auto p = get_path("unlock_pages.bin");
    create_file(p, string(100, 'U'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 100, file_access::READ));

    bool locked = mapper.lock_pages(true);
    if (locked) {
        EXPECT_TRUE(mapper.lock_pages(false));
    }
}

TEST_F(FileMapperTest, LockPagesNotMapped) {
    auto p = get_path("lock_not_mapped.bin");
    create_file(p, "data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_FALSE(mapper.lock_pages(true));
    EXPECT_FALSE(mapper.lock_pages(false));
}

TEST_F(FileMapperTest, Data) {
    auto p = get_path("data_test.bin");
    create_file(p, "pointer access");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map());
    EXPECT_NE(mapper.data(), nullptr);

    auto* data = static_cast<const char*>(mapper.data());
    EXPECT_EQ(data[0], 'p');
    EXPECT_EQ(data[string_length("pointer access") - 1], 's');
}

TEST_F(FileMapperTest, Size) {
    auto p = get_path("size_test.bin");
    create_file(p, string(42, 'S'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 42, file_access::READ));
    EXPECT_EQ(mapper.size(), 42);
}

TEST_F(FileMapperTest, Offset) {
    auto p = get_path("offset_test.bin");
    create_file(p, "0123456789");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(3, 4, file_access::READ));
    EXPECT_EQ(mapper.offset(), 3);
}

TEST_F(FileMapperTest, Access) {
    auto p = get_path("access_test.bin");
    create_file(p, "access mode test");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 0, file_access::READ));
    EXPECT_EQ(mapper.access(), file_access::READ);
}

TEST_F(FileMapperTest, IsMapped) {
    auto p = get_path("is_mapped_test.bin");
    create_file(p, "mapped state");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_FALSE(mapper.is_mapped());
    EXPECT_TRUE(mapper.map());
    EXPECT_TRUE(mapper.is_mapped());
    mapper.unmap();
    EXPECT_FALSE(mapper.is_mapped());
}

TEST_F(FileMapperTest, Info) {
    auto p = get_path("info_test.bin");
    create_file(p, string(64, 'I'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    auto info1 = mapper.info();
    EXPECT_FALSE(info1.is_mapped);
    EXPECT_EQ(info1.address, nullptr);
    EXPECT_EQ(info1.size, 0);
    EXPECT_EQ(info1.offset, 0);

    EXPECT_TRUE(mapper.map(8, 32, file_access::READ));
    auto info2 = mapper.info();
    EXPECT_TRUE(info2.is_mapped);
    EXPECT_NE(info2.address, nullptr);
    EXPECT_EQ(info2.size, 32);
    EXPECT_EQ(info2.offset, 8);
    EXPECT_EQ(info2.access, file_access::READ);
}

TEST_F(FileMapperTest, MoveConstructor) {
    auto p = get_path("move_ctor.bin");
    create_file(p, "move constructor test data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper1(f.native_handle());
    EXPECT_TRUE(mapper1.map());

    void* original_ptr = mapper1.data();
    auto original_size = mapper1.size();

    file_mapper mapper2(move(mapper1));

    EXPECT_TRUE(mapper2.is_mapped());
    EXPECT_EQ(mapper2.data(), original_ptr);
    EXPECT_EQ(mapper2.size(), original_size);

    EXPECT_FALSE(mapper1.is_mapped());
    EXPECT_EQ(mapper1.data(), nullptr);
    EXPECT_EQ(mapper1.size(), 0);
}

TEST_F(FileMapperTest, MoveAssignment) {
    auto p1 = get_path("move_assign_a.bin");
    auto p2 = get_path("move_assign_b.bin");
    create_file(p1, "first mapped data here");
    create_file(p2, "second mapped data here");

    file f1(p1, false, file_access::READ, file_shared::SHARE_READ);
    file f2(p2, false, file_access::READ, file_shared::SHARE_READ);

    file_mapper mapper1(f1.native_handle());
    file_mapper mapper2(f2.native_handle());

    EXPECT_TRUE(mapper1.map(0, 5, file_access::READ));
    EXPECT_TRUE(mapper2.map(0, 6, file_access::READ));

    auto* data2 = static_cast<char*>(mapper2.data());

    mapper1 = move(mapper2);

    EXPECT_TRUE(mapper1.is_mapped());
    EXPECT_EQ(mapper1.data(), data2);
    EXPECT_FALSE(mapper2.is_mapped());
}

TEST_F(FileMapperTest, MoveAssignmentSelf) {
    auto p = get_path("move_self.bin");
    create_file(p, "self assignment");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());
    EXPECT_TRUE(mapper.map());

    void* ptr = mapper.data();
    mapper = move(mapper);

    EXPECT_TRUE(mapper.is_mapped());
    EXPECT_EQ(mapper.data(), ptr);
}

TEST_F(FileMapperTest, ReadAfterWriteThroughMap) {
    auto p = get_path("write_through.bin");
    create_file(p, string(200, '\0'));

    {
        file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
        file_mapper mapper(f.native_handle());
        EXPECT_TRUE(mapper.map(0, 200, file_access::READ_WRITE));

        auto* data = static_cast<char*>(mapper.data());
        for (size_t i = 0; i < 200; ++i) {
            data[i] = static_cast<char>((i % 10) + '0');
        }
        EXPECT_TRUE(mapper.flush(false));
    }

    file reader(p);
    auto content = reader.read();
    EXPECT_EQ(content.size(), 200);
    for (size_t i = 0; i < 200; ++i) {
        EXPECT_EQ(content[i], static_cast<char>((i % 10) + '0'));
    }
}

TEST_F(FileMapperTest, MapZeroSizeFileZeroSize) {
    auto p = get_path("zero_size.bin");
    create_file(p, "");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    bool result = mapper.map(0, 0, file_access::READ);
    EXPECT_FALSE(result);
}

TEST_F(FileMapperTest, MapLargeOffset) {
    auto p = get_path("large_offset.bin");
    create_file(p, string(5000, 'L'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(4096, 100, file_access::READ));
    EXPECT_TRUE(mapper.is_mapped());
    EXPECT_EQ(mapper.size(), 100);

    auto* data = static_cast<char*>(mapper.data());
    EXPECT_EQ(data[0], 'L');
    EXPECT_EQ(data[99], 'L');
}

TEST_F(FileMapperTest, MapUnalignedOffset) {
    auto p = get_path("unaligned.bin");
    create_file(p, string(200, 'U'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(17, 50, file_access::READ));
    EXPECT_TRUE(mapper.is_mapped());

    auto* data = static_cast<char*>(mapper.data());
    EXPECT_EQ(data[0], 'U');
    EXPECT_EQ(data[49], 'U');
}

TEST_F(FileMapperTest, DoubleMap) {
    auto p = get_path("double_map.bin");
    create_file(p, "0123456789ABCDEF");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_mapper mapper(f.native_handle());

    EXPECT_TRUE(mapper.map(0, 5, file_access::READ));
    auto* data1 = static_cast<char*>(mapper.data());
    EXPECT_EQ(data1[0], '0');

    EXPECT_TRUE(mapper.map(8, 5, file_access::READ));
    auto* data2 = static_cast<char*>(mapper.data());
    EXPECT_EQ(data2[0], '8');

    EXPECT_NE(data1, data2);
}

class FileLockerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_locker_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_path(const string& name) const { return test_dir_ / path(name); }

    path test_dir_;
};

TEST_F(FileLockerTest, Constructor) {
    auto p = get_path("ctor_test.txt");
    filesystem::create_and_write(p, "locker constructor test");

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    SUCCEED();
}

TEST_F(FileLockerTest, LockRegion) {
    auto p = get_path("lock_region.txt");
    filesystem::create_and_write(p, string(200, 'X'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(0, 100, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(0, 100));
}

TEST_F(FileLockerTest, LockWholeFile) {
    auto p = get_path("lock_whole.txt");
    filesystem::create_and_write(p, "lock entire file test data");

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock_whole(file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock_whole());
}

TEST_F(FileLockerTest, LockZeroLength) {
    auto p = get_path("lock_zero.txt");
    filesystem::create_and_write(p, string(100, 'Z'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(50, 0, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(50, 0));
}

TEST_F(FileLockerTest, LockSharedMode) {
    auto p = get_path("lock_shared.txt");
    filesystem::create_and_write(p, string(100, 'S'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(0, 50, file_lock::SHARED));
    EXPECT_TRUE(locker.unlock(0, 50));
}

TEST_F(FileLockerTest, LockExclusiveMode) {
    auto p = get_path("lock_exclusive.txt");
    filesystem::create_and_write(p, string(100, 'E'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(0, 50, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(0, 50));
}

TEST_F(FileLockerTest, Unlock) {
    auto p = get_path("unlock_test.txt");
    filesystem::create_and_write(p, string(100, 'U'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(10, 80, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(10, 80));
}

TEST_F(FileLockerTest, LockMultipleRegions) {
    auto p = get_path("lock_multi.txt");
    filesystem::create_and_write(p, string(200, 'M'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(0, 50, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.lock(100, 50, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(0, 50));
    EXPECT_TRUE(locker.unlock(100, 50));
}

TEST_F(FileLockerTest, TryLockSuccess) {
    auto p = get_path("try_lock_ok.txt");
    filesystem::create_and_write(p, string(100, 'T'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.try_lock(0, 100, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(0, 100));
}

TEST_F(FileLockerTest, TryLockShared) {
    auto p = get_path("try_lock_shared.txt");
    filesystem::create_and_write(p, string(100, 'T'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.try_lock(0, 100, file_lock::SHARED));
    EXPECT_TRUE(locker.unlock(0, 100));
}

TEST_F(FileLockerTest, IsLockedFalse) {
    auto p = get_path("is_locked_false.txt");
    filesystem::create_and_write(p, string(100, 'I'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    file_lock lock_out = file_lock::EXCLUSIVE;
    EXPECT_FALSE(locker.is_locked(0, 100, &lock_out));
}

TEST_F(FileLockerTest, IsLockedTrueExclusive) {
    auto p = get_path("is_locked_true.txt");
    filesystem::create_and_write(p, string(100, 'I'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(0, 100, file_lock::EXCLUSIVE));

#ifdef NEFORCE_PLATFORM_WINDOWS
    bool is_locked = locker.is_locked(0, 100, nullptr);
    EXPECT_TRUE(locker.unlock(0, 100));
    EXPECT_TRUE(is_locked);
#else
    EXPECT_TRUE(locker.unlock(0, 100));
#endif
}

TEST_F(FileLockerTest, IsLockedNullOutput) {
    auto p = get_path("is_locked_null.txt");
    filesystem::create_and_write(p, string(100, 'N'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_FALSE(locker.is_locked(0, 100, nullptr));

    EXPECT_TRUE(locker.lock(0, 100, file_lock::EXCLUSIVE));
#ifdef NEFORCE_PLATFORM_WINDOWS
    bool locked = locker.is_locked(0, 100, nullptr);
    EXPECT_TRUE(locker.unlock(0, 100));
    EXPECT_TRUE(locked);
#else
    EXPECT_TRUE(locker.unlock(0, 100));
#endif
}

TEST_F(FileLockerTest, LockNegativeOffset) {
    auto p = get_path("lock_neg_offset.txt");
    filesystem::create_and_write(p, "data");

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_FALSE(locker.lock(-1, 10, file_lock::EXCLUSIVE));
}

TEST_F(FileLockerTest, LockNegativeLength) {
    auto p = get_path("lock_neg_length.txt");
    filesystem::create_and_write(p, "data");

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_FALSE(locker.lock(0, -1, file_lock::EXCLUSIVE));
}

TEST_F(FileLockerTest, UnlockNegativeOffset) {
    auto p = get_path("unlock_neg.txt");
    filesystem::create_and_write(p, "data");

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_FALSE(locker.unlock(-1, 10));
}

TEST_F(FileLockerTest, UnlockNegativeLength) {
    auto p = get_path("unlock_neg_len.txt");
    filesystem::create_and_write(p, "data");

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_FALSE(locker.unlock(0, -1));
}

TEST_F(FileLockerTest, LockThenRelockSameRegion) {
    auto p = get_path("relock_same.txt");
    filesystem::create_and_write(p, string(100, 'R'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(0, 50, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(0, 50));
    EXPECT_TRUE(locker.lock(0, 50, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(0, 50));
}

TEST_F(FileLockerTest, LockWholeFileShortcut) {
    auto p = get_path("lock_whole_shortcut.txt");
    filesystem::create_and_write(p, string(50, 'W'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock_whole(file_lock::SHARED));
    EXPECT_TRUE(locker.unlock_whole());
}

TEST_F(FileLockerTest, UnlockWholeFileShortcut) {
    auto p = get_path("unlock_whole_shortcut.txt");
    filesystem::create_and_write(p, string(50, 'U'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    EXPECT_TRUE(locker.lock(0, 50, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock_whole());
}

TEST_F(FileLockerTest, TryLockContention) {
    auto p = get_path("try_lock_contention.txt");
    filesystem::create_and_write(p, string(100, 'C'));

    file f1(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker1(f1.native_handle());

    EXPECT_TRUE(locker1.lock(0, 100, file_lock::EXCLUSIVE));

    file f2(p, false, file_access::READ, file_shared::SHARE_READ);
    file_locker locker2(f2.native_handle());

    EXPECT_FALSE(locker2.try_lock(0, 100, file_lock::EXCLUSIVE));

    EXPECT_TRUE(locker1.unlock(0, 100));
}

class FileLockGuardTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_lockguard_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_path(const string& name) const { return test_dir_ / path(name); }

    path test_dir_;
};

TEST_F(FileLockGuardTest, ConstructorLocks) {
    auto p = get_path("guard_ctor.txt");
    filesystem::create_and_write(p, string(100, 'G'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    {
        file_lock_guard guard(locker, 0, 100, file_lock::EXCLUSIVE);
        EXPECT_TRUE(guard.is_locked());
    }
}

TEST_F(FileLockGuardTest, DestructorUnlocks) {
    auto p = get_path("guard_dtor.txt");
    filesystem::create_and_write(p, string(100, 'D'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    {
        file_lock_guard guard(locker, 0, 100, file_lock::EXCLUSIVE);
        EXPECT_TRUE(guard.is_locked());
    }

    EXPECT_TRUE(locker.try_lock(0, 100, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(0, 100));
}

TEST_F(FileLockGuardTest, ManualUnlock) {
    auto p = get_path("guard_manual_unlock.txt");
    filesystem::create_and_write(p, string(100, 'M'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    file_lock_guard guard(locker, 0, 100, file_lock::EXCLUSIVE);
    EXPECT_TRUE(guard.is_locked());

    EXPECT_TRUE(guard.unlock());
    EXPECT_FALSE(guard.is_locked());
}

TEST_F(FileLockGuardTest, UnlockWhenNotLocked) {
    auto p = get_path("guard_unlock_not_locked.txt");
    filesystem::create_and_write(p, string(100, 'N'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    file_lock_guard guard(locker, 0, 100, file_lock::EXCLUSIVE);
    EXPECT_TRUE(guard.is_locked());

    EXPECT_TRUE(guard.unlock());
    EXPECT_FALSE(guard.unlock());
}

TEST_F(FileLockGuardTest, DoubleDestructorSafe) {
    auto p = get_path("guard_double_dtor.txt");
    filesystem::create_and_write(p, string(100, 'S'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    {
        file_lock_guard guard(locker, 0, 100, file_lock::EXCLUSIVE);
        EXPECT_TRUE(guard.is_locked());
        guard.unlock();
    }

    SUCCEED();
}

TEST_F(FileLockGuardTest, SharedLock) {
    auto p = get_path("guard_shared.txt");
    filesystem::create_and_write(p, string(100, 'S'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    {
        file_lock_guard guard(locker, 0, 100, file_lock::SHARED);
        EXPECT_TRUE(guard.is_locked());
    }
}

TEST_F(FileLockGuardTest, IsLocked) {
    auto p = get_path("guard_is_locked.txt");
    filesystem::create_and_write(p, string(100, 'I'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    file_lock_guard guard(locker, 0, 100, file_lock::EXCLUSIVE);
    EXPECT_TRUE(guard.is_locked());

    guard.unlock();
    EXPECT_FALSE(guard.is_locked());
}

TEST_F(FileLockGuardTest, PartialFileLock) {
    auto p = get_path("guard_partial.txt");
    filesystem::create_and_write(p, string(200, 'P'));

    file f(p, false, file_access::READ_WRITE, file_shared::SHARE_READ);
    file_locker locker(f.native_handle());

    {
        file_lock_guard guard(locker, 50, 100, file_lock::EXCLUSIVE);
        EXPECT_TRUE(guard.is_locked());
    }

    EXPECT_TRUE(locker.try_lock(50, 100, file_lock::EXCLUSIVE));
    EXPECT_TRUE(locker.unlock(50, 100));
}

class FileAsyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_async_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_path(const string& name) const { return test_dir_ / path(name); }

    path test_dir_;
};

TEST_F(FileAsyncTest, Constructor) {
    auto p = get_path("ctor_test.bin");
    filesystem::create_and_write(p, "async constructor test");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    SUCCEED();
}

TEST_F(FileAsyncTest, ReadImmediate) {
    auto p = get_path("read_immediate.bin");
    filesystem::create_and_write(p, "async read immediate test data here");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 10);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, 10);
    EXPECT_EQ(buffer.substr(0, 10), "async read");
}

TEST_F(FileAsyncTest, ReadWithOffset) {
    auto p = get_path("read_offset.bin");
    filesystem::create_and_write(p, "0123456789ABCDEFGHIJ");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 10, 10);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, 10);
    EXPECT_EQ(buffer.substr(0, 10), "ABCDEFGHIJ");
}

TEST_F(FileAsyncTest, ReadZeroSize) {
    auto p = get_path("read_zero.bin");
    filesystem::create_and_write(p, "some data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer = "original";
    auto result = async.read(buffer, 0);

    EXPECT_TRUE(result.completed);
    EXPECT_TRUE(buffer.empty());
}

TEST_F(FileAsyncTest, ReadEntireFile) {
    auto p = get_path("read_entire.bin");
    string content(1000, 'X');
    content[0] = 'S';
    content[999] = 'E';
    filesystem::create_and_write(p, content);

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 1000);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, 1000);
    EXPECT_EQ(buffer[0], 'S');
    EXPECT_EQ(buffer[999], 'E');
}

TEST_F(FileAsyncTest, ReadCurrentFilePointer) {
    auto p = get_path("read_current_ptr.bin");
    filesystem::create_and_write(p, "ABCDEFGHIJKLMNOP");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    f.seek(5, file_pointer::BEGIN);

    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 5, -1);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(buffer.substr(0, 5), "FGHIJ");
}

TEST_F(FileAsyncTest, WriteImmediate) {
    auto p = get_path("write_immediate.bin");
    filesystem::create_and_write(p, "");

    file f(p, false, file_access::WRITE, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    auto result = async.write(string("async write test data"), 21);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, 21);
    f.close();

    file reader(p);
    EXPECT_EQ(reader.read(21), "async write test data");
}

TEST_F(FileAsyncTest, WriteWithOffset) {
    auto p = get_path("write_offset.bin");
    filesystem::create_and_write(p, string(50, 'X'));

    file f(p, false, file_access::WRITE, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    auto result = async.write(string("HELLO"), 5, 10);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, 5);
    f.close();

    file reader(p);
    auto content = reader.read();
    EXPECT_EQ(content.substr(0, 10), string(10, 'X'));
    EXPECT_EQ(content.substr(10, 5), "HELLO");
    EXPECT_EQ(content.substr(15), string(35, 'X'));
}

TEST_F(FileAsyncTest, WriteMaxSize) {
    auto p = get_path("write_max_size.bin");
    filesystem::create_and_write(p, "");

    file f(p, false, file_access::WRITE, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string data = "write all of this string";
    auto result = async.write(data, numeric_traits<file_async::size_type>::max());

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, data.size());
    f.close();

    file reader(p);
    EXPECT_EQ(reader.read(data.size()), data);
}

TEST_F(FileAsyncTest, WaitWithTimeout) {
    auto p = get_path("wait_timeout.bin");
    filesystem::create_and_write(p, string(10000, 'W'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 10000);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result, 5000));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, 10000);
}

TEST_F(FileAsyncTest, WaitAlreadyCompleted) {
    auto p = get_path("wait_completed.bin");
    filesystem::create_and_write(p, "done");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 4);

    if (!result.completed) {
        async.wait(result);
    }

    EXPECT_TRUE(result.completed);
    EXPECT_TRUE(async.wait(result));
}

TEST_F(FileAsyncTest, WaitInfiniteTimeout) {
    auto p = get_path("wait_infinite.bin");
    filesystem::create_and_write(p, string(500, 'I'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 500);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, 500);
}

TEST_F(FileAsyncTest, CancelRead) {
    auto p = get_path("cancel_read.bin");
    filesystem::create_and_write(p, string(50000, 'C'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 50000);

    if (!result.completed) {
        async.cancel(result);
    }

    EXPECT_TRUE(result.completed);
}

TEST_F(FileAsyncTest, CancelWrite) {
    auto p = get_path("cancel_write.bin");
    filesystem::create_and_write(p, "");

    file f(p, false, file_access::WRITE, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    auto result = async.write(string(50000, 'C'), 50000);

    if (!result.completed) {
        async.cancel(result);
    }

    EXPECT_TRUE(result.completed);
}

TEST_F(FileAsyncTest, CancelAlreadyCompleted) {
    auto p = get_path("cancel_completed.bin");
    filesystem::create_and_write(p, "data");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 4);

    if (!result.completed) {
        async.wait(result);
    }

    EXPECT_TRUE(result.completed);
    EXPECT_NO_THROW(async.cancel(result));
}

TEST_F(FileAsyncTest, MultipleReads) {
    auto p = get_path("multi_read.bin");
    filesystem::create_and_write(p, string(3000, 'M'));

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buf1, buf2, buf3;
    auto r1 = async.read(buf1, 1000, 0);
    auto r2 = async.read(buf2, 1000, 1000);
    auto r3 = async.read(buf3, 1000, 2000);

    if (!r1.completed) {
        async.wait(r1);
    }
    if (!r2.completed) {
        async.wait(r2);
    }
    if (!r3.completed) {
        async.wait(r3);
    }

    EXPECT_TRUE(r1.completed);
    EXPECT_TRUE(r2.completed);
    EXPECT_TRUE(r3.completed);
    EXPECT_EQ(r1.bytes_transferred, 1000);
    EXPECT_EQ(r2.bytes_transferred, 1000);
    EXPECT_EQ(r3.bytes_transferred, 1000);
}

TEST_F(FileAsyncTest, MultipleWrites) {
    auto p = get_path("multi_write.bin");
    filesystem::create_and_write(p, "");

    file f(p, false, file_access::WRITE, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    auto w1 = async.write(string(100, 'A'), 100, 0);
    auto w2 = async.write(string(100, 'B'), 100, 100);
    auto w3 = async.write(string(100, 'C'), 100, 200);

    if (!w1.completed) {
        async.wait(w1);
    }
    if (!w2.completed) {
        async.wait(w2);
    }
    if (!w3.completed) {
        async.wait(w3);
    }

    EXPECT_TRUE(w1.completed);
    EXPECT_TRUE(w2.completed);
    EXPECT_TRUE(w3.completed);
    f.close();

    file reader(p);
    auto content = reader.read();
    EXPECT_EQ(content.substr(0, 100), string(100, 'A'));
    EXPECT_EQ(content.substr(100, 100), string(100, 'B'));
    EXPECT_EQ(content.substr(200, 100), string(100, 'C'));
}

TEST_F(FileAsyncTest, ReadLargeBuffer) {
    auto p = get_path("read_large.bin");
    size_t large_size = 100000;
    string content(large_size, 'L');
    content[0] = 'H';
    content[large_size - 1] = 'T';
    filesystem::create_and_write(p, content);

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, large_size);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, large_size);
    EXPECT_EQ(buffer[0], 'H');
    EXPECT_EQ(buffer[large_size - 1], 'T');
}

TEST_F(FileAsyncTest, WriteLargeData) {
    auto p = get_path("write_large.bin");
    filesystem::create_and_write(p, "");

    file f(p, false, file_access::WRITE, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    size_t large_size = 50000;
    string data(large_size, 'Z');
    data[0] = 'F';
    data[large_size - 1] = 'L';

    auto result = async.write(data, large_size);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(result.bytes_transferred, large_size);
    f.close();

    file reader(p);
    auto content = reader.read();
    EXPECT_EQ(content[0], 'F');
    EXPECT_EQ(content[large_size - 1], 'L');
}

TEST_F(FileAsyncTest, MoveConstructor) {
    auto p = get_path("move_ctor_async.bin");
    filesystem::create_and_write(p, "move constructor async test");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async1(f.native_handle());

    string buffer;
    auto result = async1.read(buffer, 11);

    file_async async2(move(async1));

    if (!result.completed) {
        EXPECT_TRUE(async2.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(buffer.substr(0, 11), "move constr");
}

TEST_F(FileAsyncTest, MoveAssignment) {
    auto p1 = get_path("move_assign_a.bin");
    auto p2 = get_path("move_assign_b.bin");
    filesystem::create_and_write(p1, "first async file data");
    filesystem::create_and_write(p2, "second async file data");

    file f1(p1, false, file_access::READ, file_shared::SHARE_READ);
    file f2(p2, false, file_access::READ, file_shared::SHARE_READ);

    file_async async1(f1.native_handle());
    file_async async2(f2.native_handle());

    string buffer;
    auto result = async2.read(buffer, 6);

    async1 = move(async2);

    if (!result.completed) {
        EXPECT_TRUE(async1.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(buffer.substr(0, 6), "second");
}

TEST_F(FileAsyncTest, MoveAssignmentSelf) {
    auto p = get_path("move_self_async.bin");
    filesystem::create_and_write(p, "self assignment async");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    auto result = async.read(buffer, 4);

    async = move(async);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_EQ(buffer.substr(0, 4), "self");
}

TEST_F(FileAsyncTest, DestructorCancelsOperations) {
    auto p = get_path("dtor_cancel.bin");
    filesystem::create_and_write(p, string(20000, 'D'));

    {
        file f(p, false, file_access::READ, file_shared::SHARE_READ);
        file_async async(f.native_handle());

        string buffer;
        auto result = async.read(buffer, 20000);
    }

    SUCCEED();
}

TEST_F(FileAsyncTest, ReadBufferPreallocation) {
    auto p = get_path("read_prealloc.bin");
    filesystem::create_and_write(p, "preallocation test data here");

    file f(p, false, file_access::READ, file_shared::SHARE_READ);
    file_async async(f.native_handle());

    string buffer;
    buffer.reserve(50);

    auto result = async.read(buffer, 21);

    if (!result.completed) {
        EXPECT_TRUE(async.wait(result));
    }

    EXPECT_TRUE(result.completed);
    EXPECT_GE(buffer.size(), 21);
    EXPECT_EQ(buffer.substr(0, 21), "preallocation test da");
}

#if 0

class FileWatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = path::temp_directory_path() / path("neforce_watcher_test");
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
        filesystem::create_directories(test_dir_);
    }

    void TearDown() override {
        if (test_dir_.exists()) {
            filesystem::remove_all(test_dir_);
        }
    }

    path get_path(const string& name) const {
        return test_dir_ / path(name);
    }

    static void wait_ms(uint32_t ms) {
        this_thread::sleep_for(milliseconds(ms));
    }

    path test_dir_;
};

TEST_F(FileWatcherTest, Constructor) {
    EXPECT_NO_THROW(file_watcher watcher(test_dir_, false));
}

TEST_F(FileWatcherTest, ConstructorRecursive) {
    EXPECT_NO_THROW(file_watcher watcher(test_dir_, true));
}

TEST_F(FileWatcherTest, ConstructorNonExistentPath) {
    path non_existent = test_dir_ / path("does_not_exist");
    EXPECT_THROW(file_watcher(non_existent, false), system_exception);
}

TEST_F(FileWatcherTest, ConstructorFileInsteadOfDirectory) {
    auto p = get_path("file_not_dir.txt");
    filesystem::create_and_write(p, "data");

    EXPECT_THROW(file_watcher(p, false), system_exception);
}

TEST_F(FileWatcherTest, StartStop) {
    file_watcher watcher(test_dir_, false);

    int call_count = 0;
    auto cb = [&](const path&, file_watch_event) {
        ++call_count;
    };

    EXPECT_TRUE(watcher.start(cb));
    EXPECT_TRUE(watcher.is_watching());
    watcher.stop();
    EXPECT_FALSE(watcher.is_watching());
}

TEST_F(FileWatcherTest, StartAlreadyWatching) {
    file_watcher watcher(test_dir_, false);

    auto cb = [](const path&, file_watch_event) {};

    EXPECT_TRUE(watcher.start(cb));
    EXPECT_FALSE(watcher.start(cb));
    watcher.stop();
}

TEST_F(FileWatcherTest, FileCreatedEvent) {
    file_watcher watcher(test_dir_, false);

    path created_path;
    file_watch_event captured_event = file_watch_event::ACCESSED;
    mutex mtx;
    condition_variable cv;
    bool event_received = false;

    auto cb = [&](const path& p, file_watch_event e) {
        lock<mutex> lk(mtx);
        created_path = p;
        captured_event = e;
        event_received = true;
        cv.notify_one();
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::CREATED));

    auto new_file = get_path("created.txt");
    wait_ms(100);

    filesystem::create_and_write(new_file, "new content");

    {
        unique_lock<mutex> lk(mtx);
        EXPECT_TRUE(cv.wait_for(lk, milliseconds(5000), [&] { return event_received; }));
    }

    watcher.stop();

    if (event_received) {
        EXPECT_EQ(captured_event, file_watch_event::CREATED);
        EXPECT_EQ(created_path.filename(), "created.txt");
    }
}

TEST_F(FileWatcherTest, FileDeletedEvent) {
    auto delete_file = get_path("to_delete.txt");
    filesystem::create_and_write(delete_file, "delete me");

    file_watcher watcher(test_dir_, false);

    path deleted_path;
    file_watch_event captured_event = file_watch_event::ACCESSED;
    mutex mtx;
    condition_variable cv;
    bool event_received = false;

    auto cb = [&](const path& p, file_watch_event e) {
        lock<mutex> lk(mtx);
        deleted_path = p;
        captured_event = e;
        event_received = true;
        cv.notify_one();
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::DELETED));

    wait_ms(100);
    filesystem::remove(delete_file);

    {
        unique_lock<mutex> lk(mtx);
        EXPECT_TRUE(cv.wait_for(lk, milliseconds(5000), [&] { return event_received; }));
    }

    watcher.stop();

    if (event_received) {
        EXPECT_EQ(captured_event, file_watch_event::DELETED);
        EXPECT_EQ(deleted_path.filename(), "to_delete.txt");
    }
}

TEST_F(FileWatcherTest, FileModifiedEvent) {
    auto modify_file = get_path("to_modify.txt");
    filesystem::create_and_write(modify_file, "original");

    file_watcher watcher(test_dir_, false);

    path modified_path;
    file_watch_event captured_event = file_watch_event::ACCESSED;
    mutex mtx;
    condition_variable cv;
    bool event_received = false;

    auto cb = [&](const path& p, file_watch_event e) {
        lock<mutex> lk(mtx);
        modified_path = p;
        captured_event = e;
        event_received = true;
        cv.notify_one();
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::MODIFIED));

    wait_ms(100);

    {
        file f(modify_file, true, file_access::WRITE, file_shared::SHARE_READ);
        f.write("modified content", 16);
    }

    {
        unique_lock<mutex> lk(mtx);
        EXPECT_TRUE(cv.wait_for(lk, milliseconds(5000), [&] { return event_received; }));
    }

    watcher.stop();

    if (event_received) {
        EXPECT_EQ(captured_event, file_watch_event::MODIFIED);
    }
}

TEST_F(FileWatcherTest, FileAccessedEvent) {
    auto access_file = get_path("to_access.txt");
    filesystem::create_and_write(access_file, "access me");

    wait_ms(100);

    file_watcher watcher(test_dir_, false);

    path accessed_path;
    file_watch_event captured_event = file_watch_event::ACCESSED;
    mutex mtx;
    condition_variable cv;
    bool event_received = false;

    auto cb = [&](const path& p, file_watch_event e) {
        lock<mutex> lk(mtx);
        accessed_path = p;
        captured_event = e;
        event_received = true;
        cv.notify_one();
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::ACCESSED));

    wait_ms(100);

    {
        file f(access_file);
        ignore = f.read();
    }

    {
        unique_lock<mutex> lk(mtx);
        cv.wait_for(lk, milliseconds(5000), [&] { return event_received; });
    }

    watcher.stop();
}

TEST_F(FileWatcherTest, AllEvents) {
    file_watcher watcher(test_dir_, false);

    int call_count = 0;
    mutex mtx;
    condition_variable cv;

    auto cb = [&](const path&, file_watch_event) {
        lock<mutex> lk(mtx);
        ++call_count;
        cv.notify_one();
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::ALL));

    wait_ms(100);

    auto new_file = get_path("all_events.txt");
    filesystem::create_and_write(new_file, "trigger");

    {
        unique_lock<mutex> lk(mtx);
        cv.wait_for(lk, milliseconds(5000), [&] { return call_count > 0; });
    }

    watcher.stop();
    EXPECT_GT(call_count, 0);
}

TEST_F(FileWatcherTest, WatchPath) {
    file_watcher watcher(test_dir_, false);

    EXPECT_EQ(watcher.watch_path(), test_dir_);
    EXPECT_EQ(watcher.watch_path().str(), test_dir_.str());
}

TEST_F(FileWatcherTest, CurrentEvents) {
    file_watcher watcher(test_dir_, false);

    EXPECT_EQ(watcher.current_events(), file_watch_event::ALL);

    auto cb = [](const path&, file_watch_event) {};
    watcher.start(cb, file_watch_event::CREATED);
    EXPECT_EQ(watcher.current_events(), file_watch_event::CREATED);
    watcher.stop();
}

TEST_F(FileWatcherTest, IsWatching) {
    file_watcher watcher(test_dir_, false);

    EXPECT_FALSE(watcher.is_watching());

    auto cb = [](const path&, file_watch_event) {};
    watcher.start(cb);
    EXPECT_TRUE(watcher.is_watching());
    watcher.stop();
    EXPECT_FALSE(watcher.is_watching());
}

TEST_F(FileWatcherTest, IsRecursive) {
    file_watcher watcher1(test_dir_, false);
    EXPECT_FALSE(watcher1.is_recursive());

    file_watcher watcher2(test_dir_, true);
    EXPECT_TRUE(watcher2.is_recursive());
}

TEST_F(FileWatcherTest, UpdateWatchWhileStopped) {
    file_watcher watcher(test_dir_, false);

    EXPECT_TRUE(watcher.update_watch(file_watch_event::CREATED));
    EXPECT_EQ(watcher.current_events(), file_watch_event::CREATED);

    EXPECT_TRUE(watcher.update_watch(file_watch_event::MODIFIED));
    EXPECT_EQ(watcher.current_events(), file_watch_event::MODIFIED);
}

TEST_F(FileWatcherTest, UpdateWatchWhileWatching) {
    file_watcher watcher(test_dir_, false);

    auto cb = [](const path&, file_watch_event) {};
    watcher.start(cb, file_watch_event::CREATED);

    EXPECT_TRUE(watcher.update_watch(file_watch_event::DELETED));
    EXPECT_EQ(watcher.current_events(), file_watch_event::DELETED);
    EXPECT_TRUE(watcher.is_watching());

    watcher.stop();
}

TEST_F(FileWatcherTest, UpdateWatchSameValue) {
    file_watcher watcher(test_dir_, false);

    auto cb = [](const path&, file_watch_event) {};
    watcher.start(cb, file_watch_event::CREATED);
    EXPECT_TRUE(watcher.update_watch(file_watch_event::CREATED));
    EXPECT_TRUE(watcher.is_watching());
    watcher.stop();
}

TEST_F(FileWatcherTest, UpdateRecursiveWhileStopped) {
    file_watcher watcher(test_dir_, false);

    EXPECT_TRUE(watcher.update_recursive(true));
    EXPECT_TRUE(watcher.is_recursive());

    EXPECT_TRUE(watcher.update_recursive(false));
    EXPECT_FALSE(watcher.is_recursive());
}

TEST_F(FileWatcherTest, UpdateRecursiveWhileWatching) {
    file_watcher watcher(test_dir_, false);

    auto cb = [](const path&, file_watch_event) {};
    watcher.start(cb);

    EXPECT_TRUE(watcher.update_recursive(true));
    EXPECT_TRUE(watcher.is_recursive());
    EXPECT_TRUE(watcher.is_watching());

    watcher.stop();
}

TEST_F(FileWatcherTest, UpdateRecursiveSameValue) {
    file_watcher watcher(test_dir_, false);

    auto cb = [](const path&, file_watch_event) {};
    watcher.start(cb);
    EXPECT_TRUE(watcher.update_recursive(false));
    EXPECT_TRUE(watcher.is_watching());
    watcher.stop();
}

TEST_F(FileWatcherTest, UpdateRecursiveWithoutCallback) {
    file_watcher watcher(test_dir_, false);

    EXPECT_TRUE(watcher.update_recursive(true));
    EXPECT_TRUE(watcher.is_recursive());
}

TEST_F(FileWatcherTest, UpdateWatchWithoutCallback) {
    file_watcher watcher(test_dir_, false);

    EXPECT_TRUE(watcher.update_watch(file_watch_event::CREATED));
    EXPECT_EQ(watcher.current_events(), file_watch_event::CREATED);
}

TEST_F(FileWatcherTest, DestructorStopsWatching) {
    auto cb = [](const path&, file_watch_event) {};

    {
        file_watcher watcher(test_dir_, false);
        watcher.start(cb);
        EXPECT_TRUE(watcher.is_watching());
    }

    SUCCEED();
}

TEST_F(FileWatcherTest, MultipleEventsReceived) {
    file_watcher watcher(test_dir_, false);

    int call_count = 0;
    mutex mtx;
    condition_variable cv;

    auto cb = [&](const path&, file_watch_event) {
        lock<mutex> lk(mtx);
        ++call_count;
        if (call_count >= 2) {
            cv.notify_one();
        }
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::CREATED));

    wait_ms(100);

    filesystem::create_and_write(get_path("multi_a.txt"), "a");
    filesystem::create_and_write(get_path("multi_b.txt"), "b");

    {
        unique_lock<mutex> lk(mtx);
        cv.wait_for(lk, milliseconds(5000), [&] { return call_count >= 2; });
    }

    watcher.stop();
    EXPECT_GE(call_count, 2);
}

TEST_F(FileWatcherTest, SubdirectoryFileCreatedNonRecursive) {
    auto subdir = get_path("subdir_nonrec");
    filesystem::create_directories(subdir);

    file_watcher watcher(test_dir_, false);

    int call_count = 0;
    mutex mtx;
    condition_variable cv;

    auto cb = [&](const path&, file_watch_event) {
        lock<mutex> lk(mtx);
        ++call_count;
        cv.notify_one();
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::CREATED));

    wait_ms(100);

    filesystem::create_and_write(subdir / path("nested.txt"), "nested");

    {
        unique_lock<mutex> lk(mtx);
        cv.wait_for(lk, milliseconds(2000));
    }

    watcher.stop();
}

TEST_F(FileWatcherTest, SubdirectoryFileCreatedRecursive) {
    auto subdir = get_path("subdir_rec");
    filesystem::create_directories(subdir);

    wait_ms(100);

    file_watcher watcher(test_dir_, true);

    path created_path;
    file_watch_event captured_event = file_watch_event::ACCESSED;
    mutex mtx;
    condition_variable cv;
    bool event_received = false;

    auto cb = [&](const path& p, file_watch_event e) {
        lock<mutex> lk(mtx);
        created_path = p;
        captured_event = e;
        event_received = true;
        cv.notify_one();
    };

    EXPECT_TRUE(watcher.start(cb, file_watch_event::CREATED));

    wait_ms(100);
    filesystem::create_and_write(subdir / path("nested_rec.txt"), "recursive");

    {
        unique_lock<mutex> lk(mtx);
        cv.wait_for(lk, milliseconds(5000), [&] { return event_received; });
    }

    watcher.stop();
}

TEST_F(FileWatcherTest, StopThenRestart) {
    file_watcher watcher(test_dir_, false);

    auto cb = [](const path&, file_watch_event) {};

    EXPECT_TRUE(watcher.start(cb, file_watch_event::CREATED));
    watcher.stop();
    EXPECT_FALSE(watcher.is_watching());

    EXPECT_TRUE(watcher.start(cb, file_watch_event::MODIFIED));
    EXPECT_TRUE(watcher.is_watching());
    watcher.stop();
}

#endif
