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

    auto root() { return logger_registry::instance().root_logger(); }
} // namespace


TEST(LogLevelTest, ToStringAllValues) {
    EXPECT_EQ(to_string(log_level::TRACE), "TRACE");
    EXPECT_EQ(to_string(log_level::DEBUG), "DEBUG");
    EXPECT_EQ(to_string(log_level::INFO), "INFO");
    EXPECT_EQ(to_string(log_level::WARN), "WARN");
    EXPECT_EQ(to_string(log_level::ERROR), "ERROR");
    EXPECT_EQ(to_string(log_level::FATAL), "FATAL");
    EXPECT_EQ(to_string(log_level::OFF), "OFF");
}

TEST(LogFormatterTest, ParseBasicPattern) {
    log_formatter formatter("[{time}] [{level}] {message}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "Hello";
    ev.loc = source_loc{"test.cpp", "test_func", 42};
    ev.context = make_shared<unordered_map<string, string>>();
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
    ev.loc = source_loc{"test.cpp", "test_func", 42};
    auto ctx = make_shared<unordered_map<string, string>>();
    (*ctx)["user"] = "admin";
    ev.context = ctx;
    string result = formatter.format(ev);
    EXPECT_EQ(result, "admin");
}

TEST(LogFormatterTest, MissingContextKey) {
    log_formatter formatter("{context.missing}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "test";
    ev.loc = source_loc{"test.cpp", "test_func", 42};
    ev.context = make_shared<unordered_map<string, string>>();
    string result = formatter.format(ev);
    EXPECT_EQ(result, "");
}

TEST(LogFormatterTest, UnknownPlaceholder) {
    log_formatter formatter("{unknown}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "test";
    ev.loc = source_loc{"test.cpp", "test_func", 42};
    ev.context = make_shared<unordered_map<string, string>>();
    string result = formatter.format(ev);
    EXPECT_EQ(result, "{unknown}");
}

TEST(LogFormatterTest, FileLineFuncThread) {
    log_formatter formatter("{file}:{line} {func} {thread}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "msg";
    ev.loc = source_loc{"test.cpp", "main", 42};
    ev.thread_id = this_thread::id();
    ev.context = make_shared<unordered_map<string, string>>();
    string result = formatter.format(ev);
    EXPECT_NE(result.find("test.cpp"), string::npos);
    EXPECT_NE(result.find("42"), string::npos);
    EXPECT_NE(result.find("main"), string::npos);
}

TEST(LogFormatterTest, FileOnlyBasename) {
    log_formatter formatter("{file}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "msg";
    ev.loc = source_loc{"/home/user/project/src/main.cpp", "main", 99};
    ev.context = make_shared<unordered_map<string, string>>();
    string result = formatter.format(ev);
    EXPECT_EQ(result, "main.cpp");
    EXPECT_EQ(result.find('/'), string::npos);
}

TEST(LogFormatterTest, FilePathFull) {
    log_formatter formatter("{filepath}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "msg";
    ev.loc = source_loc{"/home/user/project/src/main.cpp", "main", 99};
    ev.context = make_shared<unordered_map<string, string>>();
    string result = formatter.format(ev);
    EXPECT_NE(result.find("/home/user"), string::npos);
}

#ifdef NEFORCE_PLATFORM_LINUX

TEST(ConsoleSinkTest, FormatAndOutput) {
    console_sink sink;
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::WARN;
    ev.message = "console test";
    ev.loc = source_loc{"test.cpp", "test", 1};
    ev.context = make_shared<unordered_map<string, string>>();
    testing::internal::CaptureStdout();
    sink.log(ev);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_NE(output.find("WARN"), std::string::npos);
    EXPECT_NE(output.find("console test"), std::string::npos);
}

TEST(ConsoleSinkTest, FlushDoesNotThrow) {
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

    log_event make_event(string msg) {
        log_event ev;
        ev.dt = datetime::now();
        ev.level = log_level::INFO;
        ev.message = move(msg);
        ev.loc = source_loc{"test.cpp", "test", 1};
        ev.context = make_shared<unordered_map<string, string>>();
        return ev;
    }
};

TEST_F(FileSinkTest, CreateAndWrite) {
    {
        file_sink sink(filename, 1024, false);
        sink.set_formatter(make_unique<log_formatter>("[{level}] {message}"));
        sink.log(make_event("hello file"));
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
        for (int i = 0; i < 20; ++i) {
            auto ev = make_event("data block number " + to_string(i));
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
        sink.log(make_event("dated log"));
        sink.flush();
    }
    path expected(filename.str() + "." + datetime::now().date().to_string());
    file in;
    EXPECT_TRUE(in.open(expected));
    in.close();
}

TEST_F(FileSinkTest, MaxFilesRetention) {
    {
        file_sink sink(filename, 50, false, 2);
        sink.set_formatter(make_unique<log_formatter>("{message}"));
        for (int i = 0; i < 30; ++i) {
            auto ev = make_event("data block number " + to_string(i));
            sink.log(ev);
        }
        sink.flush();
    }
    file f1, f2;
    EXPECT_TRUE(f1.open(filename) || f2.open(path(filename.str() + ".1")));
    if (f1.is_opened()) {
        f1.close();
    }
    if (f2.is_opened()) {
        f2.close();
    }
}

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto log = root();
        log->set_level(log_level::TRACE);
        log->clear_context();
        log->set_filter({});
        log->disable_async();

        mock_ = make_shared<mock_sink>();
        log->add_sink(mock_);
    }

    void TearDown() override {
        auto log = root();
        log->set_level(log_level::TRACE);
        log->clear_context();
        log->set_filter({});
        log->disable_async();
    }

    shared_ptr<mock_sink> mock_;
};

TEST_F(LoggerTest, RegistrySingleton) {
    auto& reg1 = logger_registry::instance();
    auto& reg2 = logger_registry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(LoggerTest, RootLoggerExists) {
    auto r = logger_registry::instance().root_logger();
    EXPECT_NE(r, nullptr);
    EXPECT_EQ(r->name(), "root");
}

TEST_F(LoggerTest, LevelFilter) {
    auto log = root();
    log->set_level(log_level::WARN);
    NEFORCE_LOG_INFO("dropped");
    NEFORCE_LOG_WARN("appears");
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(mock_->events.size(), 1u);
    if (!mock_->events.empty()) {
        EXPECT_EQ(mock_->events[0].message, "appears");
    }
}

TEST_F(LoggerTest, ContextPropagation) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->add_context("request_id", "12345");
    NEFORCE_LOG_INFO("context test");
    this_thread::sleep_for(50_ms);
    ASSERT_GE(mock_->events.size(), 1u);
    const auto& ctx = mock_->events[0].context;
    EXPECT_EQ(ctx->at("request_id"), "12345");
}

TEST_F(LoggerTest, FilterBlocks) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->set_filter([](const log_event& ev) { return ev.message != "skip"; });
    NEFORCE_LOG_INFO("skip");
    NEFORCE_LOG_INFO("keep");
    this_thread::sleep_for(50_ms);
    ASSERT_EQ(mock_->events.size(), 1u);
    EXPECT_EQ(mock_->events[0].message, "keep");
}

TEST_F(LoggerTest, AsyncModeDelivery) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->set_filter({});
    log->enable_async(nullptr, 1024, overflow_policy::block);
    NEFORCE_LOG_INFO("async msg");
    log->flush();
    EXPECT_FALSE(mock_->events.empty());
    if (!mock_->events.empty()) {
        EXPECT_EQ(mock_->events[0].message, "async msg");
    }
    log->disable_async();
}

TEST_F(LoggerTest, FlushInSyncMode) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->disable_async();
    NEFORCE_LOG_INFO("sync msg");
    EXPECT_FALSE(mock_->events.empty());
    log->flush();
}

TEST_F(LoggerTest, ToggleAsyncFlushesQueue) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->set_filter({});
    log->enable_async(nullptr, 1024, overflow_policy::block);
    NEFORCE_LOG_INFO("before toggle");
    this_thread::sleep_for(50_ms);
    log->disable_async();
    EXPECT_FALSE(mock_->events.empty());
    if (!mock_->events.empty()) {
        EXPECT_EQ(mock_->events[0].message, "before toggle");
    }
}

TEST_F(LoggerTest, RemoveContext) {
    auto log = root();
    log->add_context("key", "value");
    log->remove_context("key");
    NEFORCE_LOG_INFO("msg");
    this_thread::sleep_for(50_ms);
    ASSERT_GE(mock_->events.size(), 1u);
    EXPECT_EQ(mock_->events[0].context->count("key"), 0u);
}

TEST_F(LoggerTest, ClearContext) {
    auto log = root();
    log->add_context("a", "1");
    log->add_context("b", "2");
    log->clear_context();
    NEFORCE_LOG_INFO("msg");
    this_thread::sleep_for(50_ms);
    ASSERT_GE(mock_->events.size(), 1u);
    EXPECT_TRUE(mock_->events[0].context->empty());
}

TEST_F(LoggerTest, MultipleSinks) {
    auto log = root();
    log->set_level(log_level::INFO);
    auto sink2 = make_shared<mock_sink>();
    log->add_sink(sink2);
    NEFORCE_LOG_INFO("multi");
    this_thread::sleep_for(50_ms);
    EXPECT_GE(mock_->events.size(), 1u);
    EXPECT_GE(sink2->events.size(), 1u);
}

TEST_F(LoggerTest, SourceLocation) {
    auto log = root();
    log->set_level(log_level::INFO);
    NEFORCE_LOG_INFO("srcloc test");
    this_thread::sleep_for(50_ms);
    ASSERT_GE(mock_->events.size(), 1u);
    EXPECT_NE(mock_->events[0].loc.file, nullptr);
    EXPECT_NE(mock_->events[0].loc.func, nullptr);
    EXPECT_GT(mock_->events[0].loc.line, 0);
}

TEST_F(LoggerTest, CompileTimeFilteringDisabled) {
    NEFORCE_LOG_TRACE("compile time test");
    this_thread::sleep_for(50_ms);
    SUCCEED();
}


TEST(HierarchicalLoggerTest, CreateNamedLogger) {
    auto app = logger_registry::instance().get_logger("app");
    EXPECT_NE(app, nullptr);
    EXPECT_EQ(app->name(), "app");
    EXPECT_EQ(app->parent(), logger_registry::instance().root_logger().get());
}

TEST(HierarchicalLoggerTest, NestedLogger) {
    auto db = logger_registry::instance().get_logger("app.db");
    EXPECT_EQ(db->name(), "app.db");
    EXPECT_NE(db->parent(), nullptr);
    EXPECT_EQ(db->parent()->name(), "app");
}

TEST(HierarchicalLoggerTest, InheritLevelFromParent) {
    auto parent = logger_registry::instance().get_logger("parent_test");
    auto child = logger_registry::instance().get_logger("parent_test.child");
    parent->set_level(log_level::WARN);
    EXPECT_EQ(child->effective_level(), log_level::WARN);
}

TEST(HierarchicalLoggerTest, OverrideLevel) {
    auto parent = logger_registry::instance().get_logger("override_test");
    auto child = logger_registry::instance().get_logger("override_test.child");
    parent->set_level(log_level::WARN);
    child->set_level(log_level::TRACE);
    EXPECT_EQ(child->effective_level(), log_level::TRACE);
    EXPECT_EQ(parent->effective_level(), log_level::WARN);
}

TEST(MDCTest, PutAndGet) {
    mdc::put("trace_id", "abc123");
    EXPECT_EQ(mdc::get("trace_id"), "abc123");
    mdc::clear();
}

TEST(MDCTest, Remove) {
    mdc::put("key", "value");
    mdc::remove("key");
    EXPECT_EQ(mdc::get("key"), "");
    mdc::clear();
}

TEST(MDCTest, Empty) {
    mdc::clear();
    EXPECT_TRUE(mdc::empty());
    mdc::put("x", "y");
    EXPECT_FALSE(mdc::empty());
    mdc::clear();
}

TEST(MDCTest, Snapshot) {
    mdc::clear();
    mdc::put("a", "1");
    mdc::put("b", "2");
    auto snap = mdc::snapshot();
    EXPECT_EQ(snap["a"], "1");
    EXPECT_EQ(snap["b"], "2");
    mdc::clear();
}

TEST(ConditionalLogTest, LogIf) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->disable_async();
    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    NEFORCE_LOG_INFO_IF(true, "should appear");
    this_thread::sleep_for(50_ms);
    EXPECT_GE(sink->events.size(), 1u);
    EXPECT_EQ(sink->events[0].message, "should appear");
}

