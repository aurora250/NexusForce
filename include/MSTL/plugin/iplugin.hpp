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
    plugin_deleter() = default;
    explicit plugin_deleter(void(* func)(iplugin*)) : func_(func) {}
    ~plugin_deleter() = default;

    void operator ()(iplugin* p) const {
        if (p) func_(p);
    }
};

using plugin_ptr = unique_ptr<iplugin, plugin_deleter>;

MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_IPLUGIN_HPP__
