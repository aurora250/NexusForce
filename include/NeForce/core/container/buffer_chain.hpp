#ifndef NEFORCE_CORE_CONTAINER_BUFFER_CHAIN_HPP__
#define NEFORCE_CORE_CONTAINER_BUFFER_CHAIN_HPP__
#include "NeForce/core/string/string.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sys/uio.h>
#endif
#ifdef NEFORCE_PLATFORM_WINDOWS
struct iovec {
    void* iov_base;
    size_t iov_len;
};
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class buffer_chain
 * @brief 链式缓冲区，用于零拷贝数据拼接
 *
 * 将多个内存块链接在一起，避免多次拷贝。
 * flatten() 时才合并为单一缓冲区。
 */
class buffer_chain {
public:
    struct segment {
        const char* data{nullptr};
        size_t size{0};
    };

private:
    vector<segment> segments_;
    size_t total_size_{0};

public:
    buffer_chain() = default;

    /**
     * @brief 追加数据视图
     */
    void append(const char* data, size_t size) {
        segments_.push_back({data, size});
        total_size_ += size;
    }

    /**
     * @brief 追加 string_view
     */
    void append(const string_view sv) { append(sv.data(), sv.size()); }

    /**
     * @brief 追加另一个 buffer_chain 的内容
     */
    void append(const buffer_chain& other) {
        segments_.insert(segments_.end(), other.segments_.begin(), other.segments_.end());
        total_size_ += other.total_size_;
    }

    /**
     * @brief 合并所有段为单一字符串
     */
    NEFORCE_NODISCARD string flatten() const {
        string result;
        result.reserve(total_size_);
        for (const auto& seg: segments_) {
            result.append(seg.data, seg.size);
        }
        return result;
    }

    /**
     * @brief 构建 writev iovec 数组
     */
    NEFORCE_NODISCARD vector<::iovec> to_iovec() const {
        vector<::iovec> iov;
        iov.reserve(segments_.size());
        for (const auto& seg: segments_) {
            ::iovec v{};
            v.iov_base = const_cast<char*>(seg.data);
            v.iov_len = seg.size;
            iov.push_back(v);
        }
        return iov;
    }

    NEFORCE_NODISCARD size_t total_size() const noexcept { return total_size_; }
    NEFORCE_NODISCARD size_t segment_count() const noexcept { return segments_.size(); }
    NEFORCE_NODISCARD bool empty() const noexcept { return total_size_ == 0; }

    void clear() noexcept {
        segments_.clear();
        total_size_ = 0;
    }

    void reserve(size_t count) { segments_.reserve(count); }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_BUFFER_CHAIN_HPP__