TEST(ConditionalLogTest, LogIfFalse) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->disable_async();
    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    NEFORCE_LOG_INFO_IF(false, "skipped");
    this_thread::sleep_for(50_ms);
    for (auto& ev: sink->events) {
        EXPECT_NE(ev.message, "skipped");
    }
}

TEST(ConditionalLogTest, LogEveryN) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->disable_async();
    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    for (int i = 0; i < 5; ++i) {
        NEFORCE_LOG_INFO_EVERY_N(2, "every_2");
    }
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(sink->events.size(), 3u);
}

TEST(ConditionalLogTest, LogFirstN) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->disable_async();
    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    for (int i = 0; i < 10; ++i) {
        NEFORCE_LOG_INFO_FIRST_N(3, "first_3");
    }
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(sink->events.size(), 3u);
}

#ifdef NEFORCE_PLATFORM_LINUX

TEST(SyslogSinkTest, CreateAndLog) {
    syslog_sink sink("test_app", syslog_facility::LOG_USER);
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "syslog test message";
    ev.loc = source_loc{"test.cpp", "test", 1};
    ev.context = make_shared<unordered_map<string, string>>();
    EXPECT_NO_THROW(sink.log(ev));
    EXPECT_NO_THROW(sink.flush());
}

TEST(SyslogSinkTest, Formatting) {
    syslog_sink sink("test_fmt", syslog_facility::LOG_DAEMON);
    sink.set_formatter(make_unique<log_formatter>("[{level}] {message}"));
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::ERROR;
    ev.message = "formatted syslog";
    ev.loc = source_loc{"test.cpp", "test", 1};
    ev.context = make_shared<unordered_map<string, string>>();
    EXPECT_NO_THROW(sink.log(ev));
}

