#ifndef MSTL_PLUGIN_PLUGIN_MANAGER_HPP__
#define MSTL_PLUGIN_PLUGIN_MANAGER_HPP__
#include "MSTL/core/async/mutex.hpp"
#include "MSTL/core/container/unordered_map.hpp"
#include "plugin_entry.hpp"
#include "dynamic_library.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API plugin_manager {
public:
    using library_ptr = unique_ptr<dynamic_library>;

private:
    mutable mutex mutex_{};
    unordered_map<string, plugin_ptr> plugins_{};
    unordered_map<string, library_ptr> libraries_{};
    unordered_map<string, string> plugin_to_library_{};

    plugin_manager() = default;\

public:
    plugin_manager(const plugin_manager&) = delete;
    plugin_manager& operator =(const plugin_manager&) = delete;
    plugin_manager(plugin_manager&&) = delete;
    plugin_manager& operator =(plugin_manager&&) = delete;
    ~plugin_manager();

    static plugin_manager& instance() {
        static plugin_manager manager;
        return manager;
    }

    size_t load_plugins(const string& dir_path);
    void load_plugin(string_view filepath);
    bool unload_plugin(const string& name);

    MSTL_NODISCARD iplugin* get_plugin(const string &name);
    MSTL_NODISCARD vector<string> list_plugins() const;

    void initialize_all();
    void shutdown_all() noexcept;
};

MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_PLUGIN_MANAGER_HPP__
