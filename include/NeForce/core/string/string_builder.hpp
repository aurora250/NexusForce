#ifndef NEFORCE_CORE_STRING_STRING_BUILDER_HPP__
#define NEFORCE_CORE_STRING_STRING_BUILDER_HPP__

/**
 * @file string_builder.hpp
 * @brief 字符串构建器
 *
 * 此文件提供了字符串构建器类，用于拼接多个字符串片段。
 */

#include "NeForce/core/interface/istringify.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup String 字符串
 * @{
 */

/**
 * @class string_builder
 * @brief 延迟拼接的字符串构建器
 *
 * 收集字符串片段，在调用 build() 时一次性分配内存并拼接。
 *
 * @warning 调用者必须保证传入的 string_view 和 const char* 参数在 build() 调用之前保持有效。
 */
class string_builder {
public:
    /**
     * @brief 默认构造函数
     */
    string_builder() = default;

    /**
     * @brief 预留片段容量
     * @param n 预计的片段数量
     */
    void reserve_pieces(const size_t n) { pieces_.reserve(n); }

    /**
     * @brief 预留输出总大小
     * @param n 预计的总字符数
     */
    void reserve(const size_t n) { reserved_size_ = n; }

    /**
     * @brief 追加字符串视图
     * @param sv 字符串视图
     * @return 自身引用
     */
    string_builder& append(const string_view sv) {
        pieces_.push_back(sv);
        total_size_ += sv.size();
        return *this;
    }

    /**
     * @brief 追加字符串
     * @param str 字符串
     * @return 自身引用
     */
    string_builder& append(const string& str) { return append(str.view()); }

    /**
     * @brief 追加 C 风格字符串
     * @param str C 风格字符串
     * @return 自身引用
     */
    string_builder& append(const char* str) { return append(string_view(str)); }

    /**
     * @brief 追加单个字符
     * @param c 字符
     * @return 自身引用
     */
    string_builder& append(const char c) {
        owned_pieces_.emplace_back(1, c);
        pieces_.push_back(owned_pieces_.back().view());
        total_size_ += 1;
        return *this;
    }

    /**
     * @brief 追加任意可转为字符串的值
     * @tparam T 支持包装类型的基本类型
     * @param value 要追加的值
     * @return 自身引用
     */
    template <typename T, typename P = package_t<T>,
              enable_if_t<is_packaged_v<T> && is_base_of_v<istringify<P>, P>, int> = 0>
    string_builder& append(const T& value) {
        owned_pieces_.push_back(P(value).to_string());
        pieces_.push_back(owned_pieces_.back().view());
        total_size_ += owned_pieces_.back().size();
        return *this;
    }

    /**
     * @brief 追加任意可转为字符串的值
     * @tparam T istringify<T> 子类类型
     * @param value 要追加的值
     * @return 自身引用
     */
    template <typename T, enable_if_t<is_base_of_v<istringify<T>, T>, int> = 0>
    string_builder& append(const T& value) {
        owned_pieces_.push_back(value.to_string());
        pieces_.push_back(owned_pieces_.back().view());
        total_size_ += owned_pieces_.back().size();
        return *this;
    }

    /**
     * @brief 构建字符串
     * @return 拼接后的字符串
     */
    NEFORCE_NODISCARD string build() const {
        string result;
        const size_t alloc_size = max(reserved_size_, total_size_);
        result.reserve(alloc_size);
        for (const auto& piece: pieces_) {
            result.append(piece.data(), piece.size());
        }
        return result;
    }

    /**
     * @brief 隐式转换为字符串
     * @return 拼接后的字符串
     */
    NEFORCE_NODISCARD operator string() const { return build(); }

    /**
     * @brief 当前总字符数
     * @return 已收集片段的总长度
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return total_size_; }

    /**
     * @brief 是否为空
     * @return 无任何片段时返回 true
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return total_size_ == 0; }

    /**
     * @brief 清空所有已收集的片段
     */
    void clear() noexcept {
        pieces_.clear();
        owned_pieces_.clear();
        total_size_ = 0;
        reserved_size_ = 0;
    }

private:
    vector<string_view> pieces_;  ///< 字符串片段视图
    vector<string> owned_pieces_; ///< 内部持有的字符串
    size_t total_size_ = 0;       ///< 已收集片段总长度
    size_t reserved_size_ = 0;    ///< 预设输出容量
};

/// @cond
NEFORCE_BEGIN_INNER__

#ifdef NEFORCE_STANDARD_17
template <typename... Args>
void __concat_append(string_builder& sb, Args&&... args) {
    (sb.append(_NEFORCE forward<Args>(args)), ...);
}
#else
template <typename First, typename... Rest>
void __concat_append(string_builder& sb, First&& first, Rest&&... rest) {
    sb.append(_NEFORCE forward<First>(first));
    __concat_append(sb, _NEFORCE forward<Rest>(rest)...);
}
template <typename Last>
void __concat_append(string_builder& sb, Last&& last) {
    sb.append(_NEFORCE forward<Last>(last));
}
#endif

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 拼接参数
 * @tparam Args 参数类型
 * @param args 要拼接的参数
 * @return 拼接后的字符串
 */
template <typename... Args, enable_if_t<(sizeof...(Args) > 0), int> = 0>
NEFORCE_NODISCARD string concatenate(Args&&... args) {
    string_builder sb;
    inner::__concat_append(sb, _NEFORCE forward<Args>(args)...);
    return sb.build();
}

/** @} */ // String

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_STRING_BUILDER_HPP__
