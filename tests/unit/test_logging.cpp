#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/logging/file_sink.hpp>
#include <NeForce/logging/logger.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
    class mock_sink : public log_sink {
    public:
        vector<log_event> events;
        size_t flush_count = 0;

        void log(const log_event& event) override { events.push_back(event); }

        void flush() override { ++flush_count; }
    };

    class relay_sink : public log_sink {
    public:
        void log(const log_event& event) override {
            if (event.message == "relay trigger") {
                logger_registry::instance()
                        .get_logger("flush.target")
                        ->log(log_level::INFO, "relayed", source_location{"test.cpp", "test", 1});
            }
        }

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
    ev.loc = source_location{"test.cpp", "test_func", 42};
    ev.context = make_shared<flat_unordered_map<string, string>>();
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
    ev.loc = source_location{"test.cpp", "test_func", 42};
    auto ctx = make_shared<flat_unordered_map<string, string>>();
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
    ev.loc = source_location{"test.cpp", "test_func", 42};
    ev.context = make_shared<flat_unordered_map<string, string>>();
    string result = formatter.format(ev);
    EXPECT_EQ(result, "");
}

TEST(LogFormatterTest, UnknownPlaceholder) {
    log_formatter formatter("{unknown}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "test";
    ev.loc = source_location{"test.cpp", "test_func", 42};
    ev.context = make_shared<flat_unordered_map<string, string>>();
    string result = formatter.format(ev);
    EXPECT_EQ(result, "{unknown}");
}

TEST(LogFormatterTest, FileLineFuncThread) {
    log_formatter formatter("{file}:{line} {func} {thread}");
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "msg";
    ev.loc = source_location{"test.cpp", "main", 42};
    ev.thread_id = this_thread::id();
    ev.context = make_shared<flat_unordered_map<string, string>>();
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
    ev.loc = source_location{"/home/user/project/src/main.cpp", "main", 99};
    ev.context = make_shared<flat_unordered_map<string, string>>();
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
    ev.loc = source_location{"/home/user/project/src/main.cpp", "main", 99};
    ev.context = make_shared<flat_unordered_map<string, string>>();
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
    ev.loc = source_location{"test.cpp", "test", 1};
    ev.context = make_shared<flat_unordered_map<string, string>>();
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
    path dir;
    path filename;
    void SetUp() override {
        dir = path("test_log_dir_" + to_string(steady_clock::now().since_epoch().count()));
        filesystem::create_directories(dir);
        filename = dir / path("test_log.log");
    }
    void TearDown() override { filesystem::remove_all(dir); }

    log_event make_event(string msg) {
        log_event ev;
        ev.dt = datetime::now();
        ev.level = log_level::INFO;
        ev.message = move(msg);
        ev.loc = source_location{"test.cpp", "test", 1};
        ev.context = make_shared<flat_unordered_map<string, string>>();
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

TEST_F(FileSinkTest, SetFormatterWhileLogging) {
    {
        file_sink sink(filename, 10 * 1024 * 1024, false);
        sink.set_formatter(make_unique<log_formatter>("{message}"));

        vector<thread> workers;
        for (int i = 0; i < 2; ++i) {
            workers.emplace_back([&sink, this] {
                for (int j = 0; j < 100; ++j) {
                    sink.log(make_event("concurrent message"));
                }
            });
        }
        workers.emplace_back([&sink] {
            for (int i = 0; i < 100; ++i) {
                sink.set_formatter(make_unique<log_formatter>(i % 2 == 0 ? "{message}" : "[{level}] {message}"));
            }
        });
        for (auto& worker: workers) {
            worker.join();
        }
        sink.flush();
    }

    file in;
    ASSERT_TRUE(in.open(filename));
    const string content = in.read();
    in.close();
    EXPECT_NE(content.find("concurrent message"), string::npos);
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
        log->clear_sinks();
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
    EXPECT_NE(mock_->events[0].loc.file_name(), "");
    EXPECT_NE(mock_->events[0].loc.func_name(), "");
    EXPECT_GT(mock_->events[0].loc.line(), 0);
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

class ConditionalLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto log = root();
        log->set_level(log_level::INFO);
        log->disable_async();
        sink_ = make_shared<mock_sink>();
        log->add_sink(sink_);
    }

    void TearDown() override {
        auto log = root();
        log->clear_sinks();
    }

    shared_ptr<mock_sink> sink_;
};

TEST_F(ConditionalLogTest, LogIf) {
    NEFORCE_LOG_INFO_IF(true, "should appear");
    this_thread::sleep_for(50_ms);
    EXPECT_GE(sink_->events.size(), 1u);
    EXPECT_EQ(sink_->events[0].message, "should appear");
}

TEST_F(ConditionalLogTest, LogIfFalse) {
    NEFORCE_LOG_INFO_IF(false, "skipped");
    this_thread::sleep_for(50_ms);
    for (auto& ev: sink_->events) {
        EXPECT_NE(ev.message, "skipped");
    }
}

TEST_F(ConditionalLogTest, LogEveryN) {
    for (int i = 0; i < 5; ++i) {
        NEFORCE_LOG_INFO_EVERY_N(2, "every_2");
    }
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(sink_->events.size(), 3u);
}

TEST_F(ConditionalLogTest, LogFirstN) {
    for (int i = 0; i < 10; ++i) {
        NEFORCE_LOG_INFO_FIRST_N(3, "first_3");
    }
    this_thread::sleep_for(50_ms);
    EXPECT_EQ(sink_->events.size(), 3u);
}

#ifdef NEFORCE_PLATFORM_LINUX

TEST(SyslogSinkTest, CreateAndLog) {
    syslog_sink sink("test_app", syslog_facility::user);
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::INFO;
    ev.message = "syslog test message";
    ev.loc = source_location{"test.cpp", "test", 1};
    ev.context = make_shared<flat_unordered_map<string, string>>();
    EXPECT_NO_THROW(sink.log(ev));
    EXPECT_NO_THROW(sink.flush());
}

TEST(SyslogSinkTest, Formatting) {
    syslog_sink sink("test_fmt", syslog_facility::daemon);
    sink.set_formatter(make_unique<log_formatter>("[{level}] {message}"));
    log_event ev;
    ev.dt = datetime::now();
    ev.level = log_level::ERROR;
    ev.message = "formatted syslog";
    ev.loc = source_location{"test.cpp", "test", 1};
    ev.context = make_shared<flat_unordered_map<string, string>>();
    EXPECT_NO_THROW(sink.log(ev));
}

#endif

class OverflowPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto log = root();
        log->disable_async();
        log->set_level(log_level::TRACE);
        log->clear_context();
        log->set_filter({});
        log->set_auto_flush(0);
        log->clear_sinks();
    }

    void TearDown() override {
        auto log = root();
        log->disable_async();
        log->set_level(log_level::TRACE);
        log->clear_context();
        log->set_filter({});
        log->set_auto_flush(0);
        log->clear_sinks();
    }
};

