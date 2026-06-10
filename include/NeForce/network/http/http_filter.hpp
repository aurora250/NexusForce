#ifndef NEFORCE_NETWORK_HTTP_HTTP_FILTER_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_FILTER_HPP__

/**
 * @file http_filter.hpp
 * @brief HTTP过滤器链实现
 *
 * 此文件提供了HTTP过滤器链的实现，用于在HTTP请求处理过程中
 * 进行预处理和后处理。支持多种内置过滤器：CORS、日志记录、
 * 静态文件服务、限流、认证等。
 *
 * 主要功能：
 * - 过滤器链管理
 * - 预过滤和后过滤
 * - CORS跨域支持
 * - 请求日志记录
 * - 静态文件服务
 * - 请求限流
 * - 认证授权
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/core/utility/byte_size.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/network/http/http_server_message.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class http_filter
 * @brief HTTP过滤器基类
 *
 * 所有HTTP过滤器的抽象基类，定义了过滤器的生命周期方法。
 *
 * 过滤器生命周期：
 * 1. pre_filter：请求处理前调用
 * 2. do_filter：实际处理
 * 3. post_filter：响应发送前调用
 */
class NEFORCE_API http_filter {
public:
    virtual ~http_filter() = default;

    /**
     * @brief 核心过滤方法
     * @param request HTTP请求
     * @param response HTTP响应
     *
     * 执行实际的过滤逻辑。通常由具体处理器实现，
     * 中间件过滤器可能不实现此方法。
     */
    virtual void do_filter(http_request& request, http_response& response) = 0;

    /**
     * @brief 预处理方法
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 返回false表示中断后续处理
     *
     * 在请求处理前调用，可以检查请求、修改请求或提前返回响应。
     */
    virtual bool pre_filter(http_request& request, http_response& response) { return true; }

    /**
     * @brief 后处理方法
     * @param request HTTP请求
     * @param response HTTP响应
     *
     * 在响应发送前调用，可以修改响应头或正文。
     */
    virtual void post_filter(http_request& request, http_response& response) {}

    /**
     * @brief 获取过滤器名称
     * @return 过滤器名称
     */
    NEFORCE_NODISCARD virtual string name() const { return "http_filter"; }
};

/**
 * @class http_filter_chain
 * @brief HTTP过滤器链
 *
 * 管理多个过滤器的有序执行，支持预过滤、后过滤和核心过滤。
 */
class NEFORCE_API http_filter_chain {
private:
    struct filter_entry {
        unique_ptr<http_filter> filter;
        bool owned = true;
    };
    vector<filter_entry> filters_; ///< 过滤器列表

public:
    http_filter_chain() = default;
    ~http_filter_chain() { clear(); }

    http_filter_chain(const http_filter_chain&) = delete;
    http_filter_chain& operator=(const http_filter_chain&) = delete;

    http_filter_chain(http_filter_chain&&) noexcept = default;
    http_filter_chain& operator=(http_filter_chain&&) noexcept = default;

    /**
     * @brief 添加过滤器（转移所有权）
     * @param filter 过滤器智能指针
     */
    void add_filter(unique_ptr<http_filter> filter);

    /**
     * @brief 添加过滤器（不转移所有权）
     * @param filter 过滤器原始指针
     */
    void add_filter_ref(http_filter* filter);

    /**
     * @brief 清空所有过滤器
     */
    void clear() noexcept;

    /**
     * @brief 获取过滤器数量
     * @return 过滤器数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return filters_.size(); }

    /**
     * @brief 检查过滤器链是否为空
     * @return 为空返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return filters_.empty(); }

    /**
     * @brief 执行所有预过滤器
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 所有预过滤器都通过返回true
     */
    bool execute_pre_filters(http_request& request, http_response& response);

    /**
     * @brief 执行所有后过滤器
     * @param request HTTP请求
     * @param response HTTP响应
     */
    void execute_post_filters(http_request& request, http_response& response);

    /**
     * @brief 异步执行所有预过滤器（回调链模式）
     * @param request HTTP请求
     * @param response HTTP响应
     * @param ctx 请求上下文
     * @param next 所有过滤器通过后调用 next(true)，任一中断调用 next(false)
     */
    void execute_pre_filters_async(http_request& request, http_response& response, http_context& ctx,
                                   function<void(bool)> next);

    /**
     * @brief 异步执行所有后过滤器（回调链模式）
     * @param request HTTP请求
     * @param response HTTP响应
     * @param ctx 请求上下文
     * @param next 所有过滤器完成后调用
     */
    void execute_post_filters_async(http_request& request, http_response& response, http_context& ctx,
                                    function<void()> next);

    /**
     * @brief 执行所有核心过滤器
     * @param request HTTP请求
     * @param response HTTP响应
     */
    void execute_filters(http_request& request, http_response& response);
};

