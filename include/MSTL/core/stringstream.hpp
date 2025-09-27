#ifndef MSTL_STRINGSTREAM_HPP__
#define MSTL_STRINGSTREAM_HPP__
#include "string.hpp"
MSTL_BEGIN_NAMESPACE__

enum class iostate {
    goodbit = 0,
    eofbit  = 1 << 0,
    failbit = 1 << 1,
    badbit  = 1 << 2
};

inline iostate operator |(iostate a, iostate b) {
    return static_cast<iostate>(static_cast<int>(a) | static_cast<int>(b));
}
inline iostate operator &(iostate a, iostate b) {
    return static_cast<iostate>(static_cast<int>(a) & static_cast<int>(b));
}


enum class seekdir {
    beg,
    cur,
    end
};

template <typename CharT>
class basic_stringbuf {
protected:
    using value_type = basic_string<CharT>;
    value_type buffer_{};
    size_t gpos_ = 0;
    iostate state_ = iostate::goodbit;

    MSTL_CONSTEXPR20 void setstate(const iostate state) noexcept {
        state_ = state_ | state;
    }
    MSTL_CONSTEXPR20 void clearstate(const iostate state = iostate::goodbit) noexcept {
        state_ = state;
    }

public:
    using char_type = CharT;
    using traits_type = typename value_type::traits_type;
    using off_type = long long;

    MSTL_CONSTEXPR20 basic_stringbuf() = default;

    MSTL_CONSTEXPR20 explicit basic_stringbuf(const value_type& str)
        : buffer_(str) {}

    MSTL_CONSTEXPR20 explicit basic_stringbuf(value_type&& str) noexcept
        : buffer_(_MSTL move(str)) {}

    MSTL_CONSTEXPR20 explicit basic_stringbuf(const char_type* str)
        : buffer_(str) {}

    MSTL_CONSTEXPR20 void str(value_type&& str) noexcept {
        buffer_ = _MSTL move(str);
        gpos_ = 0;
        clearstate();
    }

    MSTL_CONSTEXPR20 void str(const value_type& str) {
        buffer_ = str;
        gpos_ = 0;
        clearstate();
    }

    MSTL_CONSTEXPR20 void str(const char_type* str) {
        buffer_ = str;
        gpos_ = 0;
        clearstate();
    }

    MSTL_CONSTEXPR20 value_type str() const { return buffer_; }

    MSTL_CONSTEXPR20 void clear() {
        buffer_.clear();
        gpos_ = 0;
        clearstate();
    }

    MSTL_CONSTEXPR20 iostate rdstate() const noexcept { return state_; }
    MSTL_CONSTEXPR20 bool good() const noexcept { return state_ == iostate::goodbit; }
    MSTL_CONSTEXPR20 bool eof() const noexcept { return static_cast<int>(state_ & iostate::eofbit) != 0; }
    MSTL_CONSTEXPR20 bool fail() const noexcept { return static_cast<int>(state_ & (iostate::failbit | iostate::badbit)) != 0; }
    MSTL_CONSTEXPR20 bool bad() const noexcept { return static_cast<int>(state_ & iostate::badbit) != 0; }

    MSTL_CONSTEXPR20 void clear(iostate state) noexcept {
        state_ = state;
        if (gpos_ > buffer_.size()) gpos_ = buffer_.size();
    }

    MSTL_CONSTEXPR20 void seekg(off_type off, const seekdir dir) {
        size_t new_pos = 0;
        switch (dir) {
            case seekdir::beg:
                new_pos = off;
                break;
            case seekdir::cur:
                new_pos = gpos_ + off;
                break;
            case seekdir::end:
                new_pos = buffer_.size() + off;
                break;
            default: break;
        }

        if (new_pos <= buffer_.size()) {
            gpos_ = new_pos;
            clearstate(iostate::goodbit);
        } else {
            setstate(iostate::failbit);
        }
    }

    MSTL_CONSTEXPR20 void seekg(const size_t pos) {
        seekg(pos, seekdir::beg);
    }

    MSTL_CONSTEXPR20 size_t tellg() const noexcept { return gpos_; }
};


template <typename CharT>
class basic_istringstream : public basic_stringbuf<CharT> {
public:
    using base = basic_stringbuf<CharT>;
    using char_type = typename base::char_type;
    using value_type = typename base::value_type;
    using traits_type = typename base::traits_type;
    using self = basic_istringstream<CharT>;

