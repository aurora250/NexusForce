#ifndef NEFORCE_NETWORK_HTTP_HTTP_ROUTER_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_ROUTER_HPP__

/**
 * @file http_router.hpp
 * @brief HTTP路由器实现
 *
 * 此文件提供了HTTP路由器的实现，支持路径匹配、参数提取、
 * 中间件链和异常处理等功能。
 */

#include "NeForce/core/string/regex.hpp"
#include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class http_router
 * @brief HTTP路由器类
 *
 * 提供HTTP请求路由功能，将请求分发到对应的处理函数。
 * 支持路径参数、正则表达式匹配和中间件链。
 *
 * 主要功能：
 * - RESTful风格路由注册
 * - 路径参数提取
 * - 通配符匹配
 * - 正则表达式路由支持
 * - 中间件链支持
 * - 自定义异常处理器
 * - 大小写敏感/不敏感路由
 *
 * 使用示例：
 * @code
 * http_router router;
 *
 * // 静态路由
 * router.get("/", [](http_request& req, http_response& res) {
 *     res.body = "Hello World";
 * });
 *
 * // 路径参数
 * router.get("/users/:id", [](http_request& req, http_response& res) {
 *     string id = req.parameter("id");
 *     res.body = "User ID: " + id;
 * });
 *
 * // 通配符
 * router.get("/files/*", [](http_request& req, http_response& res) {
 *     string path = req.parameter("*");
 *     // 处理文件请求
 * });
 *
 * // 正则表达式路由
 * router.route("GET", "/user/([0-9]+)", [](http_request& req, http_response& res) {
 *     string id = req.parameter("0");
 * });
 *
 * // 添加中间件
 * router.use(make_unique<logging_filter>());
 * router.use(make_unique<cors_filter>());
 *
 * // 自定义404处理器
 * router.set_not_found_handler([](http_request& req, http_response& res) {
 *     res.body = "Custom 404 Page";
 * });
 *
 * // 处理请求
 * auto response = router.handle_request(request);
 * @endcode
 */
class NEFORCE_API http_router {
public:
    using http_handler_t = function<void(http_request&, http_response&)>;                        ///< HTTP处理器类型
    using exception_handler_t = function<void(http_request&, http_response&, const exception&)>; ///< 异常处理器类型

private:
    struct route_entry {
        string pattern;                ///< 路由模式
        http_handler_t handler;        ///< 处理器函数
        optional<regex> regex_pattern; ///< 正则表达式模式
        vector<string> param_names;    ///< 参数名称列表
        bool is_regex = false;         ///< 是否为正则路由
    };

    unordered_map<string, vector<route_entry>> routes_; ///< HTTP方法到路由条目的映射
    http_filter_chain middleware_chain_;                ///< 中间件链

    http_handler_t not_found_handler_;          ///< 404处理器
    http_handler_t method_not_allowed_handler_; ///< 405处理器
    exception_handler_t exception_handler_;     ///< 异常处理器

public:
    bool case_sensitive = true;  ///< 是否大小写敏感
    bool strict_routing = false; ///< 是否严格匹配尾部斜杠

private:
    route_entry* find_handler(const http_method& method, const string& path, http_request& request);

    void setup_default_handlers();

public:
    /**
     * @brief 构造函数
     *
     * 初始化默认的404、405和异常处理器。
     */
    http_router();

    ~http_router() = default;

    http_router(const http_router&) = delete;
    http_router& operator=(const http_router&) = delete;
    http_router(http_router&&) noexcept = default;
    http_router& operator=(http_router&&) noexcept = default;

    void get(const string& path, http_handler_t handler);     ///< GET请求
    void post(const string& path, http_handler_t handler);    ///< POST请求
    void put(const string& path, http_handler_t handler);     ///< PUT请求
    void del(const string& path, http_handler_t handler);     ///< DELETE请求
    void head(const string& path, http_handler_t handler);    ///< HEAD请求
    void options(const string& path, http_handler_t handler); ///< OPTIONS请求
    void trace(const string& path, http_handler_t handler);   ///< TRACE请求
    void connect(const string& path, http_handler_t handler); ///< CONNECT请求
    void patch(const string& path, http_handler_t handler);   ///< PATCH请求

    /**
     * @brief 同时注册GET和POST
     * @param path 路由路径
     * @param handler 处理器
     */
    void get_post(const string& path, http_handler_t handler);

    /**
     * @brief 同时注册POST和DELETE
     * @param path 路由路径
     * @param handler 处理器
     */
    void post_delete(const string& path, http_handler_t handler);

    /**
     * @brief 注册所有HTTP方法
     * @param path 路由路径
     * @param handler 处理器
     */
    void all(const string& path, http_handler_t handler);

    /**
     * @brief 通用路由注册
     * @param method HTTP方法
     * @param path 路由路径
     * @param handler 处理器
     *
     * 路由模式语法：
     * - `/users/:id` - 路径参数，匹配`/users/123`，提取id=123
     * - `/files/*` - 通配符，匹配`/files/a/b/c`，提取*=a/b/c
     * - `/user/([0-9]+)` - 正则表达式，匹配数字ID
     */
    void route(const http_method& method, const string& path, const http_handler_t& handler);

    /**
     * @brief 添加中间件
     * @param middleware 中间件对象（转移所有权）
     */
    void use(unique_ptr<http_filter> middleware) { middleware_chain_.add_filter(_NEFORCE move(middleware)); }

    /**
     * @brief 获取中间件链引用
     * @return 中间件链引用
     */
    http_filter_chain& middleware_chain() noexcept { return middleware_chain_; }

    /**
     * @brief 设置404处理器
     * @param handler 处理器
     */
    void set_not_found_handler(http_handler_t handler) { not_found_handler_ = _NEFORCE move(handler); }

    /**
     * @brief 设置405处理器
     * @param handler 处理器
     */
    void set_method_not_allowed_handler(http_handler_t handler) {
        method_not_allowed_handler_ = _NEFORCE move(handler);
    }

    /**
     * @brief 设置异常处理器
     * @param handler 处理器
     */
    void set_exception_handler(exception_handler_t handler) { exception_handler_ = _NEFORCE move(handler); }

    /**
     * @brief 处理HTTP请求
     * @param request HTTP请求对象
     * @return HTTP响应对象
     *
     * 处理流程：
     * 1. 执行预过滤器
     * 2. 执行核心过滤器
     * 3. 查找匹配的路由处理器
     * 4. 执行处理器
     * 5. 执行后过滤器
     * 6. 返回响应
     */
    http_response handle_request(http_request& request);

    /**
     * @brief 检查是否存在指定路由
     * @param method HTTP方法
     * @param path 路由路径
     * @return 存在返回true
     */
    NEFORCE_NODISCARD bool has_route(const http_method& method, const string& path) const;

    /**
     * @brief 获取路由总数
     * @return 路由数量
     */
    NEFORCE_NODISCARD size_t route_count() const noexcept;

    /**
     * @brief 清空所有路由
     */
    void clear_routes() noexcept { routes_.clear(); }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_ROUTER_HPP__
