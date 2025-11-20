#include <MSTL/database/redis/redis_connect.hpp>
#ifdef MSTL_SUPPORT_REDIS__
#include <MSTL/database/redis/redis_result.hpp>
MSTL_BEGIN_NAMESPACE__

_MSTL_REDIS redisReply* redis_connect::execute_command(
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

    return static_cast<_MSTL_REDIS redisReply*>(
        _MSTL_REDIS redisCommandArgv(context_, argv.size(), argv.data(), argvlen.data())
    );
}

bool redis_connect::authenticate(const string& password) const {
    if (password.empty()) return true;
    const auto reply = execute_command("AUTH", {password.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Authentication failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

bool redis_connect::select_database(const string& db_index) const {
    if (db_index.empty()) return true;
    try {
        const auto reply = execute_command("SELECT", {db_index.view()});
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) {
                last_error_ = reply->str ? reply->str : "SELECT failed";
                _MSTL_REDIS freeReplyObject(reply);
            }
            return false;
        }
        _MSTL_REDIS freeReplyObject(reply);
        return true;
    } catch (...) {
        last_error_ = "Invalid database index";
        return false;
    }
}

bool redis_connect::connect_to_host(
    const string& host, const uint16_t port,
    const string& password, const string& dbname) {
    context_ = _MSTL_REDIS redisConnect(host.c_str(), port);
    if (!context_ || context_->err) {
        if (context_) {
            last_error_ = context_->errstr;
            _MSTL_REDIS redisFree(context_);
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

bool redis_connect::reset_connect(const db_config& config) {
    close();
    return connect_to(config);
}

string_view redis_connect::get_error() const noexcept {
    if (context_ && context_->errstr[0] != '\0') {
        last_error_ = context_->errstr;
    }
    return {last_error_.data(), last_error_.size()};
}

bool redis_connect::update(const string& sql) const noexcept {
    const auto reply = static_cast<_MSTL_REDIS redisReply*>(
        _MSTL_REDIS redisCommand(context_, sql.c_str())
        );
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Command failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::query(const string& sql) const {
    const auto reply = static_cast<_MSTL_REDIS redisReply*>(
        _MSTL_REDIS redisCommand(context_, sql.c_str())
        );
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Query failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::is_valid() const noexcept {
    if (!connected()) return false;
    const auto reply = execute_command("PING");
    if (!reply || reply->type != REDIS_REPLY_STATUS ||
        string_compare(reply->str, "PONG") != 0) {
        if (reply) _MSTL_REDIS freeReplyObject(reply);
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

void redis_connect::close() noexcept {
    if (!context_) return;
    _MSTL_REDIS redisFree(context_);
    context_ = nullptr;
}

bool redis_connect::set(const string& key, const string& value) {
    const auto reply = execute_command("SET", {key.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "SET failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

bool redis_connect::setex(const string& key, const string& value, const int seconds) {
    const string sec_str = integer32(seconds).to_string();
    const auto reply = execute_command("SETEX", {key.view(), sec_str.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "SETEX failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::get(const string& key) {
    const auto reply = execute_command("GET", {key.view()});
    if (!reply) {
        last_error_ = "GET command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "GET failed";
        _MSTL_REDIS freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::del(const string& key) {
    const auto reply = execute_command("DEL", {key.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "DEL failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    _MSTL_REDIS freeReplyObject(reply);
    return result;
}

bool redis_connect::exists(const string& key) {
    const auto reply = execute_command("EXISTS", {key.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "EXISTS failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    _MSTL_REDIS freeReplyObject(reply);
    return result;
}

bool redis_connect::expire(const string& key, const int seconds) {
    const string sec_str = integer32(seconds).to_string();
    const auto reply = execute_command("EXPIRE", {key.view(), sec_str.view() });
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "EXPIRE failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    const bool result = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    _MSTL_REDIS freeReplyObject(reply);
    return result;
}

bool redis_connect::hset(const string& key, const string& field, const string& value) {
    const auto reply = execute_command("HSET", {key.view(), field.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "HSET failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::hget(const string& key, const string& field) {
    const auto reply = execute_command("HGET", {key.view(), field.view()});
    if (!reply) {
        last_error_ = "HGET command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "HGET failed";
        _MSTL_REDIS freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

unique_ptr<idb_kv_result> redis_connect::hgetall(const string& key) {
    const auto reply = execute_command("HGETALL", {key.view()});
    if (!reply) {
        last_error_ = "HGETALL command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "HGETALL failed";
        _MSTL_REDIS freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::lpush(const string& key, const string& value) {
    const auto reply = execute_command("LPUSH", {key.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "LPUSH failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

bool redis_connect::rpush(const string& key, const string& value) {
    const auto reply = execute_command("RPUSH", {key.view(), value.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "RPUSH failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::lrange(const string& key, const int start, const int stop) {
    const string start_str = integer32(start).to_string();
    const string stop_str = integer32(stop).to_string();
    const auto reply = execute_command("LRANGE", {key.view(), start_str.view(), stop_str.view() });
    if (!reply) {
        last_error_ = "LRANGE command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "LRANGE failed";
        _MSTL_REDIS freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

bool redis_connect::sadd(const string& key, const string& member) {
    const auto reply = execute_command("SADD", {key.view(), member.view()});
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "SADD failed";
            _MSTL_REDIS freeReplyObject(reply);
        }
        return false;
    }
    _MSTL_REDIS freeReplyObject(reply);
    return true;
}

unique_ptr<idb_kv_result> redis_connect::smembers(const string& key) {
    const auto reply = execute_command("SMEMBERS", {key.view()});
    if (!reply) {
        last_error_ = "SMEMBERS command failed";
        return nullptr;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        last_error_ = reply->str ? reply->str : "SMEMBERS failed";
        _MSTL_REDIS freeReplyObject(reply);
        return nullptr;
    }
    return make_unique<redis_result>(reply);
}

idb_connect* redis_factory::create_connect() {
    auto conn = new redis_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* redis_factory::create_result(void* native_result) {
    return new redis_result(static_cast<_MSTL_REDIS redisReply*>(native_result));
}

MSTL_END_NAMESPACE__
#endif
