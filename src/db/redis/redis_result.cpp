#include <NeForce/db/redis/redis_result.hpp>
#ifdef NEFORCE_SUPPORT_HIREDIS
#    include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string format_redis_reply_element(::redisReply* element) {
        switch (element->type) {
            case REDIS_REPLY_STRING:
            case REDIS_REPLY_STATUS:
            case REDIS_REPLY_ERROR:
                return {element->str, element->len};
            case REDIS_REPLY_INTEGER:
                return _NEFORCE to_string(element->integer);
            case REDIS_REPLY_NIL:
                return {};
            case REDIS_REPLY_ARRAY: {
                string result;
                for (size_t i = 0; i < element->elements; ++i) {
                    if (i > 0) {
                        result += " ";
                    }
                    result += format_redis_reply_element(element->element[i]);
                }
                return result;
            }
            default: {
                return "unsupported-type";
            }
        }
    }
} // namespace


string redis_result::get_string() const {
    if (empty()) {
        return {};
    }

    if (!kv_pairs_->empty() && kv_cursor_ > 0) {
        return {value()};
    }
    if (is_array_ && cursor_ > 0) {
        ::redisReply* element = result_->element[cursor_ - 1];
        return format_redis_reply_element(element);
    }
    return format_redis_reply_element(result_);
}

redis_result::redis_result() :
column_names_(make_unique<vector<string>>()),
kv_pairs_(make_unique<vector<pair<string, string>>>()) {}

redis_result::redis_result(::redisReply* reply) :
result_(reply),
column_names_(make_unique<vector<string>>()),
kv_pairs_(make_unique<vector<pair<string, string>>>()) {
    if (result_ == nullptr) {
        return;
    }

    switch (result_->type) {
        case REDIS_REPLY_ARRAY: {
            is_array_ = true;
            rows_ = result_->elements;

            if (rows_ % 2 == 0) {
                for (size_t i = 0; i < rows_; i += 2) {
                    const string key = format_redis_reply_element(result_->element[i]);
                    const string value = format_redis_reply_element(result_->element[i + 1]);
                    kv_pairs_->emplace_back(_NEFORCE move(key), _NEFORCE move(value));
                }
                rows_ = kv_pairs_->size();
            } else {
                column_names_->push_back("value");
            }
            break;
        }
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
        case REDIS_REPLY_INTEGER: {
            rows_ = 1;
            column_names_->push_back("result");
            string value = format_redis_reply_element(result_);
            kv_pairs_->emplace_back("", _NEFORCE move(value));
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

redis_result::~redis_result() {
    if (result_ != nullptr) {
        ::freeReplyObject(result_);
    }
}

bool redis_result::next() noexcept {
    if (empty() || cursor_ >= rows_) {
        return false;
    }
    ++cursor_;
    ++kv_cursor_;
    return cursor_ <= rows_;
}

string_view redis_result::key() const noexcept {
    if (kv_pairs_->empty() || kv_cursor_ == 0) {
        return {};
    }
    return kv_pairs_->operator[](kv_cursor_ - 1).first.view();
}

string_view redis_result::value() const noexcept {
    if (kv_pairs_->empty() || kv_cursor_ == 0) {
        return {};
    }
    return kv_pairs_->operator[](kv_cursor_ - 1).second.view();
}

bool redis_result::value_bool() const { return boolean::parse(get_string().view()).value(); }

int64_t redis_result::value_int64() const {
    if (result_ == nullptr) {
        return 0;
    }
    if (result_->type == REDIS_REPLY_INTEGER) {
        return result_->integer;
    }
    return integer64::parse(get_string().view()).value();
}

double redis_result::value_double() const { return float64::parse(get_string().view()).value(); }

vector<string> redis_result::value_array() const {
    vector<string> result;
    if (result_ != nullptr && result_->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < result_->elements; ++i) {
            string value = format_redis_reply_element(result_->element[i]);
            result.push_back(_NEFORCE move(value));
        }
    }
    return result;
}

NEFORCE_END_NAMESPACE__
#endif
