#ifndef MSTL_NETWORK_URL_HPP__
#define MSTL_NETWORK_URL_HPP__

/**
 * @file url.hpp
 * @brief URL统一资源定位符解析与构建
 *
 * 此文件提供了URL（统一资源定位符）的解析和构建功能。
 * 支持标准的URL格式解析，包括协议、主机、端口、路径和查询参数。
 */

#include "MSTL/core/interface/iobject.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Network 网络通信
 * @brief 网络通信相关组件
 * @{
 */

/**
 * @struct url
 * @brief URL统一资源定位符
 *
 * 表示一个标准的URL，包含以下组成部分：
 * - scheme：协议类型
 * - host：主机名或IP地址
 * - port：端口号
 * - path：资源路径
 * - query：查询字符串
 *
 * 提供URL的解析和字符串化功能。
 */
struct url : iobject<url> {
    string scheme;      ///< 协议类型
    string host;        ///< 主机名或IP地址
    uint16_t port = 0;  ///< 端口号（0表示使用默认端口）
    string path;        ///< 资源路径
    string query;       ///< 查询字符串

    /**
     * @brief 默认构造函数
     */
    url() = default;

    /**
     * @brief 字符串视图构造函数
     * @param text URL字符串视图
     * @throws network_exception 当URL格式无效时抛出
     */
    explicit url(const string_view text) {
        *this = parse(text);
    }

    /**
     * @brief 字符串构造函数
     * @param text URL字符串
     * @throws network_exception 当URL格式无效时抛出
     */
    explicit url(const string& text)
    : url(text.view()) {}

    /**
     * @brief C字符串构造函数
     * @param text C风格URL字符串
     * @throws network_exception 当URL格式无效时抛出
     */
    explicit url(const char* text)
    : url(string_view(text)) {}

    /**
     * @brief 解析URL字符串
     * @param str URL字符串视图
     * @return 解析后的url对象
     * @throws network_exception 当URL格式无效时抛出
     *
     * 解析标准格式的URL：scheme://host[:port][/path][?query]
     * - 必须包含scheme://前缀
     * - 端口可选，默认根据scheme推断（http:80, https:443）
     * - 路径可选，默认为"/"
     * - 查询可选
     */
    MSTL_NODISCARD static url parse(const string_view str);

    /**
     * @brief 转换为字符串
     * @return URL的标准字符串表示
     *
     * 根据各个部分重新构建URL字符串：
     * - 如果端口与协议默认端口相同，则省略端口部分
     * - 如果路径为空，则使用"/"
     */
    MSTL_NODISCARD string to_string() const;
};

/** @} */ // Network

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_URL_HPP__
