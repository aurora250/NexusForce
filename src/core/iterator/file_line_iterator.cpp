#include <MSTL/core/iterator/file_line_iterator.hpp>
#include <MSTL/core/file/file.hpp>
MSTL_BEGIN_NAMESPACE__

file_line_iterator::file_line_iterator(const file* f) : file_(f) {
    if (file_ && file_->opened()) {
        ++(*this);
    }
}

file_line_iterator& file_line_iterator::operator++() {
    if (file_ && !file_->read_line(current_line_)) {
        file_ = nullptr;
    }
    return *this;
}

file_line_iterator file_line_iterator::operator++(int) {
    file_line_iterator tmp = *this;
    ++(*this);
    return tmp;
}


MSTL_END_NAMESPACE__