    using base::base;

    MSTL_CONSTEXPR20 self& operator>>(bool& x) {
        int tmp;
        *this >> tmp;
        x = tmp != 0;
        return *this;
    }

    template <typename Integer, enable_if_t<is_integral<Integer>::value, int> = 0>
    MSTL_CONSTEXPR20 self& extract_integer(Integer& x) {
        if (this->fail()) return *this;

        while (this->gpos_ < this->buffer_.size() &&
               _MSTL is_space(this->buffer_[this->gpos_])) {
            ++this->gpos_;
        }

        if (this->gpos_ >= this->buffer_.size()) {
            this->setstate(iostate::eofbit | iostate::failbit);
            return *this;
        }

        bool negative = false;
        if (this->buffer_[this->gpos_] == '-') {
            negative = true;
            ++this->gpos_;
        } else if (this->buffer_[this->gpos_] == '+') {
            ++this->gpos_;
        }

        Integer value = 0;
        size_t start = this->gpos_;
        while (this->gpos_ < this->buffer_.size() &&
               _MSTL is_digit(this->buffer_[this->gpos_])) {
            value = value * 10 + (this->buffer_[this->gpos_] - '0');
            ++this->gpos_;
        }

        if (this->gpos_ == start) {
            this->setstate(iostate::failbit);
            return *this;
        }

        x = negative ? -value : value;
        return *this;
    }

    MSTL_CONSTEXPR20 self& operator>>(int& x) { return extract_integer(x); }
    MSTL_CONSTEXPR20 self& operator>>(unsigned int& x) { return extract_integer(x); }
    MSTL_CONSTEXPR20 self& operator>>(long& x) { return extract_integer(x); }
    MSTL_CONSTEXPR20 self& operator>>(unsigned long& x) { return extract_integer(x); }
    MSTL_CONSTEXPR20 self& operator>>(long long& x) { return extract_integer(x); }
    MSTL_CONSTEXPR20 self& operator>>(unsigned long long& x) { return extract_integer(x); }

    MSTL_CONSTEXPR20 self& operator>>(float& x) {
        value_type tmp;
        *this >> tmp;
        x = to_float32(tmp.c_str());
        return *this;
    }

    MSTL_CONSTEXPR20 self& operator>>(double& x) {
        value_type tmp;
        *this >> tmp;
        x = to_float64(tmp.c_str());
        return *this;
    }

    MSTL_CONSTEXPR20 self& operator>>(long double& x) {
        value_type tmp;
        *this >> tmp;
        x = to_decimal(tmp.c_str());
        return *this;
    }

    MSTL_CONSTEXPR20 self& operator>>(char_type& c) {
        if (this->fail()) return *this;

        while (this->gpos_ < this->buffer_.size() &&
               _MSTL is_space(this->buffer_[this->gpos_])) {
            ++this->gpos_;
        }

        if (this->gpos_ >= this->buffer_.size()) {
            this->setstate(iostate::eofbit | iostate::failbit);
            return *this;
        }

        c = this->buffer_[this->gpos_++];
        return *this;
    }

    MSTL_CONSTEXPR20 self& operator>>(value_type& str) {
        str.clear();
        if (this->fail()) return *this;

        while (this->gpos_ < this->buffer_.size() &&
               _MSTL is_space(this->buffer_[this->gpos_])) {
            ++this->gpos_;
        }

        size_t start = this->gpos_;
        while (this->gpos_ < this->buffer_.size() &&
               !_MSTL is_space(this->buffer_[this->gpos_])) {
            ++this->gpos_;
        }

        if (this->gpos_ > start) {
            str = this->buffer_.substr(start, this->gpos_ - start);
        } else {
            this->setstate(iostate::eofbit | iostate::failbit);
        }
        return *this;
    }

    MSTL_CONSTEXPR20 self& get(char_type& c) {
        if (this->gpos_ < this->buffer_.size()) {
            c = this->buffer_[this->gpos_++];
        } else {
            this->setstate(iostate::eofbit | iostate::failbit);
        }
        return *this;
    }

