#ifndef NEFORCE_NETWORK_HTTP_HTTP_SECURITY_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_SECURITY_HPP__

/**
 * @file http_security.hpp
 * @brief HTTP安全头过滤器
 *
 * 提供自动注入HTTP安全响应头的过滤器，用于防护常见的Web攻击。
 * 支持的header：HSTS, X-Frame-Options, X-Content-Type-Options,
 * CSP, X-XSS-Protection, Referrer-Policy, Permissions-Policy
 */

#include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class security_headers_filter
 * @brief HTTP安全头过滤器
 *
 * 自动在所有响应中注入安全相关的HTTP头。
 * 默认启用所有安全头并提供合理默认值，每个header可单独禁用。
 * 在post_filter阶段执行，不阻断请求流程。
 */
class NEFORCE_API security_headers_filter final : public http_filter {
public:
    /// HSTS: 强制浏览器使用HTTPS
    bool enable_hsts = true;
    seconds hsts_max_age{31536000};      ///< 默认1年
    bool hsts_include_subdomains = true; ///< 是否包含子域名
    bool hsts_preload = false;           ///< 是否加入HSTS preload列表

    /// X-Frame-Options: 防止Clickjacking
    bool enable_frame_options = true;
    string frame_option_value{"DENY"}; ///< DENY | SAMEORIGIN | ALLOW-FROM uri

    /// X-Content-Type-Options: 防止MIME类型嗅探
    bool enable_content_type_options = true;

    /// Content-Security-Policy: 内容安全策略
    bool enable_csp = true;
    string csp_value{"default-src 'self'"}; ///< 可配置的CSP策略字符串

    /// X-XSS-Protection: 浏览器XSS过滤器
    bool enable_xss_protection = false;
    string xss_protection_value{"1; mode=block"};

    /// Referrer-Policy: 控制Referer头的发送
    bool enable_referrer_policy = true;
    string referrer_policy_value{"strict-origin-when-cross-origin"};

    /// Permissions-Policy: 控制浏览器特性权限
    bool enable_permissions_policy = true;
    string permissions_policy_value{"geolocation=(), microphone=(), camera=()"};

    bool pre_filter(http_request& request, http_response& response) override { return true; }
    void post_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "security_headers_filter"; }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_SECURITY_HPP__