TEST_F(OverflowPolicyTest, DiscardPolicy) {
    auto log = root();
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

TEST_F(OverflowPolicyTest, OverrunOldestPolicy) {
    auto log = root();
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

TEST_F(OverflowPolicyTest, DiscardPolicyRepeatedCycle) {
    auto log = root();
    for (int cycle = 0; cycle < 100; ++cycle) {
        auto sink = make_shared<mock_sink>();
        log->add_sink(sink);
        log->enable_async(nullptr, 4, overflow_policy::discard);
        for (int i = 0; i < 50; ++i) {
            NEFORCE_LOG_INFO("cycle test " + to_string(i));
        }
        log->flush();
        log->disable_async();
        EXPECT_FALSE(sink->events.empty());
        log->clear_sinks();
    }
}

TEST_F(OverflowPolicyTest, DisableDeliversQueuedEventsInOrder) {
    auto log = root();
    log->enable_async(nullptr, 256, overflow_policy::block);

    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    for (int i = 0; i < 100; ++i) {
        NEFORCE_LOG_INFO("queued " + to_string(i));
    }
    log->disable_async();

    ASSERT_EQ(sink->events.size(), 100u);
    for (size_t i = 0; i < sink->events.size(); ++i) {
        EXPECT_EQ(sink->events[i].message, "queued " + to_string(i));
    }
}

TEST_F(OverflowPolicyTest, FlushDeliversQueuedEventsInOrder) {
    auto log = root();
    log->enable_async(nullptr, 1024, overflow_policy::block);

    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    for (int i = 0; i < 200; ++i) {
        NEFORCE_LOG_INFO("flushed " + to_string(i));
        if (i % 20 == 0) {
            log->flush();
        }
    }
    log->flush();

    ASSERT_EQ(sink->events.size(), 200u);
    for (size_t i = 0; i < sink->events.size(); ++i) {
        EXPECT_EQ(sink->events[i].message, "flushed " + to_string(i));
    }
    log->disable_async();
}

TEST_F(OverflowPolicyTest, ReenableAfterDisable) {
    auto log = root();
    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    log->enable_async(nullptr, 8, overflow_policy::discard);
    NEFORCE_LOG_INFO("first pass");
    log->disable_async();

    log->enable_async(nullptr, 8, overflow_policy::discard);
    NEFORCE_LOG_INFO("second pass");
    log->disable_async();

    ASSERT_EQ(sink->events.size(), 2u);
    EXPECT_EQ(sink->events[0].message, "first pass");
    EXPECT_EQ(sink->events[1].message, "second pass");
}

TEST_F(OverflowPolicyTest, ExternalPoolStaysRunning) {
    auto pool = make_shared<thread_pool>();
    ASSERT_TRUE(pool->start(1));

    auto log = root();
    log->enable_async(pool, 64, overflow_policy::block);

    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    NEFORCE_LOG_INFO("external pool");
    log->flush();
    log->disable_async();

    EXPECT_TRUE(pool->running());
    EXPECT_FALSE(sink->events.empty());
    EXPECT_FALSE(log->is_async());
    pool->stop();
}

TEST_F(OverflowPolicyTest, AutoFlushIntervalTriggersSinkFlush) {
    auto log = root();
    log->enable_async(nullptr, 64, overflow_policy::block);

    auto sink = make_shared<mock_sink>();
    log->add_sink(sink);

    log->set_auto_flush(1);
    NEFORCE_LOG_INFO("auto flush one");
    this_thread::sleep_for(5_ms);
    NEFORCE_LOG_INFO("auto flush two");

    log->set_auto_flush(0);
    log->flush();

    EXPECT_GE(sink->flush_count, 1u);
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
    NEFORCE_LOGF_DEBUG("debug format {}", 42);
    SUCCEED();
}

TEST(HierarchicalLoggerTest, NewChildInheritsCurrentLevel) {
    auto parent = logger_registry::instance().get_logger("late_parent");
    parent->set_level(log_level::ERROR);
    auto child = logger_registry::instance().get_logger("late_parent.child");
    EXPECT_EQ(child->effective_level(), log_level::ERROR);
    parent->set_level(logger::INHERIT);
}

TEST(LoggerRegistryTest, SameNameReturnsSameInstance) {
    auto first = logger_registry::instance().get_logger("same.name");
    auto second = logger_registry::instance().get_logger("same.name");
    EXPECT_EQ(first, second);
}

TEST(LoggerRegistryTest, ConcurrentCreationIsStable) {
    auto& registry = logger_registry::instance();
    constexpr size_t count = 8;
    vector<shared_ptr<logger>> created(count);
    vector<thread> workers;

    for (size_t i = 0; i < count; ++i) {
        workers.emplace_back(
                [&registry, &created, i] { created[i] = registry.get_logger("concurrent" + to_string(i) + ".child"); });
    }
    for (auto& worker: workers) {
        worker.join();
    }

    for (size_t i = 0; i < count; ++i) {
        ASSERT_NE(created[i], nullptr);
        EXPECT_EQ(created[i]->name(), "concurrent" + to_string(i) + ".child");
        ASSERT_NE(created[i]->parent(), nullptr);
        EXPECT_EQ(created[i]->parent()->name(), "concurrent" + to_string(i));
        EXPECT_EQ(registry.get_logger("concurrent" + to_string(i) + ".child"), created[i]);
    }
}

TEST(LoggerRegistryTest, FlushAllWithCrossLoggerSink) {
    auto& registry = logger_registry::instance();
    auto source = registry.get_logger("flush.source");
    auto target = registry.get_logger("flush.target");

    auto sink = make_shared<mock_sink>();
    target->add_sink(sink);
    source->add_sink(make_shared<relay_sink>());

    source->log(log_level::INFO, "relay trigger", source_location{"test.cpp", "test", 1});
    registry.flush_all();

    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_EQ(sink->events[0].message, "relayed");

    source->clear_sinks();
    target->clear_sinks();
}
