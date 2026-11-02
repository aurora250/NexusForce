#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/system_exception.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/system/environment.hpp>
#include <NeForce/plugin/iplugin.hpp>
#include <NeForce/plugin/plugin_entry.hpp>
#include <NeForce/plugin/plugin_manager.hpp>
#include <gtest/gtest.h>
using namespace neforce;

#ifndef NEFORCE_TEST_PLUGIN_PATH
#    error "NEFORCE_TEST_PLUGIN_PATH must be defined by the build system"
#endif

namespace {
    const char* const plugin_name = "neforce_test_plugin";

    int delete_calls = 0;
    iplugin* last_deleted = nullptr;

    void counting_deleter(iplugin* p) {
        ++delete_calls;
        last_deleted = p;
        delete p;
    }

    struct mock_plugin : iplugin {
        plugin_info info_{"mock", "0.0.1", "tests", "in-process mock plugin", ""};

        int initialize_calls = 0;
        int execute_calls = 0;
        int shutdown_calls = 0;

        NEFORCE_NODISCARD const plugin_info& get_info() const override { return info_; }

        void initialize() override { ++initialize_calls; }

        void execute() override { ++execute_calls; }

        void shutdown() override { ++shutdown_calls; }
    };

    class PluginManagerTest : public ::testing::Test {
    protected:
        void TearDown() override { plugin_manager::instance().shutdown_all(); }

        static const string& plugin_path() {
            static const string value = [] {
                const string override_path = environment::get("NEFORCE_TEST_PLUGIN_PATH");
                return override_path.empty() ? string{NEFORCE_TEST_PLUGIN_PATH} : override_path;
            }();
            return value;
        }
    };
} // namespace


TEST(PluginEntryTest, EntryPointNamesAreStable) {
    EXPECT_STREQ(NEFORCE_PLUGIN_CREATE_FUNC, "create_plugin");
    EXPECT_STREQ(NEFORCE_PLUGIN_DESTROY_FUNC, "destroy_plugin");
}

TEST(PluginInfoTest, DefaultConstructedFieldsAreEmpty) {
    plugin_info info;
    EXPECT_TRUE(info.name.empty());
    EXPECT_TRUE(info.version.empty());
    EXPECT_TRUE(info.author.empty());
    EXPECT_TRUE(info.description.empty());
    EXPECT_TRUE(info.library_path.empty());
}

TEST(PluginInfoTest, FieldsAreAssignable) {
    plugin_info info;
    info.name = "sample";
    info.version = "2.1.0";
    info.author = "author";
    info.description = "description";
    info.library_path = "/tmp/sample.so";

    EXPECT_EQ(info.name, "sample");
    EXPECT_EQ(info.version, "2.1.0");
    EXPECT_EQ(info.author, "author");
    EXPECT_EQ(info.description, "description");
    EXPECT_EQ(info.library_path, "/tmp/sample.so");
}

TEST(IPluginTest, VirtualCallbacksDispatch) {
    mock_plugin plugin;

    EXPECT_EQ(plugin.initialize_calls, 0);
    EXPECT_EQ(plugin.execute_calls, 0);
    EXPECT_EQ(plugin.shutdown_calls, 0);

    iplugin& base = plugin;
    base.initialize();
    base.execute();
    base.execute();
    base.shutdown();

    EXPECT_EQ(plugin.initialize_calls, 1);
    EXPECT_EQ(plugin.execute_calls, 2);
    EXPECT_EQ(plugin.shutdown_calls, 1);
}

TEST(IPluginTest, GetInfoReturnsStableReference) {
    mock_plugin plugin;
    const plugin_info& first = plugin.get_info();
    EXPECT_EQ(&first, &plugin.get_info());
    EXPECT_EQ(first.name, "mock");
}


TEST(PluginDeleterTest, DefaultConstructedDeleterIgnoresNullPlugin) {
    plugin_deleter deleter;
    EXPECT_NO_THROW(deleter(nullptr));
}

TEST(PluginDeleterTest, InvokesStoredFunction) {
    delete_calls = 0;
    last_deleted = nullptr;

    auto* plugin = new mock_plugin();
    plugin_deleter deleter(&counting_deleter);
    deleter(plugin);

    EXPECT_EQ(delete_calls, 1);
    EXPECT_EQ(last_deleted, plugin);
}

TEST(PluginDeleterTest, MoveConstructionTransfersFunction) {
    delete_calls = 0;

    auto* plugin = new mock_plugin();
    plugin_deleter source(&counting_deleter);
    plugin_deleter target(_NEFORCE move(source));
    target(plugin);

    EXPECT_EQ(delete_calls, 1);
}

