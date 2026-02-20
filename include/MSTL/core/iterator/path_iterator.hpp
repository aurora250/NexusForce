#ifndef MSTL_CORE_ITERATOR_PATH_ITERATOR_HPP__
#define MSTL_CORE_ITERATOR_PATH_ITERATOR_HPP__
#include "MSTL/core/file/file_constants.hpp"
#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class path_iterator {
public:
    using value_type = string_view;
    using reference = value_type;
    using pointer = void;
    using iterator_category = forward_iterator_tag;
    using difference_type = ptrdiff_t;

private:
    const string* p_ = nullptr;
    size_t start_ = 0;
    size_t end_ = 0;
    bool done_ = true;
    string current_part_;

    void find_next() {
        const size_t sz = p_->size();
        const size_t pos = start_;

#ifdef MSTL_PLATFORM_WINDOWS__
        if (pos == 0 && sz > 1 && (*p_)[1] == ':') {
            current_part_ = p_->substr(0, 2);
            start_ = 2;
            while (start_ < sz && ((*p_)[start_] == '/' || (*p_)[start_] == '\\'))
                ++start_;
            end_ = start_ - 1;
            done_ = false;
            return;
        }
#endif

        const size_t sep_pos = p_->find_first_of(FILE_SPLITER, pos);
        if (sep_pos == string::npos) {
            current_part_ = p_->substr(pos);
            end_ = sz;
        } else {
            current_part_ = p_->substr(pos, sep_pos - pos);
            end_ = sep_pos;
        }
    }

public:
    path_iterator() noexcept = default;

    explicit path_iterator(const string* path, const size_t pos = 0) noexcept
        : p_(path), start_(pos), done_(false) {
        if (!p_ || p_->empty() || start_ >= p_->size()) {
            done_ = true;
            return;
        }
        find_next();
    }

    reference operator *() const noexcept {
        return current_part_.view();
    }

    path_iterator& operator ++() {
        if (done_) return *this;
        start_ = end_ + 1;
        if (start_ >= p_->size()) {
            done_ = true;
            current_part_ = {};
        } else {
            find_next();
        }
        return *this;
    }

    path_iterator operator ++(int) {
        path_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    MSTL_NODISCARD bool operator ==(const path_iterator& b) const noexcept {
        if (done_ && b.done_) return true;
        if (p_ != b.p_) return false;
        return start_ == b.start_;
    }

    MSTL_NODISCARD bool operator !=(const path_iterator& b) const noexcept {
        return !(*this == b);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_PATH_ITERATOR_HPP__
