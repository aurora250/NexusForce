#include <MSTL/db/redis.hpp>
#ifdef MSTL_SUPPORT_DB__
MSTL_BEGIN_NAMESPACE__
#ifdef MSTL_SUPPORT_REDIS__

_MSTL string db_redis_result::format_redis_reply_element(::redisReply* element) {
    switch (element->type) {
        case REDIS_REPLY_STRING:
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_ERROR:
            return {element->str, element->len};
        case REDIS_REPLY_INTEGER:
            return integer32(element->integer).to_string();
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

db_redis_result::db_redis_result(::redisReply* reply) noexcept
    : reply_(reply) {
    if (reply_) {
        if (reply_->type == REDIS_REPLY_ARRAY) {
            is_array_ = true;
            rows_ = reply_->elements;
            column_names_.push_back("value");
        } else {
            rows_ = 1;
            column_names_.push_back("result");
        }
    }
}

db_redis_result::~db_redis_result() {
    if (reply_) {
        ::freeReplyObject(reply_);
    }
}

bool db_redis_result::empty() const noexcept {
    return !reply_ || rows_ == 0;
}

db_redis_result::size_type db_redis_result::row_count() const noexcept {
    return rows_;
}

db_redis_result::size_type db_redis_result::column_count() const noexcept {
    return 1;
}

const list<string_view>& db_redis_result::column_names() const noexcept {
    return column_names_;
}

bool db_redis_result::next() noexcept {
    if (empty() || cursor_ >= rows_) return false;
    ++cursor_;
    return cursor_ <= rows_;
}

string_view db_redis_result::at(size_type) const noexcept {
    return {at_string(0).data(), at_string(0).length()};
}

bool db_redis_result::at_bool(size_type) const {
    const string s = at_string(0);
    return s == "1" || s == "true" || s == "TRUE" || s == "yes";
}

int8_t db_redis_result::at_int8(size_type) const {
    return static_cast<int8_t>(this->at_int16(int()));
}

int16_t db_redis_result::at_int16(size_type) const {
    return integer16::parse(at_string(0).c_str());
}

int32_t db_redis_result::at_int32(size_type) const {
    return integer32::parse(at_string(0).c_str());
}

int64_t db_redis_result::at_int64(size_type) const {
    return integer64::parse(at_string(0).c_str());
}

float32_t db_redis_result::at_float32(size_type) const {
    return float32::parse(at_string(0).c_str());
}

float64_t db_redis_result::at_float64(size_type) const {
    return float64::parse(at_string(0).c_str());
}

decimal_t db_redis_result::at_decimal(size_type) const {
    return decimal::parse(at_string(0).c_str());
}

vector<char> db_redis_result::at_blob(size_type) const {
    const string s = at_string(0);
    return {s.begin(), s.end()};
}

string db_redis_result::at_set(size_type) const {
    return at_string(0);
}

uint64_t db_redis_result::at_bit(size_type) const {
    const string data = at_string(0);
    uint64_t value = 0;
    for (unsigned long i = 0; i < data.size(); ++i) {
        value = (value << 8) | static_cast<byte_t>(data[i]);
    }
    return value;
}

date db_redis_result::at_date(size_type) const {
    return date::parse(at_string(0).view());
}

time db_redis_result::at_time(size_type) const {
    return time::parse(at_string(0).view());
}

datetime db_redis_result::at_datetime(size_type) const {
    return datetime::parse(at_string(0).view());
}

timestamp db_redis_result::at_timestamp(size_type) const {
    return timestamp(integer64::parse(at_string(0).c_str()));
}

string db_redis_result::at_string(size_type) const {
    if (empty() || cursor_ == 0) return {};

    if (is_array_) {
        ::redisReply* element = reply_->element[cursor_ - 1];
        return format_redis_reply_element(element);
    }
    return format_redis_reply_element(reply_);
}

string_view db_redis_result::at_enum(size_type) const noexcept {
    return {at_string(0).c_str(), at_string(0).length()};
}


bool db_redis_connect::authenticate(const string& password) const {
    if (password.empty()) return true;
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, "AUTH %s", password.c_str()));
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Authentication failed";
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

bool db_redis_connect::select_database(const string& db_index) const {
    if (db_index.empty()) return true;
    try {
        const int db = integer32::parse(db_index.c_str());
        const auto reply = static_cast<::redisReply*>(::redisCommand(context_, "SELECT %d", db));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) {
                last_error_ = reply->str ? reply->str : "SELECT failed";
                ::freeReplyObject(reply);
            }
            return false;
        }
        ::freeReplyObject(reply);
        return true;
    } catch (...) {
        last_error_ = "Invalid database index";
        return false;
    }
}

