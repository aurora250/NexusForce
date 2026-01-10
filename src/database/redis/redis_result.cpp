#include <MSTL/database/redis/redis_result.hpp>
#ifdef MSTL_SUPPORT_REDIS__
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

_MSTL string redis_result::format_redis_reply_element(::redisReply* element) {
    switch (element->type) {
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
            return {element->str, element->len};
        case REDIS_REPLY_INTEGER:
            return _MSTL to_string(element->integer);
        case REDIS_REPLY_NIL:
            return {};
        case REDIS_REPLY_ARRAY: {
            string result;
            for (size_t i = 0; i < element->elements; ++i) {
                if (i > 0) result += " ";
                result += format_redis_reply_element(element->element[i]);
            }
            return result;
        }
        default:
            return "unsupported-type";
    }
}

void redis_result::process_reply() {
    if (!reply_) return;

    switch (reply_->type) {
        case REDIS_REPLY_ARRAY: {
            is_array_ = true;
            rows_ = reply_->elements;

            if (rows_ % 2 == 0) {
                for (size_t i = 0; i < rows_; i += 2) {
                    const string key = format_redis_reply_element(reply_->element[i]);
                    const string value = format_redis_reply_element(reply_->element[i + 1]);
                    kv_pairs_->emplace_back(_MSTL move(key), _MSTL move(value));
                }
                rows_ = kv_pairs_->size();
            } else {
                column_names_->push_back("value");
            }
            break;
        }
        case REDIS_REPLY_STRING: case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR: case REDIS_REPLY_INTEGER: {
            rows_ = 1;
            column_names_->push_back("result");
            string value = format_redis_reply_element(reply_);
            kv_pairs_->emplace_back("", _MSTL move(value));
            break;
        }
        case REDIS_REPLY_NIL: {
            rows_ = 0;
            break;
        }
        default: {
            rows_ = 1;
            column_names_->push_back("result");
            break;
        }
    }
}

string redis_result::get_string() const {
    if (empty()) return {};

    if (!kv_pairs_->empty() && kv_cursor_ > 0) {
        return string(value());
    }
    if (is_array_ && cursor_ > 0) {
        ::redisReply* element = reply_->element[cursor_ - 1];
        return format_redis_reply_element(element);
    }
    return format_redis_reply_element(reply_);
}

bool redis_result::next() noexcept {
    if (empty() || cursor_ >= rows_) return false;
    ++cursor_;
    ++kv_cursor_;
    return cursor_ <= rows_;
}

string_view redis_result::key() const noexcept {
    if (kv_pairs_->empty() || kv_cursor_ == 0) return {};
    return kv_pairs_->operator [](kv_cursor_ - 1).first.view();
}

string_view redis_result::value() const noexcept {
    if (kv_pairs_->empty() || kv_cursor_ == 0) return {};
    return kv_pairs_->operator [](kv_cursor_ - 1).second.view();
}

bool redis_result::value_bool() const {
    return boolean::parse(get_string().view()).value();
}

int64_t redis_result::value_int64() const {
    if (!reply_) return 0;
    if (reply_->type == REDIS_REPLY_INTEGER) {
        return reply_->integer;
    }
    return integer64::parse(get_string().view());
}

double redis_result::value_double() const {
    return float64::parse(get_string().view());
}

vector<string> redis_result::value_array() const {
    vector<string> result;
    if (reply_ && reply_->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply_->elements; ++i) {
            string value = format_redis_reply_element(reply_->element[i]);
            result.push_back(_MSTL move(value));
        }
    }
    return result;
}

MSTL_END_NAMESPACE__
#endif
