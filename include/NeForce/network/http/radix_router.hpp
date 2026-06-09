#ifndef NEFORCE_NETWORK_HTTP_RADIX_ROUTER_HPP__
#define NEFORCE_NETWORK_HTTP_RADIX_ROUTER_HPP__

/**
 * @file radix_router.hpp
 * @brief 压缩前缀树（段式Trie）路由匹配
 *
 * 将URL路径按'/'分割为段，每段作为一个trie节点，
 * 支持静态段、:param参数段、*通配符段三种匹配。
 * 匹配时间复杂度 O(k)，k为路径深度。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/network/http/http_server_message.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class route_trie
 * @brief 段式Trie路由器
 *
 * 每HTTP方法一个实例，存储非正则路由。
 */
class route_trie {
public:
    using handler_type = function<void(http_request&, http_response&)>;

    struct node {
        string segment;
        unordered_map<string, size_t> children;                ///< 静态段 → 子节点索引
        size_t param_index = numeric_traits<size_t>::max();    ///< :param 子节点索引
        size_t wildcard_index = numeric_traits<size_t>::max(); ///< * 通配符子节点索引
        string param_name;                                     ///< 参数名
        handler_type handler;                                  ///< 路由处理器
        bool has_handler = false;                              ///< 是否有处理器
    };

private:
    vector<node> nodes_{1};

public:
    route_trie() = default;

    /**
     * @brief 插入路由规则
     * @param pattern 路径模式（支持 :param 和 * 通配符）
     * @param handler 匹配时的处理器
     */
    void insert(const string& pattern, handler_type handler) {
        if (pattern.empty() || pattern.front() != '/') {
            return;
        }

        size_t node_idx = 0;
        size_t pos = 1;

        while (pos <= pattern.length()) {
            size_t slash = pattern.find('/', pos);
            if (slash == string::npos) {
                slash = pattern.length();
            }

            const string_view segment = pattern.view(pos, slash - pos);
            pos = slash + 1;

            if (segment.empty()) {
                continue;
            }

            if (segment[0] == ':') {
                string pname(segment.substr(1));
                if (nodes_[node_idx].param_index == numeric_traits<size_t>::max()) {
                    nodes_[node_idx].param_index = nodes_.size();
                    nodes_.emplace_back();
                    nodes_.back().segment = pname;
                }
                node_idx = nodes_[node_idx].param_index;
                nodes_[node_idx].param_name = pname;
            } else if (segment[0] == '*' && segment.length() == 1) {
                if (nodes_[node_idx].wildcard_index == numeric_traits<size_t>::max()) {
                    nodes_[node_idx].wildcard_index = nodes_.size();
                    nodes_.emplace_back();
                    nodes_.back().segment = "*";
                }
                node_idx = nodes_[node_idx].wildcard_index;
            } else {
                string seg(segment);
                auto& children = nodes_[node_idx].children;
                auto it = children.find(seg);
                if (it == children.end()) {
                    children[seg] = nodes_.size();
                    nodes_.emplace_back();
                    nodes_.back().segment = seg;
                    node_idx = nodes_.size() - 1;
                } else {
                    node_idx = it->second;
                }
            }
        }

        nodes_[node_idx].handler = move(handler);
        nodes_[node_idx].has_handler = true;
    }

    /**
     * @brief 匹配路径
     * @param path 请求路径
     * @param case_sensitive 是否大小写敏感
     * @param params 输出匹配到的参数
     * @return handler指针，未找到返回nullptr
     */
    NEFORCE_NODISCARD handler_type match(const string& path, bool case_sensitive,
                                         vector<pair<string, string>>& params) {
        if (path.empty() || path.front() != '/') {
            return handler_type{};
        }

        size_t node_idx = 0;
        size_t pos = 1;

        while (pos <= path.length()) {
            size_t slash = path.find('/', pos);
            if (slash == string::npos) {
                slash = path.length();
            }

            string_view segment = path.view(pos, slash - pos);
            pos = slash + 1;

            if (segment.empty()) {
                continue;
            }

            const node& current = nodes_[node_idx];

            const string seg_key(segment);
            if (!case_sensitive) {
                bool found = false;
                for (const auto& child: current.children) {
                    string key_lower = child.first;
                    if (key_lower.lowercase() == seg_key.lowercase()) {
                        node_idx = child.second;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    continue;
                }
            } else {
                auto it = current.children.find(seg_key);
                if (it != current.children.end()) {
                    node_idx = it->second;
                    continue;
                }
            }

            if (current.param_index != numeric_traits<size_t>::max()) {
                params.emplace_back(nodes_[current.param_index].param_name, string(segment));
                node_idx = current.param_index;
                continue;
            }

            if (current.wildcard_index != numeric_traits<size_t>::max()) {
                string remaining;
                remaining.reserve(path.length() - (slash - segment.length()));
                remaining.append(segment);
                if (slash < path.length()) {
                    remaining += path.view(slash);
                }
                params.emplace_back("*", remaining);
                node_idx = current.wildcard_index;
                break;
            }

            return handler_type{};
        }

        if (nodes_[node_idx].has_handler) {
            return nodes_[node_idx].handler;
        }
        return handler_type{};
    }

    /**
     * @brief 检查路径是否存在
     */
    NEFORCE_NODISCARD bool contains_path(const string& path, bool case_sensitive) const {
        if (path.empty() || path.front() != '/') {
            return false;
        }

        size_t node_idx = 0;
        size_t pos = 1;

        while (pos <= path.length()) {
            size_t slash = path.find('/', pos);
            if (slash == string::npos) {
                slash = path.length();
            }

            string_view segment = path.view(pos, slash - pos);
            pos = slash + 1;

            if (segment.empty()) {
                continue;
            }

            const node& current = nodes_[node_idx];

            string seg_key(segment);
            if (!case_sensitive) {
                seg_key = seg_key.lowercase();
                bool found = false;
                for (const auto& child: current.children) {
                    string key_lower = child.first;
                    if (key_lower.lowercase() == seg_key) {
                        node_idx = child.second;
                        found = true;
                        break;
                    }
                }
                if (found) {
                    continue;
                }
            } else {
                auto it = current.children.find(seg_key);
                if (it != current.children.end()) {
                    node_idx = it->second;
                    continue;
                }
            }

            if (current.param_index != numeric_traits<size_t>::max()) {
                node_idx = current.param_index;
                continue;
            }

            if (current.wildcard_index != numeric_traits<size_t>::max()) {
                return true;
            }

            return false;
        }

        return nodes_[node_idx].has_handler;
    }

    /// @brief 清除所有路由规则
    void clear() {
        nodes_.clear();
        nodes_.emplace_back();
    }

    /// @brief 获取 trie 节点总数
    NEFORCE_NODISCARD size_t size() const noexcept { return nodes_.size(); }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_RADIX_ROUTER_HPP__
