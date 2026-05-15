#ifndef NEFORCE_NETWORK_HTTP_HTTP_SESSION_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_SESSION_HPP__

/**
 * @file http_session.hpp
 * @brief HTTP Cookie和会话管理
 *
 * 此文件提供了HTTP Cookie的序列化和Session管理功能。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/http/http_constants.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct http_cookie
 * @brief HTTP Cookie结构
 *
 * 表示一个HTTP Cookie，包含名称、值以及各种属性。
 * 支持解析Set-Cookie头和序列化为Set-Cookie头格式。
 *
 * Cookie属性说明：
 * - Domain: 可访问该Cookie的域名
 * - Path: 可访问该Cookie的路径
 * - Max-Age: Cookie有效期，优先级高于Expires
 * - Expires: Cookie过期时间
 * - Secure: 仅在HTTPS连接中传输
 * - HttpOnly: 禁止JavaScript访问
 * - SameSite: 跨站请求策略（Strict/Lax/None）
 *
 * 使用示例：
 * @code
 * // 解析Set-Cookie头
 * auto cookie = http_cookie::parse("sessionId=abc123; Path=/; HttpOnly; Max-Age=3600");
 *
 * // 创建新Cookie
 * http_cookie new_cookie;
 * new_cookie.name = "token";
 * new_cookie.value = "xyz789";
 * new_cookie.path = "/api";
 * new_cookie.secure = true;
 * new_cookie.http_only = true;
 * new_cookie.set_expires_from_now(7200_s);  // 2小时后过期
 *
 * // 生成Set-Cookie头
 * string set_cookie_header = new_cookie.to_string();
 *
 * // 验证Cookie
 * if (cookie.is_valid() && !cookie.is_expired()) {
 *     // 使用Cookie
 * }
 * @endcode
 */
struct NEFORCE_API http_cookie : iobject<http_cookie> {
    http_cookie_name name;                ///< Cookie名称
    string value;                         ///< Cookie值
    string domain;                        ///< Domain
    string path{"/"};                     ///< Path
    seconds max_age{-1};                  ///< Max-Age（-1表示会话Cookie）
    bool secure{false};                   ///< Secure（仅HTTPS）
    bool http_only{false};                ///< HttpOnly（禁止JS）
    string same_site{http_key::Strict()}; ///< SameSite
    datetime expires;                     ///< Expires

    /**
     * @brief 解析Set-Cookie头
     * @param header Set-Cookie头的值
     * @return 解析出的Cookie对象
     *
     * 解析Set-Cookie头字符串，提取Cookie名称、值和属性。
     */
    NEFORCE_NODISCARD static http_cookie parse(string_view header);

    /**
     * @brief 解析Set-Cookie头（带默认值）
     * @param header Set-Cookie头的值
     * @param default_domain 默认Domain值
     * @param default_path 默认Path值
     * @return 解析出的Cookie对象
     *
     * 解析Set-Cookie头，如果未指定Domain或Path，使用提供的默认值。
     */
    NEFORCE_NODISCARD static http_cookie parse(string_view header, string default_domain, string default_path);

    /**
     * @brief 序列化为Set-Cookie头
     * @return Set-Cookie头字符串
     *
     * 将Cookie对象转换为Set-Cookie头的格式。
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 检查Cookie是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept;

    /**
     * @brief 检查Cookie是否已过期
     * @return 已过期返回true
     *
     * 根据Max-Age或Expires判断Cookie是否已过期。
     * 会话Cookie（max_age = -1）永不过期。
     */
    NEFORCE_NODISCARD bool is_expired() const noexcept;

    /**
     * @brief 设置从当前时间开始计算的过期时间
     * @param sec 有效期
     *
     * 设置Expires为当前时间 + seconds。
     * 同时设置max_age = seconds。
     */
    void set_expires_from_now(seconds sec);
};

/**
 * @struct http_session
 * @brief HTTP会话结构
 *
 * 表示一个服务器端HTTP会话，用于存储用户会话数据。
 * 每个会话有唯一的ID，可以存储键值对数据。
 *
 * 使用示例：
 * @code
 * // 创建新会话
 * http_session session;
 * session.id = generate_session_id();
 * session.max_age = 3600;  // 1小时
 *
 * // 存储用户数据
 * session["user_id"] = "12345";
 * session.set("username", "john_doe");
 *
 * // 读取数据
 * if (session.contains("user_id")) {
 *     string user_id = session.get("user_id");
 * }
 *
 * // 更新访问时间
 * session.touch();
 *
 * // 检查会话是否过期（30分钟无活动）
 * if (session.expired(1800)) {
 *     session.invalidate();
 * }
 *
 * // 会话无效化（登出）
 * session.invalidate();
 * @endcode
 */
struct NEFORCE_API http_session : istringify<http_session> {
    string id;                             ///< 会话唯一标识符
    unordered_map<string, string> data;    ///< 会话数据存储
    datetime last_access{datetime::now()}; ///< 最后访问时间
    datetime create_time{datetime::now()}; ///< 创建时间
    seconds max_age{1800};                 ///< 最大空闲时间，默认30分钟
    bool is_new = true;                    ///< 是否为新创建的会话
    bool invalidated = false;              ///< 是否已无效化

    /**
     * @brief 下标操作符
     * @param key 键
     * @return 值的引用
     *
     * 如果键不存在，会自动创建空字符串条目。
     * 访问时会自动更新last_access。
     */
    NEFORCE_NODISCARD string& operator[](const string& key);

    /**
     * @brief 获取值
     * @param key 键
     * @return 值的字符串视图，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view get(const string& key) const;

    /**
     * @brief 设置值
     * @param key 键
     * @param value 值
     */
    void set(const string& key, string value);

    /**
     * @brief 删除键值对
     * @param key 键
     * @return 成功删除返回true
     */
    bool remove(const string& key);

    /**
     * @brief 清空所有会话数据
     */
    void clear();

    /**
     * @brief 检查是否包含键
     * @param key 键
     * @return 存在返回true
     */
    NEFORCE_NODISCARD bool contains(const string& key) const noexcept;

    /**
     * @brief 无效化会话
     *
     * 标记会话为无效，通常用于用户登出。
     * 不会立即清除数据，但is_valid()会返回false。
     */
    void invalidate() noexcept;

    /**
     * @brief 更新最后访问时间
     *
     * 将last_access更新为当前时间，并将is_new设为false。
     * 在每次请求处理时应调用此方法。
     */
    void touch() noexcept;

    /**
     * @brief 检查会话是否有效
     * @return 有效返回true
     *
     * 会话无效的条件：
     * - 已调用invalidate()
     * - 超过max_age无活动
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept;

    /**
     * @brief 检查会话是否已过期
     * @param max_inactive 最大无活动时间，0表示使用max_age
     * @return 已过期返回true
     */
    NEFORCE_NODISCARD bool expired(seconds max_inactive = 0_s) const noexcept;

    /**
     * @brief 获取会话年龄
     * @return 从创建到现在的秒数
     */
    NEFORCE_NODISCARD seconds age() const noexcept { return seconds{datetime::now() - create_time}; }

    /**
     * @brief 获取空闲时间
     * @return 从最后访问到现在的秒数
     */
    NEFORCE_NODISCARD seconds idle_time() const noexcept { return seconds{datetime::now() - last_access}; }

    /**
     * @brief 序列化为字符串
     * @return 会话信息的字符串表示
     */
    NEFORCE_NODISCARD string to_string() const;
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_SESSION_HPP__
