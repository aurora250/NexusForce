#include <MSTL/db/redis.hpp>
#include <MSTL/core/serialize.hpp>
#ifdef MSTL_SUPPORT_REDIS__
MSTL_BEGIN_NAMESPACE__

_MSTL string db_redis_result::format_redis_reply_element(redis::redisReply* element) {
    switch (element->type) {
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
            return {element->str, element->len};
        case REDIS_REPLY_INTEGER:
            return integer64(element->integer).to_string();
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

void db_redis_result::process_reply() {
    if (!reply_) return;

    switch (reply_->type) {
        case REDIS_REPLY_ARRAY: {
            is_array_ = true;
            rows_ = reply_->elements;

            if (rows_ % 2 == 0) {
                for (size_t i = 0; i < rows_; i += 2) {
                    string key = format_redis_reply_element(reply_->element[i]);
                    string value = format_redis_reply_element(reply_->element[i + 1]);
                    kv_pairs_.emplace_back(_MSTL move(key), _MSTL move(value));
                }
                rows_ = kv_pairs_.size();
            } else {
                column_names_.push_back("value");
            }
            break;
        } case REDIS_REPLY_STRING: case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR: case REDIS_REPLY_INTEGER: {
            rows_ = 1;
            column_names_.push_back("result");
            break;
        } case REDIS_REPLY_NIL: {
            rows_ = 0;
            break;
        } default: {
            rows_ = 1;
            column_names_.push_back("result");
            break;
        }
    }
}

string db_redis_result::at_string() const {
    if (empty()) return {};

    if (!kv_pairs_.empty() && kv_cursor_ > 0) {
        return string(value());
    }
    if (is_array_ && cursor_ > 0) {
        redis::redisReply* element = reply_->element[cursor_ - 1];
        return format_redis_reply_element(element);
    }
    return format_redis_reply_element(reply_);
}

bool db_redis_result::next() noexcept {
    if (empty() || cursor_ >= rows_) return false;
    ++cursor_;
    return cursor_ <= rows_;
}

string_view db_redis_result::key() const noexcept {
    if (kv_pairs_.empty() || kv_cursor_ == 0) return {};
    return kv_pairs_[kv_cursor_ - 1].first.view();
}

string_view db_redis_result::value() const noexcept {
    if (kv_pairs_.empty() || kv_cursor_ == 0) return {};
    return kv_pairs_[kv_cursor_ - 1].second.view();
}

bool db_redis_result::value_bool() const {
    return boolean::parse(at_string().view()).value();
}

int64_t db_redis_result::value_int64() const {
    if (!reply_) return 0;
    if (reply_->type == REDIS_REPLY_INTEGER) {
        return reply_->integer;
    }
    return integer64::parse(at_string().view());
}

double db_redis_result::value_double() const {
    return float64::parse(at_string().view());
}

vector<string> db_redis_result::value_array() const {
    vector<string> result;
    if (reply_ && reply_->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply_->elements; ++i) {
            string value = format_redis_reply_element(reply_->element[i]);
            result.push_back(_MSTL move(value));
        }
    }
    return result;
}

redis::redisReply* db_redis_connect::execute_command(
    const string_view command, const vector<string_view>& args) const {
    if (!context_) return nullptr;

    vector<const char*> argv;
    vector<size_t> argvlen;

    argv.push_back(command.data());
    argvlen.push_back(command.length());

    for (const auto& arg : args) {
        argv.push_back(arg.data());
        argvlen.push_back(arg.length());
    }

    return static_cast<redis::redisReply*>(
        redis::redisCommandArgv(context_, argv.size(), argv.data(), argvlen.data())
    );
}

bool db_redis_connect::authenticate(const string& password) const {
    if (password.empty()) return true;
    const auto reply = execute_command("AUTH", {password.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Authentication failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

bool db_redis_connect::select_database(const string& db_index) const {
    if (db_index.empty()) return true;
    try {
        const auto reply = execute_command("SELECT", {db_index.view()});
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) {
                last_error_ = reply->str ? reply->str : "SELECT failed";
                redis::freeReplyObject(reply);
            }
            return false;
        }
        redis::freeReplyObject(reply);
        return true;
    } catch (...) {
        last_error_ = "Invalid database index";
        return false;
    }
}

bool db_redis_connect::connect_to_host(
    const string& host, const uint16_t port,
    const string& password, const string& dbname) {
    context_ = redis::redisConnect(host.c_str(), port);
    if (!context_ || context_->err) {
        if (context_) {
            last_error_ = context_->errstr;
            redis::redisFree(context_);
            context_ = nullptr;
        } else {
            last_error_ = "Connection failed";
        }
        return false;
    }
    if (!authenticate(password)) {
        close();
        return false;
    }
    if (!select_database(dbname)) {
        close();
        return false;
    }
    return true;
}

bool db_redis_connect::reset_connect(const db_connect_config& config) {
    close();
    return connect_to(config);
}

string_view db_redis_connect::get_error() const noexcept {
    if (context_ && context_->errstr[0] != '\0') {
        last_error_ = context_->errstr;
    }
    return {last_error_.data(), last_error_.size()};
}

bool db_redis_connect::update(const string& sql) const noexcept {
    const auto reply = static_cast<redis::redisReply*>(
        redis::redisCommand(context_, sql.c_str())
        );
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Command failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> db_redis_connect::query(const string& sql) const {
    const auto reply = static_cast<redis::redisReply*>(
        redis::redisCommand(context_, sql.c_str())
        );
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Query failed";
            redis::freeReplyObject(reply);
        }
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

bool db_redis_connect::is_valid() const noexcept {
    if (!connected()) return false;
    const auto reply = execute_command("PING");
    if (!reply || reply->type != REDIS_REPLY_STATUS ||
        string_compare(reply->str, "PONG") != 0) {
        if (reply) redis::freeReplyObject(reply);
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

void db_redis_connect::close() noexcept {
    if (!context_) return;
    redis::redisFree(context_);
    context_ = nullptr;
}

bool db_redis_connect::set(const string& key, const string& value) {
    const auto reply = execute_command("SET", {key.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "SET failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

bool db_redis_connect::setex(const string& key, const string& value, const int seconds) {
    const string sec_str = integer32(seconds).to_string();
    const auto reply = execute_command("SETEX", {key.view(), sec_str.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "SETEX failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> db_redis_connect::get(const string& key) {
    const auto reply = execute_command("GET", {key.view()});
    if (!reply) {
        last_error_ = "GET command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "GET failed";
        redis::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

bool db_redis_connect::del(const string& key) {
    const auto reply = execute_command("DEL", {key.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "DEL failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    redis::freeReplyObject(reply);
    return result;
}

bool db_redis_connect::exists(const string& key) {
    const auto reply = execute_command("EXISTS", {key.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "EXISTS failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    redis::freeReplyObject(reply);
    return result;
}

bool db_redis_connect::expire(const string& key, const int seconds) {
    const string sec_str = integer32(seconds).to_string();
    const auto reply = execute_command("EXPIRE", {key.view(), sec_str.view() });
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "EXPIRE failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    redis::freeReplyObject(reply);
    return result;
}

bool db_redis_connect::hset(const string& key, const string& field, const string& value) {
    const auto reply = execute_command("HSET", {key.view(), field.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "HSET failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> db_redis_connect::hget(const string& key, const string& field) {
    const auto reply = execute_command("HGET", {key.view(), field.view()});
    if (!reply) {
        last_error_ = "HGET command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "HGET failed";
        redis::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

unique_ptr<idb_kv_result> db_redis_connect::hgetall(const string& key) {
    const auto reply = execute_command("HGETALL", {key.view()});
    if (!reply) {
        last_error_ = "HGETALL command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "HGETALL failed";
        redis::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

bool db_redis_connect::lpush(const string& key, const string& value) {
    const auto reply = execute_command("LPUSH", {key.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "LPUSH failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

bool db_redis_connect::rpush(const string& key, const string& value) {
    const auto reply = execute_command("RPUSH", {key.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "RPUSH failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> db_redis_connect::lrange(const string& key, const int start, const int stop) {
    const string start_str = integer32(start).to_string();
    const string stop_str = integer32(stop).to_string();
    const auto reply = execute_command("LRANGE", {key.view(), start_str.view(), stop_str.view() });
    if (!reply) {
        last_error_ = "LRANGE command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "LRANGE failed";
        redis::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

bool db_redis_connect::sadd(const string& key, const string& member) {
    const auto reply = execute_command("SADD", {key.view(), member.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "SADD failed";
            redis::freeReplyObject(reply);
        }
        return false;
    }
    redis::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> db_redis_connect::smembers(const string& key) {
    const auto reply = execute_command("SMEMBERS", {key.view()});
    if (!reply) {
        last_error_ = "SMEMBERS command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "SMEMBERS failed";
        redis::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

idb_connect* db_redis_factory::create_connect() {
    auto conn = new db_redis_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_REDIS__
