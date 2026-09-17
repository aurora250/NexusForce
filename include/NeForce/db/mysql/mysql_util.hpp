#ifndef NEFORCE_DB_MYSQL_MYSQL_UTIL_HPP__
#define NEFORCE_DB_MYSQL_MYSQL_UTIL_HPP__
#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @defgroup MySQL MySQL
 * @brief MySQL数据库相关功能
 * @{
 */

/**
 * @enum mysql_option
 * @brief MYSQL 连接选项
 */
enum class mysql_option : int32_t {
    connect_timeout,              // 连接超时时间，单位秒
    compress,                     // 是否启用客户端/服务器协议压缩
    named_pipe,                   // 是否使用 Windows 命名管道连接
    init_command,                 // 连接建立后自动执行的 SQL 命令
    read_default_file,            // 读取指定的 MySQL 默认配置文件
    read_default_group,           // 从默认配置文件中读取的组名
    set_charset_dir,              // 字符集文件所在目录
    set_charset_name,             // 设置连接使用的字符集名称
    local_infile,                 // 是否允许使用 LOAD DATA LOCAL INFILE
    protocol,                     // 指定连接协议，如 TCP、Socket、Pipe 等
    shared_memory_base_name,      // Windows 共享内存连接使用的共享内存名
    read_timeout,                 // 读超时时间，单位秒
    write_timeout,                // 写超时时间，单位秒
    use_result,                   // 是否使用 mysql_use_result 模式，不缓存全部结果集
    report_data_truncation,       // 是否报告数据截断错误
    reconnect,                    // 连接断开时是否自动重连
    plugin_dir,                   // 客户端插件目录
    default_auth,                 // 默认认证插件名称
    bind,                         // 绑定客户端本地地址
    ssl_key,                      // SSL 客户端私钥文件
    ssl_cert,                     // SSL 客户端证书文件
    ssl_ca,                       // SSL CA 证书文件
    ssl_capath,                   // SSL CA 证书目录
    ssl_cipher,                   // SSL 加密算法套件
    ssl_crl,                      // SSL 证书吊销列表文件
    ssl_crlpath,                  // SSL 证书吊销列表目录
    connect_attr_reset,           // 清空连接属性
    connect_attr_add,             // 添加连接属性键值对
    connect_attr_delete,          // 删除连接属性
    server_public_key,            // 用于 RSA 密钥交换的服务器公钥
    enable_cleartext_plugin,      // 是否启用明文认证插件
    can_handle_expired_passwords, // 客户端是否能处理已过期密码
    max_allowed_packet,           // 最大允许的数据包大小
    net_buffer_length,            // 网络缓冲区初始大小
    tls_version,                  // 允许使用的 TLS 版本
    ssl_mode,                     // SSL/TLS 使用模式，如 DISABLED、PREFERRED、REQUIRED 等
    get_server_public_key,        // 是否请求获取服务器公钥
    retry_count,                  // 连接重试次数
    optional_resultset_metadata,  // 是否请求可选的结果集元数据
    ssl_fips_mode,                // SSL FIPS 模式设置
    tls_ciphersuites,             // TLS 密码套件列表
    compression_algorithms,       // 允许使用的压缩算法
    zstd_compression_level,       // Zstandard 压缩级别
    load_data_local_dir,          // LOAD DATA LOCAL 允许读取的本地目录
    user_password,                // 用户密码相关设置
    ssl_session_data              // SSL/TLS 会话数据，用于会话复用
};

// NOLINTNEXTLINE(readability-enum-initial-value)
enum class mysql_column_type {
    decimal = 0,
    tiny = 1,
    short_ = 2,
    long_ = 3,
    float_ = 4,
    double_ = 5,
    null_ = 6,
    timestamp = 7,
    longlong = 8,
    int24 = 9,
    date = 10,
    time = 11,
    datetime = 12,
    year = 13,
    varchar = 14,
    bit = 15,
    timestamp2 = 16,
    invalid = 243,
    bool_ = 244,
    json = 245,
    newdecimal = 246,
    enum_ = 247,
    set_ = 248,
    tiny_blob = 249,
    medium_blob = 250,
    long_blob = 251,
    blob = 252,
    var_string = 253,
    string_ = 254,
    geometry = 255
};

/** @} */ // MySQL

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DB_MYSQL_MYSQL_UTIL_HPP__