TEST(PluginDeleterTest, MoveAssignmentTransfersFunction) {
    delete_calls = 0;

    auto* plugin = new mock_plugin();
    plugin_deleter source(&counting_deleter);
    plugin_deleter target;
    target = _NEFORCE move(source);
    target(plugin);

    EXPECT_EQ(delete_calls, 1);
}

TEST(PluginDeleterTest, RebindReturnsWorkingDeleter) {
    delete_calls = 0;

    auto* plugin = new mock_plugin();
    plugin_deleter rebounded = plugin_deleter(&counting_deleter).rebind();
    rebounded(plugin);

    EXPECT_EQ(delete_calls, 1);
}

TEST(PluginDeleterTest, DestructorDoesNotDelete) {
    delete_calls = 0;
    auto* plugin = new mock_plugin();

    {
        plugin_deleter deleter(&counting_deleter);
    }

    EXPECT_EQ(delete_calls, 0);
    delete plugin;
}


TEST(PluginPtrTest, DestructionInvokesCustomDeleter) {
    delete_calls = 0;

    {
        plugin_ptr owned(new mock_plugin(), plugin_deleter(&counting_deleter));
        ASSERT_NE(owned.get(), nullptr);
        EXPECT_EQ(delete_calls, 0);
    }

    EXPECT_EQ(delete_calls, 1);
}

TEST(PluginPtrTest, AccessorsExposeThePlugin) {
    plugin_ptr owned(new mock_plugin(), plugin_deleter(&counting_deleter));

    EXPECT_TRUE(static_cast<bool>(owned));
    EXPECT_EQ(owned->get_info().name, "mock");
    EXPECT_EQ((*owned).get_info().name, "mock");
    EXPECT_EQ(owned.get()->get_info().version, "0.0.1");
}

TEST(PluginPtrTest, ReleaseSkipsTheDeleter) {
    delete_calls = 0;

    iplugin* raw = nullptr;
    {
        plugin_ptr owned(new mock_plugin(), plugin_deleter(&counting_deleter));
        raw = owned.release();
        EXPECT_EQ(owned.get(), nullptr);
    }

    EXPECT_EQ(delete_calls, 0);
    delete raw;
}

TEST(PluginPtrTest, ResetInvokesTheDeleter) {
    delete_calls = 0;

    plugin_ptr owned(new mock_plugin(), plugin_deleter(&counting_deleter));
    auto* replacement = new mock_plugin();
    owned.reset(replacement);

    EXPECT_EQ(delete_calls, 1);
    EXPECT_EQ(owned.get(), replacement);

    owned.reset();
    EXPECT_EQ(delete_calls, 2);
    EXPECT_EQ(owned.get(), nullptr);
}

TEST(PluginPtrTest, MoveAssignmentPreservesTheDeleter) {
    delete_calls = 0;

    // Regression: the same-type move assignment used to transfer only the pointer,
    // so the target kept its default-constructed deleter (a null function pointer)
    // and destroying it called null. plugin_manager relies on this path when it
    // stores plugins into its map.
    plugin_ptr target;
    {
        plugin_ptr source(new mock_plugin(), plugin_deleter(&counting_deleter));
        target = _NEFORCE move(source);
    }

    EXPECT_EQ(delete_calls, 0);
    ASSERT_NE(target.get(), nullptr);

    target.reset();
    EXPECT_EQ(delete_calls, 1);
}

TEST(PluginPtrTest, NullPointerDoesNotInvokeTheDeleter) {
    delete_calls = 0;

    plugin_ptr owned(static_cast<iplugin*>(nullptr), plugin_deleter(&counting_deleter));
    owned.reset();

    EXPECT_EQ(delete_calls, 0);
    EXPECT_FALSE(static_cast<bool>(owned));
}


TEST_F(PluginManagerTest, InstanceIsASingleton) { EXPECT_EQ(&plugin_manager::instance(), &plugin_manager::instance()); }

TEST_F(PluginManagerTest, GetPluginReturnsNullForUnknownName) {
    EXPECT_EQ(plugin_manager::instance().get_plugin("no_such_plugin_xyz"), nullptr);
}

TEST_F(PluginManagerTest, UnloadUnknownPluginReturnsFalse) {
    EXPECT_FALSE(plugin_manager::instance().unload_plugin("no_such_plugin_xyz"));
}

TEST_F(PluginManagerTest, LoadPluginRegistersInstance) {
    auto& manager = plugin_manager::instance();
    manager.load_plugin(plugin_path().view());

    iplugin* plugin = manager.get_plugin(plugin_name);
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->get_info().name, plugin_name);
    EXPECT_EQ(plugin->get_info().version, "1.0.0");
}

