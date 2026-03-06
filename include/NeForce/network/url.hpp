#ifndef NEFORCE_NETWORK_URL_HPP__
#define NEFORCE_NETWORK_URL_HPP__

/**
 * @file url.hpp
 * @brief URL统一资源定位符解析与构建
 *
 * 此文件提供了URL（统一资源定位符）的解析和构建功能。
 */

#include "NeForce/core/interface/iobject.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

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
 * - fragment：片段标识符
 *
 * 提供URL的解析、字符串化、编码和解码功能。
 */
struct url : iobject<url> {
    string scheme;      ///< 协议类型
    string host;        ///< 主机名或IP地址
    uint16_t port = 0;  ///< 端口号（0表示使用默认端口）
    string path;        ///< 资源路径
    string query;       ///< 查询字符串
    string fragment;    ///< 片段标识符

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
        url tmp;
        if (!tmp.try_parse(text)) {
            throw_exception(network_exception("Invalid URL format"));
        }
        *this = _NEFORCE move(tmp);
    }

    /**
     * @brief 验证URL是否有效
     * @return 如果URL有效返回true，否则返回false
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept;

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
     * - 查询和片段可选
     */
    NEFORCE_NODISCARD static url parse(string_view str);

    /**
     * @brief 转换为字符串
     * @return URL的标准字符串表示
     *
     * 根据各个部分重新构建URL字符串：
     * - 如果端口与协议默认端口相同，则省略端口部分
     * - 如果路径为空，则使用"/"
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief URL编码字符串
     * @param str 待编码的字符串
     * @param encode_slash 是否编码斜杠字符
     * @return 编码后的字符串
     *
     * 将特殊字符转换为%XX格式
     */
    NEFORCE_NODISCARD static string encode(string_view str, bool encode_slash = true) noexcept;

    /**
     * @brief URL解码字符串
     * @param str 待解码的字符串
     * @return 解码后的字符串，如果格式无效返回none
     *
     * 将%XX格式转换回原始字符
     */
    NEFORCE_NODISCARD static optional<string> decode(string_view str) noexcept;

    /**
     * @brief 获取默认端口号
     * @param scheme 协议名称
     * @return 默认端口号，未知协议返回0
     */
    NEFORCE_NODISCARD static uint16_t default_port(string_view scheme) noexcept;

    NEFORCE_NODISCARD static string_view default_scheme(uint16_t port, bool is_ws = false) noexcept;
};

/** @} */ // Network

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_URL_HPP__