/**
 * @class cors_filter
 * @brief CORS跨域过滤器
 *
 * 处理跨域资源共享（CORS）请求，添加必要的响应头。
 * 自动处理OPTIONS预检请求。
 */
class NEFORCE_API cors_filter final : public http_filter {
public:
    string allowed_origins;                                                   ///< 允许的源
    http_method allowed_methods{http_method::DEFAULT()};                      ///< 允许的方法
    string allowed_headers{"Content-Type, Cookie, Accept, X-Requested-With"}; ///< 允许的请求头
    bool allow_credentials = true;                                            ///< 是否允许携带凭证
    seconds max_age{86400};                                                   ///< 预检结果缓存时间

    cors_filter() = default;

    /**
     * @brief 构造函数
     * @param origins 允许的源
     */
    explicit cors_filter(string origins) :
    allowed_origins(_NEFORCE move(origins)) {}

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "cors_filter"; }
};

/**
 * @class logging_filter
 * @brief 日志记录过滤器
 *
 * 记录HTTP请求和响应的详细信息，用于调试和监控。
 */
class NEFORCE_API logging_filter final : public http_filter {
public:
    bool log_headers = false;          ///< 是否记录请求/响应头
    bool log_body = false;             ///< 是否记录请求/响应体
    byte_size max_body_log_size{1_KB}; ///< 最大记录正文大小

    bool pre_filter(http_request& request, http_response& response) override;
    void post_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "logging_filter"; }
};

/**
 * @class static_file_filter
 * @brief 静态文件服务过滤器
 *
 * 提供静态文件服务功能，当请求匹配文件路径时返回文件内容。
 */
class NEFORCE_API static_file_filter final : public http_filter {
private:
    string root_path_;                               ///< 文件根目录
    unordered_map<string, http_content> mime_types_; ///< MIME类型映射
    bool enable_cache_ = true;                       ///< 是否启用缓存
    bool enable_range_ = true;                       ///< 是否启用Range请求支持
    byte_size max_file_size_{10_MB};                 ///< 最大文件大小

public:
    /**
     * @brief 构造函数
     * @param root_path 文件根目录
     */
    explicit static_file_filter(string root_path);

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "static_file_filter"; }

    /**
     * @brief 检查路径是否安全
     * @param path 文件路径
     * @return 安全返回true
     *
     * 防止路径遍历攻击，拒绝包含".."或"//"的路径。
     */
    NEFORCE_NODISCARD static bool is_safe_path(const string& path);

    /**
     * @brief 获取MIME类型
     * @param path 文件路径
     * @return MIME类型，未找到返回none
     */
    NEFORCE_NODISCARD optional<http_content> get_mime_type(const string& path) const;

    /**
     * @brief 添加MIME类型映射
     * @param extension 文件扩展名
     * @param content_type MIME类型
     */
    void add_mime_type(const string& extension, http_content content_type);

    void set_enable_range(const bool enable) { enable_range_ = enable; }
};

/**
 * @class authentication_filter
 * @brief 认证过滤器
 *
 * 验证请求是否包含有效的认证信息。
 */
class NEFORCE_API authentication_filter final : public http_filter {
private:
    vector<string> excluded_paths_;                      ///< 排除的路径（不要求认证）
    vector<string> included_paths_;                      ///< 包含的路径（要求认证），为空时所有路径均需认证
    function<bool(const http_request&)> auth_validator_; ///< 认证验证器

    NEFORCE_NODISCARD bool is_path_excluded(const string& path) const;
    NEFORCE_NODISCARD bool is_path_protected(const string& path) const;

public:
    authentication_filter() = default;

    /**
     * @brief 构造函数
     * @param validator 认证验证函数
     */
    explicit authentication_filter(function<bool(const http_request&)> validator) :
    auth_validator_(_NEFORCE move(validator)) {}

    /**
     * @brief 设置认证验证器
     * @param validator 验证函数
     */
    void set_auth_validator(function<bool(const http_request&)> validator) {
        auth_validator_ = _NEFORCE move(validator);
    }

    /**
     * @brief 添加排除路径（不要求认证）
     * @param path 路径（支持前缀匹配）
     *
     * 仅在 included_paths_ 为空时生效（全量认证模式）。
     */
    void add_excluded_path(string path) { excluded_paths_.push_back(_NEFORCE move(path)); }

    /**
     * @brief 添加受保护路径（要求认证）
     * @param path 路径（支持前缀匹配）
     *
     * 设置后仅这些路径需要认证，其余路径为公开访问。
     */
    void add_included_path(string path) { included_paths_.push_back(_NEFORCE move(path)); }

    /**
     * @brief 清除所有排除路径
     */
    void clear_excluded_paths() { excluded_paths_.clear(); }

    /**
     * @brief 清除所有包含路径
     */
    void clear_included_paths() { included_paths_.clear(); }

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "authentication_filter"; }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_FILTER_HPP__