TEST_F(PluginManagerTest, LoadedPluginAppearsInListing) {
    auto& manager = plugin_manager::instance();
    manager.load_plugin(plugin_path().view());

    const vector<string> names = manager.list_plugins();
    bool found = false;
    for (const auto& name: names) {
        if (name == plugin_name) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(PluginManagerTest, LoadingSamePathTwiceThrows) {
    auto& manager = plugin_manager::instance();
    manager.load_plugin(plugin_path().view());

    EXPECT_THROW(manager.load_plugin(plugin_path().view()), system_exception);
}

TEST_F(PluginManagerTest, UnloadPluginRemovesInstance) {
    auto& manager = plugin_manager::instance();
    manager.load_plugin(plugin_path().view());

    EXPECT_TRUE(manager.unload_plugin(plugin_name));
    EXPECT_EQ(manager.get_plugin(plugin_name), nullptr);
    EXPECT_FALSE(manager.unload_plugin(plugin_name));
}

TEST_F(PluginManagerTest, PluginCanBeReloadedAfterUnload) {
    auto& manager = plugin_manager::instance();

    manager.load_plugin(plugin_path().view());
    ASSERT_TRUE(manager.unload_plugin(plugin_name));

    manager.load_plugin(plugin_path().view());
    EXPECT_NE(manager.get_plugin(plugin_name), nullptr);
}

TEST_F(PluginManagerTest, InitializeAndShutdownAllAreSafeWithoutPlugins) {
    auto& manager = plugin_manager::instance();
    manager.shutdown_all();

    EXPECT_NO_THROW(manager.initialize_all());
    EXPECT_NO_THROW(manager.shutdown_all());
    EXPECT_TRUE(manager.list_plugins().empty());
}

TEST_F(PluginManagerTest, InitializeAndShutdownAllDriveLoadedPlugins) {
    auto& manager = plugin_manager::instance();
    manager.load_plugin(plugin_path().view());

    EXPECT_NO_THROW(manager.initialize_all());
    EXPECT_NO_THROW(manager.shutdown_all());
    EXPECT_EQ(manager.get_plugin(plugin_name), nullptr);
}

TEST_F(PluginManagerTest, ShutdownAllIsIdempotent) {
    auto& manager = plugin_manager::instance();
    manager.load_plugin(plugin_path().view());

    manager.shutdown_all();
    EXPECT_NO_THROW(manager.shutdown_all());
    EXPECT_TRUE(manager.list_plugins().empty());
}

TEST_F(PluginManagerTest, LoadPluginWithMissingFileThrows) {
    EXPECT_THROW(plugin_manager::instance().load_plugin("no_such_library_xyz_12345.so"), system_exception);
}

TEST_F(PluginManagerTest, LoadPluginsRejectsMissingDirectory) {
    EXPECT_THROW(plugin_manager::instance().load_plugins("no_such_directory_xyz_12345"), value_exception);
}

TEST_F(PluginManagerTest, LoadPluginsRejectsRegularFile) {
    EXPECT_THROW(plugin_manager::instance().load_plugins(plugin_path()), value_exception);
}

TEST_F(PluginManagerTest, LoadPluginsOnEmptyDirectoryLoadsNothing) {
    const path dir = path::temp_directory_path() / path("neforce_plugin_empty_dir");
    filesystem::remove_all(dir);
    ASSERT_TRUE(filesystem::create_directories(dir));

    EXPECT_EQ(plugin_manager::instance().load_plugins(dir.to_string()), 0U);

    filesystem::remove_all(dir);
}

TEST_F(PluginManagerTest, LoadPluginsIgnoresNonLibraryFiles) {
    const path dir = path::temp_directory_path() / path("neforce_plugin_skip_dir");
    filesystem::remove_all(dir);
    ASSERT_TRUE(filesystem::create_directories(dir));

    ASSERT_TRUE(filesystem::copy(path(plugin_path().view()), dir / path("not_a_plugin.txt")));

    EXPECT_EQ(plugin_manager::instance().load_plugins(dir.to_string()), 0U);

    filesystem::remove_all(dir);
}

TEST_F(PluginManagerTest, LoadPluginsLoadsEveryLibraryInDirectory) {
    if (environment::get("NEFORCE_DARLING") == "1") {
        GTEST_SKIP() << "copied modules cannot resolve their @rpath dependencies under Darling";
    }
    const path dir = path::temp_directory_path() / path("neforce_plugin_load_dir");
    filesystem::remove_all(dir);
    ASSERT_TRUE(filesystem::create_directories(dir));

    const path source(plugin_path().view());
    const path target = dir / source.filename();
    ASSERT_TRUE(filesystem::copy(source, target));

    EXPECT_EQ(plugin_manager::instance().load_plugins(dir.to_string()), 1U);
    EXPECT_NE(plugin_manager::instance().get_plugin(plugin_name), nullptr);

    plugin_manager::instance().shutdown_all();
    filesystem::remove_all(dir);
}
