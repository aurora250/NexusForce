#ifndef MSTL_CORE_ITERATOR_FILE_LINE_ITERATOR_HPP__
#define MSTL_CORE_ITERATOR_FILE_LINE_ITERATOR_HPP__
#include "../file/file_constants.hpp"
#include "../string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API file_line_iterator {
public:
    using value_type = string;
    using reference = const string&;
    using pointer = const string*;
    using iterator_category = input_iterator_tag;
    using difference_type = ptrdiff_t;

private:
    const file* file_ = nullptr;
    mutable string current_line_;

public:
    file_line_iterator() = default;
    explicit file_line_iterator(const file* f);

    reference operator *() const noexcept { return current_line_; }
    pointer operator ->() const noexcept { return &current_line_; }

    file_line_iterator& operator ++();
    file_line_iterator operator ++(int);

    bool operator ==(const file_line_iterator& b) const noexcept {
        return file_ == b.file_;
    }
    bool operator !=(const file_line_iterator& b) const noexcept {
        return !(*this == b);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_FILE_LINE_ITERATOR_HPP__