bool db_redis_connect::connect_to_host(
    const string& host, const uint16_t port,
    const string& password, const string& dbname) {
    context_ = redisConnect(host.c_str(), port);
    if (!context_ || context_->err) {
        if (context_) {
            last_error_ = context_->errstr;
            ::redisFree(context_);
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


db_redis_connect::~db_redis_connect() {
    close();
}

bool db_redis_connect::connect_to(const string&, const string& password,
    const string& dbname, const string& host,
    const uint32_t port, const string&) {
    return connect_to_host(host, port, password, dbname);
}

bool db_redis_connect::connect_to(const db_connect_config& config) {
    return connect_to_host(config.host, config.port, config.password, config.database);
}

bool db_redis_connect::set_character_set(const string&) const noexcept {
    return true;
}
string_view db_redis_connect::get_character_set() const noexcept {
    return {};
}

string_view db_redis_connect::get_error() const noexcept {
    if (context_ && context_->errstr[0] != '\0') {
        last_error_ = context_->errstr;
    }
    return {last_error_.data(), last_error_.size()};
}

uint32_t db_redis_connect::get_errno() const noexcept {
    return context_ ? context_->err : 0;
}

bool db_redis_connect::update(const string& sql) const noexcept {
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, sql.c_str()));
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Command failed";
            ::freeReplyObject(reply);
        }
        return false;
    }
    ::freeReplyObject(reply);
    return true;
}

unique_ptr<idb_result> db_redis_connect::query(const string& sql) const {
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, sql.c_str()));
    if (!reply || reply->type == REDIS_REPLY_ERROR) {
        if (reply) {
            last_error_ = reply->str ? reply->str : "Query failed";
            ::freeReplyObject(reply);
        }
        return nullptr;
    }
    return make_unique<db_redis_result>(reply);
}

bool db_redis_connect::connected() const noexcept {
    return context_ != nullptr && !context_->err;
}

bool db_redis_connect::is_valid() const noexcept {
    if (!connected()) return false;
    const auto reply = static_cast<::redisReply*>(::redisCommand(context_, "PING"));
    if (!reply || reply->type != REDIS_REPLY_STATUS ||
        string_compare(reply->str, "PONG") != 0) {
        if (reply) ::freeReplyObject(reply);
        return false;
        }
    ::freeReplyObject(reply);
    return true;
}

void db_redis_connect::close() noexcept {
    if (context_) {
        ::redisFree(context_);
        context_ = nullptr;
    }
}

void db_redis_connect::refresh_alive() noexcept {
    alive_time_ = std::clock();
}

db_redis_connect::clock_type db_redis_connect::get_alive() const noexcept {
    return std::clock() - alive_time_;
}

bool db_redis_connect::reset_connect(const db_connect_config& config) {
    close();
    return connect_to(config);
}


db_redis_factory::db_redis_factory(const db_connect_config& config) : idb_factory(config) {}

idb_connect* db_redis_factory::create_connect() {
    auto conn = new db_redis_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* db_redis_factory::create_result(void* native_result) {
    return new db_redis_result(static_cast<::redisReply*>(native_result));
}

#endif // MSTL_SUPPORT_REDIS__
MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_DB__
