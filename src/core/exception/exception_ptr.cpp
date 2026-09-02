#include <NeForce/compress/lz4_compress.hpp>
#include <NeForce/compress/zlib_compress.hpp>
#include <NeForce/core/async/future.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/exception/exception.hpp>
#include <NeForce/core/exception/exception_ptr.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/exception/system_exception.hpp>
#include <NeForce/core/file/env/env_value.hpp>
#include <NeForce/core/file/ini/ini_value.hpp>
#include <NeForce/core/file/json/json_value.hpp>
#include <NeForce/core/file/toml/toml_value.hpp>
#include <NeForce/core/file/yaml/yaml_value.hpp>
#include <NeForce/core/serialize/serialize_exception.hpp>
#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/system/cmdline.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/daemon.hpp>
#include <NeForce/core/system/dynamic_library.hpp>
#include <NeForce/core/system/locale.hpp>
#include <NeForce/core/system/pipe.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/system/registry_key.hpp>
#include <NeForce/core/system/share_memory.hpp>
#include <NeForce/core/utility/any.hpp>
#include <NeForce/core/utility/expected.hpp>
#include <NeForce/core/utility/optional.hpp>
#include <NeForce/db/db_config.hpp>
#include <NeForce/network/dns/dns_message.hpp>
#include <NeForce/network/ftp/ftp_protocol.hpp>
#include <NeForce/network/http/http_constants.hpp>
#include <NeForce/network/smtp_socket.hpp>
#include <NeForce/network/socket_base.hpp>
#include <NeForce/network/ssl/ssl_exception.hpp>
#include <NeForce/network/util/network_exception.hpp>
#include <exception>
NEFORCE_BEGIN_NAMESPACE__

#define __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(EX) \
    catch (const EX& e) {                       \
        return make_exception_ptr(e);           \
    }

exception_ptr current_exception() noexcept {
    const std::exception_ptr std_ep = std::current_exception();
    if (!std_ep) {
        return {};
    }

    try {
        std::rethrow_exception(std_ep);
    }
    // memory family
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(anycast_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(optional_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(iterator_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(typecast_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(memory_exception)
    // device family
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(console_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(device_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(file_exception)
    // network family
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(dns_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(ftp_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(http_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(smtp_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(socket_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(ssl_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(network_exception)
    // system family
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(daemon_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(dynamic_library_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(locale_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(pipe_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(process_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(registry_key_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(share_memory_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(system_exception)
    // value family
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(missing_required_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(type_mismatch_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(deserialize_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(serialize_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(cmdline_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(env_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(ini_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(json_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(regex_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(toml_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(yaml_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(math_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(value_exception)
    // thirdparty family
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(database_typecast_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(database_stmt_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(database_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(lz4_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(zlib_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(thirdparty_exception)
    // direct exception family
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(expected_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(future_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(thread_exception)
    __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR(exception)
    catch (const std::exception& e) {
        return make_exception_ptr(value_exception(e.what()));
    }
    catch (...) {
        return make_exception_ptr(value_exception("Unknown exception"));
    }
}

#undef __NEFORCE_EXPAND_MAKE_EXCEPTION_PTR


void rethrow_exception(const exception_ptr& p) {
    if (!p || p.ecb_ == nullptr || !p.ecb_->wrapper) {
        terminate();
    }
    p.ecb_->wrapper->rethrow();
    unreachable(); // rethrow not return, help GCC to identify
}

NEFORCE_END_NAMESPACE__