#endif

TEST(OverflowPolicyTest, DiscardPolicy) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->set_filter({});
    log->enable_async(nullptr, 4, overflow_policy::discard);

    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    for (int i = 0; i < 100; ++i) {
        NEFORCE_LOG_INFO("overflow test " + to_string(i));
    }
    log->flush();
    EXPECT_FALSE(sink->events.empty());
    log->disable_async();
}

TEST(OverflowPolicyTest, OverrunOldestPolicy) {
    auto log = root();
    log->set_level(log_level::INFO);
    log->set_filter({});
    log->enable_async(nullptr, 4, overflow_policy::overrun_oldest);

    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    for (int i = 0; i < 100; ++i) {
        NEFORCE_LOG_INFO("overrun test " + to_string(i));
    }
    log->flush();
    EXPECT_FALSE(sink->events.empty());
    log->disable_async();
}

TEST(AutoFlushTest, SetInterval) {
    auto log = root();
    log->set_auto_flush(5000);
    SUCCEED();
    log->set_auto_flush(0);
}

TEST(CompileTimeFilterTest, ActiveLevelDefined) {
    int level = NEFORCE_ACTIVE_LOG_LEVEL;
    EXPECT_GE(level, NEFORCE_LOG_LEVEL_TRACE);
    EXPECT_LE(level, NEFORCE_LOG_LEVEL_OFF);
}

TEST(CompileTimeFilterTest, DebugMacroCompiles) {
    NEFORCE_LOG_DEBUG("debug compile test");
    NEFORCE_LOGF_DEBUG("debug format %d", 42);
    SUCCEED();
}
