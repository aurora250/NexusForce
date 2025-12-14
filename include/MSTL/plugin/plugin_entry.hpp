#ifndef MSTL_PLUGIN_PLUGIN_ENTRY_HPP__
#define MSTL_PLUGIN_PLUGIN_ENTRY_HPP__
#include "iplugin.hpp"
MSTL_BEGIN_NAMESPACE__

#define MSTL_PLUGIN_CREATE_FUNC "create_plugin"
#define MSTL_PLUGIN_DESTROY_FUNC "destroy_plugin"

extern "C" {
    iplugin* create_plugin();
    void destroy_plugin(iplugin* p);
}

MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_PLUGIN_ENTRY_HPP__
