#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/logging/file_sink.hpp>
#include <NeForce/logging/logger.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    class mock_sink : public log_sink {
    public:
        vector<log_event> events;
        void log(const log_event& event) override { events.push_back(event); }
        void flush() override {}
    };
} // namespace


TEST(LogLevelTest, ToStringAllValues) {
    EXPECT_EQ(to_string(log_level::TRACE), "TRACE");
    EXPECT_EQ(to_string(log_level::DEBUG), "DEBUG");
    EXPECT_EQ(to_string(log_level::INFO), "INFO");
    EXPECT_EQ(to_string(log_level::WARN), "WARN");
    EXPECT_EQ(to_string(log_level::ERROR), "ERROR");
    EXPECT_EQ(to_string(log_level::FATAL), "FATAL");
}

TEST(LogFormatterTest, ParseBasicPattern) {
    log_formatter formatter("[{time}] [{level}] {message}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "Hello";
    string result = formatter.format(ev);
    EXPECT_NE(result.find("INFO"), string::npos);
    EXPECT_NE(result.find("Hello"), string::npos);
}

TEST(LogFormatterTest, ContextPlaceholder) {
    log_formatter formatter("{context.user}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "test";
    ev.context["user"] = "admin";
    string result = formatter.format(ev);
    EXPECT_EQ(result, "admin");
}

TEST(LogFormatterTest, MissingContextKey) {
    log_formatter formatter("{context.missing}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "test";
    string result = formatter.format(ev);
    EXPECT_EQ(result, "");
}

TEST(LogFormatterTest, UnknownPlaceholder) {
    log_formatter formatter("{unknown}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "test";
    string result = formatter.format(ev);
    EXPECT_EQ(result, "{unknown}");
}

TEST(LogFormatterTest, FileLineFuncThread) {
    log_formatter formatter("{file}:{line} {func} {thread}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "msg";
    ev.file = "test.cpp";
    ev.line = 42;
    ev.func = "main";
    ev.thread_id = this_thread::id();
    string result = formatter.format(ev);
    EXPECT_NE(result.find("test.cpp"), string::npos);
    EXPECT_NE(result.find("42"), string::npos);
    EXPECT_NE(result.find("main"), string::npos);
}

#ifdef NEFORCE_PLATFORM_LINUX

class ConsoleSinkTest : public ::testing::Test {
protected:
    void SetUp() override { console_sink test_sink; }
};

TEST_F(ConsoleSinkTest, FormatAndOutput) {
    console_sink sink;
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::WARN;
    ev.message = "console test";
    testing::internal::CaptureStdout();
    sink.log(ev);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("WARN"), std::string::npos);
    EXPECT_NE(output.find("console test"), std::string::npos);
}

TEST_F(ConsoleSinkTest, FlushDoesNotThrow) {
    console_sink sink;
    EXPECT_NO_THROW(sink.flush());
}

#endif

class FileSinkTest : public ::testing::Test {
protected:
    path filename;
    void SetUp() override {
        filename = path("test_log_" + to_string(steady_clock::now().since_epoch().count()) + ".log");
    }
    void TearDown() override {
        filesystem::remove(filename);
        for (int i = 1; i < 10; ++i) {
            path rotated(filename.str() + "." + to_string(i));
            filesystem::remove(rotated);
        }
        path dateRotated(filename.str() + "." + datetime::now().date().to_string());
        filesystem::remove(dateRotated);
    }
};

TEST_F(FileSinkTest, CreateAndWrite) {
    {
        file_sink sink(filename, 1024, false);
        sink.set_formatter(make_unique<log_formatter>("[{level}] {message}"));
        log_event ev;
        ev.dt = datetime::now();
        ev.level = log_level::INFO;
        ev.message = "hello file";
        sink.log(ev);
        sink.flush();
    }
    file in;
    ASSERT_TRUE(in.open(filename));
    string content = in.read();
    EXPECT_NE(content.find("INFO"), string::npos);
    EXPECT_NE(content.find("hello file"), string::npos);
    in.close();
}

TEST_F(FileSinkTest, SizeRotation) {
    {
        file_sink sink(filename, 100, false);
        sink.set_formatter(make_unique<log_formatter>("{message}"));
        log_event ev;
        ev.dt = datetime::now();
        ev.level = log_level::INFO;
        for (int i = 0; i < 20; ++i) {
            ev.message = "data block number " + to_string(i);
            sink.log(ev);
        }
        sink.flush();
    }
    file f1;
    EXPECT_TRUE(f1.open(filename));
    f1.close();
    file f2;
    EXPECT_TRUE(f2.open(path(filename.str() + ".1")));
    f2.close();
}

TEST_F(FileSinkTest, DateRotationEnabled) {
    {
        file_sink sink(filename, 10 * 1024 * 1024, true);
        sink.set_formatter(make_unique<log_formatter>("{message}"));
        log_event ev;
        ev.dt = datetime::now();
        ev.level = log_level::INFO;
        ev.message = "dated log";
        sink.log(ev);
        sink.flush();
    }
    path expected(filename.str() + "." + datetime::now().date().to_string());
    file in;
    EXPECT_TRUE(in.open(expected));
    in.close();
}


class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& log = logger::instance();
        log.set_level(log_level::TRACE);
        log.clear_context();
        log.set_filter({});
        log.enable_async(false);
    }

    void TearDown() override {
        auto& log = logger::instance();
        log.set_level(log_level::TRACE);
        log.clear_context();
        log.set_filter({});
        log.enable_async(false);
    }

    static shared_ptr<mock_sink> create_sink() {
        auto sink = make_shared<mock_sink>();
        logger::instance().add_sink(sink);
        return sink;
    }
};

TEST_F(LoggerTest, Singleton) {
    auto& a = logger::instance();
    auto& b = logger::instance();
    EXPECT_EQ(&a, &b);
}

TEST_F(LoggerTest, LevelFilter) {
    auto& log = logger::instance();
    log.set_level(log_level::WARN);
    auto sink = create_sink();
    NEFORCE_LOG_INFO("should be dropped");
    NEFORCE_LOG_WARN("should appear");
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(sink->events.size(), 1u);
    if (!sink->events.empty()) {
        EXPECT_EQ(sink->events[0].message, "should appear");
    }
}

TEST_F(LoggerTest, ContextPropagation) {
    auto& log = logger::instance();
    log.set_level(log_level::INFO);
    auto sink = create_sink();
    log.add_context("request_id", "12345");
    NEFORCE_LOG_INFO("context test");
    this_thread::sleep_for(50_ms);
    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_EQ(sink->events[0].context["request_id"], "12345");
}

TEST_F(LoggerTest, FilterBlocks) {
    auto& log = logger::instance();
    log.set_level(log_level::INFO);
    auto sink = create_sink();
    log.set_filter([](const log_event& ev) { return ev.message != "skip"; });
    NEFORCE_LOG_INFO("skip");
    NEFORCE_LOG_INFO("keep");
    this_thread::sleep_for(50_ms);
    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_EQ(sink->events[0].message, "keep");
}

TEST_F(LoggerTest, AsyncModeDelivery) {
    auto& log = logger::instance();
    log.set_level(log_level::INFO);
    auto sink = create_sink();
    log.enable_async(true);
    NEFORCE_LOG_INFO("async msg");
    log.flush();
    EXPECT_FALSE(sink->events.empty());
    EXPECT_EQ(sink->events[0].message, "async msg");
    log.enable_async(false);
}

TEST_F(LoggerTest, FlushInSyncMode) {
    auto& log = logger::instance();
    log.set_level(log_level::INFO);
    auto sink = create_sink();
    log.enable_async(false);
    NEFORCE_LOG_INFO("sync msg");
    EXPECT_FALSE(sink->events.empty());
    log.flush();
}

TEST_F(LoggerTest, ToggleAsyncFlushesQueue) {
    auto& log = logger::instance();
    log.set_level(log_level::INFO);
    auto sink = create_sink();
    log.enable_async(true);
    NEFORCE_LOG_INFO("before toggle");
    log.enable_async(false);
    EXPECT_FALSE(sink->events.empty());
    EXPECT_EQ(sink->events[0].message, "before toggle");
}

TEST_F(LoggerTest, DestructorJoinsWorker) {
    auto& log = logger::instance();
    log.set_level(log_level::INFO);
    auto sink = create_sink();
    log.enable_async(true);
    NEFORCE_LOG_INFO("final msg");
    log.enable_async(false);
    EXPECT_EQ(sink->events.back().message, "final msg");
}

TEST_F(LoggerTest, RemoveContext) {
    auto& log = logger::instance();
    log.add_context("key", "value");
    log.remove_context("key");
    auto sink = create_sink();
    NEFORCE_LOG_INFO("msg");
    this_thread::sleep_for(50_ms);
    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_EQ(sink->events[0].context.count("key"), 0u);
}

TEST_F(LoggerTest, ClearContext) {
    auto& log = logger::instance();
    log.add_context("a", "1");
    log.add_context("b", "2");
    log.clear_context();
    auto sink = create_sink();
    NEFORCE_LOG_INFO("msg");
    this_thread::sleep_for(50_ms);
    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_TRUE(sink->events[0].context.empty());
}

TEST_F(LoggerTest, MultipleSinks) {
    auto& log = logger::instance();
    auto sink1 = create_sink();
    auto sink2 = create_sink();
    log.set_level(log_level::INFO);
    NEFORCE_LOG_INFO("multi");
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(sink1->events.size(), 1u);
    EXPECT_EQ(sink2->events.size(), 1u);
}

TEST_F(LoggerTest, ThreadSafetyUnderAsync) {
    auto& log = logger::instance();
    log.set_level(log_level::INFO);
    auto sink = create_sink();
    log.enable_async(true);
    vector<thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i]() { NEFORCE_LOG_INFO("thread " + to_string(i)); });
    }
    for (auto& t: threads) {
        t.join();
    }
    log.flush();
    EXPECT_EQ(sink->events.size(), 10u);
    log.enable_async(false);
}
