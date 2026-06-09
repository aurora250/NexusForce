#ifndef NEFORCE_NETWORK_HTTP_ASYNC_FILTER_HPP__
#define NEFORCE_NETWORK_HTTP_ASYNC_FILTER_HPP__

/**
 * @file async_filter.hpp
 * @brief 异步过滤器基类（回调模式）
 */

#include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class async_filter
 * @brief 异步过滤器基类（回调模式）
 *
 * 扩展 http_filter，为需要异步 I/O 的中间件提供回调式异步接口。。
 *
 * 使用流程：
 * 1. 框架调用 pre_filter_async(request, response, context, next)
 * 2. 过滤器执行异步操作
 * 3. 完成时调用 next(true) 继续，或 next(false) 中断
 *
 * 无异步过滤器时，http_router 走零开销同步路径。
 */
class async_filter : public http_filter {
public:
    /**
     * @brief 异步继续回调
     * @param should_continue true=继续到下一过滤器，false=中断并返回当前响应
     */
    using next_callback = function<void(bool should_continue)>;

    /**
     * @brief 异步预过滤
     * @param request HTTP请求
     * @param response HTTP响应（可预设以中断处理）
     * @param ctx 请求上下文（跨中间件数据传递）
     * @param next 完成时调用 next(true) 继续或 next(false) 中断
     *
     * 默认实现直接调用 next(true)，等效于同步 pre_filter 返回 true。
     */
    virtual void pre_filter_async(http_request& request, http_response& response, http_context& ctx,
                                  next_callback next) {
        if (pre_filter(request, response)) {
            next(true);
        } else {
            next(false);
        }
    }

    /**
     * @brief 异步后过滤
     * @param request HTTP请求
     * @param response HTTP响应
     * @param ctx 请求上下文
     * @param next 完成时调用
     *
     * 默认实现直接调用 post_filter 并 next(true)。
     */
    virtual void post_filter_async(http_request& request, http_response& response, http_context& ctx,
                                   next_callback next) {
        post_filter(request, response);
        next(true);
    }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_ASYNC_FILTER_HPP__
