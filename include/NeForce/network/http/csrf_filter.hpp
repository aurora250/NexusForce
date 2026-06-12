#ifndef NEFORCE_NETWORK_HTTP_CSRF_FILTER_HPP__
#define NEFORCE_NETWORK_HTTP_CSRF_FILTER_HPP__

/**
 * @file csrf_filter.hpp
 * @brief CSRF防护过滤器（Double-Submit Cookie模式）
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/network/http/http_filter.hpp"
#include "NeForce/network/http/http_session.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class csrf_filter
 * @brief CSRF防护过滤器
 *
 * Double-Submit Cookie模式：
 * 1. 首次请求时生成随机CSRF token，存入session + Set-Cookie
 * 2. 后续状态变更请求需在header(X-CSRF-Token)或body(_csrf)中携带token
 * 3. 比对cookie中的token与请求中的token，不匹配则拒绝(403)
 *
 * 仅验证状态变更方法：POST、PUT、PATCH、DELETE
 */
class NEFORCE_API csrf_filter final : public http_filter {
public:
    string cookie_name{"XSRF-TOKEN"};   ///< CSRF Cookie名称
    string header_name{"X-CSRF-Token"}; ///< CSRF Header名称
    string form_field_name{"_csrf"};    ///< CSRF表单字段名
    seconds token_max_age{3600};        ///< Token有效期
    bool enabled{true};                 ///< 是否启用

    csrf_filter() = default;

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& /*request*/, http_response& /*response*/) override {}

    NEFORCE_NODISCARD string name() const override { return "csrf_filter"; }

    // TODO: Synchronizer Token pattern — server-side token storage (in addition to Double-Submit Cookie) for stricter CSRF enforcement
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_CSRF_FILTER_HPP__
