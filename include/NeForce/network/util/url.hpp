#ifndef NEFORCE_NETWORK_UTIL_URL_HPP__
#define NEFORCE_NETWORK_UTIL_URL_HPP__

/**
 * @file url.hpp
 * @brief URL统一资源定位符解析与构建
 *
 * 此文件提供了URL（统一资源定位符）的解析和构建功能。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/network/util/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup NetworkUtil 网络通信工具
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
 * 提供了URL的解析和构建功能。
 *
 * URL是互联网上标识和定位资源的标准方式，由多个组成部分构成。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下URL规范与编码标准：
 *
 * **URL语法标准：**
 * - **IETF STD 66 / RFC 3986**：统一资源标识符（URI）通用语法
 *   https://www.rfc-editor.org/rfc/rfc3986.html
 * - **IETF RFC 3987**：国际化资源标识符（IRI）
 *   https://www.rfc-editor.org/rfc/rfc3987.html
 *
 * **WHATWG URL标准：**
 * - **WHATWG URL Living Standard**：URL标准（浏览器实现参考）
 *   https://url.spec.whatwg.org/
 *
 * **URL编码标准：**
 * - **IETF RFC 3986 §2.1**：百分号编码（Percent-Encoding）
 *   https://www.rfc-editor.org/rfc/rfc3986.html#section-2.1
 *
 * **HTML表单编码标准：**
 * - **WHATWG HTML Living Standard §4.10.21**：application/x-www-form-urlencoded 序列化
 *   https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#url-encoded-form-data
 * - **W3C HTML 4.01 Specification §17.13.4**：表单内容类型（历史参考）
 *   https://www.w3.org/TR/html401/interact/forms.html#h-17.13.4
 *
 * **URI与URL区分：**
 * - **IETF RFC 3305**：统一资源标识符（URI）、统一资源定位符（URL）和统一资源名称（URN）术语澄清
 *   https://www.rfc-editor.org/rfc/rfc3305.html
 *
 * **相关安全标准：**
 * - **IETF RFC 6874**：URI中IPv6区域标识符的表示
 *   https://www.rfc-editor.org/rfc/rfc6874.html
 *
 * @section url_structure URL结构定义
 * 根据RFC 3986 §3，URL（URI的定位子集）的通用语法为：
 *
 * ```
 * scheme ":" hier-part [ "?" query ] [ "#" fragment ]
 * ```
 *
 * 其中hier-part对于基于权限的URL为：
 * ```
 * "//" authority path-abempty
 * ```
 *
 * authority部分为：
 * ```
 * [ userinfo "@" ] host [ ":" port ]
 * ```
 *
 * @section url_components URL组成部分
 * 本实现解析和存储以下URL组成部分：
 *
 * | 组成部分   | RFC 3986引用 | 类型      | 示例                           | 说明                     |
 * |------------|--------------|-----------|--------------------------------|--------------------------|
 * | scheme     | §3.1         | string    | "https"                        | 协议类型（不区分大小写） |
 * | host       | §3.2.2       | string    | "example.com"                  | 主机名或IP地址           |
 * | port       | §3.2.3       | ports     | 443                            | 端口号                   |
 * | path       | §3.3         | string    | "/path/to/resource"            | 资源路径                 |
 * | query      | §3.4         | string    | "key=value&foo=bar"            | 查询字符串               |
 * | fragment   | §3.5         | string    | "section1"                     | 片段标识符               |
 *
 * @section percent_encoding 百分号编码规则
 * 根据RFC 3986 §2.1，百分号编码规则：
 *
 * **保留字符**（在特定上下文中具有特殊含义）：
 * ```
 * ! * ' ( ) ; : @ & = + $ , / ? % # [ ]
 * ```
 *
 * **非保留字符**（可直接使用无需编码）：
 * ```
 * A-Z a-z 0-9 - . _ ~
 * ```
 *
 * **编码格式**：`%` + 两个十六进制数字（大写），如空格编码为 `%20`。
 *
 * @section form_encoding 表单编码规则
 * 根据WHATWG HTML标准§4.10.21，application/x-www-form-urlencoded编码规则：
 *
 * | 规则           | 说明                                           |
 * |----------------|------------------------------------------------|
 * | 空格编码       | 空格替换为 `+`（而非 `%20`）                   |
 * | 保留字符       | `* - . _` 不编码，其他保留字符进行百分号编码   |
 * | 键值对分隔     | `&` 分隔多个参数                               |
 * | 键值赋值       | `=` 连接键和值                                 |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | Scheme解析        | 大小写不敏感，存储为小写                  |
 * | 端口推断          | 省略时根据scheme自动推断默认端口          |
 * | 默认路径          | 空路径自动设为"/"                         |
 * | IPv6支持          | 主机部分支持[::]格式                      |
 * | 编码字符集        | UTF-8（RFC 3986推荐）                     |
 * | 无效URL处理       | 抛出network_exception异常                 |
 *
 * @note 本实现兼容RFC 3986的URI语法和WHATWG的URL标准。
 *
 * @warning 根据RFC 3986 §7.3，百分号编码中未保留字符不应被编码，否则可能影响与旧实现的兼容性。
 *          userinfo部分（username:password@host）已被RFC 3986废弃，本实现不解析该部分。
 *
 * @see https://www.rfc-editor.org/rfc/rfc3986
 * @see https://url.spec.whatwg.org/
 */
struct NEFORCE_API url : iobject<url> {
    string scheme;   ///< 协议类型
    string host;     ///< 主机名或IP地址
    ports port;      ///< 端口号
    string path;     ///< 资源路径
    string query;    ///< 查询字符串
    string fragment; ///< 片段标识符

    /**
     * @brief 默认构造函数
     */
    url() = default;

    url(url&& other) = default;
    url& operator=(url&& other) = default;

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
    NEFORCE_NODISCARD static string encode(string_view str, bool encode_slash = true);

    /**
     * @brief URL解码字符串
     * @param str 待解码的字符串
     * @return 解码后的字符串，如果格式无效返回none
     *
     * 将%XX格式转换回原始字符
     */
    NEFORCE_NODISCARD static optional<string> decode(string_view str);

    /**
     * @brief 表单编码（application/x-www-form-urlencoded）
     * @param str 待编码的字符串
     * @return 编码后的字符串
     *
     * 符合WHATWG HTML标准的表单编码规则：
     * - 空格编码为'+'
     * - 非保留字符不编码
     * - 其他字符进行百分号编码
     */
    NEFORCE_NODISCARD static string encode_form(string_view str);

    /**
     * @brief 宽容解码
     * @param str 待解码的字符串
     * @return 解码后的字符串
     *
     * 尝试解码，如果遇到无效的%XX格式，保留原字符。
     * 适用于处理不规范的URL编码。
     */
    NEFORCE_NODISCARD static string decode_tolerant(string_view str);

    /**
     * @brief 解析查询字符串
     * @param query 查询字符串
     * @param params 输出参数映射
     *
     * 将"key1=value1&key2=value2"格式的查询字符串解析为键值对。
     * 键和值会自动进行URL解码。
     */
    static void parse_query(string_view query, unordered_map<string, string>& params);

    /**
     * @brief 构建查询字符串
     * @param params 参数映射
     * @return 查询字符串
     *
     * 将键值对转换为"key1=value1&key2=value2"格式的查询字符串。
     * 键和值会自动进行表单编码。
     */
    NEFORCE_NODISCARD static string build_query(const unordered_map<string, string>& params);
};

/** @} */ // NetworkUtil

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UTIL_URL_HPP__
