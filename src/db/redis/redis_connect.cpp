#include <NeForce/db/redis/redis_connect.hpp>
#ifdef NEFORCE_SUPPORT_HIREDIS
#    include <NeForce/db/redis/redis_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

::redisReply* redis_connect::execute_command(const string_view command, const vector<string_view>& args) const {
    if (link_ == nullptr) {
        return nullptr;
    }

    vector<const char*> argv;
    vector<size_t> argvlen;

    argv.push_back(command.data());
    argvlen.push_back(command.length());

    for (const auto& arg: args) {
        argv.push_back(arg.data());
        argvlen.push_back(arg.length());
    }

    return static_cast<::redisReply*>(
            ::redisCommandArgv(link_, static_cast<int>(argv.size()), argv.data(), argvlen.data()));
}

bool redis_connect::authenticate(const string& password) const {
    if (password.empty()) {
        return true;
    }
    auto* const reply = execute_command("AUTH", {password.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "Authentication failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool redis_connect::select_database(const string& db_index) const {
    if (db_index.empty()) {
        return true;
    }
    try {
        auto* const reply = execute_command("SELECT", {db_index.view()});
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            if (reply != nullptr) {
                last_error_ = reply->str != nullptr ? reply->str : "SELECT failed";
                last_errno_ = link_ != nullptr ? link_->err : 0;
                ::freeReplyObject(reply);
            }
            return false;
        }
        ::freeReplyObject(reply);
        return true;
    } catch (...) {
        last_error_ = "Invalid database index";
        last_errno_ = link_ != nullptr ? link_->err : 0;
        return false;
    }
}

bool redis_connect::connect(const db_config& config) {
    last_error_.clear();
    last_errno_ = 0;
    link_ = ::redisConnect(config.host.data(), static_cast<int>(config.port.value()));
    if (link_ == nullptr || link_->err != 0) {
        if (link_ != nullptr) {
            last_error_ = link_->errstr;
            last_errno_ = link_->err;
            ::redisFree(link_);
            link_ = nullptr;
        } else {
            last_error_ = "Connection failed";
            last_errno_ = 1;
        }
        return false;
    }
    if (!authenticate(config.password)) {
        close();
        return false;
    }
    if (!select_database(config.database)) {
        close();
        return false;
    }
    return true;
}

bool redis_connect::reconnect(const db_config& config) {
    close();
    return connect(config);
}

void redis_connect::close() noexcept {
    if (link_ == nullptr) {
        return;
    }
    ::redisFree(link_);
    link_ = nullptr;
}

string_view redis_connect::get_error() const { return last_error_.view(); }

bool redis_connect::update(const string& sql) const {
    auto* const reply = static_cast<::redisReply*>(::redisCommand(link_, sql.data()));
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "Command failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::query(const string& sql) const {
    auto* const reply = static_cast<::redisReply*>(::redisCommand(link_, sql.data()));
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "Query failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::is_valid() const {
    if (!connected()) {
        return false;
    }
    auto* const reply = execute_command("PING", {});
    if (reply == nullptr || reply->type != REDIS_REPLY_STATUS || string_compare(reply->str, "PONG") != 0) {
        if (reply != nullptr) {
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool redis_connect::begin() {
    auto* const reply = execute_command("MULTI", {});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "MULTI failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool redis_connect::commit() {
    auto* const reply = execute_command("EXEC", {});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "EXEC failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool redis_connect::rollback() {
    auto* const reply = execute_command("DISCARD", {});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "DISCARD failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool redis_connect::set(const string& key, const string& value) {
    auto* const reply = execute_command("SET", {key.view(), value.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "SET failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool redis_connect::setex(const string& key, const string& value, const int seconds) {
    const string sec_str = integer32(seconds).to_string();
    auto* const reply = execute_command("SETEX", {key.view(), sec_str.view(), value.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "SETEX failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::get(const string& key) {
    auto* const reply = execute_command("GET", {key.view()});
    if (reply == nullptr) {
        last_error_ = "GET command failed";
        last_errno_ = link_ != nullptr ? link_->err : 0;
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str != nullptr ? reply->str : "GET failed";
        last_errno_ = link_->err;
        ::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::del(const string& key) {
    auto* const reply = execute_command("DEL", {key.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "DEL failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    ::freeReplyObject(reply);
    return result;
}

bool redis_connect::exists(const string& key) {
    auto* const reply = execute_command("EXISTS", {key.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "EXISTS failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    ::freeReplyObject(reply);
    return result;
}

bool redis_connect::expire(const string& key, const int seconds) {
    const string sec_str = integer32(seconds).to_string();
    auto* const reply = execute_command("EXPIRE", {key.view(), sec_str.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "EXPIRE failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    ::freeReplyObject(reply);
    return result;
}

bool redis_connect::hset(const string& key, const string& field, const string& value) {
    auto* const reply = execute_command("HSET", {key.view(), field.view(), value.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "HSET failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::hget(const string& key, const string& field) {
    auto* const reply = execute_command("HGET", {key.view(), field.view()});
    if (reply == nullptr) {
        last_error_ = "HGET command failed";
        last_errno_ = link_ != nullptr ? link_->err : 0;
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str != nullptr ? reply->str : "HGET failed";
        last_errno_ = link_->err;
        ::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

unique_ptr<idb_kv_result> redis_connect::hgetall(const string& key) {
    auto* const reply = execute_command("HGETALL", {key.view()});
    if (reply == nullptr) {
        last_error_ = "HGETALL command failed";
        last_errno_ = link_ != nullptr ? link_->err : 0;
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str != nullptr ? reply->str : "HGETALL failed";
        last_errno_ = link_->err;
        ::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::lpush(const string& key, const string& value) {
    auto* const reply = execute_command("LPUSH", {key.view(), value.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "LPUSH failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool redis_connect::rpush(const string& key, const string& value) {
    auto* const reply = execute_command("RPUSH", {key.view(), value.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "RPUSH failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::lrange(const string& key, const int start, const int stop) {
    const string start_str = integer32(start).to_string();
    const string stop_str = integer32(stop).to_string();
    auto* const reply = execute_command("LRANGE", {key.view(), start_str.view(), stop_str.view()});
    if (reply == nullptr) {
        last_error_ = "LRANGE command failed";
        last_errno_ = link_ != nullptr ? link_->err : 0;
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str != nullptr ? reply->str : "LRANGE failed";
        last_errno_ = link_->err;
        ::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::sadd(const string& key, const string& member) {
    auto* const reply = execute_command("SADD", {key.view(), member.view()});
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply != nullptr) {
            last_error_ = reply->str != nullptr ? reply->str : "SADD failed";
            last_errno_ = link_ != nullptr ? link_->err : 0;
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::smembers(const string& key) {
    auto* const reply = execute_command("SMEMBERS", {key.view()});
    if (reply == nullptr) {
        last_error_ = "SMEMBERS command failed";
        last_errno_ = link_ != nullptr ? link_->err : 0;
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str != nullptr ? reply->str : "SMEMBERS failed";
        last_errno_ = link_->err;
        ::freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

idb_connect* redis_factory::create_connect() {
    auto* conn = new redis_connect();
    if (!conn->connect(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* redis_factory::create_result(void* native_result) {
    return new redis_result(static_cast<::redisReply*>(native_result));
}

NEFORCE_END_NAMESPACE__
#endif
