#ifndef MSTL_PLUGIN_IPLUGIN_HPP__
#define MSTL_PLUGIN_IPLUGIN_HPP__
#include "MSTL/core/string/string.hpp"
#include "MSTL/core/memory/unique_ptr.hpp"
MSTL_BEGIN_NAMESPACE__

struct plugin_info {
    string name;
    string version;
    string author;
    string description;
    string library_path;
};

struct iplugin {
    virtual ~iplugin() = default;
    virtual const plugin_info& get_info() const = 0;
    virtual void initialize() = 0;
    virtual void execute() = 0;
    virtual void shutdown() = 0;
};

struct plugin_deleter {
private:
    void(* func_)(iplugin*) = nullptr;

public:
    plugin_deleter() noexcept = default;
    ~plugin_deleter() = default;

    explicit plugin_deleter(void(* func)(iplugin*)) noexcept : func_(func) {}

    plugin_deleter(const plugin_deleter&) = delete;
    plugin_deleter& operator =(const plugin_deleter&) = delete;
    plugin_deleter(plugin_deleter&& pd) noexcept : func_(pd.func_) {
        pd.func_ = nullptr;
    }
    plugin_deleter& operator =(plugin_deleter&& pd) noexcept {
        func_ = pd.func_;
        pd.func_ = nullptr;
        return *this;
    }

    void operator ()(iplugin* p) const {
        if (p) func_(p);
    }

    plugin_deleter rebind() && noexcept {
        return plugin_deleter(move(*this));
    }
};

using plugin_ptr = unique_ptr<iplugin, plugin_deleter>;

MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_IPLUGIN_HPP__
