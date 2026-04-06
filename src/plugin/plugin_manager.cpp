#include <NeForce/core/file/path.hpp>
#include <NeForce/plugin/plugin_manager.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    bool is_plugin_file(const string_view p) {
        const auto ext = path::extension(p);
#ifdef NEFORCE_PLATFORM_WINDOWS
        return (ext == ".dll");
#else
        return (ext == ".so");
#endif
    }
} // namespace


plugin_manager::~plugin_manager() { shutdown_all(); }

size_t plugin_manager::load_plugins(const string& pth) {
    size_t count = 0;

    if (!path::exists(pth) || !path::is_directory(pth)) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid plugin directory"));
    }

    const path pths(pth);

    for (const auto& entry: pths) {
        if (is_plugin_file(entry)) {
            load_plugin(entry);
            ++count;
        }
    }
    return count;
}

void plugin_manager::load_plugin(const string_view pth) {
    lock<mutex> lock(mutex_);

    if (libraries_.count(pth)) {
        NEFORCE_THROW_EXCEPTION(system_exception("Plugin already loaded"));
    }

    auto lib = make_unique<dynamic_library>(pth);

    const auto create_func = lib->to_symbol<iplugin* (*)()>(NEFORCE_PLUGIN_CREATE_FUNC);
    const auto destroy_func = lib->to_symbol<void (*)(iplugin*)>(NEFORCE_PLUGIN_DESTROY_FUNC);

    iplugin* raw_ptr = create_func();
    if (!raw_ptr) {
        NEFORCE_THROW_EXCEPTION(system_exception("Plugin creation returned null"));
    }

    plugin_deleter deleter(destroy_func);
    plugin_ptr plugin(raw_ptr, move(deleter));

    const string& name = plugin->get_info().name;
    if (plugins_.count(name)) {
        NEFORCE_THROW_EXCEPTION(system_exception("Plugin already exists"));
    }

    const string lib_path = pth;
    libraries_[lib_path] = move(lib);
    plugin_to_library_[name] = move(lib_path);
    plugins_[name] = move(plugin);
}

bool plugin_manager::unload_plugin(const string& name) {
    lock<mutex> lock(mutex_);
    const auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return false;
    }

    it->second->shutdown();
    plugins_.erase(it);

    const auto lib_it = plugin_to_library_.find(name);
    if (lib_it != plugin_to_library_.end()) {
        libraries_.erase(lib_it->second);
        plugin_to_library_.erase(lib_it);
    }
    return true;
}

iplugin* plugin_manager::get_plugin(const string& name) {
    lock<mutex> lock(mutex_);
    const auto it = plugins_.find(name);
    return (it != plugins_.end()) ? it->second.get() : nullptr;
}

vector<string> plugin_manager::list_plugins() const {
    lock<mutex> lock(mutex_);
    vector<string> names;
    names.reserve(plugins_.size());
    for (const auto& pair: plugins_) {
        names.push_back(pair.first);
    }
    return names;
}

void plugin_manager::initialize_all() {
    lock<mutex> lock(mutex_);
    for (const auto& pair: plugins_) {
        pair.second->initialize();
    }
}

void plugin_manager::shutdown_all() noexcept {
    lock<mutex> lock(mutex_);
    for (const auto& pair: plugins_) {
        try {
            pair.second->shutdown();
            // NOLINTNEXTLINE(bugprone-empty-catch)
        } catch (...) {
            // ignore
        }
    }
    plugins_.clear();
    libraries_.clear();
    plugin_to_library_.clear();
}

NEFORCE_END_NAMESPACE__