    MSTL_CONSTEXPR20 bool getline(basic_string<char_type>& str, char_type delim = '\n') {
        str.clear();
        bool has_read = false;
        while (this->gpos_ < this->buffer_.size()) {
            has_read = true;
            char_type c = this->buffer_[this->gpos_++];
            if (c == delim) break;
            str.push_back(c);
        }
        if (!has_read) {
            this->setstate(iostate::eofbit | iostate::failbit);
            return false;
        }
        return true;
    }

    using base::seekg;
    using base::tellg;
    using base::rdstate;
    using base::good;
    using base::eof;
    using base::fail;
    using base::bad;
    using base::clear;
    using base::str;
};


#define __MSTL_EXPAND_OSS_STREAM(T) \
    MSTL_CONSTEXPR20 self& operator<<(const T& x) { \
        this->buffer_.append(_MSTL move(_MSTL to_string(x))); \
        return *this; \
    }


template <typename CharT>
class basic_ostringstream : public basic_stringbuf<CharT> {
public:
    using base = basic_stringbuf<CharT>;
    using char_type = typename base::char_type;
    using value_type = typename base::value_type;
    using traits_type = typename base::traits_type;
    using self = basic_ostringstream<CharT>;

    using base::base;

    MSTL_CONSTEXPR20 self& operator<<(const bool x) {
        this->buffer_.append(_MSTL move(_MSTL to_string(x)));
        return *this;
    }

    MSTL_MACRO_RANGE_FLOAT(__MSTL_EXPAND_OSS_STREAM)
    MSTL_MACRO_RANGE_INT(__MSTL_EXPAND_OSS_STREAM)

    MSTL_CONSTEXPR20 self& operator<<(char_type x) {
        this->buffer_.append(x);
        return *this;
    }
    MSTL_CONSTEXPR20 self& operator<<(const char_type* x) {
        this->buffer_.append(x);
        return *this;
    }
    MSTL_CONSTEXPR20 self& operator<<(const value_type& x) {
        this->buffer_.append(x);
        return *this;
    }
    MSTL_CONSTEXPR20 self& operator<<(value_type&& x) {
        this->buffer_.append(_MSTL move(x));
        return *this;
    }
    template <typename T1, typename T2>
    MSTL_CONSTEXPR20 self& operator<<(const pair<T1, T2>& pir) {
        *this << "{ " << pir.first << ", " << pir.second << " }";
        return *this;
    }
    template <typename T>
    MSTL_CONSTEXPR20 self& operator<<(const shared_ptr<T>& ptr) {
        *this << ptr.get();
        return *this;
    }
    template <typename T, typename Deleter>
    MSTL_CONSTEXPR20 self& operator<<(const unique_ptr<T, Deleter>& ptr) {
        *this << ptr.get();
        return *this;
    }

    MSTL_CONSTEXPR20 basic_string_view<char_type> view() const {
        return basic_string_view<char_type>(this->buffer_.c_str());
    }

    using base::str;
    using base::clear;
};


template <typename CharT>
class basic_stringstream : public basic_istringstream<CharT>,
                           public basic_ostringstream<CharT> {
public:
    using istream_base = basic_istringstream<CharT>;
    using ostream_base = basic_ostringstream<CharT>;
    using value_type = typename istream_base::value_type;
    using char_type = CharT;
    using self = basic_stringstream<CharT>;

    MSTL_CONSTEXPR20 basic_stringstream() = default;

    MSTL_CONSTEXPR20 explicit basic_stringstream(const value_type& str)
        : istream_base(str), ostream_base(str) {}

    MSTL_CONSTEXPR20 void str(const value_type& str) {
        istream_base::str(str);
        ostream_base::str(str);
    }

    MSTL_CONSTEXPR20 value_type str() const {
        return istream_base::str();
    }

    using istream_base::seekg;
    using istream_base::tellg;
    using istream_base::rdstate;
    using istream_base::good;
    using istream_base::eof;
    using istream_base::fail;
    using istream_base::bad;
    using istream_base::clear;
};


using istringstream = basic_istringstream<char>;
using ostringstream = basic_ostringstream<char>;
using stringstream = basic_stringstream<char>;
using wistringstream = basic_istringstream<wchar_t>;
using wostringstream = basic_ostringstream<wchar_t>;
using wstringstream = basic_stringstream<wchar_t>;

MSTL_END_NAMESPACE__
#endif // MSTL_STRINGSTREAM_HPP__